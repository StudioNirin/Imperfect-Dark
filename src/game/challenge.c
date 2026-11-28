#include <ultra64.h>
#include "constants.h"
#include "game/modunlocks.h"
#include "game/atan2f.h"
#include "game/bot.h"
#include "game/challenge.h"
#include "game/chrai.h"
#include "game/chraicommands.h"
#include "game/debug.h"
#include "game/lang.h"
#include "game/mplayer/mplayer.h"
#include "game/mplayer/scenarios.h"
#include "game/mplayer/setup.h"
#include "game/modoptions.h"
#include "game/mpstats.h"
#include "game/pad.h"
#include "game/player.h"
#include "game/playermgr.h"
#include "game/prop.h"
#include "game/training.h"
#include "bss.h"
#include "lib/dma.h"
#include "lib/rng.h"
#include "data.h"
#include "types.h"
#ifndef PLATFORM_N64
#include <string.h>
#include "romdata.h"
#include "system.h"
#include "mod.h"
// how many records the trace walks: the ROM's 44, and a mod's longer segment
// is not read past the configs the game has
#define NUM_MPCONFIGS_TRACE 44
#endif

u8 g_MpFeaturesForceUnlocked[40];
u8 g_MpFeaturesUnlocked[80];

u32 g_MpChallengeIndex = 0;
struct mpconfigfull *g_MpCurrentChallengeConfig = NULL;

// Description panel preview state, set by the menu as rows and tabs change.
static s32 s_PreviewIndex;
static s32 s_PreviewTier;

// Challenges+: 0 = stock challenge dataset, 1 = the separate Plus copy. The
// menu sets it when entering the Plus list; net RULES carry it to clients.
s32 g_ChallengeMode = 0;

struct challenge *challengeActiveList(void)
{
	return g_ChallengeMode ? g_MpChallengesPlus : g_MpChallenges;
}

struct challenge g_MpChallenges[] = {
	{ L_OPTIONS_406, MPCONFIG_CHALLENGE01 }, // "Challenge 1"
	{ L_OPTIONS_407, MPCONFIG_CHALLENGE02 }, // "Challenge 2"
	{ L_OPTIONS_408, MPCONFIG_CHALLENGE03 }, // "Challenge 3"
	{ L_OPTIONS_409, MPCONFIG_CHALLENGE04 }, // "Challenge 4"
	{ L_OPTIONS_410, MPCONFIG_CHALLENGE05 }, // "Challenge 5"
	{ L_OPTIONS_411, MPCONFIG_CHALLENGE06 }, // "Challenge 6"
	{ L_OPTIONS_412, MPCONFIG_CHALLENGE07 }, // "Challenge 7"
	{ L_OPTIONS_413, MPCONFIG_CHALLENGE08 }, // "Challenge 8"
	{ L_OPTIONS_414, MPCONFIG_CHALLENGE09 }, // "Challenge 9"
	{ L_OPTIONS_415, MPCONFIG_CHALLENGE10 }, // "Challenge 10"
	{ L_OPTIONS_416, MPCONFIG_CHALLENGE11 }, // "Challenge 11"
	{ L_OPTIONS_417, MPCONFIG_CHALLENGE12 }, // "Challenge 12"
	{ L_OPTIONS_418, MPCONFIG_CHALLENGE13 }, // "Challenge 13"
	{ L_OPTIONS_419, MPCONFIG_CHALLENGE14 }, // "Challenge 14"
	{ L_OPTIONS_420, MPCONFIG_CHALLENGE15 }, // "Challenge 15"
	{ L_OPTIONS_421, MPCONFIG_CHALLENGE16 }, // "Challenge 16"
	{ L_OPTIONS_422, MPCONFIG_CHALLENGE17 }, // "Challenge 17"
	{ L_OPTIONS_423, MPCONFIG_CHALLENGE18 }, // "Challenge 18"
	{ L_OPTIONS_424, MPCONFIG_CHALLENGE19 }, // "Challenge 19"
	{ L_OPTIONS_425, MPCONFIG_CHALLENGE20 }, // "Challenge 20"
	{ L_OPTIONS_426, MPCONFIG_CHALLENGE21 }, // "Challenge 21"
	{ L_OPTIONS_427, MPCONFIG_CHALLENGE22 }, // "Challenge 22"
	{ L_OPTIONS_428, MPCONFIG_CHALLENGE23 }, // "Challenge 23"
	{ L_OPTIONS_429, MPCONFIG_CHALLENGE24 }, // "Challenge 24"
	{ L_OPTIONS_430, MPCONFIG_CHALLENGE25 }, // "Challenge 25"
	{ L_OPTIONS_431, MPCONFIG_CHALLENGE26 }, // "Challenge 26"
	{ L_OPTIONS_432, MPCONFIG_CHALLENGE27 }, // "Challenge 27"
	{ L_OPTIONS_433, MPCONFIG_CHALLENGE28 }, // "Challenge 28"
	{ L_OPTIONS_434, MPCONFIG_CHALLENGE29 }, // "Challenge 29"
	{ L_OPTIONS_435, MPCONFIG_CHALLENGE30 }, // "Challenge 30"
};

s32 challengeHumanCount(void)
{
	s32 count = 0;
	s32 i;

	for (i = 0; i < MAX_LOCAL_PLAYERS; i++) {
		if (g_MpSetup.chrslots & (1 << i)) {
			count++;
		}
	}

	if (count < 1) {
		count = 1;
	}

	return count;
}

static bool challengePlusConverted(s32 challengeindex)
{
	return challengeindex >= 0 && challengeindex < 30
		&& g_MpChallengesPlusConverted[challengeindex] != 0;
}

bool challengeIsAvailable(s32 challengeindex)
{
	// A Plus challenge appears once its doc has been converted to the new
	// naming format, so the list grows as each one is finished.
	if (g_ChallengeMode) {
		return challengePlusConverted(challengeindex);
	}

	return (g_MpChallenges[challengeindex].availability & 1) != 0;
}

bool challengeIsAvailableToPlayer(s32 chrnum, s32 challengeindex)
{
	if (g_ChallengeMode) {
		return challengePlusConverted(challengeindex);
	}

	if ((g_MpSetup.chrslots & (1 << chrnum)) == 0) {
		return 0;
	}

	return ((g_MpChallenges[challengeindex].availability & (2 << chrnum)) != 0);
}

bool challengeIsAvailableToAnyPlayer(s32 challengeindex)
{
	if (g_ChallengeMode) {
		return challengePlusConverted(challengeindex);
	}

	return (g_MpChallenges[challengeindex].availability & (((g_MpSetup.chrslots & 0xf) << 1) | 1)) != 0;
}

void challengeDetermineUnlockedFeatures(void)
{
	s32 challengeindex;
	s32 numgifted; // number of unlocked but not completed challenges
	u8 flag;
	s32 prev;
	s32 i;
	s32 j;
	s32 k;

	// Clear all challenge availability
	for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		g_MpChallenges[challengeindex].availability = 0;
	}

	numgifted = 0;

	// Mark challenges completed by any player
	for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		flag = 0;

		if (challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 1)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 2)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 3)
				|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex, 4)) {
			// Completed challenge
			flag = 1;
		} else if (challengeindex < 4) {
			// Not yet completed, but challenges 1-4 are always available
			flag = 1;
			numgifted++;
		} else if (challengeindex > 0
				&& (challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 1)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 2)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 3)
					|| challengeIsCompletedByAnyPlayerWithNumPlayers(challengeindex - 1, 4))) {
			// Challenges are available if their previous one is complete
			flag = 1;
			numgifted++;
		}
