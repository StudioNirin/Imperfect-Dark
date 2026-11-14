# GitHub: branches, commits, releases, and the updater

Companion to `CLAUDE.md`. That file is about the game; this one is about how the
work leaves this machine and reaches a player. Read it before pushing anything,
and before cutting a release.

## Where this lives

```
origin    https://github.com/StudioNirin/Imperfect-Dark.git    (public, this fork)
dabs      https://github.com/DabDavis/perfect-dark-dabs-mod.git  (the donor branch this fork grew from)
upstream  https://github.com/perfect-dark-pc-port/perfect_dark.git
```

`gh` is authenticated as **StudioNirin** over https, so `git push` and `gh release`
both work without a prompt. The `dabs` remote is kept on purpose: it is where the
mod this fork was renamed from still publishes, and `upstream-watch.yml` fetches
`dabs dabs-mod` to notice what that donor does. Those are donor identifiers, not
this repository's, and the workflow says so in its comments.

**Set the default repository in a fresh clone**, or `gh` reads the wrong one:

```sh
gh repo set-default StudioNirin/Imperfect-Dark
```

With no default set and two remotes, `gh` picks by remote *name* against a
preference list that puts `upstream` ahead of `origin` — so a bare `gh run list`
answers about the port's CI rather than this fork's, and does it silently. What
comes back is green runs of `C/C++ CI`, which is a workflow name this repository
also has, inherited from upstream in `c-cpp.yml` — so nothing about the output
looks foreign, and it was read once as "the push built fine". The setting lives
in `.git/config` as `remote.origin.gh-resolved`, which is local to the clone and
comes back the moment anyone clones again; `-R StudioNirin/Imperfect-Dark` says it
per command. This was not hypothetical: with no default set, a bare `gh run list`
here answered about `perfect-dark-pc-port/perfect_dark`, silently, and the green
runs it showed were `C/C++ CI`, a name this repository also has.

**`imperfect-dark` is the repository's default branch and the repository is
public.** It was `dabs-mod` until 2026-11-14, when the branch, the workflow, the
rolling tag and the distributed file names were all renamed to Imperfect Dark.
The old `dabs-mod` branch still exists on the remote and the workflow still lists
it as a trigger, so a push there still builds - drop the branch and the trigger
together once nothing is pushing to it.

A push is immediately what a visitor sees, and — if it is a push to
`imperfect-dark` rather than a topic branch — it also rebuilds the dev release that
every dev-channel player's Check for Updates is pointed at. There is no staging
step between a push and a player, so confirm before pushing rather than after. One
consequence that is easy to miss: a branch push moves the rolling release
immediately, so if the commit bumps `NET_PROTOCOL_VERSION` then everyone still on
the previous build is refused from a match the moment the assets land.

| branch | what it is |
| --- | --- |
| `imperfect-dark` | the mod. Default branch, and the only one CI publishes from. Was `dabs-mod` until 2026-11-14. |
| `dabs-mod` | the same branch under its old name, still on the remote and still a workflow trigger. Nothing new lands here; delete it with the trigger. |
| `port` | stock upstream, plus the CI disable below. `git checkout port` returns to stock at any time. Merge upstream into here first, then into `imperfect-dark`. |
| `dev` | a mirror of the mod branch, fast-forwarded to it on request (`git branch -f dev imperfect-dark && git push origin dev`). CI does not build from it; work is committed on `imperfect-dark`. It has fallen thousands of commits behind and nothing reads it. |
| `wip/modstages` | incomplete runtime stage registration. Not merged, not built. |

## Committing

Commits here are written to be read later. The subject is one imperative line
with no full stop, naming what changed rather than the files it touched — *Record
on the GPU, and stop taking the frame with it*, not *update record.c*. Under it
goes prose, wrapped at about 76 columns: what was wrong, what it does now, and
why it was done that way rather than the other way. Measurements and the failure
that prompted the change belong here, because this is the only place anyone will
find them a year on.

Claude's commits end with both trailers, the session link being how a commit is
traced back to the conversation that produced it:

```
Co-Authored-By: Claude <model> <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_...
```

Build both targets before committing anything under `port/`. The Windows
cross-build is the only thing that exercises the `popen` half of the recorder and
the WinHTTP half of the network code, and neither is reachable from a Linux build:

