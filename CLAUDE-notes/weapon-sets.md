# Custom weapon sets

## Digest

Custom weapon sets: why a saved set keeps its guns rather than the page's rows;
the Combat Simulator's list and the block that never moves; why the player's
sets lead that list and what "the first entry" stopped meaning; why the six
weapon slots are locked and what unlocks them; where the online room's Weapons
page actually lives; what the wire carries and what it does not carry yet.

Random weapon groups: the four preset buttons as rules, and the bug that hid in
one of them; why a saved group keeps gun names rather than row numbers too; why
the page is a level deeper than the row that opens it; what Start Armed's Random
reads now, and the two 64-bit places that were quietly dropping the fork's rows
past the 64th.

A weapon slot holding a random group: why the marker cannot live in
`g_MpSetup.weapons[]`; who re-applies the weapon set behind the game's back, and
what that did to the first version of this; why a match is rolled exactly once and a client never
draws; what the saved setup file and the saved set each had to grow to hold (5 and
2), and the fact that the two halves of a setup block are one hand on the same bit
stream.

Start Armed, one gun or two: why the pair is its own setting rather than
`Mod.Akimbo`; why keeping the values 0/1/2 is what made the rename free; the
ammount rule and the mines' exception to it, and the hidden tick that still
doubles a mission's gun; that a dropdown popup is a text loop and cannot hold a
widget, and that row measurement re-runs every frame, so the note above it about
rows never appearing was wrong; why Random Five's sixth slot came to be a
GoldenEye Remote Mine, and why the check for it had to unlock a row to mean
anything; how the enhancements blob grows a field without dropping a player's
settings.

## A saved set keeps its guns, not the page's rows

`g_MpSetup.weapons[]`, the six slots the weapons page shows, are **option
indices**: which row of the page's list a slot is on, where row 0 is
`MPWEAPON_NONE` and the rows past the stock guns belong to whatever mod is
mounted (`g_MpWeapons[]`, and `moddata.c` rewrites those rows). So a saved set
that stored the six values as they stand would mean another gun on another
machine, or on this one with another mod mounted.

What the game's own sets store is weapon numbers, and `mpApplyWeaponSet()`
(`mplayer.c`, the `ptr` loop) turns those into option indices by looking each
one up in `g_MpWeapons[]`. A saved set stores weapon numbers
(`struct mpweaponsetentry.slots`) for exactly that reason: `mpWeaponSetsSave()`
converts option index to gun on the way in, and reading a set back is
`mpSetWeaponSet()` and nothing else - the same call the `Set:` dropdown makes -
so the six rows fill themselves and nothing outside that lookup needs to know
a set is a player's rather than the game's. `--test-mpweaponsets` holds this
down twice: "the set keeps its guns, not the page's rows" and "picking the set
puts the six slots back".

One set also carries a **second group of six** (`unk0c`-`unk11` in the decomp
layout): what the set hands out when its own guns are not earned yet.
`mpApplyWeaponSet()` reads `slots[]` when all four `requirefeatures` are earned
and the alternative otherwise, and `challengeIsFeatureUnlocked(0)` is true - so
a player's set, whose requirements are all 0, always reads `slots[]`, and its
alternative group is left `WEAPON_DISABLED`.

## The list's block never moves

`g_MpWeaponSets[]` is 64 rows and holds the stock twelve, a borrowed mod's
(`modborrow.c`), GoldenEye's (`gexplus.c`), and now `MPWEAPONSET_MAXSAVED` rows
of the player's own. Each group records where it put itself and puts nothing
after itself, and GoldenEye's already lifts its own choice and sets it down
again when the list's length moves (`geSetsNumMoved()`) - so a third group that
grew on a save and shrank on a delete would leave those records pointing at
rows that are no longer theirs. That is the Mod Options bind rows reading a
table by position in a new costume (crash-reports.md), and the first version of
this code did it: lifting the block out and setting it down again also
truncates whatever was appended after it.

