#include "types.h"
#include "constants.h"
#include "bss.h"
#include "data.h"
#include "game/menu.h"
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "system.h"
#include "mpweaponfilters.h"

/*
 * The named random weapon groups: saved selections of the weapon table that the
 * "Random" weapon set and Start Armed's Random draw from.
 *
 * The selection itself is g_MpWeaponSetRandomFilters, one byte per row of
 * g_MpWeapons, edited on the Combat Simulator's weapons page (setup.c) and in an
 * online room's (netlobbymenu.c reaches the same dialog). It was never saved by
 * name: the four buttons at the foot of that list - Dark, Classic, All, None -
 * were the whole of it, and they set the rows and threw the result away. This
 * file is those four as things a player can name and keep, and their equal.
 *
 * What goes on disk is the guns, as WEAPON_* numbers, and not the byte array.
 * A row of the table is not an identity: a mod that brings its own weapon
 * definitions rewrites the rows from the top of g_MpWeapons (moddata.c), so
 * "row 20" is a different gun with the All-in-One mounted than with nothing,
 * which is the same trap the saved weapon sets sidestepped by keeping their six
 * guns instead of the six rows the page shows (mpweaponsets.c). A group that
 * names its guns means the same thing here and there, and simply loses a gun
 * whose name this machine's table has never heard of.
 */

#define MPWEAPONFILTER_FILENAME "mpweaponfilters"

static struct mpweaponfilterentry s_Groups[MPWEAPONFILTER_MAXSAVED];
static s32 s_Num;

// A file of our own, for --test-randomgroups to write without touching the
// player's saved groups
static char s_Path[FS_MAXPATH + 1];

static s32 mpWeaponFiltersCurrentGuns(u8 *guns);

static const char *mpWeaponFiltersPath(void)
{
	return s_Path[0] ? s_Path : "$S/" MPWEAPONFILTER_FILENAME ".bin";
}

void mpWeaponFiltersTestFile(void)
{
	snprintf(s_Path, sizeof(s_Path), "$S/%s-test.bin", MPWEAPONFILTER_FILENAME);
	remove(fsFullPath(s_Path));
}

s32 mpWeaponFiltersNum(void)
{
	return s_Num;
}

const struct mpweaponfilterentry *mpWeaponFiltersGet(s32 index)
{
	if (index < 0 || index >= s_Num) {
		return NULL;
	}

	return &s_Groups[index];
}

s32 mpWeaponFiltersCountGuns(s32 index)
{
	const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(index);

	return entry != NULL ? entry->numguns : 0;
}

/**
 * The guns the curated list selects, right now.
 *
 * A selection is a set of rows; what it means is the set of guns those rows
 * hold. Sorted, so two of them can be compared by comparing the arrays.
 */
static void mpWeaponFiltersGunsHere(u8 *guns, s32 *num)
{
	s32 n = mpWeaponFiltersCurrentGuns(guns);

	// insertion sort: at most NUM_MPWEAPONS bytes, and mostly sorted already
	for (s32 i = 1; i < n; i++) {
		u8 v = guns[i];
		s32 j = i - 1;

		while (j >= 0 && guns[j] > v) {
			guns[j + 1] = guns[j];
			j--;
		}

		guns[j + 1] = v;
	}

	*num = n;
}

static bool mpWeaponFiltersGunsEqual(const u8 *a, s32 na, const u8 *b, s32 nb)
{
	if (na != nb) {
		return false;
	}

	for (s32 i = 0; i < na; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}

	return true;
}

// Case-insensitive equality, spelled out rather than strcasecmp: the same
// source builds against mingw, where the header that declares it has moved
// (mpweaponsets.c)
static bool mpWeaponFiltersNameEqual(const char *a, const char *b)
{
	while (*a != '\0' && *b != '\0') {
		char ca = (*a >= 'A' && *a <= 'Z') ? *a + ('a' - 'A') : *a;
		char cb = (*b >= 'A' && *b <= 'Z') ? *b + ('a' - 'A') : *b;

		if (ca != cb) {
			return false;
		}

		a++;
		b++;
	}

	return *a == '\0' && *b == '\0';
}

/**
 * The list as it stands, as the guns it selects.
 *
 * Returns the count and fills guns[]; the array must hold NUM_MPWEAPONS bytes,
 * which is the most rows a selection can name.
 */
static s32 mpWeaponFiltersCurrentGuns(u8 *guns)
{
	s32 num = 0;

	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
		if (g_MpWeaponSetRandomFilters[i] && num < NUM_MPWEAPONS) {
			guns[num++] = g_MpWeapons[i].weaponnum;
		}
	}

	return num;
}

