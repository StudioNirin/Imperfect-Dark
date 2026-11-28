#ifndef _IN_CHALLENGEPLUS_H
#define _IN_CHALLENGEPLUS_H

// Challenges+ stars live in their own file beside the other save files, keyed
// to the agent that earned them. The GAME note this build used to append them
// to only had room for three bits of the completion block, so medals never
// reached disk.
void challengeplusLoad(void);
void challengeplusSave(void);

#endif