#ifdef DEBUG
		else if (debugIsAllChallengesEnabled()) {
			flag = 1;
		}
#endif

		g_MpChallenges[challengeindex].availability |= flag;
	}

	// Gift up to 4 challenges
	for (challengeindex = 0; numgifted < 4 && challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		if ((g_MpChallenges[challengeindex].availability & 1) == 0) {
			g_MpChallenges[challengeindex].availability |= 1;
			numgifted++;
		}
	}

	// Now same as above, but per player
	for (j = 0; j < MAX_LOCAL_PLAYERS; j++) {
		numgifted = 0;

		for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
			flag = 0;

			if (challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 1)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 2)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 3)
					|| challengeIsCompletedByPlayerWithNumPlayers(j, challengeindex, 4)) {
				// Completed challenge
				flag = 2 << j;
			} else if (challengeindex < 4) {
				// Not yet completed, but challenges 1-4 are always available
				flag = 2 << j;
				numgifted++;
			} else if (challengeindex > 0) {
				// Challenges are available if their previous one is complete
				prev = challengeindex - 1;

				if (challengeIsCompletedByPlayerWithNumPlayers(j, prev, 1)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 2)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 3)
						|| challengeIsCompletedByPlayerWithNumPlayers(j, prev, 4)) {
					flag = 2 << j;
					numgifted++;
				}
			}

			g_MpChallenges[challengeindex].availability |= flag;
		}

		// Gift up to 4 challenges
		for (challengeindex = 0; numgifted < 4 && challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
			if ((g_MpChallenges[challengeindex].availability & (2 << j)) == 0) {
				g_MpChallenges[challengeindex].availability |= 2 << j;
				numgifted++;
			}
		}
	}

	for (j = 0; j < ARRAYCOUNT(g_MpFeaturesUnlocked); j++) {
		flag = 0;

		for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
			if (challengeIsAvailableToAnyPlayer(challengeindex)) {
				for (i = 0; i < ARRAYCOUNT(g_MpChallenges[challengeindex].unlockfeatures); i++) {
					if (g_MpChallenges[challengeindex].unlockfeatures[i] == j) {
						flag |= 1;
					}
				}
			}
		}

		for (i = 0; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
			if (g_MpFeaturesForceUnlocked[i] == j) {
				flag |= 1;
			}
		}

		for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
			for (prev = 0; prev < MAX_LOCAL_PLAYERS; prev++) {
				if (challengeIsAvailableToPlayer(prev, challengeindex)) {
					for (i = 0; i < ARRAYCOUNT(g_MpChallenges[challengeindex].unlockfeatures); i++) {
						if (g_MpChallenges[challengeindex].unlockfeatures[i] == j) {
							flag |= 2 << prev;
						}
					}
				}
			}
		}

		g_MpFeaturesUnlocked[j] = flag;
	}

	for (j = 0; j < func0f188bcc(); j++) {
		struct mpweapon *weapon = &g_MpWeapons[j];

		if (weapon->unlockfeature > 0 && weapon->unlockfeature < ARRAYCOUNT(g_MpFeaturesUnlocked)
				&& func0f19cbcc(weapon->weaponnum)) {
			g_MpFeaturesUnlocked[weapon->unlockfeature] |= 1;
		}
	}

	func0f1895e8();

	// Trim anything past the cap. Stock put four here unless MPFEATURE_8BOTS
	// was unlocked; the fork's cap is MAX_BOTS, so nothing is trimmed and a
	// setup keeps every simulant it was saved with.
	for (k = mpGetSimSlotCap(); k < MAX_BOTS; k++) {
		if (mpIsSimSlotOn(k)) {
			mpRemoveSimulant(k);
		}
	}

	if (g_Vars.mpquickteamnumsims > mpGetSimSlotCap()) {
		g_Vars.mpquickteamnumsims = mpGetSimSlotCap();
	}
}

/**
 * Turn a challenge's own simulants on for a match with numplayers humans.
 *
 * A challenge's ROM config has chrslots 0x0000: its simulants (allies and
 * enemies alike) and their per-player-count difficulties live in
 * config.simulants[], which challengeApply() loaded into
 * g_MpSimulantDifficultiesPerNumPlayers. The offline setup reaches this through
 * challengePerformSanityChecks() at menu time. A net match has no such tick, so
 * its start calls this with the room's human count - before H1 writes the sims
 * into RULES for the clients (netsession.c).
 */
void challengeSetSimulantsForNumPlayers(s32 numplayers)
{
	s32 i;

	if (numplayers < 1) {
		numplayers = 1;
	}

	if (numplayers > MAX_LOCAL_PLAYERS) {
		numplayers = MAX_LOCAL_PLAYERS;
	}

	mpClearSimSlots();

	// Stock challenges are ROM-defined and only describe MAX_BOTS_CONFIG
	// simulants. A Plus challenge can configure eight more from its template.
	{
		s32 numconfigsims = g_ChallengeMode ? MAX_BOTS_CONFIG + 8 : MAX_BOTS_CONFIG;

		for (i = 0; i < numconfigsims; i++) {
			g_BotConfigsArray[i].difficulty = g_MpSimulantDifficultiesPerNumPlayers[i][numplayers - 1];

			if (g_BotConfigsArray[i].difficulty != BOTDIFF_DISABLED) {
				mpSetSimSlotOn(i, true);
			}
		}
	}
}

void challengePerformSanityChecks(void)
{
	if (g_BossFile.locktype == MPLOCKTYPE_CHALLENGE) {
		s32 numplayers = 0;
		s32 i;

		// Reset player handicaps
		for (i = 0; i < MAX_LOCAL_PLAYERS; i++) {
			if (g_MpSetup.chrslots & (1 << i)) {
				g_PlayerConfigsArray[i].handicap = 0x80;
				numplayers++;
			}
		}

		// Turn off all simulants and turn them on if enabled
		// for this number of players
		g_MpSetup.chrslots &= 0x000f;
		challengeSetSimulantsForNumPlayers(numplayers);

		if (g_MpSetup.scenario == MPSCENARIO_KINGOFTHEHILL) {
			g_Vars.mphilltime = 10;
		}
	} else {
		mpKeepFirstSimSlots(mpGetSimSlotCap());
	}
}

s32 challengeGetNumAvailable(void)
{
	s32 challengeindex;
	s32 count = 0;

	for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			count++;
		}
	}

	return count;
}

// The Plus list shows generated names - "Challenge+ N" - so adding the mode
// needs no 30 new language strings. A static buffer is safe here: names are
// only read for the frame's render, one challenge at a time.
static char s_PlusName[32];

char *challengeGetName(s32 challengeindex)
{
	if (g_ChallengeMode) {
		snprintf(s_PlusName, sizeof(s_PlusName), "Challenge+ %d", challengeindex + 1);
		return s_PlusName;
	}

	return langGet(g_MpChallenges[challengeindex].name);
}

char *challengeGetNameBySlot(s32 slot)
{
	s32 index = 0;
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpChallenges); i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (index == slot) {
				return challengeGetName(i);
			}

			index++;
		}
	}

	return "";
}

