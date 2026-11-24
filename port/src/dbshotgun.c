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
#include "game/playermgr.h"
#include "game/botinv.h"
#include "game/mplayer/mplayer.h"
#include "system.h"
#include "romdata.h"
#include "dbshotgun.h"

#ifndef PLATFORM_N64

/**
 * The DB-Shotgun (WEAPON_GESHOTGUN) is a fourth gun of this fork's own, but
 * unlike the Rail-Laser it needs no GoldenEye release. Its mesh is the GE
 * shotgun's fitted to the stock PD Shotgun's spaces, carried in a model pack.
 * A gun has two of those spaces and they are not the same: the first-person
 * GshotgunZ has its barrel along +Z, while the held/floor PchrshotgunZ has it
 * along -X from the chr's hand. One mesh cannot be correct in both, so the
 * pack carries two OBJs, each fitted by models-custom/db-shotgun/fit-to-pd.py,
 * and two aliases name them. The aliases are extra file numbers over the
 * stock files, so the stock PD Shotgun's own rows are unchanged.
 *
 * What belongs here rather than in a static initialiser is the one rule the
 * two engines have to share: when the secondary is a full double (twelve
 * pellets for the price of two rounds) and when it is the ordinary six-pellet
 * single (one round). The player's ready check and deduction ask
 * dbShotgunAmmoCost() (bondgun.c), a simulant's gate and deduction ask it too
 * (bot.c, chraction.c), and the shot itself asks dbShotgunPellets() (prop.c)
 * from the same clip. One answer in one place is what keeps a bot's gun from
 * costing less than a player's, and a bot's pellets from staying at six.
 */

static const char s_PdDbShotgunProp[] = "PpdDbShotgunZ";    // first person, over FILE_GSHOTGUN
static const char s_PdDbShotgunChrProp[] = "PpdDbShotgunChr"; // held/floor, over FILE_PCHRSHOTGUN

bool dbShotgunIsDouble(s32 weaponnum, s32 funcnum, s32 loadedammo)
{
	return weaponnum == WEAPON_GESHOTGUN && funcnum == FUNC_SECONDARY && loadedammo >= 2;
}

s32 dbShotgunPellets(s32 weaponnum, s32 funcnum, s32 loadedammo)
{
	if (dbShotgunIsDouble(weaponnum, funcnum, loadedammo)) {
		return 12;
	}

	// The rule this call replaced, unchanged for every other gun and for the
	// DB-Shotgun's own single and short-clip secondary.
	return WEAPON_IS_GE(weaponnum) ? 5 : 6;
}

s32 dbShotgunAmmoCost(s32 weaponnum, s32 funcnum, s32 loadedammo)
{
	return dbShotgunIsDouble(weaponnum, funcnum, loadedammo) ? 2 : 1;
}

void dbShotgunModelsRefresh(void)
{
	struct modelstate *state = &g_ModelStates[MODEL_PD_GESHOTGUN];
	// The viewmodel is drawn from the weapon's hi_model under the GUNDL list's
	// own matrix. The copy in a chr's hand or on the floor is drawn from this
	// model state (playermgrGetModelOfWeapon answers it) under the hand node's
	// matrix, so it needs its own file in its own rest space.
	const s32 viewalias = romdataRegisterAliasFile(s_PdDbShotgunProp, FILE_GSHOTGUN);
	const s32 chralias = romdataRegisterAliasFile(s_PdDbShotgunChrProp, FILE_PCHRSHOTGUN);
	const s32 viewfileid = viewalias ? viewalias : FILE_GSHOTGUN;
	const s32 chrfileid = chralias ? chralias : FILE_PCHRSHOTGUN;

	// A modeldef is cached against the file it was read from; a new alias slot
	// after a mod swap needs the old one dropped, as gebean.c does for its guns.
	if (state->fileid != chrfileid) {
		state->modeldef = NULL;
	}

	state->fileid = (u16) chrfileid;
	state->scale = g_ModelStates[MODEL_CHRSHOTGUN].scale;

	if (g_Weapons[WEAPON_GESHOTGUN] != NULL) {
		g_Weapons[WEAPON_GESHOTGUN]->hi_model = (u16) viewfileid;
		g_Weapons[WEAPON_GESHOTGUN]->lo_model = (u16) viewfileid;
	}
}

#define DB_CHECK(cond, what) \
	do { \
		checks++; \
		if (cond) { \
			sysLogPrintf(LOG_NOTE, "dbshotgun test: ok   %s", what); \
		} else { \
			sysLogPrintf(LOG_NOTE, "dbshotgun test: FAIL %s", what); \
			failed++; \
		} \
	} while (0)

