#ifndef _IN_MPWEAPONFILTERS_H
#define _IN_MPWEAPONFILTERS_H

#include "types.h"

/*
Random Weapon Groups File Format
	# header
	[version{1}]
	[numgroups{1}]
	# groups
	[set_1{namesize + 1 + NUM_MPWEAPONS}]
	...
	[set_n{...}]

Written by "Save Group" on the Random Weapons page (setup.c), read at startup.
 */

#define MPWEAPONFILTER_VERSION   1
#define MPWEAPONFILTER_MAXSAVED  16

/**
 * A named curated random list: which rows of the weapon table the "Random"
 * weapon set and Start Armed's Random may draw.
 *
 * It keeps the *guns* it selects, as their WEAPON_* numbers, and not one byte
 * per row of g_MpWeapons - which is what it holds in memory. The reason is the
 * reason the saved weapon sets keep their six guns rather than the six rows the
 * page shows (mpweaponsets.c): a mod that brings its own weapon definitions
 * rewrites the table's rows from the top (moddata.c), so a row number names
 * another gun on another machine. The byte array is this build's working form;
 * the list of guns is what goes on disk and what a group means.
 *
 * guns[] is a set, not a mask, so its length is the number of rows selected and
 * nothing is said about the order the page showed them in. A gun this machine's
 * table does not have is dropped when the group is applied and said once.
 */
struct mpweaponfilterentry {
	char name[MPSETUP_MAXNAME + 1];
	s32 numguns;
	u8 guns[NUM_MPWEAPONS];
};

void mpWeaponFiltersInit(void);
void mpWeaponFiltersLoad(void);
s32 mpWeaponFiltersNum(void);
const struct mpweaponfilterentry *mpWeaponFiltersGet(s32 index);
// The group's rows, as the name says: which of them this machine has or not
s32 mpWeaponFiltersCountGuns(s32 index);

// The name's own, or -1 for a name nothing will show. Takes the current list.
// A name already taken is refused: overwriting goes through Update, as it does
// for the saved sets (mpweaponsets.c).
s32 mpWeaponFiltersSave(const char *name);
// The list as it stands now, written again under the name it already has
bool mpWeaponFiltersUpdate(s32 index);
bool mpWeaponFiltersDelete(s32 index);
// Which group goes by that name, ignoring case; -1 when none does
s32 mpWeaponFiltersFind(const char *name);

// A group's name as a page shows it: "RX: <name>" (mpweaponfilters.c)
const char *mpWeaponFiltersShown(s32 index);
// The group's selection into g_MpWeaponSetRandomFilters, clearing every other
// row. Says once what it could not find here. Returns how many rows it set.
s32 mpWeaponFiltersApply(s32 index);
// The group's rows in this build's table, in table order, up to maxrows of them
// written: what asks which row to draw when something picks *one* gun from a group
// rather than switching a whole list on (mpRollRandomWeaponSlots, mplayer.c)
s32 mpWeaponFilterRows(s32 index, s32 *rows, s32 maxrows);
// Which saved group the list as it stands is exactly equal to, or -1: what the
// page shows as the group in use after a load, when nothing selected one
s32 mpWeaponFiltersMatching(void);
// Whether the group's selection is exactly the list as it stands now
bool mpWeaponFiltersIsCurrent(s32 index);

// --test-randomgroups: a scratch file of our own, and what the store has to
// get right (tools/ci/randomgroupstest.sh)
void mpWeaponFiltersTestFile(void);
void mpWeaponFiltersSelfTest(void);

#endif