```sh
cmake --build build -j8        # Linux
cmake --build build-win -j8    # mingw, see the memory note for the prefix
```

Neither of them is clang, and the two macOS jobs are. A name the BSD half of
Apple's libc already uses — `index()`, `rindex()`, `bcopy()` — compiles here
and not there, because the fork builds at `-std=c11` and glibc hides those
behind `__STRICT_ANSI__` while Apple's `<string.h>` includes `<strings.h>`
and declares them whatever the standard asked for. That is a green Linux and
a green Windows and a release that never publishes. A third build catches it
for the price of one compile:

```sh
CC=clang cmake -G"Unix Makefiles" -Bbuild-clang . && cmake --build build-clang -j8
```

## Pushing

```sh
git push origin imperfect-dark
```

Fast-forward only. Nothing here rewrites published history — `imperfect-dark` is
what CI builds from and what the dev release tracks, so a force push would strand
every player whose game has already seen a build that no longer exists.

The one exception is the `imperfect-dark-dev` tag, which the release job itself
moves with `git tag --force` on every build. That is deliberate and is CI's to do,
not yours.

## Releases: stable and dev

Both come out of `.github/workflows/imperfect-dark.yml` (named
`Imperfect Dark CI`), and both from the same **two** builds — `x86_64-windows` and
`x86_64-linux` — plus a `publish` job. There are **no macOS builds here**, whatever
this file used to claim about four targets; check the job names in the workflow or a
run's job list before writing about platforms. What differs is the trigger and where
the result is published.

