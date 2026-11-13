#include "types.h"
#include "constants.h"
#include "bss.h"
#include "data.h"
#include "game/mplayer/mplayer.h"
#include "game/menu.h"
#include "game/lang.h"
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "system.h"
#include "mpweaponsets.h"

/*
 * The weapon sets a player made themselves on the Combat Simulator's weapons
 * page, and the same page in an online room.
 *
 * A set the game hands out is a row of g_MpWeaponSets (the Combat Simulator's
 * list, which the stock twelve share with the sets a borrowed mod adds
 * (modborrow.c) and GoldenEye's (gexplus.c)). Nothing there is written to disk,
 * and its rows name the guns as WEAPON_* numbers - so a saved set is kept as
 * exactly that, a name and six weapon numbers, and put back into the list at
 * run time.
 *
 * That is the reason the page's own six values are not what gets saved. They
 * are option indices: which row of the page's list a slot is showing, where
 * row 0 is nothing and the rows past the stock guns belong to whatever mod is
 * loaded (mpApplyWeaponSet() turns a set's weapon numbers into option indices
 * by looking each one up in g_MpWeapons). Save the index and the set means a
 * different gun on a machine with another mod loaded; save the weapon number
 * and it means the same gun there and here, and comes back through the same
 * lookup a stock set does.
 */

#define MPWEAPONSET_FILENAME "mpweaponsets"

// defined in the Combat Simulator (mplayer.c), like netrules.c and gexplus.c reach it
extern s32 g_MpWeaponSetNum;

// The weapons page's save row, driven by the self-test below
extern MenuItemHandlerResult menuhandlerMpSaveCustomSet(s32 operation, struct menuitem *item, union handlerdata *data);

// And its six slots, same reason
extern MenuItemHandlerResult menuhandlerMpWeaponSlot(s32 operation, struct menuitem *item, union handlerdata *data);
extern s32 mpGetWeaponSlot(s32 slot);

static struct mpweaponsetentry s_Sets[MPWEAPONSET_MAXSAVED];
static s32 s_Num;
// Where this file's block of rows sits in g_MpWeaponSets, -1 while it does not
static s32 s_First = -1;
// The names as the list shows them. langAddPortText() keeps the pointer, so
// each row needs its own store - one buffer reused by every row would leave the
// whole block showing whatever name was written last.
static char s_Shown[MPWEAPONSET_MAXSAVED][MPSETUP_MAXNAME + 8];
// The name the room's rules brought with them for the set the match plays by
// (netrules.c), and this machine's number for that set: -1 when the rules named
// nothing, which is every room whose host plays by the game's own sets
static char s_ForeignName[MPSETUP_MAXNAME + 1];
static s32 s_ForeignIndex = -1;

static void mpWeaponSetsPutRow(s32 index);
static void mpWeaponSetsDisableAlt(struct mpweaponset *set);

// A file of our own, for --test-mpweaponsets to write without touching the
// player's saved sets
static char s_Path[FS_MAXPATH + 1];

static const char *mpWeaponSetsPath(void)
{
	return s_Path[0] ? s_Path : "$S/" MPWEAPONSET_FILENAME ".bin";
}

void mpWeaponSetsTestFile(void)
{
	snprintf(s_Path, sizeof(s_Path), "$S/%s-test.bin", MPWEAPONSET_FILENAME);
	remove(fsFullPath(s_Path));
}

s32 mpWeaponSetsNum(void)
{
	return s_Num;
}

const struct mpweaponsetentry *mpWeaponSetsGet(s32 index)
{
	if (index < 0 || index >= s_Num) {
		return NULL;
	}

	return &s_Sets[index];
}

s32 mpWeaponSetsFirst(void)
{
	return s_First;
}

bool mpWeaponSetsOwns(s32 listindex)
{
	s32 at = listindex - s_First;

	// the rows held for sets, not the empty ones in the block
	return s_First >= 0 && at >= 0 && at < s_Num;
}