So the block is `MPWEAPONSET_MAXSAVED` rows wide from the first frame
(`mpWeaponSetsPlace()`, called once at startup after the mod's sets are in) and
its size never changes again. Saving and deleting rewrite one row in place. A
row with no set behind it is hidden, and **both** halves of the game's test for
a set worth showing have to say no - `requirefeatures[0] = MPFEATURE_NEVER`
*and* an alternative group of `WEAPON_DISABLED`, because a zeroed alternative is
`WEAPON_NONE`, which is a gun, and `mpCountWeaponSetThing()` offers a set whose
first alternative entry is any gun at all. The gate failed on precisely this the
first time it was run.

## Where the room's Weapons page lives

`handlerSetupWeapons()` (`netlobbymenu.c`) pushes `g_MpWeaponsMenuDialog` -
whose items, `g_MpWeaponsMenuItems`, are in **`src/game/mplayer/setup.c`**, the
decomp side, and are the same table the Combat Simulator opens. There is no
online copy of the page to keep in step, which is why Start Armed (a room rule,
`Mod.StartArmed`, `NETKEY_SYNC` in `netrules.c`) moved there in one edit and
appeared in both places. Rules are keyed rather than paginated: nothing in
`netrules.c` names a menu page, so moving a rule's row moves nothing on the
wire, and neither page has a host check (`handlerSetupPage()` only hides pages
in co-op setup).

Everything this fork adds to that page lives inside its existing
`#ifndef PLATFORM_N64` block, and the whole custom-set code is inside it:
`setup.c` is compiled for the N64 build too, where `WEAPONSET_CUSTOM` is a fixed
`0x0e` rather than `g_MpNumWeaponSets + 2` and there is no borrowed-mod list to
append to.

## What the wire carries, and what it does not yet

`RULES` carries `g_MpSetup` including the resolved `weapons[6]` **and**
`s32 weaponsetnum` and the random filters (`netproto.h`). A client applies the
slots it is given and never re-derives them from a foreign index:
`netRulesRead()` range-checks the received number against
`MP_MAX_WEAPONSETS + 2` and `netRulesApply()` maps an over-list number to
`Custom` with a log line, which is what a saved set already survives - the
client plays the host's guns whatever it calls them. Two things remain:

- a saved set's **name** does not travel, so a client shows `Custom` where the
  host typed something better; and
- a host's saved-set index that happens to fall *inside* the client's own list
  makes the client's `Set:` row display its own set with that number while
  playing the host's slots. Unavoidable with a bare index, and the name is the
  fix: a non-empty name is also the signal "this index is the host's, do not
  read it against my own list".

Rooms already refuse a joiner on `NETREFUSE_PROTOCOL` and `NETREFUSE_BUILD`, so
appending a field to a message cannot split a room that could otherwise have
formed; `NET_PROTOCOL_VERSION` still goes up with it.

Both of those are fixed (protocol 34 was the name, protocol 35 the filters):
`weaponsetname` travels, and nothing on a client renders it because the room's
Settings page belongs to the owner alone. `randomfilters` is now one byte per row
of `g_MpWeapons` rather than a u64 - see *The two 64-bit holes* below.

A fourth thing the wire does not carry, and does not need to: a group's **name**.
The filter array travels and the group is a thing only the page shows, so a
client receiving the host's list has nothing to label it with - the same rule as
"no field without a receiver", which is why the saved-group names are not on the
wire at all. A live room's own change detection is a different matter, and was
broken: see *A live room has to notice the list changed*.

## What the page shows

`Save Set` and `Remove Set` are rows of the weapons page, and both are keyed off
the table index of the set being looked at (`g_MpWeaponSetNum`, which the
`Set:` dropdown's `GETSELECTEDINDEX` reaches through `mpGetWeaponSet()` as an
*offered* index - the two are not the same number):

- Save Set is there when the current set is Custom or one of the player's own,
  and is **disabled** while the slots still match a saved set
  (`mpWeaponSetsIsDirty()`) - picking a set and leaving it alone has nothing to
  save again. Pressing it puts down a name page (`menuPopDialog()` then
  `menuPushDialog()`, as Save Settings does) rather than stacking one, since a
  room's setup pages already sit a layer deeper (netplay.md, the
  `menuPushDialog()` cap that is silent when it refuses).
- Remove Set is there only for the player's own sets, and leaves the six slots
  holding those guns with the page back on `WEAPONSET_CUSTOM`: the name goes,
  the loadout stays, and Save Set comes back to give them another.
- The `CX: ` mark is on the row only. `struct mpweaponsetentry.name` keeps what
  the player typed - that is what a wire field and a management list want - and
  the prefix is composed per row into `s_Shown[]` because `langAddPortText()`
  keeps the pointer it is handed, so one shared buffer would leave the whole
  block showing whichever name was written last. Deleting re-puts **every** row:
  the sets above the deleted one move up, and writing only from the deleted
  index down leaves the rows below showing sets they no longer hold (the gate
  caught exactly that).

## Why the six slots are locked, and what unlocks them

The six weapon rows are not disabled by their handler - `menuhandlerMpWeaponSlot`
has no disabled check, which is what makes this hard to find from the setup
side. **Menu.c routes their input away**: the rows carry
`MENUITEMFLAG_MPWEAPONSLOT`, and `menu.c`'s per-row input choice sends them a
`spd8` (an empty input set, so a click lands nowhere) for as long as the set
being shown is not Custom - `mpGetWeaponSet() != func0f189088()`, and
`func0f189088()` is `mpCountWeaponSetThing(MP_NUM_WEAPONSETS + 2)`, the offered
index of `WEAPONSET_CUSTOM`. A named set's slots are a picture of what
`mpApplyWeaponSet()` will put back, so the game refuses the edit rather than
letting one be silently undone.

The rule is `mpWeaponSlotsAreLocked()` (`mplayer.c`, beside the function it
compares against), and it lets the player's own sets through as well as Custom:
editing a set they saved is the point of saving it. The dropdown keeps naming
the set - it is not switched to Custom on the first edit, which would throw the
name away for the sake of an edit - and `mpWeaponSetsIsDirty()` is what says
whether what is on screen still is that set, which the Save Set row shows by
greying out. `--test-mpweaponsets` pins all three cases: a saved set unlocks
them, one of the game's sets does not, Custom does.