void challengeSetCurrentBySlot(s32 slotnum)
{
	s32 challengeindex;
	g_MpChallengeIndex = 0;

	for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			if (slotnum == 0) {
				g_MpChallengeIndex = challengeindex;
				break;
			}

			slotnum--;
		}
	}

	challengeApply();
}

s32 challengeGetCurrent(void)
{
	return g_MpChallengeIndex;
}

bool challengeIsCompletedByAnyChrWithNumPlayersBySlot(s32 slot, s32 numplayers)
{
	s32 availableindex = 0;
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpChallenges); i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (availableindex == slot) {
				return challengeIsCompletedByAnyPlayerWithNumPlayers(i, numplayers);
			}

			availableindex++;
		}
	}

	return false;
}

u32 challengeGetMedalBySlot(s32 slot, s32 numplayers)
{
	s32 availableindex = 0;
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpChallenges); i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (availableindex == slot) {
				return challengeGetMedal(i, numplayers);
			}

			availableindex++;
		}
	}

	return 0;
}

bool challengeIsCompletedByChrWithNumPlayersBySlot(s32 mpchrnum, s32 slot, s32 numplayers)
{
	s32 availableindex = 0;
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpChallenges); i++) {
		if (challengeIsAvailableToAnyPlayer(i)) {
			if (availableindex == slot) {
				return challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, i, numplayers);
			}

			availableindex++;
		}
	}

	return false;
}

#ifdef PLATFORM_N64
#define BTYPE s32
#else
#define BTYPE uintptr_t
#endif

#ifndef PLATFORM_N64
/**
 * A challenge's record in the ROM's mpconfigs segment, which is not this
 * build's struct mpconfig: the name is 12 bytes where the port's is 18, there
 * is no storedbotbits, and everything is big-endian - 104 bytes against 116.
 *
 *   0x00 name[12]  0x0c options      0x10 scenario   0x11 stagenum
 *   0x12 timelimit 0x13 scorelimit   0x14 teamscorelimit (u16)
 *   0x16 chrslots (u16)  0x18 weapons[6]  0x1e paused  0x1f pad
 *   0x20 fileguid.fileid (s32)  0x24 fileguid.deviceserial (u16)  0x26 pad
 *   0x28 8 simulants of 8 bytes: type, mpheadnum, mpbodynum, team, 4 difficulties
 *
 * Read field by field, never as a struct and never swapped in place: the
 * segment may be a mod's own allocation (see b41191528).
 */
#define MPCONFIG_ROMSIZE 104
#define MPCONFIG_ROMSIMS 0x28

static u16 challengeBe16(const u8 *p)
{
	return (u16)(p[0] << 8 | p[1]);
}

static u32 challengeBe32(const u8 *p)
{
	return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}

/**
 * Overwrites the fields a ROM record has with that record. The fields it
 * lacks (storedbotbits, the tail of the name) stay as the caller had them.
 */
static void challengeDecodeRomConfig(const u8 *rec, struct mpconfig *config)
{
	struct mpsetup *setup = &config->setup;
	s32 i;

	memset(setup->name, 0, sizeof(setup->name));
	memcpy(setup->name, rec, 12);
	setup->name[11] = '\0';
	setup->options = challengeBe32(rec + 0x0c);
	setup->scenario = rec[0x10];
	setup->stagenum = rec[0x11];
	setup->timelimit = rec[0x12];
	setup->scorelimit = rec[0x13];
	setup->teamscorelimit = challengeBe16(rec + 0x14);
	setup->chrslots = challengeBe16(rec + 0x16);
	// a slot of the list the record was written against, which is not laid
	// out like the port's (moddata.c)
	for (i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		setup->weapons[i] = modDataMpWeaponSlot(rec[0x18 + i]);
	}

	setup->paused = rec[0x1e];
	setup->fileguid.fileid = (s32)challengeBe32(rec + 0x20);
	setup->fileguid.deviceserial = challengeBe16(rec + 0x24);

	for (i = 0; i < MAX_BOTS_CONFIG; i++) {
		const u8 *sim = rec + MPCONFIG_ROMSIMS + i * 8;

		config->simulants[i].type = sim[0];
		config->simulants[i].mpheadnum = sim[1];
		config->simulants[i].mpbodynum = sim[2];
		config->simulants[i].team = sim[3];
		memcpy(config->simulants[i].difficulties, sim + 4, MAX_LOCAL_PLAYERS);
	}
}

/**
 * --moddata-trace, once per run: every record in the segment as decoded. When
 * the segment is the ROM's, each is also compared with g_MpConfigs, which is
 * the same 44 configs in the port's layout - a mismatch there means the
 * layout above is wrong, not the data.
 */
static void challengeTraceRomConfigs(void)
{
	extern struct mpconfig g_MpConfigs[];
	static bool done = false;
	const u8 *seg = romdataSegGetData("mpconfigs");
	const u32 count = romdataSegGetSize("mpconfigs") / MPCONFIG_ROMSIZE;
	const s32 stock = romdataSegIsStock("mpconfigs");
	s32 mismatches = 0;
	u32 c;

	if (done || !seg || !sysArgCheck("--moddata-trace")) {
		return;
	}

	done = true;

	for (c = 0; c < count && c < NUM_MPCONFIGS_TRACE; c++) {
		struct mpconfig config = g_MpConfigs[c];
		char sims[160];
		u32 len = 0;
		s32 i;

		challengeDecodeRomConfig(seg + c * MPCONFIG_ROMSIZE, &config);

		for (i = 0; i < MAX_BOTS_CONFIG; i++) {
			len += snprintf(sims + len, sizeof(sims) - len, " %d/%d/%d",
					config.simulants[i].type, config.simulants[i].mpheadnum, config.simulants[i].mpbodynum);
			if (len >= sizeof(sims)) {
				len = sizeof(sims) - 1;
			}
		}

		sysLogPrintf(LOG_NOTE, "challenge: %s config %2u \"%s\" stage 0x%02x scenario %d slots 0x%04x weapons %d %d %d %d %d %d sims (type/head/body)%s",
				stock ? "rom" : "mod", c, config.setup.name, config.setup.stagenum, config.setup.scenario,
				config.setup.chrslots, config.setup.weapons[0], config.setup.weapons[1], config.setup.weapons[2],
				config.setup.weapons[3], config.setup.weapons[4], config.setup.weapons[5], sims);

		if (stock) {
			const struct mpsetup *a = &config.setup, *b = &g_MpConfigs[c].setup;

			char diff[256] = "";
			u32 dlen = 0;

#define CHALLENGE_DIFF(cond, fmt, ...) do { if (cond) { dlen += snprintf(diff + dlen, dlen < sizeof(diff) ? sizeof(diff) - dlen : 0, " " fmt, __VA_ARGS__); } } while (0)
			CHALLENGE_DIFF(strcmp(a->name, b->name), "name '%s'/'%s'", a->name, b->name);
			CHALLENGE_DIFF(a->options != b->options, "options %x/%x", a->options, b->options);
			CHALLENGE_DIFF(a->scenario != b->scenario, "scenario %d/%d", a->scenario, b->scenario);
			CHALLENGE_DIFF(a->stagenum != b->stagenum, "stage %x/%x", a->stagenum, b->stagenum);
			CHALLENGE_DIFF(a->timelimit != b->timelimit || a->scorelimit != b->scorelimit || a->teamscorelimit != b->teamscorelimit,
					"limits %d,%d,%d/%d,%d,%d", a->timelimit, a->scorelimit, a->teamscorelimit, b->timelimit, b->scorelimit, b->teamscorelimit);
			CHALLENGE_DIFF(a->chrslots != b->chrslots, "slots %x/%x", a->chrslots, b->chrslots);
			CHALLENGE_DIFF(memcmp(a->weapons, b->weapons, sizeof(a->weapons)), "weapons %d %d %d %d %d %d/%d %d %d %d %d %d",
					a->weapons[0], a->weapons[1], a->weapons[2], a->weapons[3], a->weapons[4], a->weapons[5],
					b->weapons[0], b->weapons[1], b->weapons[2], b->weapons[3], b->weapons[4], b->weapons[5]);
			CHALLENGE_DIFF(a->paused != b->paused, "paused %d/%d", a->paused, b->paused);
			CHALLENGE_DIFF(a->fileguid.fileid != b->fileguid.fileid || a->fileguid.deviceserial != b->fileguid.deviceserial,
					"guid %x,%x/%x,%x", a->fileguid.fileid, a->fileguid.deviceserial, b->fileguid.fileid, b->fileguid.deviceserial);

			for (i = 0; i < MAX_BOTS_CONFIG; i++) {
				const struct mpconfigsim *sa = &config.simulants[i], *sb = &g_MpConfigs[c].simulants[i];

				CHALLENGE_DIFF(memcmp(sa, sb, sizeof(*sa)), "sim%d %d/%d/%d/%d/%d%d%d%d vs %d/%d/%d/%d/%d%d%d%d", i,
						sa->type, sa->mpheadnum, sa->mpbodynum, sa->team, sa->difficulties[0], sa->difficulties[1], sa->difficulties[2], sa->difficulties[3],
						sb->type, sb->mpheadnum, sb->mpbodynum, sb->team, sb->difficulties[0], sb->difficulties[1], sb->difficulties[2], sb->difficulties[3]);
			}
#undef CHALLENGE_DIFF

			if (dlen) {
				sysLogPrintf(LOG_WARNING, "challenge: rom config %u differs from g_MpConfigs[%u] (rom/port):%s", c, c, diff);
				mismatches++;
			}
		}
	}

	if (stock) {
		sysLogPrintf(LOG_NOTE, "challenge: %d of %u rom configs differ from g_MpConfigs", mismatches, count);
	}
}
#endif

