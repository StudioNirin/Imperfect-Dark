#ifndef _IN_MPWEAPONSETS_H
#define _IN_MPWEAPONSETS_H

#include "types.h"

/*
Weapon Sets File Format
	# header
	[version{1}]
	[numsets{1}]
	# sets
	[set_1{namesize + NUM_MPWEAPONSLOTS + NUM_MPWEAPONSLOTS * namesize}]
	... 
	[set_n{...}]

Written by "Save Custom Set" on the weapons page (setup.c), read at startup.

Version 1 is the six guns alone; version 2 adds, after them, the random weapon
group each slot rolls from by name - empty for a slot that names a plain gun.
Both are read (mpweaponsets.c).
 */

#define MPWEAPONSET_VERSION   2

// The format version that first stored a slot made random
#define MPWEAPONSET_VERSION_GROUPS 2

#define MPWEAPONSET_MAXSAVED  16

struct mpweaponsetentry {
	char name[MPSETUP_MAXNAME + 1];
	// The set's own weapon numbers, as a stock set's slots[] holds them, not
	// the page's option indices (mpweaponsets.c)
	u8 slots[NUM_MPWEAPONSLOTS];
	/**
	 * The random weapon group each slot draws from, by name; "" for a slot that
	 * names a gun.
	 *
	 * By name because a group's number is its place in a list that grows and
	 * shrinks as its owner saves and deletes - the same reason the set itself is
	 * picked by name rather than by row (weapon-sets.md, "the first entry"), and
	 * the reason the saved setup can use the number and this cannot: a setup is
	 * this machine's, a set is picked out of a moving list. The guns stay in
	 * slots[] as well, which is what the slot plays if the name has gone.
	 */
	char groups[NUM_MPWEAPONSLOTS][MPSETUP_MAXNAME + 1];
};

void mpWeaponSetsInit(void);
void mpWeaponSetsLoad(void);
s32 mpWeaponSetsNum(void);
const struct mpweaponsetentry *mpWeaponSetsGet(s32 index);
// The name's own, or -1 for a name nothing will show. slots as g_MpSetup.weapons,
// and each slot that was made random by the group on screen at the press
s32 mpWeaponSetsSave(const char *name, const u8 *optionslots);
bool mpWeaponSetsDelete(s32 index);
// The set's guns written again under the name it already has
bool mpWeaponSetsUpdate(s32 index, const u8 *optionslots);
// Where the saved sets sit in the Combat Simulator's list, -1 for none
s32 mpWeaponSetsFirst(void);
// Whether the game's own list's index is one of the saved sets
bool mpWeaponSetsOwns(s32 listindex);
// The name given to the set at that place in the list, or ""
const char *mpWeaponSetsNamedFor(s32 listindex);
// Which saved set goes by that name, ignoring case; -1 when none does
s32 mpWeaponSetsFind(const char *name);
// The set's place in the list the dropdown shows, or -1
s32 mpWeaponSetsOfferedIndex(s32 index);
// The group a set's slot rolls from, by name, or "" for a slot that names a gun.
// listindex is the table's own index, as g_MpWeaponSetNum holds it. The game asks
// one question and the store owns the block's layout - as with NamedFor, and for
// the same reason: the game side cannot reach the net code and should not have to
// know how a set is stored (mpWeaponSetsGroupFor, mplayer.c)
const char *mpWeaponSetsGroupFor(s32 listindex, s32 slot);
// Whether the page's six slots have moved away from this set's guns
bool mpWeaponSetsIsDirty(s32 listindex);
// --test-mpweaponsets: a scratch file of our own, and what the store has to
// get right (tools/ci/mpweaponsetstest.sh)
void mpWeaponSetsTestFile(void);
void mpWeaponSetsSelfTest(void);

#endif
