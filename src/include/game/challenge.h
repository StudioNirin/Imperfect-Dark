#ifndef _IN_GAME_CHALLENGE_H
#define _IN_GAME_CHALLENGE_H
#include <ultra64.h>
#include "data.h"
#include "types.h"

extern u32 g_MpChallengeIndex;
extern struct mpconfigfull *g_MpCurrentChallengeConfig;

// Challenges+ (a separate 30-challenge dataset with its own records). 0 = the
// stock challenges, 1 = Challenges+. Set by the menu and carried in net RULES.
extern s32 g_ChallengeMode;
// The active challenge list for the current mode - the completion APIs and the
// net marking path use this so a Plus star never touches a standard record.
struct challenge *challengeActiveList(void);

// Challenges+ medal bonus requirements. The registry is fixed; thresholds are
// typed by requirement (counts, percent, or 60 Hz ticks for times).
enum challengebonusrequirement {
	CHALBONUS_NONE = 0,
	CHALBONUS_HEADSHOTS,
	CHALBONUS_ACCURACY,
	CHALBONUS_KILLS,
	CHALBONUS_DEATHS,
	CHALBONUS_KD_RATIO,
	CHALBONUS_FASTEST_2KILLS,
	CHALBONUS_LONGEST_LIFE,
	CHALBONUS_MAX_SIMUL_KILLS,
	CHALBONUS_MULTIKILLS,
	CHALBONUS_LONGEST_SPREE,
	CHALBONUS_TOKEN_HELD_TIME,
	CHALBONUS_BACKSHOTS,
	CHALBONUS_DISTANCE,
	CHALBONUS_DAM_RECEIVED,
	CHALBONUS_ITEM_USE,
	CHALBONUS_FINISH_TIME,
	CHALBONUS_COUNT
};

struct challengebonus {
	u8 requirement;
	s32 silver[4];
	s32 gold[4];
	s32 perfect[4];
};

// Set per Plus challenge when its doc has been renamed to the challenge+-NN
// format. Only converted entries appear in the in-game Plus lists.
extern u8 g_MpChallengesPlusConverted[30];
// Per-challenge Start Armed setting (SPAWNWEAPON_*), synced by net rules.
extern u8 g_MpPlusStartArmed[30];
// One score row per challenge, author-selected per player-count star tier.
extern u16 g_MpPlusScoreLimits[30][4];
// Number of humans in the match, used to pick the tier's goal and thresholds.
s32 challengeHumanCount(void);
// Description panel preview: which challenge is highlighted and which of its
// four player-count tiers the tabs are showing.
extern char g_MpPlusTierDescriptions[30][4][320];
void challengeSetPreviewIndex(s32 index);
void challengeSetPreviewTier(s32 tier);
s32 challengeGetPreviewTier(void);
char *challengeGetTierDescription(s32 index, s32 tier);
extern struct challengebonus g_MpChallengeBonusPlus[30];
// ROM configs only hold eight sims. Plus challenges may configure up to 16;
// slots 8-15 live in these parallel arrays.
extern struct mpconfigsim g_MpPlusExtraSims[30][8];
extern char g_MpPlusExtraBotNames[30][8][15];

// Medal level for a completed Plus challenge: 0 bronze, 1 silver, 2 gold,
// 3 perfect (the hardest, with a purple star).
// Computed at match end from the team-aggregated bonus requirement.
extern s32 g_ChallengeMedalLevel;
extern s32 g_ChallengeAchievedValue;
extern u8 g_ChallengeAchievedRequirement;
// How the end screen describes the graded bonus: won, not finished, or voided by
// cheats. Only a won challenge earns a star, whatever the thresholds say.
enum challengeendscreenaward {
	CHALEND_UNFINISHED,
	CHALEND_AWARDED,
	CHALEND_VOID
};
extern s32 g_ChallengeEndscreenAward;
void challengeEvaluateBonus(s32 numplayers);
const char *challengeBonusName(u8 requirement);
s32 challengeBonusFormat(u8 requirement);
s32 challengeBonusDisplayValue(u8 requirement, s32 value);
u32 challengeGetMedalIn(struct challenge *list, s32 index, s32 numplayers);
void challengeSetMedalIn(struct challenge *list, s32 index, s32 numplayers, u32 medal);
u32 challengeGetMedal(s32 index, s32 numplayers);
void challengeSetMedal(s32 index, s32 numplayers, u32 medal);
u32 challengeGetMedalBySlot(s32 slot, s32 numplayers);
// True when the given challenge is visible in the current mode. Every Plus
// challenge is visible; the stock list keeps its unlock-based availability.