struct mpconfigfull *challengeLoadConfig(s32 confignum, u8 *buffer, s32 len)
{
	struct mpconfigfull *mpconfig;
	u8 buffer2[sizeof(struct mpstrings) + 40];
	struct mpstrings *loadedstrings;
	BTYPE bank;
	u32 language_id = langGetFileNumOffset();
#ifdef PLATFORM_N64
	extern struct mpconfig _mpconfigsSegmentRomStart[];
#else
	extern u8 EXT_SEG _mpconfigsSegmentRomStart;
#endif
	extern struct mpstrings EXT_SEG _mpstringsESegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsJSegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsPSegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsGSegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsFSegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsSSegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsISegmentRomStart;
	extern struct mpstrings EXT_SEG _mpstringsESegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsJSegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsPSegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsGSegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsFSegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsSSegmentRomEnd;
	extern struct mpstrings EXT_SEG _mpstringsISegmentRomEnd;

	extern struct mpconfig g_MpConfigs[];

	BTYPE banks[][2] = {
		{ (BTYPE)REF_SEG _mpstringsESegmentRomStart, (BTYPE)REF_SEG _mpstringsESegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsJSegmentRomStart, (BTYPE)REF_SEG _mpstringsJSegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsPSegmentRomStart, (BTYPE)REF_SEG _mpstringsPSegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsGSegmentRomStart, (BTYPE)REF_SEG _mpstringsGSegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsFSegmentRomStart, (BTYPE)REF_SEG _mpstringsFSegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsSSegmentRomStart, (BTYPE)REF_SEG _mpstringsSSegmentRomEnd },
		{ (BTYPE)REF_SEG _mpstringsISegmentRomStart, (BTYPE)REF_SEG _mpstringsISegmentRomEnd },
	};

	// Load mpconfigs
#ifdef PLATFORM_N64
	mpconfig = dmaExecWithAutoAlign(buffer, (BTYPE)&_mpconfigsSegmentRomStart[confignum], sizeof(struct mpconfig));
#else
	// Not from the segment. A record in the ROM is 104 bytes and this build's
	// struct mpconfig is 116 - the setup grew a longer name and storedbotbits
	// - so indexing the segment by the native size reads the wrong bytes for
	// every config but the first, and for the last of the 44 it reads past
	// the segment altogether: into the rest of the ROM image when the segment
	// is the ROM's, and off the end of the allocation when a mod replaced it
	// with its own (ASan found this one under GE-X, 2026-09-12).
	//
	// Nothing is lost by not reading it: g_MpConfigs[confignum] is the same
	// 44 configs in this build's own layout and overwrites every byte of the
	// record below. A length of zero asks dmaExecWithAutoAlign for the
	// address in the buffer it would have loaded to, and copies nothing.
	mpconfig = dmaExecWithAutoAlign(buffer, (BTYPE)REF_SEG _mpconfigsSegmentRomStart, 0);
#endif

	// Load mpstrings
	bank = banks[language_id][0];
	loadedstrings = dmaExecWithAutoAlign(buffer2, bank + confignum * sizeof(struct mpstrings), sizeof(struct mpstrings));

	mpconfig->config = g_MpConfigs[confignum];

#ifndef PLATFORM_N64
	// A mod that ships its own mpconfigs (GE-X rebuilds all 44: arenas,
	// weapons and 243 simulants) has its records decoded over the native
	// config; the ROM's own segment is g_MpConfigs already and is left alone.
	challengeTraceRomConfigs();

	if (!romdataSegIsStock("mpconfigs") && confignum >= 0
			&& (u32)(confignum + 1) * MPCONFIG_ROMSIZE <= romdataSegGetSize("mpconfigs")) {
		challengeDecodeRomConfig(romdataSegGetData("mpconfigs") + confignum * MPCONFIG_ROMSIZE, &mpconfig->config);
	}
#endif

	mpconfig->strings = *loadedstrings;

	return mpconfig;
}

struct mpconfigfull *challengeLoad(s32 challengeindex, u8 *buffer, s32 len)
{
	struct mpconfigfull *cfg = challengeLoadConfig(challengeActiveList()[challengeindex].confignum, buffer, len);

	// Plus uses its own copy of the config and the description strings, so an
	// edit to a Plus entry can never reach the stock challenge (or vice versa).
	// challengeLoadConfig still does the ROM/segment work above; override the
	// two data members it filled in.
	if (g_ChallengeMode && cfg) {
		s32 confignum = g_MpChallengesPlus[challengeindex].confignum;

		cfg->config = g_MpConfigsPlus[confignum];
		cfg->strings = g_MpStringsPlus[confignum];
	}

	return cfg;
}

struct mpconfigfull *challengeLoadBySlot(s32 n, u8 *buffer, s32 len)
{
	s32 numavailable = 0;
	s32 challengeindex;

	for (challengeindex = 0; challengeindex < ARRAYCOUNT(g_MpChallenges); challengeindex++) {
		if (challengeIsAvailableToAnyPlayer(challengeindex)) {
			if (numavailable == n) {
				challengeSetPreviewIndex(challengeindex);
				return challengeLoad(challengeindex, buffer, len);
			}

			numavailable++;
		}
	}