void dbShotgunSelfTest(void)
{
	const struct weapon *gun = g_Weapons[WEAPON_GESHOTGUN];
	const struct weapon *stock = g_Weapons[WEAPON_SHOTGUN];
	const struct weaponfunc *single;
	const struct weaponfunc *dbl;
	struct aibotweaponpreference pref;
	struct savebuffer written;
	struct savebuffer readback;
	u8 keep[NUM_MPWEAPONS];
	s32 alias;
	s32 checks = 0;
	s32 failed = 0;
	s32 i;

	DB_CHECK(gun != NULL && gun->functions[FUNC_PRIMARY] != NULL
			&& gun->functions[FUNC_SECONDARY] != NULL && stock != NULL,
			"the DB-Shotgun exists with two firing modes, and the stock Shotgun has a row");

	if (gun == NULL || gun->functions[FUNC_PRIMARY] == NULL
			|| gun->functions[FUNC_SECONDARY] == NULL || stock == NULL) {
		sysLogPrintf(LOG_NOTE, "dbshotgun test: %d of %d checks failed", failed + 1, checks);
		return;
	}

	single = gun->functions[FUNC_PRIMARY];
	dbl = gun->functions[FUNC_SECONDARY];
	pref = AIBOTPREF(WEAPON_GESHOTGUN);

	// The row: a two-round magazine of the existing shotgun ammunition, so no
	// new ammunition type, no new pickup names and no new quantity tables.
	DB_CHECK(gun->ammos[0] != NULL && gun->ammos[1] == NULL
			&& gun->ammos[0]->type == AMMOTYPE_SHOTGUN && gun->ammos[0]->clipsize == 2,
			"the magazine holds two rounds of the existing shotgun ammunition");
	DB_CHECK((gun->ammos[0]->flags & AMMOFLAG_INCREMENTALRELOAD) != 0,
			"and reloads one shell at a time");
	DB_CHECK((single->flags & FUNCFLAG_BURST2) == 0 && (dbl->flags & FUNCFLAG_BURST2) == 0,
			"neither mode is the engine's two-frame burst: a double is one shot of twelve");

	// The whole rule, in all three states the player can be in.
	DB_CHECK(!dbShotgunIsDouble(WEAPON_GESHOTGUN, FUNC_PRIMARY, 2)
			&& dbShotgunPellets(WEAPON_GESHOTGUN, FUNC_PRIMARY, 2) == 6
			&& dbShotgunAmmoCost(WEAPON_GESHOTGUN, FUNC_PRIMARY, 2) == 1,
			"the primary is a single blast of six for one round");
	DB_CHECK(dbShotgunIsDouble(WEAPON_GESHOTGUN, FUNC_SECONDARY, 2)
			&& dbShotgunPellets(WEAPON_GESHOTGUN, FUNC_SECONDARY, 2) == 12
			&& dbShotgunAmmoCost(WEAPON_GESHOTGUN, FUNC_SECONDARY, 2) == 2,
			"a full-clip secondary is one blast of twelve for two rounds");
	DB_CHECK(!dbShotgunIsDouble(WEAPON_GESHOTGUN, FUNC_SECONDARY, 1)
			&& dbShotgunPellets(WEAPON_GESHOTGUN, FUNC_SECONDARY, 1) == 6
			&& dbShotgunAmmoCost(WEAPON_GESHOTGUN, FUNC_SECONDARY, 1) == 1,
			"with one round the secondary falls back to the six-pellet single");
	DB_CHECK(!dbShotgunIsDouble(WEAPON_GESHOTGUN, FUNC_SECONDARY, 0)
			&& dbShotgunAmmoCost(WEAPON_GESHOTGUN, FUNC_SECONDARY, 0) == 1,
			"an empty secondary is not a double either");

	// Nothing else moved.
	DB_CHECK(dbShotgunPellets(WEAPON_SHOTGUN, FUNC_PRIMARY, 9) == 6
			&& dbShotgunPellets(WEAPON_SHOTGUN, FUNC_SECONDARY, 9) == 6
			&& dbShotgunAmmoCost(WEAPON_SHOTGUN, FUNC_SECONDARY, 9) == 1,
			"the stock PD Shotgun still fires six and costs one");
	DB_CHECK(dbShotgunPellets(WEAPON_GE_SHOTGUN, FUNC_PRIMARY, 2) == 5
			&& dbShotgunAmmoCost(WEAPON_GE_SHOTGUN, FUNC_PRIMARY, 2) == 1,
			"and the GE shotgun still fires five");
	DB_CHECK(dbShotgunPellets(WEAPON_CROSSBOW, FUNC_PRIMARY, 1) == 6
			&& dbShotgunAmmoCost(WEAPON_CROSSBOW, FUNC_PRIMARY, 1) == 1,
			"and an unrelated gun's figures are untouched");

	// The model: a dedicated state one past the Moonrailer's, inside the range
	// GoldenEye walks but below the remake's models. First person reads a file
	// over the stock viewmodel, the held/floor state a different file over the
	// stock chr model - the two rest spaces are not interchangeable.
	alias = romdataFileGetNumForName(s_PdDbShotgunChrProp);
	DB_CHECK(alias > 0 && alias == g_ModelStates[MODEL_PD_GESHOTGUN].fileid,
			"the chr alias fills the held/floor model state");
	DB_CHECK(romdataFileGetNumForName(s_PdDbShotgunProp) > 0
			&& (s32) gun->hi_model == romdataFileGetNumForName(s_PdDbShotgunProp)
			&& (s32) gun->lo_model == gun->hi_model,
			"the viewmodel alias fills both first-person rows");
	DB_CHECK(g_ModelStates[MODEL_PD_GESHOTGUN].fileid != gun->hi_model,
			"the held mesh is a different file from the viewmesh - one fit cannot serve both");
	DB_CHECK(MODEL_PD_GESHOTGUN == MODEL_PD_RAILLASER + 1
			&& MODEL_PD_GESHOTGUN < MODEL_REMAKE_FIRST,
			"the model state is the next one after the Moonrailer's and below the remake's");
	DB_CHECK(playermgrGetModelOfWeapon(WEAPON_GESHOTGUN) == MODEL_PD_GESHOTGUN,
			"a body holding it holds the alias model, not nothing");
	DB_CHECK(g_ModelStates[MODEL_PD_GESHOTGUN].scale == g_ModelStates[MODEL_CHRSHOTGUN].scale,
			"the fitted mesh keeps the PD Shotgun's own scale");

	// Simulants: AICANUSE for the third-person pose, and a preference row with
	// both goal flags and real scores so the row is actually read.
	DB_CHECK((gun->flags & WEAPONFLAG_AICANUSE) != 0 && weaponHost(WEAPON_GESHOTGUN) == WEAPON_GESHOTGUN,
			"a simulant can hold it and it behaves as its own number");
	DB_CHECK(botinvGetNumWeaponPreferences() > WEAPON_GESHOTGUN
			&& pref.haspriammogoal && pref.hassecammogoal
			&& pref.unk00 > 0 && pref.unk01 > 0,
			"a simulant's preference table reaches it and gives both modes a score");
	DB_CHECK(pref.targetammopri <= 100 && pref.targetammosec <= 100
			&& pref.criticalammopri <= 100 && pref.criticalammosec <= 100
			&& pref.reloaddelay <= 7,
			"its ammunition goals fit the shotgun reserve and its reload wait is in range");

	// Combat Simulator: its own row, its own floor model, and nothing unlocking it.
	DB_CHECK(MPWEAPON_GESHOTGUN == NUM_MPWEAPONS - 1
			&& g_MpWeapons[MPWEAPON_GESHOTGUN].weaponnum == WEAPON_GESHOTGUN
			&& (s32) g_MpWeapons[MPWEAPON_GESHOTGUN].model == playermgrGetModelOfWeapon(WEAPON_GESHOTGUN),
			"the Combat Simulator has a last row for it, on the same floor model");
	DB_CHECK(g_MpWeapons[MPWEAPON_GESHOTGUN].priammotype == AMMOTYPE_SHOTGUN
			&& challengeIsFeatureUnlocked(g_MpWeapons[MPWEAPON_GESHOTGUN].unlockfeature)
			&& mpCanSpawnWithWeapon(&g_MpWeapons[MPWEAPON_GESHOTGUN]),
			"its spot stocks shotgun shells and nothing unlocks it");

	// The stock Shotgun is the control: the alias must not have moved its row,
	// its model or its Combat Simulator entry.
	DB_CHECK(stock->ammos[0]->clipsize == 9 && (s32) stock->hi_model == FILE_GSHOTGUN
			&& (s32) stock->lo_model == FILE_GSHOTGUNLOD,
			"the stock Shotgun keeps its nine-round clip and its own first-person file");
	DB_CHECK(playermgrGetModelOfWeapon(WEAPON_SHOTGUN) == MODEL_CHRSHOTGUN
			&& (s32) g_MpWeapons[MPWEAPON_SHOTGUN].model == MODEL_CHRSHOTGUN,
			"and it keeps its own floor and held model");

	// Saved setup: the curated list is one bit per row. The newest width keeps
	// every row; a version 9 file cannot name the DB-Shotgun's row, so it reads
	// back selected while the rows before it still come from the file.
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

		DB_CHECK(back, "a version 10 setup keeps the curated list row for row, the new one included");
	}

	savebufferClear(&written);
	mpsetupfileSaveWad(&written, MPSETUP_VERSION_SHOCKRIFLE);
	savebufferClear(&readback);
	memcpy(readback.bytes, written.bytes, MPSETUP_BLOCKSIZE);
	memset(g_MpWeaponSetRandomFilters, 0, sizeof(keep));
	mpsetupfileLoadWad(&readback, MPSETUP_VERSION_SHOCKRIFLE);

	DB_CHECK(g_MpWeaponSetRandomFilters[MPWEAPON_GESHOTGUN] != 0
			&& g_MpWeaponSetRandomFilters[MPWEAPON_SHOCKRIFLE] != 0,
			"a version 9 setup reads with the DB-Shotgun and Shock Rifle rows selected");

	memcpy(g_MpWeaponSetRandomFilters, keep, sizeof(keep));

	if (failed) {
		sysLogPrintf(LOG_NOTE, "dbshotgun test: %d of %d checks failed", failed, checks);
	} else {
		sysLogPrintf(LOG_NOTE, "dbshotgun test: all %d checks as expected", checks);
	}
}

#endif
