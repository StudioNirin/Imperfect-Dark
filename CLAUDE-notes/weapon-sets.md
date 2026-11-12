# Custom weapon sets

## Digest

Custom weapon sets: why a saved set keeps its guns rather than the page's rows;
the Combat Simulator's list and the block that never moves; why the player's
sets lead that list and what "the first entry" stopped meaning; why the six
weapon slots are locked and what unlocks them; where the online room's Weapons
page actually lives; what the wire carries and what it does not carry yet.

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