	return 0;
}

struct mpconfigfull *challengeLoadCurrent(u8 *buffer, s32 len)
{
	return challengeLoad(g_MpChallengeIndex, buffer, len);
}

/**
 * This is adding featurenum to the array, provided it's unique.
 */
s32 challengeForceUnlockFeature(s32 featurenum, u8 *array, s32 tail, s32 len)
{
	s32 i;

	for (i = 0; i < tail; i++) {
		if (array[i] == featurenum) {
			break;
		}
	}

	if (i >= tail && tail < len) {
		array[tail] = featurenum;
		tail++;
	}

	return tail;
}

s32 challengeForceUnlockSetupFeatures(struct mpsetup *setup, u8 *array, s32 len)
{
	s32 index = 0;
	s32 i;

	// Force unlock the weapons (if never held before)
	for (i = 0; i < ARRAYCOUNT(setup->weapons); i++) {
		s32 featurenum = g_MpWeapons[setup->weapons[i]].unlockfeature;

		if (featurenum && featurenum < ARRAYCOUNT(g_MpFeaturesUnlocked)) {
			index = challengeForceUnlockFeature(featurenum, array, index, len);
		}
	}

	// Force unlock the stage
	for (i = 0; i < mpGetNumStages(); i++) {
		if (g_MpArenas[i].stagenum == setup->stagenum) {
			s32 featurenum = g_MpArenas[i].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, array, index, len);
			}
		}
	}

	// Force unlock the scenario
	if (setup->scenario < NUM_MPSCENARIOS) {
		s32 featurenum = g_MpScenarioOverviews[setup->scenario].requirefeature;

		if (featurenum) {
			index = challengeForceUnlockFeature(featurenum, array, index, len);
		}
	}

	// Force unlock the scenario options
	if (setup->options & MPOPTION_ONEHITKILLS) {
		index = challengeForceUnlockFeature(MPFEATURE_ONEHITKILLS, array, index, len);
	}

	if (setup->options & (MPOPTION_SLOWMOTION_ON | MPOPTION_SLOWMOTION_SMART)) {
		index = challengeForceUnlockFeature(MPFEATURE_SLOWMOTION, array, index, len);
	}

	return index;
}

void challengeForceUnlockConfigFeatures(struct mpconfig *config, u8 *array, s32 len, s32 challengeindex)
{
	s32 index = challengeForceUnlockSetupFeatures(&config->setup, array, len);
	s32 featurenum;
	s32 numplayers;
	s32 i;

	// config->simulants[] is ROM/save-resident and only has MAX_BOTS_CONFIG
	// entries. Iterating to MAX_BOTS reads far past the end of the struct,
	// which callers place in a small stack buffer. Plus challenges have eight
	// more in a separate array.
	{
		s32 numconfigsims = (g_ChallengeMode && challengeindex >= 0 && challengeindex < 30)
			? MAX_BOTS_CONFIG + 8 : MAX_BOTS_CONFIG;

		for (i = 0; i < numconfigsims; i++) {
			struct mpconfigsim *sim = i < MAX_BOTS_CONFIG
				? &config->simulants[i]
				: &g_MpPlusExtraSims[challengeindex][i - MAX_BOTS_CONFIG];
			s32 simtype = mpFindBotProfile(sim->type, BOTDIFF_NORMAL);

			if (simtype >= 0) {
				featurenum = g_BotProfiles[simtype].requirefeature;

				if (featurenum) {
					index = challengeForceUnlockFeature(featurenum, array, index, len);
				}
			}

			for (numplayers = 0; numplayers < MAX_LOCAL_PLAYERS; numplayers++) {
				simtype = mpFindBotProfile(0, sim->difficulties[numplayers]);

				if (simtype >= 0) {
					featurenum = g_BotProfiles[simtype].requirefeature;

					if (featurenum) {
						index = challengeForceUnlockFeature(featurenum, array, index, len);
					}
				}
			}

			if (sim->mpbodynum < g_MpListCounts.bodies) {
				featurenum = g_MpBodies[sim->mpbodynum].requirefeature;

				if (featurenum) {
					index = challengeForceUnlockFeature(featurenum, array, index, len);
				}
			}

			if (sim->mpheadnum < g_MpListCounts.heads) {
				featurenum = g_MpHeads[sim->mpheadnum].requirefeature;

				if (featurenum) {
					index = challengeForceUnlockFeature(featurenum, array, index, len);
				}
			}
		}
	}

#if VERSION >= VERSION_NTSC_1_0
	if (challengeindex >= 25) {
		index = challengeForceUnlockFeature(MPFEATURE_BOTDIFF_DARK, array, index, len);
	} else if (challengeindex >= 20) {
		index = challengeForceUnlockFeature(MPFEATURE_STAGE_CARPARK, array, index, len);
	} else if (challengeindex >= 15) {
		index = challengeForceUnlockFeature(MPFEATURE_SCENARIO_PAC, array, index, len);
	}

	if (challengeindex >= 10) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, array, index, len);
	}
#else
	if (challengeindex >= 10) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, array, index, len);
	}

	if (challengeindex >= 15) {
		index = challengeForceUnlockFeature(MPFEATURE_SCENARIO_PAC, array, index, len);
	}

	if (challengeindex >= 20) {
		index = challengeForceUnlockFeature(MPFEATURE_STAGE_CARPARK, array, index, len);
	}
#endif

	// Clear the remainder of the array
	for (i = index; i < len; i++) {
		array[i] = 0;
	}
}