static s32 mpWeaponSetsWriteFile(void)
{
	const char *filename = fsFullPath(mpWeaponSetsPath());
	u8 version = MPWEAPONSET_VERSION;
	u8 num = (u8) s_Num;
	FILE *f;
	s32 written = 0;

	if (fsFileSize(filename) < 0) {
		f = fsFileOpenWrite(filename);
		fsFileFree(f);
	}

	f = fsFileOpenWrite(filename);

	if (f == NULL) {
		sysLogPrintf(LOG_ERROR, "mpweaponsets: cannot open %s", mpWeaponSetsPath());
		return -1;
	}

	written += fwrite(&version, 1, 1, f);
	written += fwrite(&num, 1, 1, f);

	for (s32 i = 0; i < num; i++) {
		written += fwrite(s_Sets[i].name, sizeof(s_Sets[i].name), 1, f);
		written += fwrite(s_Sets[i].slots, sizeof(s_Sets[i].slots), 1, f);
	}

	if (written != 2 + 2 * num) {
		sysLogPrintf(LOG_ERROR, "mpweaponsets: wrote %d of the %d blocks of %d saved sets",
				written, 2 + 2 * num, num);
	}

	fsFileFree(f);

	return 0;
}

void mpWeaponSetsLoad(void)
{
	const char *filename = fsFullPath(mpWeaponSetsPath());
	u8 version;
	u8 num;
	FILE *f;

	s_Num = 0;

	if (fsFileSize(filename) < 0) {
		return;
	}

	f = fsFileOpenRead(filename);

	if (f == NULL) {
		sysLogPrintf(LOG_ERROR, "mpweaponsets: cannot read %s", mpWeaponSetsPath());
		return;
	}

	if (fread(&version, 1, 1, f) != 1 || fread(&num, 1, 1, f) != 1) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets: %s has no header", mpWeaponSetsPath());
		fsFileFree(f);
		return;
	}

	// Nothing older exists yet, so an unknown version is a file from a newer
	// build: read nothing rather than write half of it back next time
	if (version != MPWEAPONSET_VERSION) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets: %s is version %d, this build writes %d: none read",
				mpWeaponSetsPath(), version, MPWEAPONSET_VERSION);
		fsFileFree(f);
		return;
	}

	if (num > MPWEAPONSET_MAXSAVED) {
		num = MPWEAPONSET_MAXSAVED;
	}

	for (s32 i = 0; i < num; i++) {
		if (fread(s_Sets[i].name, sizeof(s_Sets[i].name), 1, f) != 1
				|| fread(s_Sets[i].slots, sizeof(s_Sets[i].slots), 1, f) != 1) {
			break;
		}

		s_Sets[i].name[MPSETUP_MAXNAME] = '\0';
		s_Num++;
	}

	fsFileFree(f);

	if (s_Num > 0) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets: %d saved weapon sets", s_Num);
	}
}

s32 mpWeaponSetsSave(const char *name, const u8 *optionslots)
{
	struct mpweaponsetentry *entry;
	s32 at;

	if (name == NULL || name[0] == '\0') {
		return -1;
	}

	if (s_Num >= MPWEAPONSET_MAXSAVED) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets: %d saved sets is the most this holds", s_Num);
		return -1;
	}

	at = mpWeaponSetsFind(name);

	if (at >= 0) {
		// One name, one set. The page asks before it gets here (a name already
		// taken opens the overwrite confirm, which writes through
		// mpWeaponSetsUpdate), so arriving with a taken name is a caller that
		// skipped that question - and two rows the player reads as the same set
		// with different guns is not a thing a store should hold quietly.
		sysLogPrintf(LOG_NOTE, "mpweaponsets: a set named `%s` is already saved", name);
		return -1;
	}

	at = s_Num;
	entry = &s_Sets[at];

	snprintf(entry->name, sizeof(entry->name), "%s", name);

	// the page's rows are option indices; the set keeps their guns
	for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		s32 option = optionslots[i];

		entry->slots[i] = (option >= 0 && option < NUM_MPWEAPONS)
			? g_MpWeapons[option].weaponnum : WEAPON_NONE;
	}

	s_Num++;

	if (mpWeaponSetsWriteFile() < 0) {
		s_Num--;
		return -1;
	}

	mpWeaponSetsPutRow(at);

	return at;
}

