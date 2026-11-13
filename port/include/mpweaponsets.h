#ifndef _IN_MPWEAPONSETS_H
#define _IN_MPWEAPONSETS_H

#include "types.h"

/*
Weapon Sets File Format
	# header
	[version{1}]
	[numsets{1}]
	# sets
	[set_1{namesize + NUM_MPWEAPONSLOTS}]
	...
	[set_n{...}]

Written by "Save Custom Set" on the weapons page (setup.c), read at startup.
 */

#define MPWEAPONSET_VERSION   1
#define MPWEAPONSET_MAXSAVED  16

struct mpweaponsetentry {
	char name[MPSETUP_MAXNAME + 1];
	// The set's own weapon numbers, as a stock set's slots[] holds them, not
	// the page's option indices (mpweaponsets.c)
	u8 slots[NUM_MPWEAPONSLOTS];
};

void mpWeaponSetsInit(void);
void mpWeaponSetsLoad(void);
s32 mpWeaponSetsNum(void);
const struct mpweaponsetentry *mpWeaponSetsGet(s32 index);
// The name's own, or -1 for a name nothing will show. slots as g_MpSetup.weapons
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
// The name the room's rules brought for the set the match plays by, and whether
// it belongs at that place in this machine's list (NULL when it does not)
void mpWeaponSetsSetForeignName(const char *name, s32 listindex);
const char *mpWeaponSetsForeignNameFor(s32 listindex);
const char *mpWeaponSetsForeignName(void);
// The set's place in the list the dropdown shows, or -1
s32 mpWeaponSetsOfferedIndex(s32 index);
// Whether the page's six slots have moved away from this set's guns
bool mpWeaponSetsIsDirty(s32 listindex);
// --test-mpweaponsets: a scratch file of our own, and what the store has to
// get right (tools/ci/mpweaponsetstest.sh)
void mpWeaponSetsTestFile(void);
void mpWeaponSetsSelfTest(void);

#endif