void challengeForceUnlockBotFeatures(void)
{
	s32 numsims = 0;
	s32 index = challengeForceUnlockSetupFeatures(&g_MpSetup, g_MpFeaturesForceUnlocked, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_BotConfigsArray); i++) {
		// Force unlock the simulant type
		s32 simtypeindex = mpFindBotProfile(g_BotConfigsArray[i].type, BOTDIFF_NORMAL);

		if (simtypeindex >= 0) {
			s32 featurenum = g_BotProfiles[simtypeindex].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		// Force unlock the simulant difficulty
		simtypeindex = mpFindBotProfile(BOTTYPE_GENERAL, g_BotConfigsArray[i].difficulty);

		if (simtypeindex >= 0) {
			s32 featurenum = g_BotProfiles[simtypeindex].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		if (simtypeindex >= 0) {
			numsims++;
		}

		// Force unlock the simulant's body
		if (g_BotConfigsArray[i].base.mpbodynum < g_MpListCounts.bodies) {
			s32 featurenum = g_MpBodies[g_BotConfigsArray[i].base.mpbodynum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}

		// Force unlock the simulant's head
		if (g_BotConfigsArray[i].base.mpheadnum < g_MpListCounts.heads) {
			s32 featurenum = g_MpHeads[g_BotConfigsArray[i].base.mpheadnum].requirefeature;

			if (featurenum) {
				index = challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
			}
		}
	}

	// Force unlock 8 simulants
	if (numsims > 4) {
		index = challengeForceUnlockFeature(MPFEATURE_8BOTS, g_MpFeaturesForceUnlocked, index, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
	}

	// Clear the remainder of the array
	for (i = index; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
		g_MpFeaturesForceUnlocked[i] = 0;
	}

	challengeDetermineUnlockedFeatures();
}

/**
 * Force unlock one feature, on top of whatever is already forced.
 *
 * challengeForceUnlockBotFeatures() rebuilds the whole array from the current
 * simulant configuration. This appends to it instead, for callers that know a
 * feature is needed but are not part of that rebuild.
 */
void challengeForceUnlockOneFeature(s32 featurenum)
{
	s32 tail = 0;

	while (tail < ARRAYCOUNT(g_MpFeaturesForceUnlocked) && g_MpFeaturesForceUnlocked[tail]) {
		tail++;
	}

	challengeForceUnlockFeature(featurenum, g_MpFeaturesForceUnlocked, tail, ARRAYCOUNT(g_MpFeaturesForceUnlocked));
	challengeDetermineUnlockedFeatures();
}

void challengeRemoveForceUnlocks(void)
{
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpFeaturesForceUnlocked); i++) {
		g_MpFeaturesForceUnlocked[i] = 0;
	}

	challengeDetermineUnlockedFeatures();
}

/**
 * A Plus challenge can configure 16 sims. The ROM-resident config only holds
 * eight, so mpApplyConfig() clones the rest disabled; overwrite slots 8-15
 * from the Plus-only arrays loaded from the challenge-NN.md files.
 */
void challengeApplyPlusSims(void)
{
	s32 i;
	s32 j;
	s32 slot;
	struct mpconfigsim *sim;
	u8 *name;

	if (!g_ChallengeMode || g_MpChallengeIndex < 0 || g_MpChallengeIndex >= 30) {
		return;
	}

	for (i = 0; i < 8; i++) {
		slot = MAX_BOTS_CONFIG + i;
		sim = &g_MpPlusExtraSims[g_MpChallengeIndex][i];
		name = g_MpPlusExtraBotNames[g_MpChallengeIndex][i];

		g_BotConfigsArray[slot].type = sim->type;
		mpResetBotStats(slot);

		for (j = 0; j < MAX_LOCAL_PLAYERS; j++) {
			g_MpSimulantDifficultiesPerNumPlayers[slot][j] = sim->difficulties[j];
		}

		g_BotConfigsArray[slot].difficulty = g_MpSimulantDifficultiesPerNumPlayers[slot][0];
		strcpy(g_BotConfigsArray[slot].base.name, name);
		g_BotConfigsArray[slot].base.mpheadnum = sim->mpheadnum;
		g_BotConfigsArray[slot].base.mpbodynum = sim->mpbodynum;
		g_BotConfigsArray[slot].base.team = sim->team;
	}
}

void challengeApply(void)
{
	s32 i;
	u8 buffer[0x1ca];

	mpApplyConfig(challengeLoadCurrent(buffer, 0x1ca));
	challengeApplyPlusSims();
	mpSetLock(MPLOCKTYPE_CHALLENGE, 5);

	for (i = 0; i < MAX_LOCAL_PLAYERS; i++) {
		g_PlayerConfigsArray[i].base.team = 0;
	}

	// Every challenge is a team game, even a lone player against sims. One
	// authored score row feeds the team limit and the player score limit is
	// left off, so the tier's value is the only goal that ends the match.
	g_MpSetup.options |= MPOPTION_TEAMSENABLED;
	g_MpSetup.scorelimit = 100;

	if (g_ChallengeMode && g_MpChallengeIndex >= 0 && g_MpChallengeIndex < 30) {
		s32 tier = challengeHumanCount() - 1;

		if (tier < 0) {
			tier = 0;
		} else if (tier > 3) {
			tier = 3;
		}

		g_MpSetup.teamscorelimit = g_MpPlusScoreLimits[g_MpChallengeIndex][tier];
		// Mod.StartArmed is a NETKEY_SYNC option, so setting it here on the
		// host before RULES go out hands clients the same starting gun.
		g_ModOptions.spawnweapon = g_MpPlusStartArmed[g_MpChallengeIndex];
	}

#ifndef PLATFORM_N64
	// Enable Friendly Fire option
	g_MpSetup.options |= MPOPTION_FRIENDLYFIRE;
#endif

}

s32 challengeRemovePlayerLock(void)
{
	return mpSetLock(MPLOCKTYPE_NONE, 0);
}

void challengeLoadAndStoreCurrent(u8 *buffer, s32 len)
{
	g_MpCurrentChallengeConfig = challengeLoadCurrent(buffer, len);
}

void challengeUnsetCurrent(void)
{
	g_MpCurrentChallengeConfig = NULL;
}

bool challengeIsLoaded(void)
{
	return g_MpCurrentChallengeConfig != NULL;
}

char *challengeGetCurrentDescription(void)
{
	if (g_ChallengeMode) {
		return challengeGetTierDescription(g_MpChallengeIndex, s_PreviewTier);
	}

	if (g_MpCurrentChallengeConfig) {
		return g_MpCurrentChallengeConfig->strings.description;
	}

	return "";
}

char *challengeGetConfigDescription(struct mpconfigfull *mpconfig)
{
	if (g_ChallengeMode) {
		return challengeGetTierDescription(s_PreviewIndex, s_PreviewTier);
	}

	if (mpconfig) {
		return mpconfig->strings.description;
	}

	return "";
}

/**
 * Return the index of the first incomplete challenge, but if it's in the first
 * 4 then return index 4 because the first 4 are always shown.
 */
s32 challengeGetAutoFocusedIndex(s32 mpchrnum)
{
	s32 challengeindex;
	s32 index = 0;

	for (challengeindex = ARRAYCOUNT(g_MpChallenges) - 1; challengeindex >= 0; challengeindex--) {
		if (challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 1) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 2) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 3) ||
				challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, challengeindex, 4)) {
			index = challengeindex + 1;
			break;
		}
	}

	if (index < 4) {
		index = 4;
	}

	// In Plus mode only converted challenges are listed, so a saved
	// completion on an unconverted one must not focus past the last row.
	if (g_ChallengeMode) {
		s32 numavailable = challengeGetNumAvailable();

		if (numavailable < 1) {
			return 0;
		}

		if (index >= numavailable) {
			index = numavailable - 1;
		}
	}

	return index;
}

char *challengeGetName2(s32 playernum, s32 challengeindex)
{
	return challengeGetName(challengeindex);
}

bool challengeIsCompletedByPlayerWithNumPlayers2(s32 mpchrnum, s32 index, s32 numplayers)
{
	return challengeIsCompletedByPlayerWithNumPlayers(mpchrnum, index, numplayers);
}

// Explicit-list record accessors - the save writes both datasets regardless
// of the current mode, so it names the list it means.
bool challengeIsCompletedByAnyPlayerIn(struct challenge *list, s32 index, s32 numplayers)
{
	return (list[index].completions[numplayers - 1] & 1) != 0;
}

void challengeSetCompletedByAnyPlayerIn(struct challenge *list, s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		list[index].completions[numplayers - 1] |= 1;
		return;
	}

	list[index].completions[numplayers - 1] &= ~1;
}

// All record reads/writes go through the active list so a Plus completion
// never touches a standard challenge's stars (and vice versa).
#define CHAL_REC (challengeActiveList())

bool challengeIsCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers)
{
	return (CHAL_REC[index].completions[numplayers - 1] & 1) != 0;
}

void challengeSetCompletedByAnyPlayerWithNumPlayers(s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		CHAL_REC[index].completions[numplayers - 1] |= 1;
		return;
	}

	CHAL_REC[index].completions[numplayers - 1] &= ~1;
}

bool challengeIsCompletedByPlayerWithNumPlayers(s32 mpchrnum, s32 index, s32 numplayers)
{
	return (CHAL_REC[index].completions[numplayers - 1] & (2 << mpchrnum)) != 0;
}

