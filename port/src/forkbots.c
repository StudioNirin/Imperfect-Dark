#include <stdio.h>
#include <string.h>
#include <ultra64.h>
#include <PR/ultratypes.h>
#include "platform.h"
#include "constants.h"
#include "types.h"
#include "data.h"
#include "bss.h"
#include "game/bondgun.h"
#include "game/botinv.h"
#include "game/game_0b0fd0.h"
#include "system.h"
#include "forkbots.h"

#ifndef PLATFORM_N64

/**
 * What a simulant may do with this fork's guns.
 *
 * A simulant's whole opinion of a weapon number is one row of
 * g_AibotWeaponPreferences (src/game/botinv.c), read through AIBOTPREF(), and a
 * row of zeroes is not "no opinion": botinvScoreWeapon() only reads the row's
 * scores when the row's goal flag for that firing function matches what the
 * caller asked for, so with the flag clear the scores stay 0 and the gun is
 * never wanted, never picked up, never fired. Every one of the fork's three guns
 * sat at that zero row until they had rows written for them, and nothing about
 * it failed - no log line, no crash, no wrong number. A simulant handed a Shock
 * Rifle by a mission simply stood there holding it, which reads as a broken gun
 * to anyone watching and as a working one to anyone who only tests with players.
 *
 * So this test reads the table the way the game does. It is not checking that
 * the numbers are tasteful; that is a hand on a controller. It is checking that
 * the table has rows at all, that the rows are reachable at these weapon numbers
 * rather than at the end of an array that stopped short of them, that the goals
 * they set are figures a simulant can actually reach in that ammunition, and that
 * the one gun whose host has opinions of its own is not read through them.
 *
 * What it cannot reach from here is whether a simulant that now wants the gun is
 * any good with it, and whether the charge on the Rail-Laser behaves in bot hands
 * at all. The first is a match with 80 of them in it; the second is bot.c's
 * charge, which has its own gate (tools/ci/raillaser.sh) for the player's half.
 */

#define FORKBOT_CHECK(cond, what) \
	do { \
		checks++; \
		if (cond) { \
			sysLogPrintf(LOG_NOTE, "forkbot test: ok   %s", what); \
		} else { \
			sysLogPrintf(LOG_NOTE, "forkbot test: FAIL %s", what); \
			failed++; \
		} \
	} while (0)