So Save Set means two things, and which one it is depends on what is on screen:
for Custom it asks for a name and appends; for the player's own set with its
slots moved it writes the guns **back into that set** (`mpWeaponSetsUpdate()`)
and never asks, because the name is already chosen and a second entry with the
same name and other guns is not what the button pressing means. Renaming is
still Custom's business: pick the set, and save as Custom under the new name.
- The name on the wire, and `Custom (my pistols)` in the client's rules page.
- `mpsetups.bin` has an export/import pair (`mpsetups-ext.bin`); the weapon
  sets' file has no such thing, and sharing sets between machines means copying
  `mpweaponsets.bin` by hand.

## The player`s sets lead the list, and what that quietly meant

The list has no order of its own: it is the order of `g_MpWeaponSets`, and both
directions of its numbering walk that order - `func0f188f9c()` counting on
through the shown rows to turn a row number into a table index, and
`mpCountWeaponSetThing()` (which is what `mpGetWeaponSet()` is) counting back to
go the other way. So showing the player`s sets first means passing over their
block first **in both**, and the rows themselves stay where they are: a saved
setup keeps its set by number, `mpSetWeaponSet()` of a stock row still means
that stock row, and the sets a mod borrows and GoldenEye`s keep the positions
they recorded for themselves. The predicate both walkers duplicated is
`mpWeaponSetIsShown()` now, kept as the game wrote it - `!= WEAPON_DISABLED` and
its `@bug?` comment and all.

Two traps, both found by the gate rather than by reading the code:

- **Counting under yourself is not counting the block.** The block sits
  *numerically above* the game`s own rows in the array, so a "how many shown rows
  come before table row 0" loop sees nothing at all - block included - and every
  stock row came back with the rank of one of the player`s sets. The block has to
  be counted over the whole list, and only the same-kind rows counted under the
  row being asked about.
- **"The first entry" is no longer a default.** `mpInit()` ends with
  `mpSetWeaponSet(0)`, and `menuhandlerMpQuickTeamOption` clamps a stale
  selection the same way: with the block first, every fresh setup would have
  started on whatever the player saved first, and a quick-team change could drop
  a room quietly onto one of their sets. Both now ask
  `mpWeaponSetStockOfferedFirst()` - the first shown row that is not one of
  theirs. Any future "pick the first set" is the same question.

With the store empty - every CI gate, which runs a scratch `--savedir` - the
block holds no shown rows, both walkers reduce to the walks they were, and the
ordering is a no-op. So the ordering`s own coverage is `--test-mpweaponsets`:
the player`s sets are the list`s first rows, the numbering round-trips on every
shown row (`func0f188f9c(mpCountWeaponSetThing(x)) == x`), the game`s sets still
follow them, and a default lands under the block.

`--mp-weaponset` (`pdmain.c`) is still an offered-list index, so on a machine
with saved sets it counts theirs first: a debugging knob, and no gate uses it.

## Not done

- A saved set`s **name** does not travel to a client yet, and the index it
  travels by can collide with the client's own list (above, "What the wire
  carries").
- `mpsetups.bin` has an export/import pair (`mpsetups-ext.bin`); the weapon
  sets` file has none, so sharing sets between machines means copying
  `mpweaponsets.bin` by hand.

## Three menu facts the name page paid for (2026-11-13)

**A keyboard row cannot close the dialog it sits in.** `menuPopDialog()` walks the
menu's layers, and from a keyboard row's handler it takes the current frame down
under itself - `menuUpdateCurFrame` through `menuPopDialog`, and the crash looks
the same whether it comes from `MENUOP_SETTEXT` or `MENUOP_SET`, which was assumed
otherwise for one round here. The gate drives both operations
(`menuhandlerMpSaveCustomSet` under `MENUOP_SET`), so it is what found this rather
than a reading of menu.c. Closing a page goes through a selectable row, which is
why the overwrite question's Yes row carries
`MENUITEMFLAG_SELECTABLE_CLOSESDIALOG`.

**A selectable row with no `MENUOP_CHECKHIDDEN` case is never hidden.** The
question is three rows and only the label was hidden, so the page offered
`Yes`/`No` about a collision that had not happened, and pressing them did nothing
visible. A row that should disappear has to be asked, each of them, and
`mpWeaponSetOverwriteIsAsking()` is named and non-static so the gate can assert the
predicate rather than the screenshot having to.

**Push on top of the page you came from; pop only when leaving it is the point.**
This page copied `menuhandlerMpSaveSetupCopy` ("Save a Copy"), which pops then
pushes - correct for a copy, where the page you were on is done with. Here it meant
closing the name page landed the player on the main menu with the Weapons page gone
under them. `menuhandlerMpSaveSettings` two rows away is the save shape: push, and
leave what is underneath. Worth checking which of those two a new page is like
before copying either.

**Any name already in the store asks, the set's own name included.** The first cut
saved silently when the page was standing on that very set, on the reasoning that a
page asking about its own name asks twice; the owner's call was that overwriting a
set's guns is worth a question whoever typed the name, and the question's Yes is
first so the cursor lands on it.

## The overwrite question was built twice and neither shape worked (2026-11-13)

Both shapes are gone; the page now states the rule instead of asking. Recorded
because three rounds went into it and the reasons are not visible in the code.

**A row cannot appear inside an already-laid-out page.** *(This diagnosis is
wrong; the failure it explains was real. See the correction under* Start Armed,
one gun or two *below: the measurement this rests on re-runs every frame.)*
`MENUOP_CHECKHIDDEN` is consulted in `src/game/menu.c`'s row measurement, where a
hidden row is measured to `*width = 0; *height = 0`. That is done when the dialog
is laid out, so three rows hidden at open never get a size back when a collision
is detected later - the flag was set correctly, nothing drew, nothing wrote, and
the player saw a silent failure. Pages that do reveal rows do so after the player
activates a row, which re-lays-out; a keyboard commit does not.