void challengeSetCompletedByPlayerWithNumPlayers(u32 mpchrnum, s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		CHAL_REC[index].completions[numplayers - 1] |= 2 << mpchrnum;
		return;
	}

	CHAL_REC[index].completions[numplayers - 1] &= ~(2 << mpchrnum);
}

// The medal level lives in spare bits 5-7 of each per-tier completion byte.
// Bits 0-4 are the any-player bit and the four per-player bits.
u32 challengeGetMedalIn(struct challenge *list, s32 index, s32 numplayers)
{
	return (list[index].completions[numplayers - 1] >> 5) & 7;
}

void challengeSetMedalIn(struct challenge *list, s32 index, s32 numplayers, u32 medal)
{
	u8 *completion = &list[index].completions[numplayers - 1];

	*completion = (*completion & ~(7 << 5)) | ((medal & 7) << 5);
}

u32 challengeGetMedal(s32 index, s32 numplayers)
{
	return challengeGetMedalIn(challengeActiveList(), index, numplayers);
}

void challengeSetMedal(s32 index, s32 numplayers, u32 medal)
{
	challengeSetMedalIn(challengeActiveList(), index, numplayers, medal);
}

#undef CHAL_REC

void challengeSetPreviewIndex(s32 index)
{
	s_PreviewIndex = index;
}

void challengeSetPreviewTier(s32 tier)
{
	if (tier < 0) {
		tier = 0;
	} else if (tier > 3) {
		tier = 3;
	}

	s_PreviewTier = tier;
}

s32 challengeGetPreviewTier(void)
{
	return s_PreviewTier;
}

char *challengeGetTierDescription(s32 index, s32 tier)
{
	if (!g_ChallengeMode || index < 0 || index >= 30) {
		return "";
	}

	if (tier < 0) {
		tier = 0;
	} else if (tier > 3) {
		tier = 3;
	}

	return g_MpPlusTierDescriptions[index][tier];
}

s32 g_ChallengeMedalLevel;
s32 g_ChallengeAchievedValue;
u8 g_ChallengeAchievedRequirement;
s32 g_ChallengeEndscreenAward = CHALEND_UNFINISHED;

/**
 * Display strings for the end-screen challenge result panel. The format code
 * is 0 for a plain count, 1 for seconds and 2 for a percentage.
 */
const char *challengeBonusName(u8 requirement)
{
	switch (requirement) {
	case CHALBONUS_HEADSHOTS:    return "Headshots";
	case CHALBONUS_ACCURACY:     return "Accuracy";
	case CHALBONUS_KILLS:        return "Kills";
	case CHALBONUS_DEATHS:       return "Deaths";
	case CHALBONUS_KD_RATIO:     return "K/D Ratio";
	case CHALBONUS_FASTEST_2KILLS: return "Fastest 2 Kills";
	case CHALBONUS_LONGEST_LIFE: return "Longest Life";
	case CHALBONUS_MAX_SIMUL_KILLS: return "Max Multi-Kills";
	case CHALBONUS_MULTIKILLS:   return "Multi-Kills";
	case CHALBONUS_LONGEST_SPREE: return "Longest Spree";
	case CHALBONUS_TOKEN_HELD_TIME: return "Objective Time";
	case CHALBONUS_BACKSHOTS:    return "Back Shots";
	case CHALBONUS_DISTANCE:     return "Distance";
	case CHALBONUS_DAM_RECEIVED: return "Damage Received";
	case CHALBONUS_ITEM_USE:     return "Item Use";
	case CHALBONUS_FINISH_TIME:  return "Finish Time";
	default:                     return "None";
	}
}

s32 challengeBonusFormat(u8 requirement)
{
	switch (requirement) {
	case CHALBONUS_FINISH_TIME:
	case CHALBONUS_TOKEN_HELD_TIME:
	case CHALBONUS_LONGEST_LIFE:
	case CHALBONUS_FASTEST_2KILLS:
		return 1;
	case CHALBONUS_ACCURACY:
	case CHALBONUS_KD_RATIO:
		return 2;
	default:
		return 0;
	}
}

/**
 * Achieved value in display units: times are stored as 60 Hz ticks and shown
 * in seconds, except token-held time which accumulates at 240 Hz.
 */
s32 challengeBonusDisplayValue(u8 requirement, s32 value)
{
	if (challengeBonusFormat(requirement) == 1) {
		return value / (requirement == CHALBONUS_TOKEN_HELD_TIME ? 240 : 60);
	}

	return value;
}

struct challengemetrics {
	s32 kills;
	s32 deaths;
	s32 headshots;
	s32 hits;
	s32 shots;
	s32 backshots;
	s32 tokentime;
	s32 distance;
	s32 damreceived;
	s32 cloaktime;
	s32 fastest2kills;
	s32 longestlife;
	s32 maxsimulkills;
	s32 multikills;
	s32 longestspree;
};

/**
 * Combine one team's stats across every chr on team 0. Team 0 is the human
 * players plus any ally sims the config placed on MPTEAM_0; enemy sims on
 * other teams are deliberately excluded. setCurrentPlayerNum() is required
 * because the stat accessors read the global currentplayerstats.
 */
static void challengeGetTeamMetrics(struct challengemetrics *m)
{
	s32 i;
	s32 v;
	s32 prevplayernum = g_Vars.currentplayernum;

	memset(m, 0, sizeof(*m));
	m->fastest2kills = S32_MAX;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		struct mpchrconfig *mpchr = mpGetChrConfigBySlotNum(i);

		if (!mpchr || mpchr->team != 0) {
			continue;
		}

		setCurrentPlayerNum(i);

		m->kills += g_Vars.currentplayerstats->killcount;
		m->deaths += mpchr->numdeaths;
		m->headshots += mpstatsGetPlayerShotCountByRegion(SHOTREGION_HEAD);
		m->hits += mpstatsGetPlayerShotCountByRegion(SHOTREGION_HEAD)
			+ mpstatsGetPlayerShotCountByRegion(SHOTREGION_BODY)
			+ mpstatsGetPlayerShotCountByRegion(SHOTREGION_LIMB)
			+ mpstatsGetPlayerShotCountByRegion(SHOTREGION_GUN)
			+ mpstatsGetPlayerShotCountByRegion(SHOTREGION_HAT)
			+ mpstatsGetPlayerShotCountByRegion(SHOTREGION_OBJECT);
		m->shots += mpstatsGetPlayerShotCountByRegion(SHOTREGION_TOTAL);
		m->backshots += g_Vars.currentplayerstats->backshotcount;
		m->tokentime += g_Vars.currentplayerstats->tokenheldtime;
		m->distance += (s32)(g_Vars.currentplayerstats->distance / 10000.0f);
		m->damreceived += (s32)g_Vars.currentplayerstats->damreceived;
		m->cloaktime += g_Vars.currentplayerstats->cloaktime;

		v = g_Vars.currentplayerstats->fastest2kills;
		if (v > 0 && v < m->fastest2kills) {
			m->fastest2kills = v;
		}

		v = g_Vars.currentplayerstats->longestlife;
		if (v > m->longestlife) {
			m->longestlife = v;
		}

		v = g_Vars.currentplayerstats->maxsimulkills;
		if (v > m->maxsimulkills) {
			m->maxsimulkills = v;
		}

		m->multikills += g_Vars.currentplayerstats->multikillcount;

		// A team's spree is its best single life, not the lives added together
		v = g_Vars.currentplayerstats->maxkills;
		if (v > m->longestspree) {
			m->longestspree = v;
		}
	}

	setCurrentPlayerNum(prevplayernum);
}