bool mpWeaponSetsDelete(s32 index)
{
	if (index < 0 || index >= s_Num) {
		return false;
	}

	for (s32 i = index; i < s_Num - 1; i++) {
		s_Sets[i] = s_Sets[i + 1];
	}

	s_Num--;

	if (mpWeaponSetsWriteFile() < 0) {
		return false;
	}

	// Every row again: the sets above the deleted one moved up a row each, and
	// the row it vacated goes out of the list. Writing from the deleted index
	// alone would leave the rows below showing the sets they no longer hold.
	for (s32 i = 0; i < MPWEAPONSET_MAXSAVED; i++) {
		mpWeaponSetsPutRow(i);
	}

	return true;
}

/**
 * A set's alternative group, emptied.
 *
 * The group holds what a set hands out when its own guns are not earned yet,
 * and mpCountWeaponSetThing() offers a set whose first entry is anything other
 * than WEAPON_DISABLED - including 0, which is a gun.
 */
static void mpWeaponSetsDisableAlt(struct mpweaponset *set)
{
	set->unk0c = WEAPON_DISABLED;
	set->unk0d = WEAPON_DISABLED;
	set->unk0e = WEAPON_DISABLED;
	set->unk0f = WEAPON_DISABLED;
	set->unk10 = WEAPON_DISABLED;
	set->unk11 = WEAPON_DISABLED;
}

/**
 * One saved set's row in the Combat Simulator's list, or an empty one.
 *
 * The row is written whether or not the set exists, because the block's size
 * never changes (mpWeaponSetsPlace). A row with no set behind it is hidden the
 * way a gun a player has not earned is: an unearnable requirement keeps it out
 * of the list, and an alternative group of nothing keeps mpCountWeaponSetThing()
 * from seeing it either.
 */
static void mpWeaponSetsPutRow(s32 index)
{
	struct mpweaponset *set;
	const struct mpweaponsetentry *entry = mpWeaponSetsGet(index);

	if (s_First < 0 || index < 0 || index >= MPWEAPONSET_MAXSAVED) {
		return;
	}

	set = &g_MpWeaponSets[s_First + index];
	memset(set, 0, sizeof(*set));

	if (entry == NULL) {
		// Out of the list, and both halves of the test the game applies have to
		// say no: an unearnable requirement, and an alternative group of
		// WEAPON_DISABLED, which is not what zeroing it gives - 0 is a gun
		// (WEAPON_NONE), so mpCountWeaponSetThing() would count the row
		set->requirefeatures[0] = MPFEATURE_NEVER;
		set->name = langAddPortText(" ");
		mpWeaponSetsDisableAlt(set);

		return;
	}

	for (s32 j = 0; j < NUM_MPWEAPONSLOTS; j++) {
		set->slots[j] = entry->slots[j];
	}

	// Nothing locks a player's set: a requirement of 0 counts as earned
	// (challengeIsFeatureUnlocked), which is what offers the set in the list,
	// and mplayer.c then reads slots[] rather than the alternative group a set
	// keeps for its not-yet-earned guns.
	mpWeaponSetsDisableAlt(set);

	// CX: because these sit in the list among the game's own sets, and a player
	// picking "Pistols, mine" out of that list has no other way to know the row
	// is theirs. The store keeps the bare name; only the row carries the mark.
	// langAddPortText() dedups, so writing the same name back is free.
	snprintf(s_Shown[index], sizeof(s_Shown[index]), "CX: %s", entry->name);
	set->name = langAddPortText(s_Shown[index]);
}

/**
 * The saved sets in the Combat Simulator's list, at its end, once.
 *
 * A block of MPWEAPONSET_MAXSAVED rows, whether or not that many sets are
 * saved, and the block never moves or changes size after this. Both halves
 * matter: the sets a mod borrows (modborrow.c) and GoldenEye's (gexplus.c)
 * record where they put themselves and put nothing after themselves, so a
 * block that grew or shrank - or that had to be lifted out and set down again
 * - would leave those records pointing at rows that are no longer theirs,
 * which is the same shape of bug as the Mod Options bind rows reading a table
 * by position (crash-reports.md). Rows that hold nothing are hidden rather
 * than removed, so nothing after this block ever moves either.
 */
static void mpWeaponSetsPlace(void)
{
	if (g_MpNumWeaponSets + MPWEAPONSET_MAXSAVED > MP_MAX_WEAPONSETS) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets: the list is at %d, no room for %d saved sets",
				g_MpNumWeaponSets, MPWEAPONSET_MAXSAVED);
		return;
	}

	s_First = g_MpNumWeaponSets;
	g_MpNumWeaponSets += MPWEAPONSET_MAXSAVED;

	for (s32 i = 0; i < MPWEAPONSET_MAXSAVED; i++) {
		mpWeaponSetsPutRow(i);
	}

	sysLogPrintf(LOG_NOTE, "mpweaponsets: %d saved sets, held at %d of the list's %d",
			s_Num, s_First, g_MpNumWeaponSets);
}

/**
 * The saved sets read in and put in the list, once, at startup.
 *
 * After a mod's own sets are in (main.c), so the block sits at the end of the
 * list as a whole. It stays there for the session: a mod entered from the
 * Perfect Menu borrows more sets, and GoldenEye's cheat sets come and go with
 * a checkbox, and both append past the block without moving it.
 */
void mpWeaponSetsInit(void)
{
	mpWeaponSetsLoad();
	mpWeaponSetsPlace();
}

// The self-test's own counts, so the checks read as checks
static s32 s_TestChecks;
static s32 s_TestFailed;

static void mpWeaponSetsTestCheck(const char *what, bool ok)
{
	s_TestChecks++;

	if (!ok) {
		s_TestFailed++;
	}

	sysLogPrintf(LOG_NOTE, "mpweaponsets test: %s %s", ok ? "ok  " : "FAIL", what);
}

/**
 * What the store has to get right, run once at startup under
 * --test-mpweaponsets against a scratch file (mpWeaponSetsTestFile).
 *
 * The one worth the most is the third: a saved set keeps the guns, not the
 * page's row numbers, because the rows a mod adds come between the stock ones
 * and Disabled. Everything else here is what the list has to not do to the
 * sets a mod and GoldenEye put in it.
 */