**Dev is a branch push.** Every push to `imperfect-dark` builds, force-moves the
`imperfect-dark-dev` tag to that commit, and re-uploads the assets to a prerelease
called *Imperfect Dark (latest dev build)*. It is one rolling release that always
describes the tip of the branch, so it is a different program every time and has no
version number of its own — it is named `dev-<shortsha>` after the commit it was
built from, with a patch-notes stamp on the end (`dev-4d809eb-nx-21-2026-11-14`:
the newest `notes N` line in `patchnotes.txt` minus the `mod-base` line at its
head, so a player can see how many entries a build carries before the version
linter's `nx` marker and the date).

**Stable is a `v*` tag.** Tagging cuts a release named after the tag, created
once and thereafter only ever added to. It is not a prerelease, so GitHub makes
it the repository's *Latest*, which is where a visitor who is not looking for a
dev build lands.

```sh
git tag v3.1.0 && git push origin v3.1.0    # cuts the stable release
```

The job creates it with `--title "Imperfect Dark v3.1.0"` and a stock paragraph,
and the guard is `gh release view "$VER" || gh release create ...`, so pushing the
tag is enough to get a titled release with assets. What a human still adds is the
real changelog, after the fact: `gh release edit vX.Y.Z --notes-file notes.md`.
(Creating it by hand *before* the tag exists would be a second push event.)
Existing tags: `v3.9.0`, `v3.8.0`, `v3.7.0`, `v3.6.0`, `v3.5.0`, `v3.4.0`, `v3.3.3`, `v3.2.2`, `v3.2.1`, `v3.2.0`, `v3.1.3`, `v3.1.2`, `v3.1.1`, `v3.1.0`, `v3.0.0`, `v2.0.4`, `v2.0.3`, `v2.0.2`, `v2.0.1`, `v2.0.0`.

**A push that changes only `CLAUDE.md`, `CLAUDE-notes/`, `DabDavisGitHub.md`,
`docs/`, `tools/package/` or `deploy/` does not build** — `paths-ignore` names
exactly those six, and when it fires the workflow never runs, so the branch moves
and the rolling release does not. Four builds and a fresh executable offered to
every dev-channel player is too much to ask for a file that is neither in the
download nor in the binary. `README.md` and `LICENSE` are deliberately *not* in the
list: they are copied into every package by the packaging steps, so a fix to one of
them does have to be built and published like anything else. Note what the list
keys on: this file's own path, so renaming it is a workflow edit as well. And it is safe
for stable releases, because **GitHub does not evaluate path filters for tag
pushes at all** — a `v*` tag builds whatever it points at, docs-only or not.
That asymmetry is real and undocumented; do not reason about it from the branch
case.

Each release carries eight assets: an archive per platform
(`pd-imperfect-dark-x86_64-windows.zip`, `pd-imperfect-dark-x86_64-linux.tar.gz`),
the bare executable for each (`pd.x86_64-windows.exe`, `pd.x86_64-linux`), the
separate debug file for each (`pd.x86_64-windows.exe.debug`,
`pd.x86_64-linux.debug`), `patchnotes.txt`, and `update.txt`. The bare executables
exist because the updater downloads one file and puts it where the running program
is; teaching it to unpack a zip and a tarball, with no decompressor linked in, is
what they avoid. They are named after the platform string the binary carries in
`VERSION_TARGET`, since `pd.x86_64` for two platforms would otherwise be one asset
name for two programs. The `.debug` files are the DWARF, split out of the binaries
from `82b7a71` (v3.1.2 shipped it inside): most of each binary, and the updater
does not download them, so a Windows download is 26 MB where it was 66. When a
crash report names an offset, `-e` the `.debug` and `addr2line` gives exactly what
the whole executable gave — see the memory note.

Alongside them is `update.txt`, the manifest, written by the release job:

```
version v3.1.3                          (or dev-4d809eb-nx-21-2026-11-14)
commit  4d809eb
build   x86_64-windows pd.x86_64-windows.exe <sha256> <bytes>
build   x86_64-linux   pd.x86_64-linux       <sha256> <bytes>
```

## The updater, from the release side

`port/src/update.c`. `CLAUDE-notes/updater.md` covers how the game replaces itself; what
matters here is that **the channel is baked in at build time and the two channels
never mix**.

CMake takes `UPDATE_CHANNEL` (`dev` or `stable`, defaulting to `dev`) into
`VERSION_CHANNEL`, and the game reads one of two URLs accordingly:

| channel | where it looks |
| --- | --- |
| `stable` | `.../releases/latest/download/` — GitHub's own redirect to the newest non-prerelease, so which release that is stays the server's decision |
| `dev` | `.../releases/download/imperfect-dark-dev/` — named directly, because the rolling tag is one GitHub would never call "latest" |

The release job sets it from the trigger: a tag build is `stable`, everything
else is `dev`. This is why the binary cannot work it out for itself — the job
runs `git checkout -B "$MOD_BRANCH"` for tag and branch builds alike, so nothing
inside the checkout says which it was. (`MOD_BRANCH` is `imperfect-dark`, and it
also lands on the title screen: `CMakeLists.txt` puts the checked-out branch name
there, which is why the job names the commit before checking it out.)

A stable player is offered stable releases and a dev player dev builds, and
nothing moves anyone between the two. Cutting a release therefore updates only
half the players; the other half were already updated by the push that preceded
it.

**Testing it without cutting a release.** `Mod.UpdateServer` in `pd.ini` points
the whole mechanism at a server of your own, and is the only way to exercise the
download and swap for real. It stays in `pd.ini` once set — a test that leaves it
there is a game that can no longer see real updates.

## Two things that will trip you up

**`git rev-parse --short` picks its own length.** It grows with the number of
objects in the repository, so a CI runner's fresh clone says seven characters
where this working copy says nine — the manifest and the binary can name the same
commit and disagree about how to spell it. Compare by the shorter of the two.

**Upstream's CI is disabled on `port` only.** `c-cpp.yml` builds seven targets
and uploads to a `ci-dev-build` release that exists in upstream's repository and
not in this one, so it would spend an hour to fail at the last step. It could not
be disabled through the API — GitHub registers a workflow when it first runs, and
disabling one that never has returns 404 — so the triggers were emptied instead,
on `port`, where upstream's future edits to the jobs below still merge cleanly.

That commit has **not** been merged into `imperfect-dark`, which still carries
upstream's original triggers naming `port`. Nothing fires them from an
`imperfect-dark` push and a `port` push uses `port`'s own disabled copy, so it is
inert — but it is a real difference between the branches, and worth knowing before
concluding that a workflow file has been tampered with.

## What this file is still called

`DabDavisGitHub.md`, and the workflow's `paths-ignore` names that path — so if it
ever gets renamed to match the project, edit `imperfect-dark.yml` in the same
commit or a documentation-only push starts costing four builds and a new executable
for every dev-channel player.