static s32 mpWeaponFiltersWriteFile(void)
{
	const char *filename = fsFullPath(mpWeaponFiltersPath());
	u8 version = MPWEAPONFILTER_VERSION;
	u8 num = (u8) s_Num;
	FILE *f;
	long written = 0;
	long expected = 2;

	if (fsFileSize(filename) < 0) {
		f = fsFileOpenWrite(filename);
		fsFileFree(f);
	}

	f = fsFileOpenWrite(filename);

	if (f == NULL) {
		sysLogPrintf(LOG_ERROR, "mpweaponfilters: cannot open %s", mpWeaponFiltersPath());
		return -1;
	}

	written += (long) fwrite(&version, 1, 1, f);
	written += (long) fwrite(&num, 1, 1, f);

	for (s32 i = 0; i < num; i++) {
		// A group's block is its name, how many guns it names, and those guns -
		// so its size is not a constant, and the count below is in bytes
		expected += (long) sizeof(s_Groups[i].name) + 1 + s_Groups[i].numguns;

		// Element size 1 throughout, so what fwrite reports is bytes: the count
		// below is in bytes and would never match an item count
		written += (long) fwrite(s_Groups[i].name, 1, sizeof(s_Groups[i].name), f);
		written += (long) fwrite(&s_Groups[i].numguns, 1, 1, f);
		written += (long) fwrite(s_Groups[i].guns, 1, s_Groups[i].numguns, f);
	}

	if (written != expected) {
		sysLogPrintf(LOG_ERROR, "mpweaponfilters: wrote %ld of the %ld bytes of %d saved groups",
				written, expected, num);
	}

	fsFileFree(f);

	return 0;
}

void mpWeaponFiltersLoad(void)
{
	const char *filename = fsFullPath(mpWeaponFiltersPath());
	u8 version;
	u8 num;
	FILE *f;

	s_Num = 0;

	if (fsFileSize(filename) < 0) {
		return;
	}

	f = fsFileOpenRead(filename);

	if (f == NULL) {
		sysLogPrintf(LOG_ERROR, "mpweaponfilters: cannot read %s", mpWeaponFiltersPath());
		return;
	}

	if (fread(&version, 1, 1, f) != 1 || fread(&num, 1, 1, f) != 1) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: %s has no header", mpWeaponFiltersPath());
		fsFileFree(f);
		return;
	}

	// Nothing older exists yet, so an unknown version is a file from a newer
	// build: read nothing rather than write half of it back next time
	if (version != MPWEAPONFILTER_VERSION) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: %s is version %d, this build writes %d: none read",
				mpWeaponFiltersPath(), version, MPWEAPONFILTER_VERSION);
		fsFileFree(f);
		return;
	}

	if (num > MPWEAPONFILTER_MAXSAVED) {
		num = MPWEAPONFILTER_MAXSAVED;
	}

	for (s32 i = 0; i < num; i++) {
		u8 count;

		if (fread(s_Groups[i].name, sizeof(s_Groups[i].name), 1, f) != 1
				|| fread(&count, 1, 1, f) != 1) {
			break;
		}

		s_Groups[i].name[MPSETUP_MAXNAME] = '\0';

		if (count > NUM_MPWEAPONS) {
			// A selection cannot name more guns than the table has rows; a file
			// that does is not one, and the rest of it would read out of place
			sysLogPrintf(LOG_NOTE, "mpweaponfilters: `%s` names %d guns, more than the table's %d rows: none read",
					s_Groups[i].name, count, NUM_MPWEAPONS);
			break;
		}

		if (fread(s_Groups[i].guns, count, 1, f) != 1) {
			break;
		}

		s_Groups[i].numguns = count;
		s_Num++;
	}

	fsFileFree(f);

	if (s_Num > 0) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: %d saved random weapon groups", s_Num);
	}
}

void mpWeaponFiltersInit(void)
{
	mpWeaponFiltersLoad();
}

s32 mpWeaponFiltersFind(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return -1;
	}

	for (s32 i = 0; i < s_Num; i++) {
		if (mpWeaponFiltersNameEqual(s_Groups[i].name, name)) {
			return i;
		}
	}

	return -1;
}