void challengesInit(void);

void challengeDetermineUnlockedFeatures(void);
void challengePerformSanityChecks(void);
char *challengeGetNameBySlot(s32 slot);
bool challengeIsCompletedByAnyChrWithNumPlayersBySlot(s32 slot, s32 numplayers);
struct mpconfigfull *challengeLoadConfig(s32 confignum, u8 *buffer, s32 len);
s32 challengeForceUnlockFeature(s32 featurenum, u8 *array, s32 tail, s32 len);
s32 challengeForceUnlockSetupFeatures(struct mpsetup *mpsetup, u8 *array, s32 len);
void challengeForceUnlockConfigFeatures(struct mpconfig *config, u8 *array, s32 len, s32 challengeindex);
void challengeForceUnlockBotFeatures(void);
void challengeForceUnlockOneFeature(s32 featurenum);
void challengeApply(void);
// Turn a challenge's ROM-defined simulants on for a match with numplayers humans. A net match's start calls this with the room's human count, as the offline setup menus do through challengePerformSanityChecks.
void challengeSetSimulantsForNumPlayers(s32 numplayers);
char *challengeGetCurrentDescription(void);
char *challengeGetConfigDescription(struct mpconfigfull *mpconfig);
bool challengeIsCompleteForEndscreen(void);
bool aiMpInitSimulants(void);
void vmPrintStatsIfEnabled(void);
bool challengeIsAvailableToPlayer(s32 chrnum, s32 challengeindex);
bool challengeIsAvailableToAnyPlayer(s32 challengeindex);
s32 challengeGetNumAvailable(void);
char *challengeGetName(s32 challengeindex);
void challengeSetCurrentBySlot(s32 slotnum);
s32 challengeGetCurrent(void);
struct mpconfigfull *challengeLoad(s32 challengeindex, u8 *buffer, s32 len);
struct mpconfigfull *challengeLoadBySlot(s32 n, u8 *buffer, s32 len);
struct mpconfigfull *challengeLoadCurrent(u8 *buffer, s32 len);
void challengeRemoveForceUnlocks(void);
s32 challengeRemovePlayerLock(void);
void challengeLoadAndStoreCurrent(u8 *buffer, s32 len);
void challengeUnsetCurrent(void);
bool challengeIsLoaded(void);
s32 challengeGetAutoFocusedIndex(s32 mpchrnum);
char *challengeGetName2(s32 playernum, s32 challengeindex);
bool challengeIsCompletedByPlayerWithNumPlayers2(s32 mpchrnum, s32 index, s32 numplayers);
// Explicit-list record accessors (the save reads/writes both datasets).
bool challengeIsCompletedByAnyPlayerIn(struct challenge *list, s32 index, s32 numplayers);
void challengeSetCompletedByAnyPlayerIn(struct challenge *list, s32 index, s32 numplayers, bool completed);
bool challengeIsCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers);
void challengeSetCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers, bool completed);
bool challengeIsCompletedByPlayerWithNumPlayers(s32 mpchrnum, s32 index, s32 numplayers);
void challengeSetCompletedByPlayerWithNumPlayers(u32 mpchrnum, s32 index, s32 numplayers, bool completed);
void challengeConsiderMarkingComplete(void);
bool challengeIsFeatureUnlocked(s32 feature);

#endif
