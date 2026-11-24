#ifndef _IN_GAME_MPLAYER_SCENARIOS_H
#define _IN_GAME_MPLAYER_SCENARIOS_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

extern struct menudialogdef g_MpScenarioMenuDialog;
extern struct menudialogdef g_MpQuickTeamScenarioMenuDialog;

struct mpscenariooverview {
	u16 name;
	u16 shortname;
	u8 requirefeature;
	u8 teamonly;
};

extern struct mpscenariooverview g_MpScenarioOverviews[];

MenuItemHandlerResult menuhandlerMpOpenOptions(s32 operation, struct menuitem *item, union handlerdata *data);
void scenarioReadSave(struct savebuffer *buffer, u8 version);
void scenarioWriteSave(struct savebuffer *buffer);
void scenarioInit(void);
bool mpScenarioHasFixedLoadout(s32 scenario);
void scenarioFixedLoadoutInvalidate(void);
void mpUpdateSetupWeaponLocks(void);
void scenarioSelect(s32 scenario);
void fixedLoadoutSelfTest(void);
s32 scenarioNumProps(void);
void scenarioInitProps(void);
void scenarioTick(void);
void scenarioTickChr(struct chrdata *chr);
Gfx *scenarioRadarExtra(Gfx *gdl);
bool scenarioRadarChr(Gfx **gdl, struct prop *prop);
f32 scenarioChooseSpawnLocation(f32 chrradius, struct coord *pos, RoomNum *rooms, struct prop *prop);
s32 scenarioGetMaxTeams(void);
void scenarioHighlightRoom(RoomNum room, s32 *arg1, s32 *arg2, s32 *arg3);

// One Shall Stand...: elimination, match end and the death feed message.
// Defined in scenarios/lastmanstanding.inc (included into scenarios.c).
extern s32 g_LmsStartingLives;
bool lmsLivesSpent(struct chrdata *chr);
s32 lmsMatchOver(void);
void lmsHandleDeath(s32 attackerindex, s32 victimindex);
#ifndef PLATFORM_N64
extern u8 g_LmsNetLives[MAX_MPCHRS];
extern u8 g_LmsNetOut[MAX_MPCHRS];
extern bool g_LmsNetBlockSeen;

extern u8 g_ZombiesNetRole[MAX_MPCHRS];
extern s16 g_ZombiesNetScore[MAX_MPCHRS];
extern u8 g_ZombiesNetRound;
extern u8 g_ZombiesNetPhase;
extern u16 g_ZombiesNetTime60;
extern bool g_ZombiesNetBlockSeen;
#endif

// Zombies: rounds, roles, scoring and the body rebuild request.
// Defined in scenarios/zombies.inc (included into scenarios.c).
extern s32 g_ZombiesRounds;
extern bool g_ZombiesRadarZombies;
s32 zombiesMatchOver(void);
void zombiesHandleDeath(s32 attackerindex, s32 victimindex);
bool zombiesBodyStaleIs(s32 playernum);
bool zombiesBodyStaleTakeForPlayer(s32 playernum);
bool zombiesIsInfected(struct chrdata *chr);
bool zombiesSeatIsInfected(s32 playernum);
void zombiesSeatWokeUp(s32 playernum);
bool zombiesRoundInProgress(void);
bool zombiesCountdownActive(void);
bool zombiesRadarChr(Gfx **gdl, struct prop *prop);
bool zombiesBodyStaleTake(void);
void zombiesNotifyChrSpawn(struct chrdata *chr, s32 *bodynum, s32 *headnum, bool *sp60);
s32 zombiesBodyNumForChr(struct chrdata *chr);
void zombiesPrepareBotSpawn(struct chrdata *chr);

#endif
