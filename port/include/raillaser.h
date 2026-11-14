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
 * Whether a weapon number is the Rail-Laser's, and how long its trigger has
 * to be held before it fires (in 60ths). bondgun.c asks these where a gun is
 * allowed to be itself rather than the general case - the charge gate, and
 * the one-shot-to-a-press after it - so the rules of the gun stay here.
 */
bool raillaserOwn(s32 weaponnum);
s32 raillaserCharge60(void);

/**
 * Whether a hand of the Rail-Laser is still charging: the hand's stateframes,
 * and its primetimer60, which bondgun.c sets to -1 when the press has fired.
 */
bool raillaserCharging(s32 frames, s32 spent);

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