void mpWeaponSetsSelfTest(void)
{
	u8 slots[NUM_MPWEAPONSLOTS];
	s32 first = mpWeaponSetsFirst();
	s32 oddjob = -1;

	if (first < 0) {
		sysLogPrintf(LOG_ERROR, "mpweaponsets test: FAIL the block is not in the list");
		return;
	}

	mpWeaponSetsTestCheck("the block sits at the list's end",
			g_MpNumWeaponSets == first + MPWEAPONSET_MAXSAVED);

	for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		if (g_MpWeapons[i].weaponnum != i && g_MpWeapons[i].weaponnum != WEAPON_NONE) {
			oddjob = i;
			break;
		}
	}

	for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		slots[i] = (i == 0 && oddjob >= 0) ? (u8) oddjob : (u8) (i + 1);
	}

	mpWeaponSetsTestCheck("a set with no name is refused",
			mpWeaponSetsSave("", slots) < 0);

	s32 at = mpWeaponSetsSave("My Pistols", slots);
	mpWeaponSetsTestCheck("a set is saved", at == 0);

	if (at == 0) {
		bool guns = true;

		for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
			guns = guns && g_MpWeaponSets[first].slots[i] == g_MpWeapons[slots[i]].weaponnum;
		}

		mpWeaponSetsTestCheck("the set keeps its guns, not the page's rows", guns);
		char *shown = langGet(g_MpWeaponSets[first].name);

		mpWeaponSetsTestCheck("the list marks the set as the player's own",
				shown != NULL && strcmp(shown, "CX: My Pistols") == 0);

		// Every offered row shows once and the rows held for sets that were
		// never saved show nowhere: this is what keeps the sets a mod borrows
		// and GoldenEye's where they were put
		s32 offered = func0f189058(false);
		s32 seen = 0;
		s32 emptyseen = 0;
		bool seenname = false;

		for (s32 i = 0; i < offered; i++) {
			s32 index = func0f188f9c(i);

			if (index < first || index >= first + MPWEAPONSET_MAXSAVED) {
				continue;
			}

			char *name;

			if (index < first + s_Num) {
				seen++;
				name = mpGetWeaponSetName(i);
				seenname = seenname
					|| (name != NULL && strcmp(name, "CX: My Pistols") == 0);
			} else {
				emptyseen++;
			}
		}

		mpWeaponSetsTestCheck("the saved set is offered once", seen == 1);
		mpWeaponSetsTestCheck("the rows held for nothing are not offered", emptyseen == 0);
		mpWeaponSetsTestCheck("the list names the saved set as CX: and by name", seenname);

		mpWeaponSetsLoad();
		mpWeaponSetsTestCheck("the file reads back the same set",
				s_Num == 1 && strcmp(s_Sets[0].name, "My Pistols") == 0
				&& memcmp(s_Sets[0].slots, g_MpWeaponSets[first].slots, NUM_MPWEAPONSLOTS) == 0);

		mpWeaponSetsLoad();
		mpWeaponSetsPutRow(0);

		mpWeaponSetsTestCheck("the count never moved while the list grew a set",
				g_MpNumWeaponSets == first + MPWEAPONSET_MAXSAVED);

		mpWeaponSetsSave("Second", slots);
		mpWeaponSetsTestCheck("a second set takes the next row",
				s_Num == 2 && mpWeaponSetsGet(1) != NULL
				&& strcmp(mpWeaponSetsGet(1)->name, "Second") == 0
				&& strcmp(langGet(g_MpWeaponSets[first + 1].name), "CX: Second") == 0);

		mpWeaponSetsTestCheck("deleting the first moves the second up",
				mpWeaponSetsDelete(0) && s_Num == 1
				&& strcmp(langGet(g_MpWeaponSets[first].name), "CX: Second") == 0);

		mpWeaponSetsTestCheck("deleting past the last is refused",
				!mpWeaponSetsDelete(5));

		s32 left = 0;

		for (s32 i = 0; i < func0f189058(false); i++) {
			if (mpWeaponSetsOwns(func0f188f9c(i))) {
				left++;
			}
		}

		mpWeaponSetsTestCheck("the row the delete vacated is out of the list", left == 1);

		// The weapons page's own rows, driven here the way netlobbysimtest.sh
		// walks a room's pages: what the player's keyboard row and the saved
		// set's list entry do, rather than what this module does when they do.
		{
			union handlerdata data;
			struct menuitem item;
			u8 picked[NUM_MPWEAPONSLOTS];

			memset(&data, 0, sizeof(data));
			memset(&item, 0, sizeof(item));

			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				picked[i] = (u8) ((i + 2) % 6);
				g_MpSetup.weapons[i] = picked[i];
			}

			data.keyboard.string = "UI Set";
			menuhandlerMpSaveCustomSet(MENUOP_SETTEXT, &item, &data);

			mpWeaponSetsTestCheck("the page's row saves the six slots",
					s_Num == 2 && mpWeaponSetsGet(1) != NULL
					&& strcmp(mpWeaponSetsGet(1)->name, "UI Set") == 0);

			struct menuitem slotitem = item;
			u8 saved_slots[NUM_MPWEAPONSLOTS];
			memcpy(saved_slots, mpWeaponSetsGet(1)->slots, NUM_MPWEAPONSLOTS);

			// The page follows a set it just saved, so the collision only
			// arrives from somewhere else: Custom, with the same name typed in
			// another case - the page's own question proven by the typing rather
			// than only by the store's lookup
			g_MpWeaponSetNum = WEAPONSET_CUSTOM;

			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				g_MpSetup.weapons[i] = (u8) ((i + 3) % 6);
			}

			data.keyboard.string = "ui set";
			menuhandlerMpSaveCustomSet(MENUOP_SETTEXT, &item, &data);

			mpWeaponSetsTestCheck("a name already taken is not written over by the page",
					memcmp(saved_slots, mpWeaponSetsGet(1)->slots, NUM_MPWEAPONSLOTS) == 0
					&& s_Num == 2);

			g_MpWeaponSetNum = mpWeaponSetsFirst() + 1;
			data.keyboard.string = "UI Set";
			menuhandlerMpSaveCustomSet(MENUOP_SETTEXT, &item, &data);

			mpWeaponSetsTestCheck("the set's own name saves it where it stands",
					memcmp(saved_slots, mpWeaponSetsGet(1)->slots, NUM_MPWEAPONSLOTS) != 0
					&& s_Num == 2);

			// The set back as the checks below read it
			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				g_MpSetup.weapons[i] = picked[i];
			}

			menuhandlerMpSaveCustomSet(MENUOP_SETTEXT, &item, &data);

			s32 offered = mpWeaponSetsOfferedIndex(1);

			if (offered >= 0) {
				mpSetWeaponSet(offered);
			}

			bool back = true;

			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				back = back && g_MpSetup.weapons[i] == picked[i];
			}

			mpWeaponSetsTestCheck("picking the set puts the six slots back", back);

			mpWeaponSetsTestCheck("a set as picked is not dirty",
					!mpWeaponSetsIsDirty(first + 1));

			g_MpSetup.weapons[0] = (u8) ((picked[0] + 1) % 6);

			mpWeaponSetsTestCheck("a slot moved off it is dirty",
					mpWeaponSetsIsDirty(first + 1));

						g_MpWeaponSetNum = first + 1;

			mpWeaponSetsTestCheck("the six slots take input for a saved set",
					!mpWeaponSlotsAreLocked());

			g_MpWeaponSetNum = 0;

			mpWeaponSetsTestCheck("and not for one of the game's sets",
					mpWeaponSlotsAreLocked());

			g_MpWeaponSetNum = WEAPONSET_CUSTOM;

			mpWeaponSetsTestCheck("and do for Custom", !mpWeaponSlotsAreLocked());

			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				g_MpSetup.weapons[i] = (u8) ((i + 3) % 6);
			}

			mpWeaponSetsUpdate(1, g_MpSetup.weapons);

			bool kept = true;

			for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
				kept = kept && g_MpWeaponSets[first + 1].slots[i]
					== g_MpWeapons[g_MpSetup.weapons[i]].weaponnum;
			}

			mpWeaponSetsTestCheck("saving a set that has its name already keeps it",
					s_Num == 2 && kept && mpWeaponSetsGet(1) != NULL
					&& strcmp(mpWeaponSetsGet(1)->name, "UI Set") == 0
					&& !mpWeaponSetsIsDirty(first + 1));

			s32 shown = func0f189058(false);
			bool shownFirst = true;
			bool roundtrip = true;
			s32 after = 0;

			// not `first`: that is the block's index in the table, declared
			// outside this block, and C lets a shadowing bool quietly win
			for (s32 i = 0; i < s_Num; i++) {
				shownFirst = shownFirst && func0f188f9c(i) == first + i;
			}

			for (s32 i = 0; i < shown; i++) {
				s32 index = func0f188f9c(i);

				roundtrip = roundtrip && func0f188f9c(mpCountWeaponSetThing(index)) == index;

				if (!mpWeaponSetsOwns(index)) {
					after++;
				}
			}
			mpWeaponSetsTestCheck("the player's sets are the list's first rows", shownFirst);
			mpWeaponSetsTestCheck("the list's numbering round-trips on every row", roundtrip);
			mpWeaponSetsTestCheck("the game's own sets still follow them in the list",
					after >= s_Num);
			mpWeaponSetsTestCheck("a default still means the game's first set",
					func0f188f9c(mpWeaponSetStockOfferedFirst()) < first
					&& !mpWeaponSetsOwns(func0f188f9c(mpWeaponSetStockOfferedFirst())));

			mpWeaponSetsSetForeignName(mpWeaponSetsGet(1)->name, first + 1);

			mpWeaponSetsTestCheck("a name the rules brought is shown for that row",
					mpWeaponSetsForeignNameFor(first + 1) != NULL
					&& strcmp(mpWeaponSetsForeignNameFor(first + 1), "UI Set") == 0
					&& strstr(mpGetWeaponSetName(mpCountWeaponSetThing(first + 1)), "UI Set") != NULL);

			mpWeaponSetsTestCheck("and only for that row",
					mpWeaponSetsForeignNameFor(first) == NULL
					&& mpWeaponSetsForeignNameFor(0) == NULL);

			mpWeaponSetsSetForeignName("", 0);

			mpWeaponSetsTestCheck("a room that named nothing names nothing here",
					mpWeaponSetsForeignNameFor(first + 1) == NULL);

			s32 findable = mpWeaponSetsSave("Findable", g_MpSetup.weapons);
			s32 again = mpWeaponSetsNum();

			{
				s32 before = mpGetWeaponSlot(0);

				g_MpWeaponSetNum = mpWeaponSetStockOfferedFirst();
				slotitem.param3 = 0;
				data.dropdown.value = (s32) ((before + 1) % mpGetNumWeaponOptions());
				menuhandlerMpWeaponSlot(MENUOP_SET, &slotitem, &data);

				mpWeaponSetsTestCheck("touching a slot on one of the game's sets makes it Custom",
						g_MpWeaponSetNum == WEAPONSET_CUSTOM
						&& mpGetWeaponSlot(0) != before
						&& mpWeaponSetsOwns(g_MpWeaponSetNum) == false);

				g_MpSetup.weapons[0] = (u8) before;
			}

			mpWeaponSetsTestCheck("one name is one set, whichever case it is typed in",
					findable >= 0 && mpWeaponSetsFind("findable") == findable
					&& mpWeaponSetsFind("FINDABLE") == findable
					&& mpWeaponSetsFind("No Such Set") == -1);

			mpWeaponSetsTestCheck("and a name taken is not saved a second time",
					mpWeaponSetsSave("Findable", g_MpSetup.weapons) < 0
					&& mpWeaponSetsNum() == again);

			mpWeaponSetsDelete(findable);

			mpWeaponSetsTestCheck("the list owns the set and no empty row",
					mpWeaponSetsOwns(first + 1) && !mpWeaponSetsOwns(first + MPWEAPONSET_MAXSAVED - 1));
		}

		remove(fsFullPath(s_Path));
	}

	if (s_TestFailed == 0) {
		sysLogPrintf(LOG_NOTE, "mpweaponsets test: all %d checks as expected", s_TestChecks);
	} else {
		sysLogPrintf(LOG_NOTE, "mpweaponsets test: %d of %d CHECKS FAILED", s_TestFailed, s_TestChecks);
	}
}