**A keyboard row cannot change the dialog stack in either direction.** Popping its
own dialog from `MENUOP_SETTEXT` or `MENUOP_SET` crashes in `menuUpdateCurFrame`;
pushing a dialog from the same place crashes in `dialogCalculateContentSize`
measuring text for a page mid-edit - both symbolised, both found by the gate
driving `MENUOP_SETTEXT`, which is the only operation a keyboard row is ever given
(`menuitem.c`, OK at line 1722). So the game's own `menuhandlerMpSetupName` pushing
`g_FilemgrFileSavedMenuDialog` from a `MENUOP_SET` case is code that has probably
never run in anybody's copy, and is not evidence that the shape works.

**What shipped instead:** one `Save Set` row, the name page, and a line under the
keyboard - *"Entering an existing custom set name will overwrite that set."* A name
already used writes that set with the guns on screen; a new name adds one. No
question, because the store holds only this player's own sets, so nothing but a set
they made themselves is reachable by typing a name, and the name is the thing they
just typed on purpose. A name matching one of the game's own sets cannot collide
with anything (those are not in the store) and shows as `CX: Pistols` beside them,
which is what the prefix is for.

Ask-before-overwrite is not a bad idea, but in this menu it has to be a dialog
pushed from a **selectable** row, before the name page opens - which is the
two-row shape (Save Set / Save Set As) the owner rejected for failing silently when
a taken name was typed into Save As. Either the ask comes before the typing, or the
page says what the typing does. It cannot come after.


## Random weapon groups, and the curated list (2026-11-14)

`g_MpWeaponSetRandomFilters[]` - one byte per row of `g_MpWeapons` - is the list
the `Random` weapon set draws the arena's six slots from (`mpSetRandomWeapons()`),
and since 2026-11-14 also the list Start Armed's `Random` draws from
(`mpGetSpawnWeapon()`). One list, two readers, edited on one page.

- **The four presets were rules, not buttons.** "Select Dark / Classic / All /
  None" were the last four rows of the checkbox list, each writing the whole
  array and vanishing - a selection a player built by hand could only be rebuilt
  by hand. They are `mpRandomGroupBuiltInBits()` (`setup.c`) now, four rows of the
  new page's `Group` dropdown alongside the player's saved groups, so the dropdown
  and the checkboxes describe one thing.
