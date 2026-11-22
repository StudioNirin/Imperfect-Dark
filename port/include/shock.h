#ifndef _IN_SHOCK_H
#define _IN_SHOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The self-test (--test-shock): the Shock Rifle's row and what the game reads
 * off it, the two firing modes and the damage figures they were built to, the
 * orb's flight and what its row must not carry, the Combat Simulator's row at
 * the index a saved setup names it by, the ammunition's own name, and a saved
 * setup read at both widths (src/game/invitems.c's invitem_shockrifle).
 */
void shockSelfTest(void);

/**
 * Ammunition spent by one shot of a firing function. One for everything in the
 * game, five for the Shock Rifle's orb: the beam is cheap and the orb is a
 * deliberate investment, and one figure shared by the player's and a
 * simulant's ammunition (bondgun.c, chraction.c) keeps the two paying the same.
 */
s32 shockAmmoCostForFunc(s32 weaponnum, s32 funcnum);

#ifdef __cplusplus
}
#endif

#endif