/**
 * The saved set's place in the list the dropdown shows.
 *
 * The dropdown speaks in offered sets - the rows the player can see, with the
 * feature-locked ones and the rows held for nothing cut out
 * (func0f188f9c) - and this module's own numbering counts the rows, empty or
 * not. So the two have to be met in the middle here rather than anywhere the
 * player's choice is used, and -1 means the set is not in the list at all.
 */
s32 mpWeaponSetsOfferedIndex(s32 index)
{
	s32 first = mpWeaponSetsFirst();

	if (first < 0 || index < 0 || index >= s_Num) {
		return -1;
	}

	for (s32 i = 0; i < func0f189058(false); i++) {
		if (func0f188f9c(i) == first + index) {
			return i;
		}
	}

	return -1;
}

/**
 * Whether the six slots have moved away from this set's guns.
 *
 * The page's rows are option indices and a set keeps weapon numbers, so the
 * two are met in the middle the same way mpApplyWeaponSet() meets them. This
 * is what a "Save" row asks before greying itself out: a set picked and left
 * alone has nothing to save again.
 */
bool mpWeaponSetsIsDirty(s32 listindex)
{
	const struct mpweaponsetentry *entry;
	s32 at = listindex - s_First;

	if (s_First < 0 || at < 0 || at >= s_Num) {
		return false;
	}

	entry = &s_Sets[at];

	for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		s32 option = g_MpSetup.weapons[i];
		u8 gun = (option >= 0 && option < NUM_MPWEAPONS)
			? g_MpWeapons[option].weaponnum : WEAPON_NONE;

		if (gun != entry->slots[i]) {
			return true;
		}
	}

	return false;
}