/**
 * Judge the Plus medal level from the team's aggregated stat. The host calls
 * this once at match end; clients take the verdict from MATCH_END. 0 is bronze
 * (any completion), 1 silver, 2 gold.
 */
void challengeEvaluateBonus(s32 numplayers)
{
	struct challengemetrics m;
	struct challengebonus *bonus;
	s32 value;
	bool lowerisbetter = false;

	g_ChallengeMedalLevel = 0;
	g_ChallengeAchievedValue = 0;
	g_ChallengeAchievedRequirement = CHALBONUS_NONE;

	if (!g_ChallengeMode || g_MpChallengeIndex < 0 || g_MpChallengeIndex >= 30) {
		return;
	}

	bonus = &g_MpChallengeBonusPlus[g_MpChallengeIndex];
	if (bonus->requirement == CHALBONUS_NONE) {
		return;
	}

	challengeGetTeamMetrics(&m);

	switch (bonus->requirement) {
	case CHALBONUS_HEADSHOTS:
		value = m.headshots;
		break;
	case CHALBONUS_ACCURACY:
		value = m.shots > 0 ? m.hits * 100 / m.shots : 0;
		break;
	case CHALBONUS_KILLS:
		value = m.kills;
		break;
	case CHALBONUS_DEATHS:
		value = m.deaths;
		lowerisbetter = true;
		break;
	case CHALBONUS_KD_RATIO:
		value = m.deaths > 0 ? m.kills * 100 / m.deaths : m.kills * 100;
		break;
	case CHALBONUS_FASTEST_2KILLS:
		value = m.fastest2kills;
		lowerisbetter = true;
		break;
	case CHALBONUS_LONGEST_LIFE:
		value = m.longestlife;
		break;
	case CHALBONUS_MAX_SIMUL_KILLS:
		value = m.maxsimulkills;
		break;
	case CHALBONUS_MULTIKILLS:
		value = m.multikills;
		break;
	case CHALBONUS_LONGEST_SPREE:
		value = m.longestspree;
		break;
	case CHALBONUS_TOKEN_HELD_TIME:
		value = m.tokentime;
		break;
	case CHALBONUS_BACKSHOTS:
		value = m.backshots;
		break;
	case CHALBONUS_DISTANCE:
		value = m.distance;
		break;
	case CHALBONUS_DAM_RECEIVED:
		value = m.damreceived;
		lowerisbetter = true;
		break;
	case CHALBONUS_ITEM_USE:
		value = m.cloaktime;
		break;
	case CHALBONUS_FINISH_TIME:
		value = playerGetMissionTime();
		lowerisbetter = true;
		break;
	default:
		return;
	}

	// Kept for the end-screen's challenge result panel, which shows what the
	// team actually achieved against the requirement's thresholds.
	g_ChallengeAchievedValue = value;
	g_ChallengeAchievedRequirement = bonus->requirement;

	{
		s32 tier = numplayers - 1;

		if (tier < 0) {
			tier = 0;
		} else if (tier > 3) {
			tier = 3;
		}

		if (lowerisbetter) {
			if (bonus->perfect[tier] > 0 && value <= bonus->perfect[tier]) {
				g_ChallengeMedalLevel = 3;
			} else if (value <= bonus->gold[tier]) {
				g_ChallengeMedalLevel = 2;
			} else if (value <= bonus->silver[tier]) {
				g_ChallengeMedalLevel = 1;
			}
		} else {
			if (bonus->perfect[tier] > 0 && value >= bonus->perfect[tier]) {
				g_ChallengeMedalLevel = 3;
			} else if (value >= bonus->gold[tier]) {
				g_ChallengeMedalLevel = 2;
			} else if (value >= bonus->silver[tier]) {
				g_ChallengeMedalLevel = 1;
			}
		}
	}
}

bool challengeIsCompleteForEndscreen(void)
{
	s32 prevplayernum = g_Vars.currentplayernum;
	s32 result = false;
	s32 aborted = false;
	s32 i;
	u32 stack;

	for (i = 0; i < PLAYERCOUNT(); i++) {
		setCurrentPlayerNum(i);

		if (g_Vars.currentplayer->aborted) {
			aborted = true;
		}
	}

	setCurrentPlayerNum(prevplayernum);

	if (!aborted) {
		struct ranking rankings[MAX_MPCHRS];
		mpGetTeamRankings(rankings);

		if (rankings[0].teamnum == 0) {
			result = true;
		}
	}

	return result;
}

void challengeConsiderMarkingComplete(void)
{
	bool result = challengeIsCompleteForEndscreen();

#if VERSION >= VERSION_NTSC_1_0 && defined(DEBUG)
	if ((g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0) && (result || debugIsSetCompleteEnabled()))
#elif VERSION >= VERSION_NTSC_1_0
	if (g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0 && result)
#else
	if (result && g_CheatsActiveBank0 == 0 && g_CheatsActiveBank1 == 0)
#endif
	{
		u32 prevplayernum;
		s32 i;

		// completions[] holds 1-4 players (gamefile-serialized)
		if (PLAYERCOUNT() > MAX_LOCAL_PLAYERS) {
			return;
		}

		challengeSetCompletedByAnyPlayerWithNumPlayers(g_MpChallengeIndex, PLAYERCOUNT(), 1);
		challengeSetMedal(g_MpChallengeIndex, PLAYERCOUNT(), g_ChallengeMedalLevel);
		prevplayernum = g_Vars.currentplayernum;

		for (i = 0; i < PLAYERCOUNT(); i++) {
			setCurrentPlayerNum(i);
			challengeSetCompletedByPlayerWithNumPlayers(g_Vars.currentplayerstats->mpindex, g_MpChallengeIndex, PLAYERCOUNT(), true);
		}

		setCurrentPlayerNum(prevplayernum);
		// A Plus completion grants nothing: it must not feed the shared feature
		// unlocks or change which challenges are available. Availability stays
		// driven by the stock list. (The marking above already routed to the
		// active list, so the Plus star is recorded in its own array.)
		if (!g_ChallengeMode) {
			challengeDetermineUnlockedFeatures();
		}
	}
}

bool challengeIsFeatureUnlocked(s32 featurenum)
{
	if (featurenum == 0) {
		return true;
	}

#ifndef PLATFORM_N64
	// MPFEATURE_NEVER, and anything else past the table
	if (featurenum < 0 || featurenum >= ARRAYCOUNT(g_MpFeaturesUnlocked)) {
		return false;
	}
#endif

	if ((g_ModUnlocks & MODUNLOCK_MPOPTIONS) && (featurenum == MPFEATURE_SLOWMOTION || featurenum == MPFEATURE_ONEHITKILLS)) {
		return true;
	}

	return (g_MpFeaturesUnlocked[featurenum] & 1) != 0;
}

bool challengeIsFeatureUnlockedByPlayer(u32 numplayers, s32 featurenum)
{
	if (featurenum == 0) {
		return true;
	}

	return (g_MpFeaturesUnlocked[featurenum] & (2 << numplayers)) != 0;
}

bool challengeIsFeatureUnlockedByDefault(s32 featurenum)
{
	if (featurenum) {
		return false;
	}

	return true;
}
