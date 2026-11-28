# Save format

## Digest (moved from CLAUDE.md, 2026-09-30)

The entries CLAUDE.md carried for this note, verbatim. The sections below are
the long form.

- **Saves, eeprom, where pd.ini lives, the migration** — [save-format.md](CLAUDE-notes/save-format.md)


`mpsetupfileSaveWad()`/`LoadWad()` take a version. The port's file has a version byte
and per-version block sizes (`MPSETUP_BLOCKSIZE_V1`, `_V2`, and `MPSETUP_BLOCKSIZE`
for version 3). Files stay in the base format unless a setup actually needs >8
simulants (version 2) or has a simulant stat slider moved (version 3, the sliders
appended after everything version 2 writes), so unmodified builds can still read
them; changing version re-encodes every block. `mpsetupLiveVersion()` in
mpsetups.c is where a new reason for a newer version goes.

Saved **weapon sets** are a file of the same neighbourhood, `mpweaponsets.bin`: a
version byte, a count, and per set a name the width of a setup's plus its six gun
numbers (`port/src/mpweaponsets.c`). Version 1 only, so an unknown version reads
nothing rather than half of it back, and the file is only ever written by
"Save Custom Set" and its delete - never by a match or a settings save. It is one
file for every mod, unlike `mpsetups.bin`, because a set names guns by their
weapon numbers and a setup names arenas and weapons by a mod's numbers
(`mpsetupSwitchFile()`).

## Save folder

`$S` is the directory the game writes into, but save files now live in its
`saves/` child, reached as `$V`. `fsInit()` in `port/src/fs.c` sets that up:
with no `--savedir`, `$V` is `$S/saves` and a one-time migration moves
`eeprom.bin`, the setup and weapon files, `modsaves/`, `ghosts/` and `exported/`
into it with `fsRename()`, skipping anything already there. With an explicit
`--savedir`, `$V` is that directory itself, so gates and players choosing their
own folder keep the flat layout they asked for. Config (`pd.ini`), update
backups, times files and recordings stay on `$S`.

Paths that must use `$V`: `EEPROM_PATH` and the `--eeprom-file` fallback in
`port/src/libultra.c`, setup and export paths in `port/src/mpsetups.c`, weapon
sets and filters, `MODGHOST_DIR` in `src/include/game/modghost.h`,
`MODMODE_SAVES_DIR` in `port/src/modmode.c`, and `challengeplus.bin` below.

## Challenge records

The tier byte is indexed by human count, not `PLAYERCOUNT()`: `g_Vars.players[]`
holds humans, while simulants live in `g_MpSimSlots[]` as chrs. The old guard
that threw away a whole mark when `PLAYERCOUNT()` passed four was for rooms with
more than four humans; `challengeRecordTier()` now clamps the tier instead, and
per-player bits are written only for the four slots the byte can hold.

Stock completions are bits in the GAME note. Challenges+ stars are *not*: that
note is 0xa0 bytes with only three bits spare, so `port/src/challengeplus.c`
keeps them in `$V/challengeplus.bin`, one file per save directory with a block
per agent keyed by `{deviceserial, fileid, agent name}`. Each block is 120
three-bit records (30 challenges x 4 player counts), 0 for no record and 1-4
for bronze through perfect; the file is versioned, and a missing or damaged
file reads as blank progress. `gamefileLoad()` and `gamefileSave()` call it;
the host's net verdict writes its own medal too, and the grader's tier is the
room's human count so it matches the tier the record is stored at.

`savebufferOr()` does no bounds checking. `stagenum` is stored in 7 bits.

**Everything lives beside the executable now** — saves, `pd.ini`, screenshots and
recordings — because a copy of the game should be one folder a player can open,
back up or move. `~/.local/share/perfectdark` is where it used to be and is still
the fallback for a copy that cannot write to its own directory: installed under
`/usr`, on a read-only mount, inside an app bundle. `fsChooseOutputDir()` makes
that choice for the player's own files and `fsInit()` makes it for the saves.

**The first run of a build that does this copies the old save directory across**
(`fsMigrateSaves()`), because otherwise someone who has played before starts with
a fresh eeprom, no unlocks and the simulant count back at four — which looks
exactly like the fork being broken. It copies rather than moves and never writes
over a file already there. It takes every file at the top of the old directory
rather than a named list — that directory is one the game itself chose and has
been saving into, and a save missed by an allowlist is exactly the failure this
exists to prevent — plus `ghosts/` and `exported/`. Screenshots and recordings
are left behind deliberately, being potentially gigabytes and nothing that stops
working. A copy that fails part way removes what it wrote, because nothing here
overwrites an existing file and a truncated eeprom would be loaded every run
from then on.

The old default was the working directory whenever a config happened to be
sitting in one, and that was never checked for writability — it is now, or a
read-only working directory silently swallows every save.