/**
 * A saved set's guns, written again where they lie.
 *
 * This is what Save Set means while one of the player's own sets is the one on
 * screen and its slots have moved: the set already has a name, and a second
 * entry with the same name and different guns is not a thing a player asked
 * for. The name is left alone, so a set is renamed by saving it as Custom
 * under a new name.
 */
bool mpWeaponSetsUpdate(s32 index, const u8 *optionslots)
{
	struct mpweaponsetentry *entry;

	if (index < 0 || index >= s_Num) {
		return false;
	}

	entry = &s_Sets[index];

	for (s32 i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		s32 option = optionslots[i];

		entry->slots[i] = (option >= 0 && option < NUM_MPWEAPONS)
			? g_MpWeapons[option].weaponnum : WEAPON_NONE;
	}

	if (mpWeaponSetsWriteFile() < 0) {
		return false;
	}

	mpWeaponSetsPutRow(index);

	return true;
}

/**
 * The name a player gave the set at this place in the list, or "".
 *
 * The bare name, without the `CX: ` the list puts on it: this is for the room's
 * rules, where a client wraps the host's own words around it
 * (netRulesWeaponSetShownFor), and a mark invented by this machine's list has
 * no business travelling.
 */
const char *mpWeaponSetsNamedFor(s32 listindex)
{
	s32 at = listindex - s_First;

	if (s_First < 0 || at < 0 || at >= s_Num) {
		return "";
	}

	return s_Sets[at].name;
}

