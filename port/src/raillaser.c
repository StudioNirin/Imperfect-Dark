#include <stdio.h>
#include <string.h>
#include <ultra64.h>
#include <PR/ultratypes.h>
#include "platform.h"
#include "constants.h"
#include "types.h"
#include "data.h"
#include "bss.h"
#include "game/lang.h"
#include "game/savebuffer.h"
#include "game/bondgun.h"
#include "game/challenge.h"
#include "game/mplayer/mplayer.h"
#include "system.h"
#include "raillaser.h"

#ifndef PLATFORM_N64

/**
 * The Rail-Laser: this fork's own gun.
 *
 * The row is in invitems.c (invitem_raillaser, WEAPON_RAILLASER) and it is a
 * Moonraker Laser in every respect it has not been given its own of yet - the
 * model, the hand position, the equip animation, the firing sound, and here,
 * the name. This file is where the things the row cannot carry in a static
 * initialiser go: text the language banks do not have, and the GoldenEye
 * XBLA Moonraker's model laid over the borrowed one when a player has the
 * XBLA package - which is the whole point of the gun, and is not written yet.
 *
 * What the gun does that the Moonraker does not: it holds one shot, out of
 * five, and it has to be charged by holding the trigger, which is
 * bgunChargeTick() in bondgun.c. Releasing early costs nothing and fires
 * nothing. Its zoom is the AR34's, on the aim input, because in Perfect Dark
 * a zoom is a mode on the aim input rather than a gun function of its own.
 *
 * Simulants cannot have it: no WEAPONFLAG_AICANUSE, because a simulant's
 * charge is its own state (aibot->maulercharge[] is the Mauler's copy,
 * bot.c) and none of that is written for this gun. No mission hands it out
 * either - no stage's item table can name a weapon number the ROM never had,
 * so it reaches a player through the multiplayer lists and the Combat
 * Simulator only.
 */

static const char s_RailLaserName[] = "Rail-Laser\n";
static const char s_RailLaserChargeName[] = "Charged Shot\n";

/**
 * Whether this weapon number is the Rail-Laser's, in either function.
 *
 * The three places bondgun.c lets a gun be itself instead of the general case
 * ask this, so that the charge is one rule in one file rather than a weapon
 * number repeated down the file. The host of the Rail-Laser is itself
 * (weaponHost()), so nothing else in the game treats it as GoldenEye's.
 */
bool raillaserOwn(s32 weaponnum)
{
	return weaponnum == WEAPON_RAILLASER;
}

/**
 * How long the trigger has to be held: one and a half seconds.
 *
 * In the same 60th-of-a-second base as hand->stateframes, which is what the
 * charge counts, so it keeps its length above 60 fps the way the GoldenEye
 * grenade cook's does (bgunTickIncAttackingGeThrow()).
 */
s32 raillaserCharge60(void)
{
	return TICKS(90);
}

void raillaserInit(void)
{
	struct weapon *gun = g_Weapons[WEAPON_RAILLASER];
	u16 name;

	if (!(name = langAddPortText(s_RailLaserName))) {
		// The port text bank (512) is full: the row keeps the Moonraker's
		// name, which is wrong but reads, and nothing else about the gun
		// changes.
		return;
	}

	gun->shortname = name;
	gun->name = name;

	if ((name = langAddPortText(s_RailLaserChargeName))) {
		((struct weaponfunc *) gun->functions[FUNC_PRIMARY])->name = name;
	}
}

/**
 * Whether the Rail-Laser is still charging a shot.
 *
 * The whole of what bondgun.c asks where it lets a gun be itself: `frames` is
 * the hand's stateframes, the ticks since it came into the attacking state,
 * and `spent` is hand->primetimer60, which is -1 once the press has fired - a
 * press fires one shot, and the trigger has to be let go for another.
 */
bool raillaserCharging(s32 frames, s32 spent)
{
	if (spent < 0) {
		return true;
	}

	return frames < raillaserCharge60();
}

#define RAILLASER_CHECK(cond, what) \
	do { \
		checks++; \
		if (cond) { \
			sysLogPrintf(LOG_NOTE, "raillaser test: ok   %s", what); \
		} else { \
			sysLogPrintf(LOG_NOTE, "raillaser test: FAIL %s", what); \
			failed++; \
		} \
	} while (0)

