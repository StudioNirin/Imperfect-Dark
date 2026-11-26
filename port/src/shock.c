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
#include "game/game_0b0fd0.h"
#include "game/challenge.h"
#include "game/playermgr.h"
#include "game/propobj.h"
#include "game/botact.h"
#include "game/botinv.h"
#include "game/mplayer/mplayer.h"
#include "system.h"
#include "shock.h"

#ifndef PLATFORM_N64

/**
 * The Shock Rifle (WEAPON_SHOCKRIFLE, invitems.c's invitem_shockrifle): this
 * fork's third own gun, and the first whose two firing modes are a combination
 * rather than two versions of one shot. The primary is an instant-hit plasma
 * beam; the secondary lobs a large, slow orb that bounces off surfaces; and a
 * beam that reaches an orb already in the air detonates it, once, in a single
 * burst rather than the rocket's two seconds of flame.
 *
 * The row lives in invitems.c and the gun borrows the AR34's body, pose, reload
 * and name text until it has any of them of its own. What belongs here instead
 * is what a static initialiser cannot carry: the detonation's rules (stage 3),
 * and these checks.
 *
 * There are checks here at this stage rather than after the detonation exists
 * because the failure modes of the shell are not visible from the row, and two
 * of them are on this fork's own record. A weapon number missing from a beam
 * whitelist fires with nothing drawn in the air (the Rail-Laser fired invisibly
 * for a week that way, and the whitelists are in bondgun.c and chraction.c, so
 * tools/ci/shock.sh greps them rather than this file, which cannot see them). An
 * ammunition type missing from a pickup-name switch gives "Picked up ." with no
 * name and no determiner, and that switch is a switch rather than a table, so it
 * fails silently at run time and quietly at compile time. The rest pin the two
 * figures the gun was designed around, the orb's row having to *not* carry the
 * flag that would let any bullet in an arena pop every orb, and the Combat
 * Simulator's row at the index a saved setup names it by.
 */

#define SHOCK_CHECK(cond, what) \
	do { \
		checks++; \
		if (cond) { \
			sysLogPrintf(LOG_NOTE, "shock test: ok   %s", what); \
		} else { \
			sysLogPrintf(LOG_NOTE, "shock test: FAIL %s", what); \
			failed++; \
		} \
	} while (0)

s32 shockAmmoCostForFunc(s32 weaponnum, s32 funcnum)
{
	// The orb has its own pool now, the way the SuperDragon's grenades do, so
	// one orb costs one round in that pool. The beam still spends one cell of
	// AMMOTYPE_SHOCK. Keeping the helper at every call site means simulants
	// pay the same price as the player.
	(void) weaponnum;
	(void) funcnum;
	return 1;
}

