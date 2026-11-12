# Custom weapon sets

## Digest

Custom weapon sets: why a saved set keeps its guns rather than the page's rows;
the Combat Simulator's list and the block that never moves; where the online
room's Weapons page actually lives; what the wire carries and what it does not
carry yet.

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

## Not done

- Deleting a saved set from the menu (the store does it: `mpWeaponSetsDelete()`,
  and `--test-mpweaponsets` covers the shift-up into the vacated row).
- The name on the wire, and `Custom (my pistols)` in the client's rules page.
- `mpsetups.bin` has an export/import pair (`mpsetups-ext.bin`); the weapon
  sets' file has no such thing, and sharing sets between machines means copying
  `mpweaponsets.bin` by hand.
