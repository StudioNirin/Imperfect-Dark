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
#include "game/game_0b0fd0.h"
#include "game/playermgr.h"
#include "gebean.h"
#include "geguns.h"
#include "game/botact.h"
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
 * five, and it has to be charged by holding the trigger. That charge is the
 * Mauler's machine - bgunTickMaulerCharge() in bondgun.c, widened to this
 * number by weaponChargesLikeMauler() below - because everything the player
 * needs to feel a charge is already hung off that one field, hand->matmot1: the
 * rising loop that cuts out when the charge ends, the model building red, and
 * the shot's own voice pitched down by how full it is. What this gun does
 * differently is when the field moves at all: the Mauler fills whenever its
 * charged function is selected and pays a round out of the magazine for each
 * unit of charge, where this gun fills only while the trigger is held, spends
 * nothing until the shot leaves, begins again from nothing if the press is let
 * go, and fires itself the moment the charge is full. Its zoom is the AR34's,
 * on the aim input, because in Perfect Dark a zoom is a mode on the aim input
 * rather than a gun function of its own.
 *
 * hand->matmot1 is per-hand scratch with other owners, and two of them are
 * close enough to matter here: the Moonraker's stream (bgunUpdateLaser()) and
 * the shotgun's starburst (bgunUpdateShotgun()) both write it. Neither is
 * reached by this row - the first wants FUNCFLAG_LASERSTREAM on a function, the
 * second WEAPONFLAG3_SHOTGUNMODEL on the gun - and the self-test pins both,
 * because either would fight the charge and neither is visible from the row.
 *
 * Simulants cannot have it: no WEAPONFLAG_AICANUSE, because a simulant's
 * charge is its own state (aibot->maulercharge[] is the Mauler's copy,
 * bot.c) and none of that is written for this gun. No mission hands it out
 * either - no stage's item table can name a weapon number the ROM never had,
 * so it reaches a player through the multiplayer lists and the Combat
 * Simulator only.
 */

// What the weapon list calls it. The code keeps the Rail-Laser name through the
// enum, the ammo type, this file and the gate, so the two names are not a
// mistake: this string is the only place the gun's face is decided.
static const char s_RailLaserName[] = "Moonrailer\n";
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
	return weaponHost(weaponnum) == WEAPON_RAILLASER;
}

bool weaponChargesLikeMauler(s32 weaponnum)
{
	weaponnum = weaponHost(weaponnum);

	return weaponnum == WEAPON_MAULER || weaponnum == WEAPON_RAILLASER;
}

/**
 * The model a Rail-Laser on the floor is drawn from.
 *
 * One answer in one place: playermgrGetModelOfWeapon() already weighs this gun's
 * own model state against Perfect Dark's Laser's, and reading g_ModelStates[] here
 * would make a second place that knows which of the two is worn - the same reason
 * the first-person check asks gebeanMoonrailerSlot() instead of looking at the file.
 */
s32 raillaserFloorModel(s32 weaponnum, s32 fallback)
{
	return raillaserOwn(weaponnum) ? playermgrGetModelOfWeapon(WEAPON_RAILLASER) : fallback;
}

/**
 * How long the trigger has to be held: one and a half seconds.
 *
 * In the same 60th-of-a-second base the charge itself counts in (the Mauler's
 * own g_Vars.lvupdate60freal), so it keeps its length above 60 fps.
 */
s32 raillaserCharge60(void)
{
	return TICKS(90);
}

f32 raillaserChargeFull(void)
{
	// The Mauler's own cap (bgunTickMaulerCharge()). The cues that read the
	// field are written against it: the shot's voice is pitched by matmot1 / 3
	// clamped at 1, so a lower full would fire a shot that sounds nothing like
	// the Mauler's full charge, and a higher one would sit at that same pitch
	// and colour while the charge ran on.
	return 5.0f;
}