void forkbotSelfTest(void)
{
	s32 checks = 0;
	s32 failed = 0;
	s32 pri1;
	s32 pri2;
	s32 sec1;
	s32 sec2;
	s32 cross1;
	s32 cross2;

	// The bound, asked of the code rather than of the enum. AIBOTPREF() indexes
	// this table by weapon number for anything that is not GoldenEye's, so a
	// number past its end is a read of whatever follows it - which is what a
	// fourth gun appended to the enum would walk into if the bound were written
	// as a count instead of following NUM_WEAPONS (BOTINV_MAX_WEAPONPREFS).
	FORKBOT_CHECK(botinvGetNumWeaponPreferences() > WEAPON_SHOCKORB,
			"the preference table reaches past this fork's last weapon number");

	// A row with no score is a gun that does not exist as far as a simulant is
	// concerned, whatever else the row says.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_RAILLASER).unk00 > 0, "the Rail-Laser has a score a simulant can want it for");
	FORKBOT_CHECK(AIBOTPREF(WEAPON_BOWCASTER).unk00 > 0, "and the Bowcaster");
	FORKBOT_CHECK(AIBOTPREF(WEAPON_SHOCKRIFLE).unk00 > 0, "and the Shock Rifle");

	// The flag is the gate in front of the score (botinvScoreWeapon's first
	// condition), so a row that scores well and flags nothing still answers 0.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_RAILLASER).haspriammogoal
			&& AIBOTPREF(WEAPON_BOWCASTER).haspriammogoal
			&& AIBOTPREF(WEAPON_SHOCKRIFLE).haspriammogoal,
			"each asks for ammunition for its primary, which is what lets its score be read at all");

	// Read the way the game reads it, through the scorer rather than the struct,
	// because the scorer is where a case for the host can override the row.
	botinvScoreWeaponByItself(NULL, WEAPON_SHOCKRIFLE, FUNC_PRIMARY, -1, false, &pri1, &pri2);
	botinvScoreWeaponByItself(NULL, WEAPON_SHOCKRIFLE, FUNC_SECONDARY, -1, false, &sec1, &sec2);
	FORKBOT_CHECK(pri1 > 0, "the scorer will score a Shock Rifle's beam for a simulant");
	FORKBOT_CHECK(sec1 > 0, "and the orb it lobs as the gun's second function");

	botinvScoreWeaponByItself(NULL, WEAPON_RAILLASER, FUNC_PRIMARY, -1, false, &pri1, &pri2);
	FORKBOT_CHECK(pri1 > 0, "and a Rail-Laser's charged shot");

	// The Bowcaster hosts on the crossbow so that everything number-keyed comes
	// with the host (weaponHost(), data.h), and this scorer switches on the host,
	// so without botScoringWeapon() the crossbow's case answers for it: 49 for
	// the primary, a figure written for a gun that fires one bolt which sticks in
	// what it hits and can be walked back and pulled out again. That is not this
	// gun, whose bolt bounces and is spent and whose clip holds thirty.
	botinvScoreWeaponByItself(NULL, WEAPON_BOWCASTER, FUNC_PRIMARY, -1, false, &pri1, &pri2);
	botinvScoreWeaponByItself(NULL, WEAPON_CROSSBOW, FUNC_PRIMARY, -1, false, &cross1, &cross2);
	FORKBOT_CHECK(cross1 > 0 && pri1 > 0 && pri1 != cross1,
			"the Bowcaster is scored on its own row, not on the crossbow's judgement");

	// The distance band is how far a simulant tries to stand from what it is
	// shooting. Both of these are moves away from the host's row: the Rail-Laser
	// off the Moonraker's, which is a stream-weapon's spacing, and the Shock
	// Rifle's orb onto the band every explosive weapon uses, because an orb that
	// goes off beside you is worse than one that misses.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_RAILLASER).pridistconfig == BOTDISTCFG_FARSIGHT
			&& AIBOTPREF(WEAPON_RAILLASER).pridistconfig != AIBOTPREF(WEAPON_LASER).pridistconfig,
			"the Rail-Laser keeps a long gun's distance rather than the Moonraker's");
	FORKBOT_CHECK(AIBOTPREF(WEAPON_SHOCKRIFLE).secdistconfig == BOTDISTCFG_SHOOTEXPLOSIVE,
			"and the Shock Rifle's orb is shot from the standoff explosives are");

	// Goals a simulant can reach. targetammo is the figure it stops hunting
	// ammunition at and criticalammo the one it will cross the arena for, so a
	// target above what the ammunition can hold is a simulant that never stops
	// going after crates, and a critical of zero is one that never goes at all.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_RAILLASER).criticalammopri > 0
			&& AIBOTPREF(WEAPON_RAILLASER).targetammopri <= bgunGetCapacityByAmmotype(AMMOTYPE_RAIL),
			"the Rail-Laser's ammunition goals fit inside its five-round reserve");
	FORKBOT_CHECK(AIBOTPREF(WEAPON_BOWCASTER).criticalammopri > 0
			&& AIBOTPREF(WEAPON_BOWCASTER).targetammopri <= bgunGetCapacityByAmmotype(AMMOTYPE_BOWCASTER),
			"the Bowcaster's inside its bolts");
	FORKBOT_CHECK(AIBOTPREF(WEAPON_SHOCKRIFLE).criticalammopri > 0
			&& AIBOTPREF(WEAPON_SHOCKRIFLE).targetammopri <= bgunGetCapacityByAmmotype(AMMOTYPE_SHOCK),
			"and the Shock Rifle's inside its cells");

	// reloaddelay is three bits (types.h), so 8 is 0 and a simulant reloads the
	// instant it is empty; 0 on a gun that has a magazine to fill is the same
	// mistake made on purpose.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_RAILLASER).reloaddelay > 0
			&& AIBOTPREF(WEAPON_BOWCASTER).reloaddelay > 0
			&& AIBOTPREF(WEAPON_SHOCKRIFLE).reloaddelay > 0
			&& AIBOTPREF(WEAPON_SHOCKRIFLE).reloaddelay <= 7,
			"each gives a simulant a real, in-range reload delay");

	// WEAPONFLAG_AICANUSE is not what lets a simulant shoot: it is read by
	// playerChooseThirdPersonAnimation() and by the firing poses in chraction.c,
	// so without it anyone seen from behind holds the gun in bare hands. Every
	// host these three stand on carries it, and when the row was written the
	// flag was left off for a reason that has since gone away: the table used to
	// end before these numbers, so the row could not exist.
	FORKBOT_CHECK(weaponHasFlag(WEAPON_RAILLASER, WEAPONFLAG_AICANUSE)
			&& weaponHasFlag(WEAPON_BOWCASTER, WEAPONFLAG_AICANUSE)
			&& weaponHasFlag(WEAPON_SHOCKRIFLE, WEAPONFLAG_AICANUSE),
			"all three are carried as guns in the third-person animations");

	// And what must stay un-wanted: the fired bolt and the fired orb are props
	// with weapon rows so the engine can give them behaviour, not guns a
	// simulant could equip and fire. A row for either would put a projectile in
	// the list of things worth walking across a room for.
	FORKBOT_CHECK(AIBOTPREF(WEAPON_BOWCASTERBOLT).unk00 == 0
			&& AIBOTPREF(WEAPON_SHOCKORB).unk00 == 0,
			"the fired bolt and the fired orb are still not things to want");

	if (!failed) {
		sysLogPrintf(LOG_NOTE, "forkbot test: all %d checks as expected", checks);
	} else {
		sysLogPrintf(LOG_NOTE, "forkbot test: %d of %d checks failed", failed, checks);
	}
}

#endif
