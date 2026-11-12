# Settings, defaults and presets

## Digest (moved from CLAUDE.md, 2026-09-30)

The entry CLAUDE.md carried for this, verbatim; the code is the long form.

- **Defaults and the Settings Preset** — `src/game/modoptions.c` and `g_ModPresets` in `port/src/optionsmenu.c`: since 2026-09-09 a fresh pd.ini is stock Perfect Dark with the fixes on and the additions off (and since 2026-09-13 Smooth Text and Thin Text Outlines - "Clean Text Outlines" until 2026-09-12, and still that in pd.ini and in the code - are off too, in the defaults and in the Vanilla and Ghost Trials presets, so the text is the ROM's), because a tester met a game that jumped, rolled, tilted and flew (F was the Spectator); the fork's old defaults are the "Dab's Settings" preset, and a preset is applied on selection and remembered by nothing - the dropdown reads Custom once any covered setting differs. The third person camera has its own preset and is not covered; a pd.ini written by an older build keeps every value it has, including F


- **The five defaults that moved on 2026-11-11, and what a default can reach** — [settings.md](CLAUDE-notes/settings.md): `PLAYER_EXT_CFG_DEFAULT` (`src/game/mplayer/mplayer.c`) is the one macro behind all four `Game.PlayerN.CrosshairHealth` rows, so a per-player default is one edit; `Game.Language` is looked up **by code** among the packs this build embedded and falls back to English (US) with a log line, so its default is `"en-GB"` and never an index - and it is one setting for the whole game, however per-player the page it sits on looks; `Mod.AkimboTriggers` **rewrites the pad's binds** (`inputApplyAkimboTriggers()`: Fire off the right trigger onto Left Shoulder, the D-pad becoming the two fire-mode buttons), which is why it belongs to a fresh install and not to a migration; the Settings Preset dropdown now reads **Custom** on a fresh install, because every preset forces akimbo off and `menuhandlerModPresetMatches()` insists on it; and `Mod.SettingsRevision` was deliberately **not** bumped, so the way to hand an existing install one of these is to delete that one key's line from its pd.ini rather than the file - the binds, `[Net] Name=` and `DirectRecent=`, all of `[Video]` and `Mod.XblaPackage=` live in that file too

## A setting that is on online and off offline (Mod.KillFeed, 2026-10-12)

The kill feed (`port/src/killfeed.c`) was wanted on in online matches by default and, by the rule above, off offline
unless chosen. One key does both: Mod.KillFeed -1 (the default, "Online Only" in Extended Options > Dab's Mod >
Display) is on while `g_NetMode != NETMODE_NONE`, 0 off and 1 on everywhere. It is a picture setting of each
machine's own, so it is in no netrules.c list. Vanilla and Ghost Trials carry -1, Dab's Settings 1. The
Enhancements On/Off note (`Mod.EnhancementsSaved`) is the preset's fields in order: a note written before a field
was added has one value fewer, and `modEnhancementsSetOn()` now puts the new field's default in its place rather
than dropping the note (`modEnhancementsCapture()` had also never captured Sit In Chairs; it does now).

## Five defaults changed for a fresh install (2026-11-11)

The table in `src/game/modoptions.c` holds the mod options' defaults; the rest
live with their subsystem. What is on now, and where:

| key | default lives at |
|---|---|
| `Game.Player1..4.CrosshairHealth` = `CROSSHAIR_HEALTH_ON_GREEN` | `PLAYER_EXT_CFG_DEFAULT`, `src/game/mplayer/mplayer.c` |
| `Game.Language` = `en-GB` | `g_LanguageSetting`, `port/src/langpack.c` |
| `Audio.MuteWhenUnfocused` = 1 | `muteUnfocused`, `port/src/audio.c` |
| `Mod.Akimbo` = `MODAKIMBO_EVERYONE`, `Mod.AkimboTriggers` = 1 | `g_ModOptions`, `src/game/modoptions.c` |
| `Mod.MapMods` = `"*"` | `mapModsSetting`, `port/src/mod.c` (the Stage Loader's "Use maps from every installed mod") |

Three things this touched that the code does not say.

**A per-player default is one edit.** `g_PlayerExtCfg[MAX_LOCAL_PLAYERS]` is
initialised from a single macro, so the four `Game.PlayerN.CrosshairHealth` rows
in pd.ini all come out of that line. The Extended Options > Game page has a
Player dropdown and most of its rows are per-player, but **Language is not one of
them** - the handler says so in its comment, and `struct extplayerconfig` has no
language field - so "English (UK) for players 1 to 4" is one global setting.

**A language default is a code, not an index.** `langpackInit()` resolves
`g_LanguageSetting` with `langpackIndexOfCode()` and, finding nothing, logs and
uses English rather than nothing. So the default is `"en-GB"` and survives a pack
list that reorders itself. `--lang` still wins, and the packs are generated from
`lang/` alone (`build/langpacks.c`, tools/langpack/build.py) - not per ROMID -
which is why the PAL and JPN builds need no guard for this.

**Akimbo Triggers is a binds change, not a checkbox.** Turning it on runs
`inputApplyAkimboTriggers()` (`port/src/input.c`), which takes Fire off the right
trigger, puts it on Left Shoulder, frees the D-pad's four directions and binds the
left-hand and right-hand fire modes to them. As a fresh install's default that is
the point; aimed at somebody's existing config it would be a surprise, which is
the second reason (after the one below) that this is not a migration. `inputEnsure
Bind()`/`inputRemoveBind()` are idempotent, so running it every start is fine.

### Why nothing moved for anyone who already has a pd.ini

The game writes every registered key on exit, so an install that has run any
recent build has a line for all of these and reads those instead of the defaults.
`Mod.SettingsRevision` (`port/src/main.c`) is the mechanism for reaching those
installs, and it was deliberately left at 3.

So to give an existing install one of the new values, **delete that key's line**
and start once:

```sh
awk '!/^(MapMods|XblaMeshes|MuteWhenUnfocused|Akimbo|AkimboTriggers|Language|CrosshairHealth)=/' pd.ini > pd.new && mv pd.new pd.ini
```

Filter by key name and not by section - a written pd.ini carries five separate
`[Mod]` sections - and know that `CrosshairHealth` appears once per
`[Game.PlayerN]`, which is the whole point. Deleting the file works too and is
the wrong tool: `[Input.PlayerN.Binds]`, all of `[Video]`, `[Net]`'s `Name=` and
`DirectRecent=`, `ModDir=`, the chosen pack names and `Mod.XblaPackage=` (the
hand-set path to the Xbox 360 content) are in it as well. Progress is not: that is
`eeprom.bin`, beside it.

To see what a fresh install gets without touching an install at all, run once with
a scratch `--savedir` - it has no pd.ini *and* no eeprom, so nothing of anybody's
is involved - and read the pd.ini it writes.

### The Settings Preset now reads Custom on a fresh install

Every preset ends with "the ways of playing, off in every preset" and writes
`akimbo = MODAKIMBO_OFF` (`port/src/optionsmenu.c`), and
`menuhandlerModPresetMatches()` requires that value to name the preset. Akimbo on
by default therefore matches no preset, and the dropdown says Custom. That is the
honest answer rather than a bug: the config genuinely is not any preset's set. A
preset chosen still switches akimbo back off, which is the preset doing its job.


## Imperfect Settings (added 2026-11-12)

`Extended Options` opens a page called **Imperfect Settings**, one row above
**Dab's Mod Options** in that same list (`port/src/optionsmenu.c`: a
`MENUITEMTYPE_SELECTABLE` row carrying `MENUITEMFLAG_SELECTABLE_OPENSDIALOG` and
`(void *)&g_ImperfectSettingsMenuDialog`). It is for settings this fork *replaces*
rather than adds, so ours are not found by scrolling Dab's list.

It is a page of the same kind as the rest: a `struct menudialogdef` with a
`MENUITEMTYPE_END`-terminated item table, and its `next` dialog pointer set to
`&g_ExtendedDabsModMenuDialog` - the chain head of Dab's pages ("the one Extended
Options opens" says so at its definition). So the page turn carries on from ours
into Dab's, and nothing else in the tree had to change: `g_ExtendedDabsModMenuDialog`
is referenced by exactly one menu row.

**Only two rows have moved over**: Sprint and Combat Roll. The migration is one row
at a time, when something else touches a setting. Reshuffling every table in
`optionsmenu.c` beside a gameplay change is how a bind row ended up naming the
wrong setting and crashed Windows on a fresh install (crash-reports.md, "The Mod
Options bind rows name their binds, not their places in a table").

Rows are *moved*, not copied: a handler must live in one page. Item text is
`MENUITEMFLAG_LITERAL_TEXT`, so there is no text-bank work and the rows are English
until the language packs are redone, which is how new copy already behaves.

The two defaults that came with the page are fresh-install defaults like entry 74's:
`sprint` on, `roll` `MODROLL_EVERYONE` (`modoptions.c`'s positional defaults list -
**at the end of it**, never inserted into the middle).