- **Dark was selecting GoldenEye's guns.** Its rule ended in `i >=
  MPWEAPON_SHIELD`, written when `MPWEAPON_DISABLED` was the table's last row; the
  fork's table runs on through `MPWEAPON_GE_FIRST` to row 80, so "Dark" switched on
  every GoldenEye row including the cheat guns. Invisible for years because those
  rows are `MPFEATURE_NEVER` and so absent from `mpGetNumWeaponOptions()` - the
  checkbox list never showed a row it had just selected. Any rule written as
  "everything from X up" against this table should ask what has been appended.
- **A saved group keeps gun names, for the same reason a saved set does.** The
  store (`port/src/mpweaponfilters.c`, `$S/mpweaponfilters.bin`) serialises the
  selection as the `weaponnum`s it selects and expands back by looking each one up
  in `g_MpWeapons[]`, because a mod's import rewrites those rows from the top
  (`moddata.c`) - see *A saved set keeps its guns* above. `--test-randomgroups`
  proves it by shifting every row's weapon number along one (a bijection, which is
  the shape of an import) and checking the group still selects the same guns and
  not the rows it was saved from.
- **The page is a level deeper than the row that opens it, on purpose.** Buttons
  that set the list and a checkbox list that shows it cannot share a page without
  the list lying about what is in use, so the row opens a page (Select guns /
  Group / Save group / Remove group) and the page re-derives the group in use on
  `MENUOP_OPEN` (`mpRandomGroupInUse()`: saved groups first, then the built-ins,
  else `Manual`). `Manual` is a real row because a dropdown cannot display
  "nothing", and picking it changes nothing - which is what "leave my boxes alone"
  has to mean. A hand-ticked box sets it immediately.
- **Three doors, one page.** `g_MpRandomWeaponsMenuDialog` is pushed from the
  Combat Simulator's weapons page, from an online room's (they are the same
  dialog: `handlerSetupWeapons` pushes `g_MpWeaponsMenuDialog`, netlobbymenu.c -
  so the room needs no row of its own), and from Mods: Player under Start Armed,
  because that page is where a solo or co-op player sets the roll that adds a gun
  to a mission's kit and they should not have to enter the multiplayer menu to
  reach the list. It carries no `MENUDIALOGFLAG_MPLOCKABLE`: the popup it replaced
  does not either, and `menu.c` blocks flagged dialogs for a locked-out player.
  The two doors' rows do not hide on the same test, and should not: the weapons
  page's comes out for **either** reader of the list - the random set, or Start Armed
  being on Random, because that page carries the Start Armed row too and the roll
  reads the same list (`mpGetSpawnWeapon`) - while the Mods: Player row comes out for
  Start Armed alone. A row on a page hides on what that page is standing on.
- **The row that opens it says what the list is.** `RX Selection: All`,
  `RX Selection: RX: Pistols`, `RX Selection: Manual` - from
  `mpMenuTextRandomWeaponGroup()`, on both rows that open the page. Named from the
  list (`mpRandomGroupInUse()`) and not from the page's own dropdown state, because
  the row is drawn on pages whose player never opened the page, and after a restart,
  when only the list comes back; and a saved group is asked before the built-ins, so
  a group that happens to spell `All` is named as itself. Getting there cost a menu
  fact: the rows were `MENUITEMFLAG_LITERAL_TEXT`, and **a literal row can hold no
  text function** - see `languages.md`, same list as `param2`'s lookup.
- **Nothing selected means nothing handed out.** `mpGetSpawnWeapon()` intersects
  the curated list with `mpCanRollSpawnWeapon()` - so the list can only narrow the
  roll, never widen it past `MPFEATURE_NEVER` or a stage's own set of guns - and a
  zero-candidate list returns -1 *before* the roll. That ordering is load bearing:
  the empty answer consumes no rng, so a peer that drew anyway would take a
  different number of draws from the stage's stream (netplay.md, the split stream).
  `--test-randomgroups` asserts both halves, including that `g_RngSeed` is
  untouched - the trick player.c already uses to keep one roll off the stream.

### The two 64-bit holes, and why they were invisible

`NUM_MPWEAPONS` is 81 in this fork (`constants.h`: the GoldenEye rows, a ROM hack's
pistols, the cheat guns). Two places carried the list in a `u64`:

- **the wire** - `RULES`' `randomfilters`, written, read and applied with
  `i < NUM_MPWEAPONS && i < 64`, so rows 64-80 were never sent and a client kept
  its own bits there;
- **the saved setup** - `pack/unpackWeaponSetRandomFilters()` (`mplayer.c`), so
  those rows came back as the boot default (all on) after a restart, silently
  undoing anything a player had switched off.

Neither showed, because nothing on a client *read* the array: the client writes the
host's six arena slots straight through (`netrules.c`) and never re-runs
`mpApplyWeaponSet()`. Give it a reader inside the sim - which `mpGetSpawnWeapon()`
is, called from `playerSpawn()` and `bot.c` on every peer - and both become a
mismatch in what peers spawn holding *and* in how many draws they take. So:

- `RULES` now sends `u8 randomfilters[NUM_MPWEAPONS]` (protocol 35).
- The setup file grew to `MPSETUP_VERSION_RANDOMFILTERS` (4), which writes one bit
  per row. The older versions are still written when they lose nothing, decided by
  `mpNeedsWideRandomFilters()` alongside `mpNeedsSimStats()` in
  `mpsetupLiveVersion()` - the oldest format that loses nothing of the live setup.
  Reading a version < 4 sets rows 64+ to selected, since that is the only state
  such a file could describe, and doing it explicitly is what keeps a decoded block
  equal to a live setup for the re-encode walk in `mpsetups.c`.
- `mpsetupfileSaveWad()` ends with a check that the block's bits fit
  `MPSETUP_BLOCKSIZE`, and says so at `LOG_ERROR` if not. `savebufferOr()` writes
  wherever `bitpos` points, so a format that outgrows its block was silent.

### A live room has to notice the list changed

`dialogRoomSettings` (`netlobbymenu.c`) re-sends the room's settings on close only
if its snapshot differs: `g_MpSetup`, the bots, co-op, and `settingsExtra()`. The
filter array is in none of those - not a config key, so not in
`netRulesSyncHash()`, and not in `g_MpSetup`. A host who unticked a gun in Room
Setup > Weapons > Random Weapons had changed what `RULES` would carry without
anything deciding the room had changed, so no `netLobbySendSettings()` and no new
`RULES`. Invisible while nothing on a client read the array; a real difference now.
`settingsExtra()` hashes `g_MpWeaponSetRandomFilters` for that reason.

The general shape to keep: **the room's change detection is a snapshot
comparison**, so any new editable thing that is not a config key and not in
`g_MpSetup` has to be added to that hash or the room will not send it.

### A confirm is fine from a selectable row

The delete confirm on the group page is a `MENUDIALOGTYPE_DANGER` dialog with two
`MENUITEMFLAG_SELECTABLE_CLOSESDIALOG | ..._CENTRE` rows, "No" with a NULL handler
and "Yes" with the one that deletes, pushed from the Remove row's `MENUOP_SET`. The
model is the texture pack delete (`optionsmenu.c`). What cannot pop a dialog is a
*keyboard* row - it is only ever given `MENUOP_SETTEXT` (the name-page sections
above) - and that is the failure the earlier attempts at a confirm hit. The index to
delete is stashed in a static when the asking row pushes, because the Yes handler is
a dialog away from anything that knows which group was in use.

## A weapon slot holding a random group (2026-11-14)

A slot on the weapons page can name a saved group instead of a gun: the dropdown
offers the groups below the guns, the row reads `RX: Automatics`, and the gun the
arena gets is drawn from that group at the start of each match
(`mpRollRandomWeaponSlots()`, `mplayer.c`). The rules this took to find:

- **The marker cannot live in `g_MpSetup.weapons[]`.** Those six bytes are read as
  row numbers of `g_MpWeapons[]` by about fifteen places, several of them
  decompiled and none of them bounds-checked (`botinv.c`, `activemenu.c`,
  `modalarm.c`, `mpGetMpWeaponByLocation()`), so any sentinel value put in the
  array is an out-of-bounds read waiting for the one path that forgot to
  translate. The marker is a separate `static s8 s_MpWeaponSlotGroup[]` beside it,
  `-1` for an ordinary slot, and the worst a missed roll can now do is leave a slot
  holding a gun the page does not claim. `mpGetMpWeaponByLocation()` clamps and
  says so at `LOG_ERROR`, as the last line of that defence rather than the first.
- **Initialized by hand.** `= { -1, -1, -1, -1, -1, -1 }`, because a bss zero means
  group 0 in all six - the shape this fork keeps meeting, where 0 is a real answer
  rather than "nothing yet".
- **Something has to remember what the roll chose.** The marker outlives the match
  (the row should still read `RX: Automatics` afterwards), so
  `s_MpWeaponSlotRolled[]` records what each random slot was rolled to, and
  `mpReconcileRandomWeaponSlots()` - called from `mpReset()`, so on every way into
  a stage - retires any marker whose slot stopped holding its own roll. **It never
  re-rolls, and that is the load-bearing part**: a host that re-drew at stage entry
  would move what its clients were given at the match's start, and the two sides
  would stock an arena's floor from different guns, which the stage hash calls a
  mismatch. A match is rolled once, in `mpStartMatch()` before
  `netHostMatchStarting()` writes `RULES`; `RULES` already carries the six slots per
  match, so no new message and no protocol bump. **A client never draws** - its
  candidate list is its own unlock state, and a draw it took alone would put it out
  of step with the stage's stream - and retires all of its markers, on the theory
  that the guns it carries are the host's whatever its own file said. `netRulesApply()`
  clears them at the `RULES` handover rather than waiting for `mpReset()` to notice,
  because a host migration inside that window would otherwise roll the local group
  instead of keeping the host's choice.
- **The game re-applies the weapon set behind your back, at the end of every setup
  load.** `mpsetupfileLoadWad()` finishes with `challengeForceUnlockBotFeatures()` -
  which is `challengeDetermineUnlockedFeatures()` - which calls `func0f1895e8()`, and
  that calls `mpApplyWeaponSet()` whenever `g_MpWeaponSetNum < MP_NUM_WEAPONSETS`.
  So loading a saved setup whose set is one of the game's rewrites all six slots from
  the set, after the decode put the file's values (and markers) in. That is what the
  first version of this lost the round-trip check to: the file gave slot 2 back
  honestly and the load's own set re-application replaced it with the set's gun while
  the marker went on claiming the group. Two consequences written down:
  `mpApplyWeaponSet()`'s deriving path clears the markers *unconditionally* (not only
  for a player's own set, which is how it was first), then puts back the ones a saved
  set was saved with; and a random slot is a thing a **Custom** setup or a saved set
  has, never a set of the game's - which the page already enforces
  (`mpWeaponSlotsEditable()`), and which `--test-randomgroups` now asserts both ways.
  The hunt for it is a gdb watchpoint on `g_MpSetup.weapons[2]` from a breakpoint on
  `challengeForceUnlockBotFeatures()`, which named the writer in one hit after an
  afternoon of reading the load path sideways.
- **The two halves of a setup block are one hand on one bit stream.** The v5 group
  names are written after the stat sliders and were first read *before* them. Nothing
  failed: the decode read somebody else's fields, and the symptom was one slot of six
  coming back as 1 instead of 0. Any format change here has to be written and read in
  the same order and checked by a round-trip that puts a value in the new field, not
  by a check that the old fields still decode (they do, when the shift is after them).
  `MPSETUP_BLOCKSIZE` grew again for it (`MPSETUP_BLOCKSIZE_V3` is what 3 and 4 used,
  the macro itself is version 5's) - a v5 block needs 5651 of its 6032 bits, which
  `--test-randomgroups` prints, and `mpsetupfileSaveWad()`'s overflow check from the
  section above is what catches the next one.
- **Saved setups and saved sets name the group, never its number.** Both files are
  saved apart from `mpweaponfilters.bin`, and deleting a group shifts every number
  above it down, so a stored `3` would quietly start naming another group - the
  mistake this file records as *the first entry* elsewhere. A name that does not
  resolve leaves the slot plain and playing the concrete gun the group last rolled,
  which is why the saved set (`mpweaponsets.bin`, version 2) stores the six guns as
  before *and* a name per slot: the gun is the fallback, the name is the intent.
  `--test-mpweaponsets` grew the checks for that; deleting a group also renumbers the
  live markers and clears the one that named it (`setup.c`).
- **A slot's roll rule is not Start Armed's.** `mpCanRollWeaponSlot()` is not
  `mpCanRollSpawnWeapon()`: the spawn rule demands a *gun*, so it would drop the
  Shield and the gadgets an arena's slots do hold (a slot naming the Shield becomes a
  `shieldobj`; `mpApplyConfig()` clears the Shield slots when the feature is not
  earned). The slot rule is the dropdown's own: never `WEAPON_DISABLED`, never
  `MPFEATURE_NEVER`, and the stage's set. And it is asked always, not behind
  `normmplayerisrunning` as the spawn roll has it - at `mpStartMatch()` that flag
  still describes the previous stage, since `mpReset()` sets it on the next
  `mainLoop()` iteration.
- **Auto Random wins, on purpose.** `MPOPTION_AUTORANDOMWEAPON_START` writes the six
  slots through `mpSetWeaponSlot()`, which clears the marker, so a match that takes
  the slots for itself takes the random ones too. That was the agreed behaviour; a
  slot made random is the player's pick for the *slot*, not a reservation against a
  rule they switched on.
- **A closed dropdown shows its option's text**, so `RX: ...` on the row itself comes
  from `MENUOP_GETSELECTEDINDEX` returning `numguns + group`, and everywhere else the
  gun would have been named needs the same answer (`mpMenuTextWeaponNameForSlot()`)
  or a row beside the dropdown quietly shows the gun standing in for the group.

### What the gate does and does not see

`tools/ci/randomgroupstest.sh` runs `--test-randomgroups` and needs all five
verdicts (it counts them, so a half that never ran cannot pass silently): the store
(`mpWeaponFiltersSelfTest`), the roll (`mpRandomRollSelfTest`), the slots
(`mpRandomSlotSelfTest`), the saved setup (`mpRandomSetupFileSelfTest`) and the page
(`mpRandomGroupMenuSelfTest`, driving `MENUOP_SETTEXT` and never `MENUOP_SET` on a
keyboard row, and no row that would push a dialog while no menu is up). The roll and
slot halves live in `mplayer.c` because `mpCanRollSpawnWeapon()` and the slot markers
are private there. Measured 3s, so a 180s budget is generous. It cannot see the
layout: the new page's rows, and the two rows added to the weapons pages, are the
sort of thing only a screen shows - the same limit `weapon-sets.md` records for the
bind pages.

## Start Armed, one gun or two, and how much of it (2026-11-14)

Start Armed had two options and one of them meant three things at once: *Random*
was a fresh gun each spawn, a pair of guns when Akimbo allowed it, and a full
reserve of both. Four ways to start armed came out of that as **two settings**:
`Mod.StartArmed` (Off / First / Fiesta, values 0 1 2 unchanged) and
`Mod.StartArmedAkimbo` (Akimbo Start?, default **on**). Notes that cost time:

**The pair could not stay inside `Mod.Akimbo`.** That setting decides who is
*handed* two of a gun and who may *hold* two of what they picked up, and which
simulants and guards (`MODAKIMBO_EVERYONE / _PLAYERSANDSIMS / _GUARDS`), so
"one gun at the spawn, Akimbo still on for the rest of the match" is not
reachable through it. `modGetSpawnAkimbo()` is the spawn's own switch, and
`modSpawnIsAkimbo(weaponnum)` is the three-term answer - Akimbo for players, the
tick, and the gun pairing at all - because both spawn sites used to spell those
terms out (`playerSpawnWeapons()`, `botSpawnChr()`'s armed branch) and the
mission branch spelled two of them. Same reason `modSpawnWantsMaxAmmo()` exists
rather than `== SPAWNWEAPON_RANDOM` written at each site: the sites are three
(player, the mixed second hand, the simulants) and the rule is a player's
decision, not a detail of each.

**Keeping the values 0/1/2 is the whole reason this was small.** `mpGetSpawnWeapon()`
still means "row of slot 0, or roll the curated list", so the roll, the RX
Selection row's visibility, `modCanChrSpawnArmed()`, the saved setups, the presets'
`== OFF` test and `netrules.c` all went untouched by the rename of *Random* to
*Fiesta*. **Mines are the exception to the ammunition rule, by ammo type.** The
two First options hand out `min(capacity, 2 * priammoqty)` - `priammoqty` being
exactly what `setup.c:1306` fills a multiplayer crate with, so "the hand-out plus
one crate" is the doubling, and the map keeps the rest - and Fiesta keeps the
whole reserve. The three mine types are the hand-out alone, 5 of a carrying limit
of 10: doubling them lands on the limit, so First would hand out precisely what
Fiesta does, and a mine's arena spot is a crate and nothing else, so they are the
class where the difference is most visible. Keyed off `AMMOTYPE_TIMED_MINE /
_PROXY_MINE / _REMOTE_MINE` and **not** off the row's `hasweapon` flag, which is 0
for the grenade and the NB bomb as well - the detonator's Plastique row (also 5 of
10) is out of the list and the gate says so. `bgunAmmotypeAllowsUnlimitedAmmo()`
(Chicago's remote mines, Air Force One's timed mines) is a *pickup* rule read where
a crate is collected; a spawn figure must not consult it.

**One means one in a mission too, and that is where a hidden setting is still
live.** `playerSpawnWeapons()`'s mission branch (`!g_Vars.normmplayerisrunning`)
doubles the mission's own starting gun on `modIsAkimboForPlayers()`; it asks
`modGetSpawnAkimbo()` now. With Start Armed on Off the row that edits the key
**hides** - both pages hide it, `modSpawnAkimboRowHidden()` - so this is the
setting's remaining reach while it has no visible row. Hiding a row has never
meant the value stops being read anywhere; this is the one place it nearly did.

**A dropdown's popup cannot hold a widget.** `menuitemDropdownOverlay()`
(`src/game/menuitem.c:1069`) draws the open list as a text loop over
`MENUOP_GETOPTIONTEXT` strings and the tick maps a highlighted row to an index ->
`MENUOP_SET`, which is what closed the "put a tickbox in the popup" idea. A row
*underneath* is the shape, and it is the shape the fork already uses elsewhere
(Akimbo Triggers' bind rows).

**Correction to "a row cannot appear inside an already-laid-out page"** (the
2026-11-13 note above): `dialogCalculateContentSize()` - where `MENUOP_CHECKHIDDEN`
is consulted - runs **every frame** for a dialog without
`MENUDIALOGFLAG_DISABLERESIZE` (`src/game/menu.c:5183`), and neither
`g_MpWeaponsMenuDialog` nor the Mods: Player dialog sets that flag. So a row can
appear and disappear while the page is open: *Akimbo Start?* reveals itself the
moment Start Armed moves off Off. The fork's own Akimbo Triggers rows have been
doing this on Mods: Player all along, which is the evidence the old note should
have weighed against. The 2026-11-13 failure was real, so if that shape is ever
revisited, diagnose it again rather than from that note. This section's row is
asserted by the gate (`MENUOP_CHECKHIDDEN`), not by a screen, so treat the visual
as unconfirmed until one is looked at.

**`Akimbo` moved off Room Rules to the Weapons page**, above the Start Armed row
it changes, on both pages that carry Start Armed (the room's Weapons page *is*
`g_MpWeaponsMenuDialog`, so a room's host gets it there - and `MENUDIALOGFLAG_MPLOCKABLE`
on that dialog blocks the whole page for a locked-out player (`menu.c:4963`),
which is why neither the moved row nor Start Armed's needs `MENUITEMFLAG_LOCKABLEMINOR`).
`g_NetRoomRulesMenuItems` is referenced only by its own definition and its dialog,
so deleting a row from it moves nothing else.

**Random Five's sixth slot was "the last unlocked row", which is not the Disabled
row.** `mpApplyWeaponSet()` filled five slots and wrote `mpSetWeaponSlot(i,
mpGetNumWeaponOptions() - 1)` for the sixth. An *option* index, converted by
`mpSetWeaponSlot()` walking the rows beneath it - so the answer is whatever row
last unlocks, and on a GoldenEye stage list that is row `0x49`, **GoldenEye's
remote mine** (`geguns.c` lists `0x31..0x49` and leaves the hack pistols and cheat
guns behind), which is exactly what the report said the slot held. The fix names
the row: `mpGetDisabledWeaponRow()` finds it by `weaponHost(...) == WEAPON_DISABLED`
- not by `MPWEAPON_DISABLED`, which is where the stock table keeps it and a mod's
import rewrites the table from its top (`moddata.c`) - and `mpSetWeaponSlotRow()`
writes a row index where `mpSetWeaponSlot()` wants an option index. That pair is
the lesson: anything that knows which row it wants should not have to count the
rows around it to say so. The `#ifdef PLATFORM_N64` arm (`- 2`) is untouched, this
tree not building N64.