void shockSelfTest(void)
{
	const struct weapon *gun = g_Weapons[WEAPON_SHOCKRIFLE];
	const struct weapon *orb = g_Weapons[WEAPON_SHOCKORB];
	const struct weaponfunc_shootsingle *beam;
	const struct weaponfunc_shootprojectile *ball;
	const struct weaponfunc_shootprojectile *bowcasterbolt;
	struct savebuffer written;
	struct savebuffer readback;
	u8 keep[NUM_MPWEAPONS];
	s32 checks = 0;
	s32 failed = 0;
	s32 i;

	SHOCK_CHECK(gun != NULL && orb != NULL
			&& gun->functions[FUNC_PRIMARY] != NULL && gun->functions[FUNC_SECONDARY] != NULL,
			"the Shock Rifle exists with two firing modes, and its orb has a row of its own");

	if (gun == NULL || orb == NULL || gun->functions[FUNC_PRIMARY] == NULL
			|| gun->functions[FUNC_SECONDARY] == NULL) {
		sysLogPrintf(LOG_NOTE, "shock test: %d of %d checks failed", failed + 1, checks);
		return;
	}

	// The orb number is the projectile's identity, not another Shock Rifle to
	// pick up. It has no firing function, no ammo and no display name of its
	// own, and it cannot be cycled to in the weapon wheel.
	SHOCK_CHECK(orb->functions[FUNC_PRIMARY] == NULL && orb->functions[FUNC_SECONDARY] == NULL
			&& orb->ammos[0] == NULL,
			"the orb row carries no gun a player could hold");
	SHOCK_CHECK(!INV_CYCLEABLE(WEAPON_SHOCKORB),
			"the orb cannot be cycled to as a second weapon");
	SHOCK_CHECK(g_Weapons[WEAPON_SHOCKRIFLE]->name != g_Weapons[WEAPON_SHOCKORB]->name,
			"a stray orb can never be mistaken for another Shock Rifle");

	beam = (const struct weaponfunc_shootsingle *) gun->functions[FUNC_PRIMARY];
	ball = (const struct weaponfunc_shootprojectile *) gun->functions[FUNC_SECONDARY];
	bowcasterbolt = (const struct weaponfunc_shootprojectile *)
		g_Weapons[WEAPON_BOWCASTER]->functions[FUNC_PRIMARY];

	// The two modes, and the shape of the shot each one is. The primary is the
	// engine's own instant hit: every gun of this function type is resolved by
	// one trace from the camera (shotCreate -> shotCalculateHits), which is why
	// the beam has no projectile of its own and no damage code of its own.
	SHOCK_CHECK(beam->base.base.type == INVENTORYFUNCTYPE_SHOOT_SINGLE,
			"the primary is the engine's own instant hit, not a fast projectile");
	SHOCK_CHECK(ball->base.base.type == INVENTORYFUNCTYPE_SHOOT_PROJECTILE,
			"the secondary is a shot that travels");
	SHOCK_CHECK(beam->base.damage == 1.5f, "the beam does the 1.5 it was specified at");
	SHOCK_CHECK(ball->base.damage == 2.5f, "and the orb 2.5 on a direct hit");
	SHOCK_CHECK(beam->base.shootsound == SFX_813E,
			"the lance fires with the Farsight's brighter discharge");
	SHOCK_CHECK(ball->projectilemodelnum == MODEL_CHRFLASHBANG && ball->scale >= 1.5f,
			"the orb is a large round ball, not a grenade");

	// Five cells for an orb, one for everything else. The figure is read by the
	// player's ammunition and by a simulant's, so a wrong answer is either a gun
	// that spends its whole reserve in ten shots or two people paying different
	// prices for the same orb.
	SHOCK_CHECK(shockAmmoCostForFunc(WEAPON_SHOCKRIFLE, FUNC_SECONDARY) == 1,
			"an orb spends one round of its own pool");
	SHOCK_CHECK(shockAmmoCostForFunc(WEAPON_SHOCKRIFLE, FUNC_PRIMARY) == 1,
			"and a beam still spends one cell");
	SHOCK_CHECK(shockAmmoCostForFunc(WEAPON_CROSSBOW, FUNC_PRIMARY) == 1,
			"and no other gun's price has moved");

	// Two pools on one weapon, as the SuperDragon has: the beam's cells in
	// ammoindex 0 and the orbs in ammoindex 1. The HUD follows the function's
	// ammoindex, so switching to Orb now shows the other count.
	SHOCK_CHECK(beam->base.base.ammoindex == 0 && ball->base.base.ammoindex == 1
			&& gun->ammos[0] != NULL && gun->ammos[1] != NULL
			&& gun->ammos[0]->type == AMMOTYPE_SHOCK
			&& gun->ammos[1]->type == AMMOTYPE_SHOCKORB,
			"the beam and the orb draw from two pools on one gun");
	SHOCK_CHECK(bgunGetAmmoTypeForWeapon(WEAPON_SHOCKRIFLE, FUNC_PRIMARY) == AMMOTYPE_SHOCK,
			"and the game finds the beam's type from the gun");
	SHOCK_CHECK(bgunGetAmmoTypeForWeapon(WEAPON_SHOCKRIFLE, FUNC_SECONDARY) == AMMOTYPE_SHOCKORB,
			"the secondary now names its own reserve, as the SuperDragon's does");
	SHOCK_CHECK(bgunGetCapacityByAmmotype(AMMOTYPE_SHOCK) == 100, "a hundred cells carried");
	SHOCK_CHECK(gun->ammos[0]->clipsize == 50, "and fifty in the magazine");
	SHOCK_CHECK(bgunGetCapacityByAmmotype(AMMOTYPE_SHOCKORB) == 20
			&& gun->ammos[1]->clipsize == 10,
			"twenty orbs carried, ten to the magazine");

	// The number, and the tables keyed by it. WEAPON_IS_FORK is deliberately
	// anchored on the end of the enum rather than on the first fork gun, so the
	// third fork gun is in it without anything being widened (data.h).
	SHOCK_CHECK(WEAPON_IS_FORK(WEAPON_SHOCKRIFLE) && INV_CYCLEABLE(WEAPON_SHOCKRIFLE),
			"it is a fork gun, and the hands can cycle to it");
	SHOCK_CHECK(weaponHost(WEAPON_SHOCKRIFLE) == WEAPON_SHOCKRIFLE
			&& weaponHost(WEAPON_SHOCKORB) == WEAPON_SHOCKORB,
			"it hosts itself, and so does its orb - so the look it borrows is not the number it behaves as");
	SHOCK_CHECK(gunfuncBitIndex(WEAPON_SHOCKRIFLE) == WEAPON_SHOCKRIFLE
			&& gunfuncBitIndex(WEAPON_SHOCKRIFLE) < GUNFUNCS_BYTES * 8,
			"its saved mode choice has a bit of its own, inside the block");
	SHOCK_CHECK(gunfuncBitIndex(WEAPON_SHOCKRIFLE) != gunfuncBitIndex(WEAPON_AR34),
			"and that bit is not the AR34's, whose body it borrows - the two guns' modes are independent");

	// The orb's row. Every one of these is a thing the row must NOT carry: with
	// STICKSTOWALL it would stick in the wall instead of bouncing, with BLADEHIT
	// it would embed like the crossbow's bolt, with EXPLODESWHENSHOT - which the
	// Rocket Launcher's rounds do carry - any bullet in an arena would pop every
	// orb in it, and the design says this gun's beam alone does that.
	SHOCK_CHECK(!(orb->flags2 & WEAPONFLAG2_STICKSTOWALL) && !(orb->flags2 & WEAPONFLAG2_BLADEHIT),
			"the orb neither sticks in the wall nor embeds in a body");
	SHOCK_CHECK(!(orb->flags2 & WEAPONFLAG2_EXPLODESWHENSHOT),
			"and it does not carry the any-bullet-sets-it-off flag: only the beam will");
	SHOCK_CHECK((orb->flags2 & WEAPONFLAG2_NOPICKUPINFLIGHT) != 0,
			"nor is a live orb collectible as ammunition");

	// The orb's flight, against the Bowcaster's bolt: same powered flat flight
	// and the same near-perfect bounce, but a slower shot that lives longer, and
	// - the one thing the bolt is not - a model you can see and therefore hit.
	// objTestHit() hit-tests the model's boxes, so a tiny mesh would be a beam
	// that passes straight through the thing it is meant to detonate.
	SHOCK_CHECK((ball->base.base.flags & FUNCFLAG_PROJECTILE_POWERED) != 0,
			"the orb flies flat rather than arcing to the floor");
	SHOCK_CHECK(ball->reflectangle > 0.8f, "and it keeps bouncing off surfaces");
	SHOCK_CHECK(bowcasterbolt != NULL && ball->traveldist < bowcasterbolt->traveldist,
			"it travels slower than the Bowcaster's bolt, so it hangs in the air");
	SHOCK_CHECK(ball->timer60 >= 240, "and it lives long enough to be shot at");
	SHOCK_CHECK(ball->projectilemodelnum == MODEL_CHRFLASHBANG,
			"its shot is a round ball loaded from this row, not a grenade");
	SHOCK_CHECK(ball->scale >= 1.0f, "at a scale big enough for a beam to hit it");

	// The beam's picture. The tracer itself is drawn from a whitelist in
	// bondgun.c and another in chraction.c - neither visible from here, so
	// tools/ci/shock.sh checks both by adjacency - and these flags are the other
	// half: which beam is drawn, and that bodies draw one at all.
	SHOCK_CHECK((gun->flags2 & (WEAPONFLAG2_LASERBEAM | WEAPONFLAG2_CROSSBEAM | WEAPONFLAG2_LASERHIT))
			== (WEAPONFLAG2_LASERBEAM | WEAPONFLAG2_CROSSBEAM | WEAPONFLAG2_LASERHIT),
			"the shot draws a laser bolt, not a bullet tracer");
	SHOCK_CHECK((gun->flags3 & WEAPONFLAG3_CHRSHOTBEAM) != 0,
			"and other players see it: a gun without this flag fires unseen from every body but the player's own");
	SHOCK_CHECK((beam->base.base.flags & FUNCFLAG_NOMUZZLEFLASH) == 0,
			"the beam keeps its muzzle flash, which is the shot's picture on the borrowed model");

	// Held and lying on the floor: playermgrGetModelOfWeapon answers -1 for
	// anything past WEAPON_PSYCHOSISGUN unless the number is named beside a
	// model, which is the whole of this fork's block. The Combat Simulator's row
	// carries a model of its own, and the two have to agree or a gun on the floor
	// and a gun in a body are different guns.
	SHOCK_CHECK(playermgrGetModelOfWeapon(WEAPON_SHOCKRIFLE) == MODEL_CHRAR34,
			"a body holding it holds the AR34-shaped gun, not nothing");
	SHOCK_CHECK(playermgrGetModelOfWeapon(WEAPON_AR34) == MODEL_CHRAR34,
			"and the AR34's own answer is unmoved by the borrowing");
	SHOCK_CHECK(g_MpWeapons[MPWEAPON_SHOCKRIFLE].weaponnum == WEAPON_SHOCKRIFLE
			&& g_MpWeapons[MPWEAPON_SHOCKRIFLE].model == playermgrGetModelOfWeapon(WEAPON_SHOCKRIFLE),
			"the Combat Simulator's row is the gun, and its floor model is the model a body holds");
	SHOCK_CHECK(g_MpWeapons[MPWEAPON_SHOCKRIFLE].priammotype == AMMOTYPE_SHOCK
			&& g_MpWeapons[MPWEAPON_SHOCKRIFLE].priammoqty == 50
			&& g_MpWeapons[MPWEAPON_SHOCKRIFLE].secammotype == AMMOTYPE_SHOCKORB
			&& g_MpWeapons[MPWEAPON_SHOCKRIFLE].secammoqty == 10,
			"its spot stocks both pools, one magazine of each");
	SHOCK_CHECK(g_MpWeapons[MPWEAPON_SHOCKRIFLE].hasweapon
			&& challengeIsFeatureUnlocked(g_MpWeapons[MPWEAPON_SHOCKRIFLE].unlockfeature)
			&& mpCanSpawnWithWeapon(&g_MpWeapons[MPWEAPON_SHOCKRIFLE]),
			"nothing unlocks it, and a spawn can hand it out");

	// Simulants: WEAPONFLAG_AICANUSE, as every one of the hosts these three guns
	// stand on carries it. It is the seam the third-person animations read, so a
	// gun without it is held in bare hands by anyone seen from behind; whether a
	// simulant wants the gun at all is the preference table's decision, and the
	// table has a row for it (botinv.c, checked by --test-forkbots). The reason it
	// used to be off - that the table was indexed by weapon number and ended
	// before this appended one - went with the bound following NUM_WEAPONS.
	// The ammunition type still has to fit the arrays bots index by type, which is
	// what once put a canary on the stack.
	SHOCK_CHECK(gun->flags & WEAPONFLAG_AICANUSE,
			"a simulant is handed it, as with both other fork guns");
	SHOCK_CHECK(botactGetAmmoTypeByFunction(WEAPON_SHOCKRIFLE, FUNC_PRIMARY) == AMMOTYPE_SHOCK
			&& botactGetAmmoTypeByFunction(WEAPON_SHOCKRIFLE, FUNC_PRIMARY) < NUM_AMMOTYPES
			&& botactGetAmmoTypeByFunction(WEAPON_SHOCKRIFLE, FUNC_SECONDARY) == AMMOTYPE_SHOCKORB
			&& botactGetAmmoTypeByFunction(WEAPON_SHOCKRIFLE, FUNC_SECONDARY) < NUM_AMMOTYPES,
			"and both ammunition types fit the arrays a bot indexes by type");
	// The other half of the same class, and the one the ROM's own source warned
	// about: a simulant scores the guns on the floor by indexing a preference
	// table by weapon number, and that table was a literal 128 rows - below every
	// number this fork has appended. Any bot walking past a dropped Shock Rifle
	// asks the question; the widened bound is what makes the answer a zero row
	// ("no interest in this") rather than whatever follows the table in .bss.
	SHOCK_CHECK(botinvGetNumWeaponPreferences() > WEAPON_SHOCKORB
			&& botinvGetNumWeaponPreferences() >= NUM_WEAPONS,
			"and a simulant's preference table has a row for its number, not one short of it");

#if VERSION < VERSION_PAL_BETA
	// The ammunition's own words. The type is the game's newest, so no language
	// bank has a name for it, and an ammunition type with no case in the pickup
	// switches gives a message with neither name nor determiner: "Picked up .".
	//
	// Only the gun's own name is asked here, at a quantity of one, because that
	// is all this point in start-up can answer: the fork's words come from the
	// port text bank, which is built in code, while the determiner ("some") and
	// the plural "s" the same function appends both come out of the ROM's
	// language banks, which are not loaded until a stage or a menu loads them.
	// Asking for one here is a segfault in langGet's caller, found the hard way.
	// Those two halves are gated at the source instead, by tools/ci/shock.sh.
	{
		char nm[64];

		*nm = '\0';
		ammotypeGetPickupName(nm, AMMOTYPE_SHOCK, 1);
		SHOCK_CHECK(strstr(nm, "plasma cell") != NULL,
				"a crate of them is named, in the gun's own words, and not left blank");

		// Not the Bowcaster's plasma bolts: two guns on one word read as one
		// ammunition that happens to be usable by both, which it is not.
		*nm = '\0';
		ammotypeGetPickupName(nm, AMMOTYPE_BOWCASTER, 1);
		SHOCK_CHECK(strstr(nm, "plasma bolt") != NULL,
				"while the Bowcaster still fires plasma bolts, its own separate reserve");
	}
#endif

	// The saved setup, both widths. The curated random weapon list is one bit per
	// row, so the row appended for this gun moves every field written after it by
	// one bit: a version 8 file is read at 83 bits and a version 9 one at 84
	// (mpsetupfileSaveWad, mpsetupfileLoadWad). A file that cannot name the row
	// cannot have excluded it either, so the row reads back selected.
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

		SHOCK_CHECK(back, "a version 9 setup keeps the curated list row for row, the new one included");
	}

	savebufferClear(&written);
	mpsetupfileSaveWad(&written, MPSETUP_VERSION_BOWCASTER);
	savebufferClear(&readback);
	memcpy(readback.bytes, written.bytes, MPSETUP_BLOCKSIZE);
	memset(g_MpWeaponSetRandomFilters, 0, sizeof(keep));
	mpsetupfileLoadWad(&readback, MPSETUP_VERSION_BOWCASTER);

	SHOCK_CHECK(g_MpWeaponSetRandomFilters[NUM_MPWEAPONS - 1] != 0,
			"a version 8 setup reads with the Shock Rifle selected");
	SHOCK_CHECK(g_MpWeaponSetRandomFilters[NUM_MPWEAPONS - 2] != 0,
			"and the Bowcaster's row, its last, came from the file");

	memcpy(g_MpWeaponSetRandomFilters, keep, sizeof(keep));

	if (failed) {
		sysLogPrintf(LOG_NOTE, "shock test: %d of %d checks failed", failed, checks);
	} else {
		sysLogPrintf(LOG_NOTE, "shock test: all %d checks as expected", checks);
	}
}

#endif