/**
 * The Rail-Laser's own decisions and the wiring under them (--test-raillaser).
 *
 * Written because the two ways this gun could have been added both went wrong
 * before they were fixed, and neither is visible from the row: the GoldenEye
 * block used to run to the end of the weapon enum, so a number appended to it
 * was read as a 42nd GoldenEye gun, and the curated random weapon list is one
 * bit per row in a saved setup, so a row appended to the table moves every
 * field below it by a bit. Both are pinned here by a literal, and the charge
 * is checked at its edges.
 */
void raillaserSelfTest(void)
{
	const struct weapon *gun = g_Weapons[WEAPON_RAILLASER];
	struct savebuffer written;
	struct savebuffer readback;
	u8 keep[NUM_MPWEAPONS];
	s32 checks = 0;
	s32 failed = 0;
	s32 i;

	// The row, and what the game reads off it. The rows themselves (invitem_
	// invfunc_ invammo_) have no declarations outside invitems.c, so this asks
	// the questions the game asks rather than naming the objects.
	RAILLASER_CHECK(gun != NULL && gun->hi_model == FILE_GLASER, "the weapon table has the Rail-Laser, on the Moonraker's model for now");
	RAILLASER_CHECK(gun->functions[FUNC_SECONDARY] == NULL, "it has one function: a zoom is the aim input here, not an alt-fire");
	RAILLASER_CHECK(((struct weaponfunc *) gun->functions[FUNC_PRIMARY])->type == INVENTORYFUNCTYPE_SHOOT_SINGLE, "its shot is a single hitscan beam, not a projectile");
	RAILLASER_CHECK(gun->ammos[FUNC_PRIMARY] != NULL && gun->ammos[FUNC_PRIMARY]->clipsize == 1, "one shot a magazine");
	RAILLASER_CHECK(gun->ammos[FUNC_PRIMARY]->reload_animation != NULL, "and it has an animation to reload with");
	RAILLASER_CHECK(bgunGetAmmoTypeForWeapon(WEAPON_RAILLASER, FUNC_PRIMARY) == AMMOTYPE_RAIL, "its ammunition is the Rail-Laser's own type");
	RAILLASER_CHECK(bgunGetAmmoCapacityForWeapon(WEAPON_RAILLASER, FUNC_PRIMARY) == 5, "five of them, which is its own type's capacity");
	RAILLASER_CHECK(!(gun->flags & WEAPONFLAG_AICANUSE), "no simulant can be given it");
	RAILLASER_CHECK(!(gun->flags3 & WEAPONFLAG3_FREESHOTS), "it runs out of ammunition, unlike the Moonraker");
	RAILLASER_CHECK((gun->aimsettings->flags & INVAIMFLAG_MANUALZOOM) && gun->aimsettings->zoomfov == 20.0f, "its zoom is the AR34's, on the aim input");
	RAILLASER_CHECK(gun->shortname != L_GUN_047, "it is named for itself, not for the Moonraker");

	// The number, and the GoldenEye block it sits past
	RAILLASER_CHECK(NUM_GE_WEAPONS == 41, "GoldenEye's guns are still 41: the block ends at its own last number");
	RAILLASER_CHECK(!WEAPON_IS_GE(WEAPON_RAILLASER), "the Rail-Laser is not one of GoldenEye's");
	RAILLASER_CHECK(WEAPON_IS_GE(WEAPON_GE_TASER), "and GoldenEye's last gun still is");
	RAILLASER_CHECK(weaponHost(WEAPON_RAILLASER) == WEAPON_RAILLASER, "and its host is itself");
	RAILLASER_CHECK(NUM_WEAPONS <= WEAPON_MPLOCATION00, "below the pads' numbers");

	// Appended, not inserted: the rows a saved setup, a saved set and a match's
	// RULES name are found at the same index they were before the Rail-Laser
	RAILLASER_CHECK(g_MpWeapons[0x21].weaponnum == WEAPON_LASER, "the Moonraker's row is still at 0x21");
	RAILLASER_CHECK(g_MpWeapons[0x50].weaponnum == WEAPON_GE_TASER, "GoldenEye's last row is still at 0x50");
	RAILLASER_CHECK(g_MpWeapons[MPWEAPON_RAILLASER].weaponnum == WEAPON_RAILLASER, "and the Rail-Laser's row is the one past it");
	RAILLASER_CHECK(NUM_MPWEAPONS == MPWEAPON_RAILLASER + 1, "and is the last row there is");
	RAILLASER_CHECK(g_MpWeapons[MPWEAPON_RAILLASER].priammotype == AMMOTYPE_RAIL, "its hand-out names the Rail-Laser's ammunition");

	// Offered to a player, and where a page puts it. The pages index the
	// unlocked rows rather than the table, so this walks them the way one does -
	// which is also the answer to "where in the list is it".
	{
		s32 display = 0;

		for (i = 0; i < MPWEAPON_RAILLASER; i++) {
			if (challengeIsFeatureUnlocked(g_MpWeapons[i].unlockfeature)) {
				display++;
			}
		}

		RAILLASER_CHECK(strstr(mpGetWeaponLabel(display), "Rail-Laser") != NULL, "the weapon list offers it, and by its own name");
		sysLogPrintf(LOG_NOTE, "raillaser test: note  row %d of %d in the weapon list; a stage's roll takes it: %s",
				display + 1, mpGetNumWeaponOptions(),
				mpWeaponRowSuitsStage(&g_MpWeapons[MPWEAPON_RAILLASER]) ? "yes" : "no");
	}

	RAILLASER_CHECK(challengeIsFeatureUnlocked(g_MpWeapons[MPWEAPON_RAILLASER].unlockfeature), "nothing has to be unlocked for it");
	RAILLASER_CHECK(mpCanSpawnWithWeapon(&g_MpWeapons[MPWEAPON_RAILLASER]), "a spawn can hand it out");

	// The charge, at its edges
	RAILLASER_CHECK(raillaserCharge60() == TICKS(90), "the charge is one and a half seconds");
	RAILLASER_CHECK(raillaserCharging(TICKS(90) - 1, 0), "a tick short of it, the gun holds");
	RAILLASER_CHECK(!raillaserCharging(TICKS(90), 0), "at it, the gun fires");
	RAILLASER_CHECK(raillaserCharging(TICKS(400), -1), "and once the press has fired, holding does nothing");

	// The saved setup, both widths. The list is one bit per row, so the row
	// appended for this gun moves everything written after it by one bit: a
	// version 5 file is read at 81 bits and a version 6 one at 82 (mplayer.c).
	memcpy(keep, g_MpWeaponSetRandomFilters, sizeof(keep));

	for (i = 0; i < NUM_MPWEAPONS; i++) {
		g_MpWeaponSetRandomFilters[i] = (i % 3) != 0;
	}

	savebufferClear(&written);
	mpsetupfileSaveWad(&written, MPSETUP_VERSION_LATEST);
	savebufferClear(&readback);
	memcpy(readback.bytes, written.bytes, MPSETUP_BLOCKSIZE);
	memset(g_MpWeaponSetRandomFilters, 0, sizeof(keep));
	mpsetupfileLoadWad(&readback, MPSETUP_VERSION_LATEST);

	{
		bool back = true;

		for (i = 0; i < NUM_MPWEAPONS; i++) {
			if ((g_MpWeaponSetRandomFilters[i] != 0) != ((i % 3) != 0)) {
				back = false;
			}
		}

		RAILLASER_CHECK(back, "a version 6 setup keeps the curated list row for row");
	}

	// An older file, read the way it was written: its last list row is the last
	// GoldenEye one, and the Rail-Laser's bit was not in it, so the row is
	// selected - the state a file that cannot name the row cannot have refused.
	savebufferClear(&written);
	mpsetupfileSaveWad(&written, MPSETUP_VERSION_SLOTGROUPS);
	savebufferClear(&readback);
	memcpy(readback.bytes, written.bytes, MPSETUP_BLOCKSIZE);
	memset(g_MpWeaponSetRandomFilters, 0, sizeof(keep));
	mpsetupfileLoadWad(&readback, MPSETUP_VERSION_SLOTGROUPS);

	RAILLASER_CHECK(g_MpWeaponSetRandomFilters[NUM_MPWEAPONS - 1] != 0, "a version 5 setup reads with the Rail-Laser selected");
	RAILLASER_CHECK(g_MpWeaponSetRandomFilters[NUM_MPWEAPONS - 2] != 0, "and its own last row came from the file");

	memcpy(g_MpWeaponSetRandomFilters, keep, sizeof(keep));

	if (failed) {
		sysLogPrintf(LOG_NOTE, "raillaser test: %d of %d checks failed", failed, checks);
	} else {
		sysLogPrintf(LOG_NOTE, "raillaser test: all %d checks as expected", checks);
	}
}

#endif
