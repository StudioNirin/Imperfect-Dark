#include <ultra64.h>
#include "constants.h"
#include "game/challenge.h"
#include "game/mplayer/mplayer.h"
#include "bss.h"
#include "data.h"
#include "types.h"

void challengesInit(void)
{
	struct mpconfigfull *mpconfig;
	u8 buffer[0x1ca];
	s32 i;

	for (i = 0; i < ARRAYCOUNT(g_MpChallenges); i++) {
		g_MpChallenges[i].availability = 0;
		g_MpChallenges[i].completions[0] = 0;
		g_MpChallenges[i].completions[1] = 0;
		g_MpChallenges[i].completions[2] = 0;
		g_MpChallenges[i].completions[3] = 0;

		mpconfig = challengeLoad(i, buffer, 0x1ca);
		challengeForceUnlockConfigFeatures(&mpconfig->config, g_MpChallenges[i].unlockfeatures, 16, i);
	}

	// Challenges+: initialise its records/unlocks from its own configs. The
	// completions themselves are filled by gamefileLoad from the save's Plus
	// block; here we zero them and derive the (unused) unlock features.
	for (i = 0; i < ARRAYCOUNT(g_MpChallengesPlus); i++) {
		g_MpChallengesPlus[i].availability = 0;
		g_MpChallengesPlus[i].completions[0] = 0;
		g_MpChallengesPlus[i].completions[1] = 0;
		g_MpChallengesPlus[i].completions[2] = 0;
		g_MpChallengesPlus[i].completions[3] = 0;

		g_ChallengeMode = 1;
		mpconfig = challengeLoad(i, buffer, 0x1ca);
		g_ChallengeMode = 0;
		challengeForceUnlockConfigFeatures(&mpconfig->config, g_MpChallengesPlus[i].unlockfeatures, 16, i);
	}

	for (i = 0; i < mpGetNumPresets(); i++) {
		mpconfig = challengeLoadConfig(g_MpPresets[i].confignum, buffer, 0x1ca);
		challengeForceUnlockConfigFeatures(&mpconfig->config, g_MpPresets[i].requirefeatures, 16, -1);
	}

	challengeDetermineUnlockedFeatures();
}