/**
 * The rules' name for the set the room plays by, and where it belongs here.
 *
 * The client's own list cannot name a set its host saved by hand, and the
 * number it travels by is the host's numbering - read against this machine's
 * list it would name one of this machine's own sets. So the rules' name outranks
 * the list, for the one row the room is playing by and for nothing else. A room
 * that named nothing clears it, so a machine that played by a named set does not
 * go on naming its own afterwards.
 */
void mpWeaponSetsSetForeignName(const char *name, s32 listindex)
{
	snprintf(s_ForeignName, sizeof(s_ForeignName), "%s", name != NULL ? name : "");
	s_ForeignIndex = s_ForeignName[0] != '\0' ? listindex : -1;
}

const char *mpWeaponSetsForeignNameFor(s32 listindex)
{
	return (s_ForeignIndex >= 0 && listindex == s_ForeignIndex) ? s_ForeignName : NULL;
}

// The name on its own, for the migration snapshot to carry on
const char *mpWeaponSetsForeignName(void)
{
	return s_ForeignName;
}

// Case-insensitive equality, spelled out rather than strcasecmp: the same
// source builds against mingw, where the header that declares it has moved
static bool mpWeaponSetsNameEqual(const char *a, const char *b)
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
 * Which saved set goes by this name, or -1.
 *
 * Ignoring case: two rows the player reads as Pistols and pistols are one set
 * with two guns, and the name is the only thing that tells them apart in a list.
 */
s32 mpWeaponSetsFind(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return -1;
	}

	for (s32 i = 0; i < s_Num; i++) {
		if (mpWeaponSetsNameEqual(s_Sets[i].name, name)) {
			return i;
		}
	}

	return -1;
}