s32 mpWeaponFiltersSave(const char *name)
{
	struct mpweaponfilterentry *entry;
	s32 at;

	if (name == NULL || name[0] == '\0') {
		return -1;
	}

	if (s_Num >= MPWEAPONFILTER_MAXSAVED) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: %d saved groups is the most this holds", s_Num);
		return -1;
	}

	at = mpWeaponFiltersFind(name);

	if (at >= 0) {
		// One name, one group, and the page writes through
		// mpWeaponFiltersUpdate when the typed name is one the player has used:
		// arriving here with a taken name is a caller that skipped that, and two
		// rows the player reads as the same selection is not something a store
		// should hold quietly (mpweaponsets.c is written the same way)
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: a group named `%s` is already saved", name);
		return -1;
	}

	at = s_Num;
	entry = &s_Groups[at];

	snprintf(entry->name, sizeof(entry->name), "%s", name);
	entry->numguns = mpWeaponFiltersCurrentGuns(entry->guns);

	s_Num++;

	if (mpWeaponFiltersWriteFile() < 0) {
		s_Num--;
		return -1;
	}

	sysLogPrintf(LOG_NOTE, "mpweaponfilters: saved group `%s` with %d guns", entry->name, entry->numguns);

	return at;
}

bool mpWeaponFiltersUpdate(s32 index)
{
	struct mpweaponfilterentry *entry;

	if (index < 0 || index >= s_Num) {
		return false;
	}

	entry = &s_Groups[index];
	entry->numguns = mpWeaponFiltersCurrentGuns(entry->guns);

	return mpWeaponFiltersWriteFile() == 0;
}

bool mpWeaponFiltersDelete(s32 index)
{
	if (index < 0 || index >= s_Num) {
		return false;
	}

	for (s32 i = index; i < s_Num - 1; i++) {
		s_Groups[i] = s_Groups[i + 1];
	}

	s_Num--;

	if (mpWeaponFiltersWriteFile() < 0) {
		return false;
	}

	sysLogPrintf(LOG_NOTE, "mpweaponfilters: %d saved groups left", s_Num);

	return true;
}

s32 mpWeaponFiltersApply(s32 index)
{
	const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(index);
	s32 set = 0;
	s32 missing = 0;

	if (entry == NULL) {
		return 0;
	}

	// Clear first: a group says which rows it selects, and every other row is
	// out. A gun named twice in the file, or a table holding the same gun in two
	// rows, ends with both rows in - which is what selecting a gun means here.
	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeaponSetRandomFilters); i++) {
		g_MpWeaponSetRandomFilters[i] = 0;
	}

	for (s32 g = 0; g < entry->numguns; g++) {
		bool found = false;

		for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
			if (g_MpWeapons[i].weaponnum == entry->guns[g]) {
				g_MpWeaponSetRandomFilters[i] = 1;
				set++;
				found = true;
			}
		}

		if (!found) {
			missing++;
		}
	}

	if (missing > 0) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters: group `%s` names %d gun(s) this table does not have; %d row(s) selected",
				entry->name, missing, set);
	}

	return set;
}

/**
 * The group's rows in this build's table, in table order.
 *
 * The same walk as above, for the question it cannot answer: which row, when
 * something draws *one* gun out of a group instead of switching a list on
 * (mpRollRandomWeaponSlots, mplayer.c). Walking the table rather than the group's
 * list is what makes the answer a set: a row is named once however often its gun
 * appears in the file, and a table holding one gun in two rows takes both, as
 * above does.
 *
 * What cannot be written because maxrows ran out is not counted either - a caller
 * drawing from what it was given must not draw an index it does not hold.
 */
s32 mpWeaponFilterRows(s32 index, s32 *rows, s32 maxrows)
{
	const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(index);
	s32 num = 0;

	if (entry == NULL || rows == NULL || maxrows <= 0) {
		return 0;
	}

	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
		for (s32 g = 0; g < entry->numguns; g++) {
			if (g_MpWeapons[i].weaponnum == entry->guns[g]) {
				if (num < maxrows) {
					rows[num++] = i;
				}

				break;
			}
		}
	}

	return num;
}

bool mpWeaponFiltersIsCurrent(s32 index)
{
	const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(index);

	if (entry == NULL) {
		return false;
	}

	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
		bool want = false;

		for (s32 g = 0; g < entry->numguns; g++) {
			if (entry->guns[g] == g_MpWeapons[i].weaponnum) {
				want = true;
				break;
			}
		}

		if (want != (g_MpWeaponSetRandomFilters[i] != 0)) {
			return false;
		}
	}

	return true;
}

s32 mpWeaponFiltersMatching(void)
{
	for (s32 i = 0; i < s_Num; i++) {
		if (mpWeaponFiltersIsCurrent(i)) {
			return i;
		}
	}

	return -1;
}

/**
 * A group's name as the page shows it.
 *
 * RX: because the dropdown lists the four built-in selections and the player's
 * own together, and a group called "Dark" would otherwise read as a fifth thing
 * that happens to spell one of them. The store keeps the bare name; only the
 * label carries the mark (mpweaponsets.c does the same with CX: for its rows).
 *
 * Each index needs its own buffer: the menu keeps the pointer a text function
 * hands it rather than a copy of the string.
 */
