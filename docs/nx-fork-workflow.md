# Working on this fork

The short version of how a change gets from an idea to players. Upstream's own
notes (`CLAUDE-notes/`, `docs/`) still explain the engine itself; this page is
about *this* repo: where things run, what to type, and what CI does. The full
accumulated reference - how versions and updates work, how to change a file,
how players migrate - is `nx-handbook.md` next door.

## Layout

| Path | |
| --- | --- |
| `build/` | the Linux dev build: `cmake --build build -j32` (~2 min clean, seconds after one file) |
| `run/` | a scratch install to play in: the binary and `data/` symlinked from `build/` and `payload/`; its `pd.ini` and saves stay here |
| `payload/` | gitignored and never in the build: the ROM and third-party content, laid out as the game expects (`payload/README.txt`) |
| `dist/` | packages `tools/package/package.sh` writes (gitignored) |
| `deploy/lobby/` | the lobby's container and TLS front, see its README |
| `reference/` | archived older builds, for comparing |

To build and play:

```sh
cmake --build build -j32
cd run && ./pd.x86_64 --log      # pd.log records what the game says
```

`build/data/pd.ntsc-final.z64` is a symlink into `payload/`, so the dev build
and `run/` share one ROM.

## A content change (no rebuild)

Anything that lives in `payload/` - mods, texture and model packs, added
content - the game reads at start; drop the file in `run/` (or `payload/` for
the packaged copies) and restart. The game converts GoldenEye and the XBLA
releases on first start, so that one start is slow.

## An engine change

Behind a `Mod.` setting where it needs to be switchable, the setting is five
touch points (copy any existing one): `src/include/game/modoptions.h` (field),
`src/game/modoptions.c` (default, getter), `port/src/main.c` (`configRegister
Int("Mod.X", ...)`), `port/src/optionsmenu.c` (the menu row), and the consumer.
Read the matching `CLAUDE-notes/` file before touching the area; netplay work
means `netplay.md`.

Build, then run it two ways:

```sh
cmake --build build -j32
cd run && ./pd.x86_64 --log                      # by hand
SDL_VIDEODRIVER=offscreen tools/ci/netteamcolourtest.sh   # or the nearest gate
```

The `tools/ci/net*test.sh` gates drive real clients against a local `pdlobbyd`
and screenshot the menus (the lobby script's `--net-lobby-*` and
`--net-test-lobby-*` arguments; see `netlobbyuitest.sh` for the pattern).
Everything at once is `tools/ci/netallgates.sh` - long, and several gates are
load-sensitive, so run it alone. If the change is something players will
notice, add a `patchnotes.txt` entry: one `notes <n+1> <date>` block, the
number one above the highest already pushed, never edited afterwards.

Then push. CI (`.github/workflows/dabs-mod.yml`) builds Windows and Linux and
force-updates the `dabs-mod-dev` prerelease; the in-game Check for Updates
offers it to everyone running a fork build. That is the whole release train;
there is nothing else to run.

Every version a player sees is a commit hash plus a release stamp,
`nx-<n>-<date>`, where `<n>` counts this fork's own `patchnotes.txt` entries
from 1: the `# mod-base 69` line in that file marks the last entry that
predates the fork's own work, so entry 70 stamps `nx-1` and every new entry
raises the number by one. The stamp rolls forward on its own whenever an entry
is added (the game derives it from the notes baked into the build; the release
job derives it from the same file for the release's version). A fresh stamp is
therefore exactly "newer fork content to read" - the hash is what changes for
any commit, and it alone decides which builds may play together.

## Packaging to hand someone

```sh
tools/package/package.sh --linux --clean            # shareable
tools/package/package.sh --linux --full             # with payload/, private
gh run download <id> -R StudioNirin/Imperfect-Dark -D /tmp/art
tools/package/package.sh --windows --full --from-artifact /tmp/art
```

See `tools/package/README.md`. `--full` prints and records exactly what went
in, so the same tree still makes the clean one.

## Playing online

The lobby container is `deploy/lobby/` (its README covers the VPS, the domain
and the firewall). `pd.ini`'s `Net.LobbyServer` points a client anywhere; LAN
and Direct Connect need no server at all.

## What DabDavis did upstream

`.github/workflows/upstream-watch.yml` files an issue listing commits this
branch lacks. When it has one: read the list, and take what is worth taking
with `git fetch dabs dabs-mod && git merge dabs/dabs-mod` (or cherry-picks).
Anything that changes netplay code means everyone on the fork's build plays
apart from anyone still on his - that is the deal with this fork, and the
lobby is why we run one.
