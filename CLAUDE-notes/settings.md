# Settings, defaults and presets

## Digest (moved from CLAUDE.md, 2026-09-30)

The entry CLAUDE.md carried for this, verbatim; the code is the long form.

- **Defaults and the Settings Preset** — `src/game/modoptions.c` and `g_ModPresets` in `port/src/optionsmenu.c`: since 2026-09-09 a fresh pd.ini is stock Perfect Dark with the fixes on and the additions off (and since 2026-09-13 Smooth Text and Thin Text Outlines - "Clean Text Outlines" until 2026-09-12, and still that in pd.ini and in the code - are off too, in the defaults and in the Vanilla and Ghost Trials presets, so the text is the ROM's), because a tester met a game that jumped, rolled, tilted and flew (F was the Spectator); the fork's old defaults are the "Dab's Settings" preset, and a preset is applied on selection and remembered by nothing - the dropdown reads Custom once any covered setting differs. The third person camera has its own preset and is not covered; a pd.ini written by an older build keeps every value it has, including F


- **The five defaults that moved on 2026-11-11, and what a default can reach** — [settings.md](CLAUDE-notes/settings.md): `PLAYER_EXT_CFG_DEFAULT` (`src/game/mplayer/mplayer.c`) is the one macro behind all four `Game.PlayerN.CrosshairHealth` rows, so a per-player default is one edit; `Game.Language` is looked up **by code** among the packs this build embedded and falls back to English (US) with a log line, so its default is `"en-GB"` and never an index - and it is one setting for the whole game, however per-player the page it sits on looks; `Mod.AkimboTriggers` **rewrites the pad's binds** (`inputApplyAkimboTriggers()`: Fire off the right trigger onto Left Shoulder, the D-pad becoming the two fire-mode buttons), which is why it belongs to a fresh install and not to a migration; the Settings Preset dropdown now reads **Custom** on a fresh install, because every preset forces akimbo off and `menuhandlerModPresetMatches()` insists on it; and `Mod.SettingsRevision` was deliberately **not** bumped, so the way to hand an existing install one of these is to delete that one key's line from its pd.ini rather than the file - the binds, `[Net] Name=` and `DirectRecent=`, all of `[Video]` and `Mod.XblaPackage=` live in that file too

- **A new `g_ModOptions` field, and the string that holds them all (2026-11-14)** — the section of that name: `Mod.StartArmedAkimbo` defaults **on**, because the pair is what an Akimbo install already handed out and a default that changes what a player sees mid-match is a different decision from a fresh install's defaults; and the enhancements blob (`MODENHANCE_EXTRA_*`, `MODENHANCE_SAVED_VERSION`) grows a field by appending it to the enum and chaining a second "one field short" clause in the read path, so no saved note is dropped - which matters because a dropped note leaves the settings at Vanilla.

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


## A new `g_ModOptions` field, and the string that holds them all (2026-11-14)

Start Armed's pair became its own setting, `Mod.StartArmedAkimbo`
(`weapon-sets.md`, *Start Armed, one gun or two*). Two things about it are
settings decisions rather than feature work.

**Its default is on, which is against the fork's rule.** A fresh pd.ini is stock
Perfect Dark with the fixes on and the additions off, and Akimbo itself is on for
a fresh install (the 2026-11-11 list below) - so the pair is what an install
without a line for this key already did, and a default of off would have reached
into existing players' matches and handed them one gun instead of two. The rule
is about what a *new* install plays like; this key is not an addition the player
has to discover, it is a decision they already made by turning Akimbo on. The
`g_ModOptions` initialiser in `src/game/modoptions.c` is positional, so the new
line goes where the field is in the struct, not at the end.

**The enhancements blob is a second store nobody thinks of.** `port/src/optionsmenu.c`
saves the player's Mods settings as one string when enhancements are turned off
(everything goes Vanilla) and restores them when turned on. Its shape: every s32
of `struct modpreset` from `jumpheight` on, then the `MODENHANCE_EXTRA_*` fields,
as a versioned comma list. The read path drops a note whose version or field count
it does not recognise - *"saved settings unreadable ... left as they are"* - which
means a player's whole Mods page falls back to Vanilla, so adding a field is the
whole risk and it is not in the field's own code:

- Append the new member to the enum, **before `_COUNT` and last**. The extras sit
  at the end of `values[]`, so an old note is short by exactly that one field and
  the fix is one appended default rather than a `memmove`.
- Then chain a clause after the kill feed's, both asking
  `count == ARRAYCOUNT(values) - 1`: the kill-feed clause inserts its field and
  bumps `count`, so a note that predates both passes the first and then the
  second. `MODENHANCE_SAVED_VERSION` stays **2** - the version means "the shape
  of the list", and the clauses exist precisely to absorb shape changes without
  paying for them in lost settings.
- Do **not** grow `struct modpreset` instead. `MODENHANCE_PRESET_FIELDS` is
  arithmetic on that struct, so every field count in every saved note moves, the
  insert lands in the middle, and the migration has to know where each field went.

A default reaches only a pd.ini with no line for it (the rule the 2026-11-11
section spells out), so `Mod.SettingsRevision` was not bumped: nobody's existing
file has the key, so everybody without one gets the default, and nobody with one
is overruled.

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

## A command holds several bindings, and the two pages that show them

`binds[player][contkey][INPUT_MAX_BINDS]` in `port/src/input.c`, four slots, and
pd.ini writes them one line comma-separated (`CK_0100=LEFT_SHIFT,
JOY1_LSTICK`). So a command bound on both devices is one line with two names, and
it has always been able to be: the Sprint default ships that way.

Nothing records which slot belongs to which device, and the slots take codes from
one space that is itself laid out by device - scancodes, then the mouse buttons
from `VK_MOUSE_BEGIN`, then each controller's from `VK_JOY_BEGIN` - so
`inputBindFamily()` is range arithmetic and `inputBindFamilySlot()` answers "the
slot that is already this device's, else the first empty one, else -1". Those two
are the whole split, they take the four slots as an array, and
`--test-bindfamilies` (`tools/ci/bindfamilytest.sh`) runs them over arrays built
in the test, so a player's bindings are nowhere near it.

The Key Bindings page became two: **Keyboard & Mouse Binds** and **Controller
Binds**, both listing every command from the same `menuBinds[]`, each row showing
only its own device's binding or `NONE`, and Delete on one page cannot strip the
other's. A row is a dropdown with **one** option rather than the four slots, and
the page picks the slot - which is what makes rebinding a pad button replace it,
binding the other device add to the same command, and nobody have to know what
"slot 3" means. `inputBindFamilySlot` returning -1 is said in the log rather than
worked around: all four slots being the other device's is a state the player has
to make room in, and quietly overwriting a binding they can see on the other page
is not the answer.

The rows find their command by which array they live in (`bindDeviceRow`), not by
a positional index handed to the row: each page carries two reset rows, Back and a
terminator after the command rows, so an offset alone says nothing about whether
it names a command, and a row pointing past the table it reads is the crash this
file has already had (crash-reports.md).

**Left to delete:** the single page the two replace still exists unreferenced -
`g_ExtendedBindsMenuItems`, `g_ExtendedBindsMenuDialog`, `menutextBind`,
`menuhandlerBind` (`menuhandlerOpenBindsMenu` is gone, which is the only reason
the build is warning-free). It is inert, not slow, but it is two ways to edit the
same thing sitting in one file, and `menuhandlerBind`'s "first filled slot" fix
belongs to it. Deleting it is a line-range job across interleaved code: resolve
every boundary first and refuse to write if they overlap, which is the guard that
stopped one such attempt from corrupting this file rather than only warning.