static char s_Shown[MPWEAPONFILTER_MAXSAVED][MPSETUP_MAXNAME + 8];

const char *mpWeaponFiltersShown(s32 index)
{
	const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(index);

	if (entry == NULL) {
		return "";
	}

	snprintf(s_Shown[index], sizeof(s_Shown[index]), "RX: %s", entry->name);

	return s_Shown[index];
}

// The self-test's own counts, so the checks read as checks
static s32 s_TestChecks;
static s32 s_TestFailed;

static void mpWeaponFiltersTestCheck(const char *what, bool ok)
{
	s_TestChecks++;

	if (!ok) {
		s_TestFailed++;
	}

	sysLogPrintf(LOG_NOTE, "mpweaponfilters test: %s %s", ok ? "ok  " : "FAIL", what);
}

/**
 * What the store has to get right, run once at startup under
 * --test-randomgroups against a scratch file (mpWeaponFiltersTestFile).
 *
 * The check worth the most is the round trip through a *changed* table: a group
 * saved with one mod's rows and applied with another's must still select the
 * same guns, because it kept their names. Everything else is the housekeeping -
 * a refused name, an overwrite that replaces rather than adds, a delete that
 * leaves the rest in order.
 */
void mpWeaponFiltersSelfTest(void)
{
	u8 save[NUM_MPWEAPONS];
	u8 table[NUM_MPWEAPONS];
	u8 gunsbefore[NUM_MPWEAPONS];
	u8 gunsafter[NUM_MPWEAPONS];
	s32 nbefore = 0;
	s32 nafter = 0;
	bool unique = true;
	s32 at;
	s32 second;
	s32 first;

	for (s32 i = 0; i < NUM_MPWEAPONS; i++) {
		save[i] = g_MpWeaponSetRandomFilters[i];
		table[i] = g_MpWeapons[i].weaponnum;
	}

	// The moved-table check below needs a table where a gun names one row; a mod
	// that lists the same weapon twice is entitled to, and then a selection by
	// name legitimately marks both rows and the arithmetic does not apply
	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons) && unique; i++) {
		for (s32 j = i + 1; j < ARRAYCOUNT(g_MpWeapons); j++) {
			if (g_MpWeapons[i].weaponnum == g_MpWeapons[j].weaponnum) {
				unique = false;
				break;
			}
		}
	}

	mpWeaponFiltersTestCheck("a group with no name is refused",
			mpWeaponFiltersSave("") < 0);

	// A selection of the first few rows, which is enough to be unequal to
	// everything else the test writes
	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeaponSetRandomFilters); i++) {
		g_MpWeaponSetRandomFilters[i] = i < 4 ? 1 : 0;
	}

	at = mpWeaponFiltersSave("My Guns");
	mpWeaponFiltersTestCheck("a group is saved", at == 0);

	if (at == 0) {
		const struct mpweaponfilterentry *entry = mpWeaponFiltersGet(at);

		mpWeaponFiltersTestCheck("the group keeps the guns, not the rows",
				entry != NULL && entry->numguns == 4);

		mpWeaponFiltersGunsHere(gunsbefore, &nbefore);

		bool named = entry != NULL && nbefore == 4;

		for (s32 i = 0; named && i < 4; i++) {
			bool holds = false;

			for (s32 g = 0; g < entry->numguns; g++) {
				holds = holds || entry->guns[g] == g_MpWeapons[i].weaponnum;
			}

			named = holds;
		}

		mpWeaponFiltersTestCheck("named by the weapon each row holds", named);

		mpWeaponFiltersTestCheck("the list the group is showing is recognised",
				mpWeaponFiltersMatching() == at);

		mpWeaponFiltersTestCheck("the page marks the group as the player's own",
				strcmp(mpWeaponFiltersShown(at), "RX: My Guns") == 0);

		mpWeaponFiltersTestCheck("a name is found whatever case it was typed in",
				mpWeaponFiltersFind("my guns") == at && mpWeaponFiltersFind("No Such Group") < 0);

		mpWeaponFiltersTestCheck("a taken name is refused, not added",
				mpWeaponFiltersSave("My Guns") < 0 && mpWeaponFiltersNum() == 1);
	}

	// A name the player has used writes that group again rather than adding a
	// second one the page cannot tell apart
	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeaponSetRandomFilters); i++) {
		g_MpWeaponSetRandomFilters[i] = i < 7 ? 1 : 0;
	}

	mpWeaponFiltersTestCheck("a list the group does not match is not shown as it",
			mpWeaponFiltersMatching() < 0);
	mpWeaponFiltersTestCheck("the overwrite goes through the update path",
			mpWeaponFiltersUpdate(0) && mpWeaponFiltersMatching() == 0);

	second = mpWeaponFiltersSave("Second");
	mpWeaponFiltersTestCheck("a new name adds a group", second == 1);

	first = mpWeaponFiltersApply(second);
	mpWeaponFiltersGunsHere(gunsbefore, &nbefore);
	mpWeaponFiltersTestCheck("a group selects as many rows as it names guns",
			!unique || first == mpWeaponFiltersCountGuns(second));

	/**
	 * The one that matters: a group follows its guns, not the rows they were in
	 * when it was saved.
	 *
	 * Every row's weapon number is moved along one, which is the shape of what a
	 * mod's import does to the table (moddata.c rewrites it from the top). The
	 * group still holds the same weapon numbers, so after the move it must select
	 * the rows that hold those now - not the rows it was saved from. The table is
	 * put back either way.
	 */
	if (unique) {
		for (s32 i = ARRAYCOUNT(g_MpWeapons) - 1; i > 0; i--) {
			g_MpWeapons[i].weaponnum = g_MpWeapons[i - 1].weaponnum;
		}

		g_MpWeapons[0].weaponnum = table[ARRAYCOUNT(g_MpWeapons) - 1];

		mpWeaponFiltersApply(second);
		mpWeaponFiltersGunsHere(gunsafter, &nafter);

		mpWeaponFiltersTestCheck("a moved table still selects the same guns",
				mpWeaponFiltersGunsEqual(gunsbefore, nbefore, gunsafter, nafter));

		// The gun that was in row 0 is in row 1 now: selecting row 0 again would
		// say the group kept the row it was saved from, not the gun
		mpWeaponFiltersTestCheck("and not the rows it was saved from",
				g_MpWeaponSetRandomFilters[0] == 0 && g_MpWeaponSetRandomFilters[1] != 0);

		for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
			g_MpWeapons[i].weaponnum = 0xff;
		}

		// A table holding none of the group's guns selects nothing, and says so
		// (mpWeaponFiltersApply logs the count it could not place)
		mpWeaponFiltersTestCheck("a table without the group's guns selects nothing",
				mpWeaponFiltersApply(second) == 0);
	}

	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
		g_MpWeapons[i].weaponnum = table[i];
	}

	mpWeaponFiltersTestCheck("a delete keeps the rest in order",
			mpWeaponFiltersDelete(0) && mpWeaponFiltersNum() == 1
			&& mpWeaponFiltersFind("Second") == 0);

	mpWeaponFiltersTestCheck("the last group goes",
			mpWeaponFiltersDelete(0) && mpWeaponFiltersNum() == 0);
	mpWeaponFiltersTestCheck("deleting what is not there says so",
			!mpWeaponFiltersDelete(0));

	// Read the file back: what was written has to be what a start finds
	for (s32 i = 0; i < ARRAYCOUNT(g_MpWeaponSetRandomFilters); i++) {
		g_MpWeaponSetRandomFilters[i] = i < 3 ? 1 : 0;
	}

	if (mpWeaponFiltersSave("Reloaded") >= 0) {
		mpWeaponFiltersLoad();
		mpWeaponFiltersTestCheck("the file reads back as it was written",
				mpWeaponFiltersNum() == 1 && mpWeaponFiltersMatching() == 0);
		mpWeaponFiltersDelete(0);
	}

	for (s32 i = 0; i < NUM_MPWEAPONS; i++) {
		g_MpWeaponSetRandomFilters[i] = save[i];
		g_MpWeapons[i].weaponnum = table[i];
	}

	{
		bool back = true;

		for (s32 i = 0; i < ARRAYCOUNT(g_MpWeapons); i++) {
			back = back && g_MpWeapons[i].weaponnum == table[i]
					&& g_MpWeaponSetRandomFilters[i] == save[i];
		}

		mpWeaponFiltersTestCheck("the table and the list are as they were found", back);
	}

	// The scratch file goes with the test, so a run leaves nothing of itself in
	// the save directory (mpweaponsets.c does the same, and the gate checks it)
	remove(fsFullPath(mpWeaponFiltersPath()));

	if (s_TestFailed == 0) {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters test: all %d checks as expected", s_TestChecks);
	} else {
		sysLogPrintf(LOG_NOTE, "mpweaponfilters test: %d of %d checks as expected: %d FAILED",
				s_TestChecks - s_TestFailed, s_TestChecks, s_TestFailed);
	}
}
