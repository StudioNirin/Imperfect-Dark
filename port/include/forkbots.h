#ifndef _IN_FORKBOTS_H
#define _IN_FORKBOTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The self-test (--test-forkbots): what a simulant is allowed to make of this
 * fork's own guns, read out of the preference table and out of the scorer that
 * reads it (src/game/botinv.c's g_AibotWeaponPreferences).
 */
void forkbotSelfTest(void);

#ifdef __cplusplus
}
#endif
#endif