**The gate had to break the rule to test it.** On the test's own table, with
nothing above it unlocked, "the last unlocked row" *is* the Disabled row, so the
check would have been a tautology: `mpRandomSlotSelfTest()` unlocks
`MPWEAPON_GE_FIRST` for the duration and asserts the sixth slot is still the
Disabled row and the five it fills are not. Reverting the fix by hand turns it into
`FAIL ... the sixth slot row 73 holding gun 79`, which is `0x49`, the mine. Same
trick as that half's `numunoffered` guard: a check whose failure and success are
the same value proves nothing.

**The enhancements blob takes a new field without dropping anyone's settings.**
`g_ModOptions` reaches a preset through `MODENHANCE_EXTRA_*` (`optionsmenu.c`), and
the read path drops a note whose field count it does not recognise with *"left as
they are"* - settings silently Vanilla. Appending the new member to the enum and
chaining a second `count == ARRAYCOUNT(values) - 1` clause after the kill-feed's
appends the default, so `MODENHANCE_EXTRA_COUNT` grows, `MODENHANCE_SAVED_VERSION`
stays 2, and a note predating *both* fields passes the first clause and then the
second. Appending last also means appending a value rather than `memmove`-ing the
block. `struct modpreset` was left alone: growing it moves
`MODENHANCE_PRESET_FIELDS` and every note's field count.

### What the gate does and does not see

`tools/ci/startarmedtest.sh` runs `--test-startarmed`
(`mpStartArmedSelfTest()`, `src/game/mplayer/setup.c`, living there because the
rows' handlers are the things under test) and wants all six verdicts: the tick's
hide predicate and the value surviving the hiding; `modSpawnIsAkimbo()` across
Akimbo off, tick off, Guards-only, and a location that is not a gun; the four
combinations; the amounts over the whole table (never above the capacity, the
capacity when maxed, and **under** it for at least one row, which is what stops a
helper that just returned the capacity passing); the mine rows at their hand-out;
and Start Armed's three option strings. Mods: Player's copy of the tick row is
`static` in `optionsmenu.c`, so what is asserted is the one predicate both rows
answer. Nothing here sees the layout, the mission branch, or the simulants'
hands - the same limit as every gate above.
