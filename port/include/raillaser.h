#ifndef _IN_RAILLASER_H
#define _IN_RAILLASER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The Rail-Laser (WEAPON_RAILLASER, invitems.c's invitem_raillaser): this
 * fork's own gun - a charged railgun on a Moonraker Laser's look. Gives the
 * row the text the language banks do not have, and will lay GoldenEye XBLA's
 * Moonraker model over the borrowed one where the player has that package.
 *
 * Called once at startup, from port/src/main.c beside the other port-side
 * weapon set-ups.
 */
void raillaserInit(void);

/**
 * Whether a weapon number is the Rail-Laser's. bondgun.c asks this where a gun
 * is allowed to be itself rather than the general case - the charge gate, and
 * the one-shot-to-a-press after it - so the rules of the gun stay here.
 */
bool raillaserOwn(s32 weaponnum);

/**
 * The charge, and how long it takes: raillaserCharge60() is the length in 60ths
 * of a second, raillaserChargeFull() the height hand->matmot1 has to reach, and
 * raillaserChargeRate60() what a hand adds to it each 60th. The full is the
 * Mauler's own 5, so every cue that field drives - the rising charge loop, the
 * red build over the model, the shot's voice pitched down to 0.6 - arrives at
 * the same moment and with the same weight as the Mauler's fully charged shot.
 */
s32 raillaserCharge60(void);
f32 raillaserChargeFull(void);
f32 raillaserChargeRate60(void);

/**
 * Whether a hand of the Rail-Laser is still charging: the hand's charge (its
 * matmot1), and its primetimer60, which bondgun.c sets to -1 when the press has
 * fired - a press is one shot, and the trigger has to be let go for another.
 */
bool raillaserCharging(f32 charge, s32 spent);

/**
 * Whether a weapon number charges on hand->matmot1 in bgunTickMaulerCharge():
 * the Mauler, and this gun beside it, by host, so a mod's reskinned Mauler
 * keeps its charge. This is the one rule that says which guns share that
 * machine; bondgun.c asks it where the Mauler's name stands alone.
 */
bool weaponChargesLikeMauler(s32 weaponnum);

/**
 * The model a Rail-Laser lying on the floor, or held by a body, is drawn from:
 * its own model state where the release's Moonraker is on it, and `fallback`
 * where it is not. The Combat Simulator's weapon spots and the Randomizer's
 * crates take their model out of the static `g_MpWeapons[]` row, which cannot
 * name a runtime answer, so those two ask this after `gegunsFloorModel()` - the
 * GoldenEye guns go through that first and are not touched here.
 */
s32 raillaserFloorModel(s32 weaponnum, s32 fallback);

/**
 * The self-test (--test-raillaser): the row and what the game reads off it,
 * the weapon number against GoldenEye's block, the Combat Simulator's rows at
 * the indexes a saved setup names them by, the charge at its edges, and a
 * saved setup read at both widths.
 */
void raillaserSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif
