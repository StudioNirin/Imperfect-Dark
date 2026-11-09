# The nx handbook: everything learned about this project

The accumulated picture: what this fork is, how to change an existing file,
how to add a mod, how releases and updates work, and how a player on an old
DabDavis install moves over. `nx` is the tag for this fork's own work - the
release stamps (nx-1, nx-2, ...) count its entries, and this handbook is the
companion document to it. The day-to-day runbook is `fork-workflow.md`;
this is the reference the runbook assumes you did not have yet.

## 1. What this project is

- Fork lineage: `perfect-dark-pc-port/perfect_dark` (the decompiled PC port)
  -> `DabDavis/perfect-dark-dabs-mod` (the mods branch) ->
  **`StudioNirin/Imperfect-Dark`** (this fork). Source pinned at `a8c2ab2`,
  the `dabs-mod-dev` tag = v3.15.1.
- Git remotes: `origin` = this fork, `dabs` = DabDavis, `upstream` = the PC
  port. `gh` run in this directory resolves to the *upstream* repo, so every
  `gh` call needs `-R StudioNirin/Imperfect-Dark`.
- Copyrighted content (the ROM, `payload/`) never enters git or the build
  graph. Only local `--full` packages carry it; the clean package and every
  CI artifact are shareable.

## 2. Repo map

- `src/game/`, `src/include/` - the decompiled original game. Decompiled
  conventions everywhere (`chr0f04c874` style symbols, uninitialized-var
  compiler warnings are normal noise, not something to "fix" in passing).
- `port/src/` - the PC layer: `system.c` (version strings, logging),
  `main.c` (config registration), `net/` (session, lobby, ghostnet),
  `update.c` + `updatemenu.c` (Check for Updates), `patchnotes.c`,
  `netlobbymenu.c` (lobby menus); the menu engine lives under `game/menu`.
- `CLAUDE-notes/*.md` - area notes from upstream (netplay.md, mods.md,
  languages.md, save-format.md, goldfinger64.md, ...). Read the matching
  file before touching an area; `docs/` holds longer analyses.
- `tools/ci/net*test.sh` - the scripted gates: real clients against a local
  `pdlobbyd`, screenshots in `/tmp`-style output dirs. All of them in
  `netallgates.sh` - long and load-sensitive, run it alone.
- `tools/package/package.sh` - packaging (section 5).
- `deploy/lobby/` - the lobby container (section 7).
- `run/` - a scratch playtest install whose binary is a hard link into
  `build/` (section 8 has the trap).
- `payload/` - private content for `--full` packages; its verified NTSC-U
  ROM md5 is `e03b088b6ac9e0080440efed07c1e40f`. `build/data/` and
  `run/data/` are symlinks into it.

## 3. How a build names itself

- CMake stamps at configure time: `VERSION_HASH` from
  `git rev-parse --short HEAD`, the branch name, the update channel
  (`dev`/`stable`) and the target. It is configure-only, so a build directory
  that never reconfigures keeps a stale label while its code is current.
  Refresh with `cmake -B build . && cmake --build build -j32`.
- Netplay allows only identical builds (`netBuildSame()`, netsession.c),
  compared by **common prefix with a 7-character minimum** because git's
  `--short` length varies: CI's shallow clones get 7 characters, a full local
  clone 9. `NET_PROTOCOL_VERSION` is 33; any engine commit splits the online
  pool by hash alone, which is expected for a fork in development.
