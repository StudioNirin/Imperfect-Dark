#ifndef _IN_DBSHOTGUN_H
#define _IN_DBSHOTGUN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether the DB-Shotgun's secondary is a full double blast. Only when the gun
 * is selected on the secondary and the two-round magazine is still full; with
 * one round left the secondary falls back to the ordinary single blast.
 */
bool dbShotgunIsDouble(s32 weaponnum, s32 funcnum, s32 loadedammo);

/**
 * Pellets in one trigger pull for a weapon carrying WEAPONFLAG2_PELLETS. Twelve
 * for a full DB-Shotgun double; otherwise exactly the old rule, six for a PD
 * gun and five for a GoldenEye one, so neither stock shotgun moves.
 */
s32 dbShotgunPellets(s32 weaponnum, s32 funcnum, s32 loadedammo);

/**
 * Rounds spent by one shot. Two for a full DB-Shotgun double, one for
 * everything else. This is a price, separate from hand->shotstotake, which
 * stays the number of projectiles (one shot, however many pellets it carries).
 */
s32 dbShotgunAmmoCost(s32 weaponnum, s32 funcnum, s32 loadedammo);

/**
 * Register the PpdDbShotgunZ alias over the stock PD Shotgun's file and point
 * the DB-Shotgun's model state and first-person rows at it, falling back to
 * FILE_GSHOTGUN until the alias exists. Called at startup and again after a
 * mod swap empties every file slot (romdataResetFiles).
 */
void dbShotgunModelsRefresh(void);

/**
 * The self-test (--test-dbshotgun): the alias and model state, the two-round
 * magazine, the pellets and price in all three states, the untouched stock
 * Shotgun, the simulant's row, the Combat Simulator's row and both widths of a
 * saved setup.
 */
void dbShotgunSelfTest(void);

#ifdef __cplusplus
}
#endif

#endif