f32 raillaserChargeRate60(void)
{
	// Per 60th, in the same base as the Mauler's own g_Vars.lvupdate60freal * 0.05
	return raillaserChargeFull() / (f32) raillaserCharge60();
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
 * The whole of what bondgun.c asks where it lets a gun be itself: `charge` is
 * the hand's matmot1, and `spent` is hand->primetimer60, which is -1 once the
 * press has fired - a press fires one shot, and the trigger has to be let go
 * for another. Held past the shot, then, the gun stays held rather than
 * beginning a second charge the player never asked for.
 */
bool raillaserCharging(f32 charge, s32 spent)
{
	if (spent < 0) {
		return true;
	}

	return charge < raillaserChargeFull();
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
	// Which model it is wearing, and whether the two agree. Written first as "it is
	// on Perfect Dark's Laser at boot, and no release mesh is claimed yet", which
	// was wrong twice over: the gun refresh has already run by the time the test
	// does, and the release is installed on this machine, so the alias was
	// registered and the gun was wearing it - a gate catching a guess about
	// ordering, which is what the gates are for. Either answer is legitimate, so
	// what is pinned is that the row's model is the stand-in or exactly the slot the
	// seam claims, never a stale number in between.
	RAILLASER_CHECK(gun != NULL
			&& gun->hi_model == (u16)(gebeanMoonrailerSlot() ? gebeanMoonrailerSlot() : FILE_GLASER),
		"what it wears is Perfect Dark's Laser, or exactly the release mesh it says it wears");
	sysLogPrintf(LOG_NOTE, "raillaser test: note  first-person model: %s (file %d); the fit logs in a stage as fpfit: row %d",
		gebeanMoonrailerSlot() ? "the release's Moonraker" : "Perfect Dark's Laser", gebeanMoonrailerSlot(),
		NUM_GE_GUNS);
	RAILLASER_CHECK(gun->functions[FUNC_SECONDARY] == NULL, "it has one function: a zoom is the aim input here, not an alt-fire");
	RAILLASER_CHECK(((struct weaponfunc *) gun->functions[FUNC_PRIMARY])->type == INVENTORYFUNCTYPE_SHOOT_SINGLE, "its shot is a single hitscan beam, not a projectile");
	RAILLASER_CHECK(gun->ammos[FUNC_PRIMARY] != NULL && gun->ammos[FUNC_PRIMARY]->clipsize == 1, "one shot a magazine");
	// Asked the other way until a player told us otherwise: this check demanded a
	// non-NULL reload animation, which is what made the borrowed Rocket Launcher's
	// script look like the right answer - and its part toggles hid the gun mid-reload
	// (src/game/invitems.c:invammo_raillaser). A gate that asserts a guess is worse
	// than none, because it makes the guess look like a decision. What is pinned now
	// is the decision: no animation of another gun's model. If this gun is ever given
	// a model with a reload animation of its own, change this here and say so.
	RAILLASER_CHECK(gun->ammos[FUNC_PRIMARY]->reload_animation == NULL,
		"it reloads on the engine's own lower/raise, naming no other gun's model");

	// How it is held, which is what the look of it comes down on. posx is the hand's
	// camera-space x, so a negative one puts the gun left of the crosshair however it
	// is equipped, and ONEHANDED picks the arm set.
	RAILLASER_CHECK(gun->posx > 0.0f, "it sits right of the crosshair, as every gun but the wrist Laser does");
	RAILLASER_CHECK(!(gun->flags & WEAPONFLAG_ONEHANDED), "it is held with two hands, as a rifle is");

	// Reachable at all. The cycle and the wheel both decide by enumerating the stock
	// table and GoldenEye's block, and a number past both is absent by construction
	// rather than by a missing model - which is what a player sees as "it is not in
	// the wheel". The negatives are here so that the predicate added for this is
	// pinned to this, and not to everything the stock tests happen to miss.
	RAILLASER_CHECK(INV_CYCLEABLE(WEAPON_RAILLASER), "next/previous weapon stops at it");
	RAILLASER_CHECK(!WEAPON_IS_FORK(WEAPON_LASER) && !WEAPON_IS_FORK(WEAPON_GE_MOONRAKER),
		"and only this fork's numbers are called its own, not the ROM's or GoldenEye's");

	// Where the mesh is put in the hand. It is placed where GoldenEye places its
	// Moonraker, which means asking a GoldenEye row - fpFitFrom in gebean.c - and a
	// borrowed ask that comes back empty is the same wrong-place failure it replaced,
	// since the build then falls back to the box centre without saying so. The ask
	// does not depend on the release being installed; only its answer does.
	{
		f32 own[3];
		f32 host[3];
		const s32 place = WEAPON_GE_MOONRAKER - WEAPON_GE_FIRST;

		RAILLASER_CHECK(GE_GUN_INDEX(place), "the row it borrows its placement from is a GoldenEye row, as that ask requires");
		RAILLASER_CHECK(gegunsViewPlacement(place, own, host),
			"and that row will answer, so the Moonrailer is placed and not merely boxed");
	}
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

	// botFindPickup() (src/game/bot.c) keeps a stack array per ammunition type, and a
	// simulant aborting is what found that they were sized by a literal 33 - the count
	// from before this gun existed. botactGetAmmoTypeByFunction() answers with any type
	// the game knows, so every type the game knows must fit the tables bots index by it.
	// Walking the whole gun table rather than just ours, because the next type to be
	// appended will not be ours, and a stack array has no name to grep for.
	{
		s32 worst = 0;
		s32 bad = -1;
		s32 w;

		for (w = 0; w < NUM_WEAPONS; w++) {
			s32 t = botactGetAmmoTypeByFunction(w, FUNC_PRIMARY);

			if (t > worst) {
				worst = t;
			}
			if (t >= NUM_AMMOTYPES) {
				bad = w;
			}
		}
		RAILLASER_CHECK(bad < 0, "no gun's ammunition type is past the arrays a bot indexes by it");
		RAILLASER_CHECK(worst == AMMOTYPE_RAIL, "the Rail-Laser's type is the highest there is, so it is the one that found the bound");
		RAILLASER_CHECK(botactGetAmmoTypeByFunction(WEAPON_RAILLASER, FUNC_PRIMARY) == AMMOTYPE_RAIL, "a bot sees the Moonrailer as needing Rail ammunition");
	}

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

		const char *label = mpGetWeaponLabel(display);

		// Both halves matter: the name players see, and not the name the code
		// carries, because the second is what a copy-paste of the enum would leave
		// in the list
		RAILLASER_CHECK(strstr(label, "Moonrailer") != NULL
				&& strstr(label, "Rail-Laser") == NULL,
			"the weapon list offers it as Moonrailer, which is not what the code calls it");
		sysLogPrintf(LOG_NOTE, "raillaser test: note  row %d of %d in the weapon list; a stage's roll takes it: %s",
				display + 1, mpGetNumWeaponOptions(),
				mpWeaponRowSuitsStage(&g_MpWeapons[MPWEAPON_RAILLASER]) ? "yes" : "no");
	}

	// The lists a weapon number has to be named in, as far as a test can reach
	// them. The two beam lists have no accessor, so the script looks at the
	// source for those (tools/ci/raillaser.sh) - a number missing from one of
	// them is silent, and silence is what let this ship without a bolt.
	// On the floor and in a body's hand. Pinned as the either/or, because which of
	// the two is worn depends on the release being installed and this test runs on
	// machines with and without it; what is pinned is that the answer is one of them,
	// that the two paths that put a gun on the floor agree, that the Moonraker's own
	// answer is the plain one, and that no other gun is answered for - a fork number
	// must not move a stock gun's model, and MODEL_CHRLASER is shared with it.
	RAILLASER_CHECK(playermgrGetModelOfWeapon(WEAPON_RAILLASER) == MODEL_PD_RAILLASER
			|| playermgrGetModelOfWeapon(WEAPON_RAILLASER) == MODEL_CHRLASER,
		"a body holding one holds the release's Moonraker or Perfect Dark's Laser, by what is installed");
	RAILLASER_CHECK(raillaserFloorModel(WEAPON_RAILLASER, MODEL_CHRSNIPERRIFLE)
			== playermgrGetModelOfWeapon(WEAPON_RAILLASER),
		"and a weapon spot or a crate puts it on the floor with the same answer");
	RAILLASER_CHECK(raillaserFloorModel(WEAPON_MAULER, MODEL_CHRSNIPERRIFLE) == MODEL_CHRSNIPERRIFLE,
		"and no other gun is answered for");
	RAILLASER_CHECK(playermgrGetModelOfWeapon(WEAPON_LASER) == MODEL_CHRLASER,
		"and the Moonraker's own model is unmoved by it");

	RAILLASER_CHECK(challengeIsFeatureUnlocked(g_MpWeapons[MPWEAPON_RAILLASER].unlockfeature), "nothing has to be unlocked for it");
	RAILLASER_CHECK(mpCanSpawnWithWeapon(&g_MpWeapons[MPWEAPON_RAILLASER]), "a spawn can hand it out");

	// The charge, at its edges. It rides hand->matmot1, the field the Mauler's
	// cues already read, so its full is the Mauler's 5 and its rate is whatever
	// fills that in the gun's own length.
	RAILLASER_CHECK(raillaserCharge60() == TICKS(90), "the charge is one and a half seconds");
	RAILLASER_CHECK(raillaserChargeFull() == 5.0f, "it fills to the Mauler's own full, so its cues land with the Mauler's");
	{
		f32 ticks = raillaserChargeFull() / raillaserChargeRate60();

		RAILLASER_CHECK(ticks > 89.5f && ticks < 90.5f, "and its rate fills it in exactly that many 60ths");
	}
	RAILLASER_CHECK(raillaserCharging(raillaserChargeFull() - 0.01f, 0), "a hair short of full, the gun holds");
	RAILLASER_CHECK(!raillaserCharging(raillaserChargeFull(), 0), "at full, the gun fires itself");
	RAILLASER_CHECK(raillaserCharging(raillaserChargeFull(), -1), "and once the press has fired, holding does nothing");
	RAILLASER_CHECK(raillaserCharging(0.0f, 0), "a press let go early begins again from nothing");

	// The Mauler's machine, and what has to stay out of it
	RAILLASER_CHECK(weaponChargesLikeMauler(WEAPON_MAULER), "the Mauler still charges the way it always did");
	RAILLASER_CHECK(weaponChargesLikeMauler(WEAPON_RAILLASER), "and the Rail-Laser charges beside it");
	RAILLASER_CHECK(!weaponChargesLikeMauler(WEAPON_LASER), "the Moonraker does not: it owns no charge");
	RAILLASER_CHECK(!weaponChargesLikeMauler(WEAPON_GE_MOONRAKER), "and GoldenEye's own Moonraker is not swept in beside Perfect Dark's");
	RAILLASER_CHECK(gun->flags2 & WEAPONFLAG2_CHARGEABLE, "its shot's voice is pitched by how full the charge is");
	RAILLASER_CHECK(!(gun->flags3 & WEAPONFLAG3_CHARGEBEAM), "its beam is the Moonraker's bolt, not the Mauler's charge beam");
	RAILLASER_CHECK(!weaponfuncHasFlag(WEAPON_RAILLASER, FUNC_PRIMARY, FUNCFLAG_LASERSTREAM)
		&& !weaponfuncHasFlag(WEAPON_RAILLASER, FUNC_SECONDARY, FUNCFLAG_LASERSTREAM),
		"nothing asks for the Moonraker's stream, which writes the charge field too");
	RAILLASER_CHECK(!(gun->flags3 & WEAPONFLAG3_SHOTGUNMODEL), "nor the shotgun's starburst, which writes it a third");
	{
		const struct weaponfunc_shoot *ours = (const struct weaponfunc_shoot *) gun->functions[FUNC_PRIMARY];
		const struct weaponfunc_shoot *mauler = (const struct weaponfunc_shoot *)
			g_Weapons[WEAPON_MAULER]->functions[FUNC_SECONDARY];

		RAILLASER_CHECK(ours->shootsound == mauler->shootsound, "it fires with the sound of the Mauler's full charge");
		// Asked the other way until a player watched the gun twist through 90 degrees
		// on the first press: an animation belongs to a model, not to a gun, and the
		// model in this hand is the Moonraker's, whose bones are not the Mauler's.
		// Pinned as NULL so the borrowed animation is not reached for again - a shot
		// animation here means a first-person model of this gun's own.
		RAILLASER_CHECK(ours->base.fire_animation == NULL,
			"its shot animates nothing, as the only model under it has no shot animation");
		// The Mauler's charged shot scales its damage by the charge that took it
		// (game_0b0fd0.c: damage *= unk063a / 3 + 1, unk063a being matmot1 * 10).
		// This gun's is a fixed Farsight-class kill and takes none of that, so
		// the field that carries it is left alone - pinned here so that the day
		// the third-person charge animation is wanted, that multiplier is
		// understood as the thing not to widen along with it.
		RAILLASER_CHECK(mauler->damage < 2.0f && ours->damage > mauler->damage * 10.0f,
			"the Mauler's charged shot is still the damage that scales, this gun's still the one that does not");
	}

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