- The **nx release stamp**: builds and the release job both derive
  `nx-<n>-<date>` from `patchnotes.txt`, where `<n>` is the newest entry
  number less the `# mod-base 69` marker (69 = the newest entry that predates
  this fork's own work). So entry 70 is nx-1, entry 71 is nx-2, and the stamp
  rolls forward by itself whenever a new entry is added. Builds whose newest
  entry is at or below the base carry no stamp. It appears beside the commit
  hash in the updater menu, update notices, the log banner, and trace and
  crash reports; the release's `update.txt` version token carries it too
  (that field is 32 characters - a dev name plus a stamp stays inside it).
- **Patch notes** (`patchnotes.txt`, `port/src/patchnotes.c`): the newest
  entry number and the whole file are baked into every build. Check for
  Updates fetches the release's copy and shows every entry newer than the
  running build (cumulative: skip five releases, see all five batches). The
  main menu pops a what's-new once after an update, showing everything since
  `Mod.PatchNotesSeen`; `Mod.PatchNotesPopup 0` disables it.

## 4. Changing an existing file (the working method)

1. Read the matching `CLAUDE-notes/` file first. Netplay work means
   `netplay.md`; menu text means `text-rendering.md` and `languages.md`.
2. Switchable behaviour is a `Mod.` setting, which is five touch points -
   copy any existing one: `src/include/game/modoptions.h` (field),
   `src/game/modoptions.c` (default and getter), `port/src/main.c`
   (`configRegisterInt/String`), `port/src/optionsmenu.c` (the menu row),
   and the consumer.
3. Menu work: rows can carry a per-row colour via
   `MENUITEMFLAG_LABEL_CUSTOMCOLOUR` with a `MENUOP_GETCOLOUR` handler (added
   in `menuitemSelectableRender()` for the team picker); dialogs follow
   `netlobbymenu.c`. All player-facing strings go through `langTr()` - a
   missing key falls back to the English text, so code can ship strings
   before translations do. The menu font is ASCII-only: no curly quotes or
   dashes, `|` is a heading marker, and text that looks like an IPv4 address
   is silently dropped from notes.
4. Build with `cmake --build build -j32`. `versioninfo.h` is force-included
   everywhere, so any reconfigure cascades a full rebuild; `patchnotesdata.h`
   is included by `patchnotes.c` alone, so adding a notes entry rebuilds one
   file. A `patchnotes.txt` change reconfigures by itself (it is a
   `CMAKE_CONFIGURE_DEPENDS`).
5. Test both ways: by hand (`cd run && ./pd.x86_64 --log`) and with the
   nearest gate from `tools/ci/`. GUI flows are verified with script hooks,
   not clicking: the lobby scripts take `--net-lobby-*` and
   `--net-test-lobby-*` arguments (for example
   `--net-test-lobby-team-dialog`, which proves the eight-colour picker end
   to end) and screenshot menus. Two scripting facts learned the hard way:
   screenshot capture lands on a *later* frame's swap, so hold a dialog about
   1.5 s after `screenshotRequest()` before popping it; and multi-client
   tests need per-instance binary dirs (hard-link the binary with `cp -l`)
   because `fsChooseOutputDir` resolves `$E` (the exe's directory) before
   `$S`.
6. If players will notice the change, add a `patchnotes.txt` entry: a
   `notes <n+1> <YYYY-MM-DD>` line, one short player-words line per fix,
   credit with `(thanks Name)`. **Never renumber and never edit an entry once
   pushed** - installed builds and popups key off those numbers forever, and
   a player who has seen a number is never shown it again.
7. Push to `dabs-mod`: CI (`.github/workflows/dabs-mod.yml`) builds Windows
   and Linux and force-updates the `dabs-mod-dev` prerelease. That is the
   whole release train. Docs and tooling (`docs/**`, `tools/package/**`,
   `deploy/**`, a few root `.md` files) are in `paths-ignore` and
   deliberately do not rebuild. If the Windows job fails inside the MSYS2
   installer with a 504, that is GitHub being flaky:
   `gh run rerun <id> --failed`.
8. Stable releases are cut by pushing a `v*` tag; a tag's release is created
   once and never rewritten afterwards.
9. Keeping up with Dab: `upstream-watch.yml` (manual) reports commits ahead
   on the upstream branches; `fork-workflow.md` covers the merge cadence.

## 5. Modding and packaging

- One tree makes both packages: `tools/package/package.sh --linux --clean`
  (shareable, no copyrighted content) and `--full` (adds `payload/`, and
  prints and records exactly what went in, so the same tree still makes the
  clean one). Windows `--full` is assembled from CI artifacts:
  `gh run download <id>` then `--windows --full --from-artifact <dir>`;
  artifacts expire quickly (`retention-days: 0`), so the release zip is the
  practical source.
- An install carries `data/` (ROM and friends), `pd.ini`, saves and mods.
  GoldenEye mods convert on first use; the conversion format is versioned
  (`GECONVERT_VERSION_STR`), a half-finished conversion is cleared and
  redone, and netplay refuses peers with a different conversion version with
  a message that says so.
- The modding systems themselves are documented upstream:
  `CLAUDE-notes/mods.md`, `model-packs.md`, `texture-packs.md`,
  `goldfinger64.md`, `randomizer.md`; `tools/ci/moddatadump.sh` dumps mod
  data for inspection.

## 6. The updater, in detail

- The dev prerelease `dabs-mod-dev` is flat and anonymously fetchable - the
  in-game updater needs no token: `update.txt` (lines `version <v>`,
  `commit <c>`, `build <target> <asset> <sha> <size>`), `patchnotes.txt`,
  the bare binaries `pd.x86_64-windows.exe` / `pd.x86_64-linux`, and their
  `.debug` symbol files (referenced by crash reports, never by the game).
- An update is offered when the manifest's commit differs from the build's
  `VERSION_HASH` by prefix. The download is checked against the manifest's
  SHA-256 and size before the binary replaces itself.
- The repo URL is compiled in (`UPDATE_REPO` in `update.c`); the ini escape
  hatch is `Mod.UpdateServer` below.

## 7. Migrating a player off DabDavis builds

No reinstall is needed, and nothing of theirs is lost: `pd.ini`, saves, the
ROM and all mods carry over, and the fork is the same game lineage.

1. Any install whose Check for Updates already works (every release since
   2026-09-02) takes **one line in `pd.ini`**:

   ```
   Mod.UpdateServer=https://github.com/StudioNirin/Imperfect-Dark/releases/download/dabs-mod-dev
   ```

   The old build then reads `update.txt` and `patchnotes.txt` from that base,
   sees a new release, verifies the SHA and replaces itself with the fork's
   binary. Its first what's-new popup shows every fork entry it has not
   seen. Afterwards the line can be deleted - fork builds point at this
   repository from birth.
2. An install too old for Check for Updates migrates by **replacing the
   executable itself**: rename the old `pd.x86_64*` binary out of the way,
   put this fork's release binary in under the old name, start it once.
3. Consequences to state up front: the game logs a warning while
   `Mod.UpdateServer` is set (it will replace itself with whatever that
   serves - tell players to only ever paste a URL they trust); the
   Stable/Dev channel switch looks for `<server>/dev` and `<server>/stable`
   subfolders and reports none with a flat URL (harmless, and gone once the
   line is removed); and the switch matters anyway, because old Dab builds
   and fork builds can never join the same netplay game - the hash check
   refuses them.

## 8. Environment notes and traps

- **`run/pd.x86_64` is a hard link to `build/pd.x86_64`, and linking a new
  build detaches it.** After every build, relink
  (`ln -f build/pd.x86_64 run/pd.x86_64`) or the playtest install keeps
  running the old binary. Never let the in-game updater replace `run/` in
  place, for the same reason.
- A version label that disagrees with `git log` is a missed reconfigure, not
  stale code; reconfigure before believing it.
- Published things are immutable: patch notes entries, and a `v*` tag's
  release. Corrections go in a new entry, not an edit.
- Patch-notes numbers are the fork's counter; entry numbers themselves keep
  counting up whatever the stamp reads.
- Windows behaviour is smoke-tested under Wine (11.21 verified): package
  with `--from-artifact`, boot the exe, check the title renders.
- `git commit -m` with double quotes inside the message breaks the shell;
  use `git commit -F -` with a heredoc.
- Screenshot-based verification: dialogs need the 1.5 s hold; per-instance
  binary dirs for multi-client tests (`cp -l`).

- **A dead menu button is usually a refused push, not a dead handler.**
  `menuPushDialog()` opens a page only while the menu's dialog stack has room
  (`MENU_MAX_LAYERS`, `MENU_MAX_DIALOGS` in `src/include/types.h`), and it used
  to refuse in silence - no log, no sound - so a live lobby's Simulants rows
  looked broken rather than full (netplay.md: "A live room's Simulants pages").
  It logs `menu: push of dialog ... refused` now; look for that line before
  reading a handler.

## 9. Open threads (as of 2026-11-09)

- The hosted lobby (`deploy/lobby/`) is built and container-tested but not
  deployed: it needs a public host or VPS with TCP 443 and UDP 27101 +
  27110-27141, and a domain to point the client defaults
  (`Net.LobbyServer`, `g_GhostNetUrl`) at. The auth mode choice (`open` vs
  `ghost`) is still the user's, and `pdghostd` is deferred.
- Player-facing migration text (section 7) is written up but not yet placed
  anywhere players will find it - the release page or a `docs/` link.
