#include "types.h"
#include "constants.h"
#include "bss.h"
#include "data.h"
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "system.h"
#include "game/challenge.h"
#include "challengeplus.h"

/*
 * Challenges+ star records.
 *
 * The GAME note's body stopped at 0xa0 bytes, while the stock fields already
 * used all but three bits of it, so the completions block was cut short and the
 * medal block never made it to disk at all. Rather than repack a save format
 * every machine already has, the stars go in their own small file next to the
 * other save files. One file per save directory, with a block per agent, keyed
 * by the same guid as the GAME note and checked against the agent's name so a
 * reused fileid cannot inherit someone else's stars.
 *
 * A block stores 30 challenges x 4 player counts x 3 bits: 0 is no record and
 * 1..4 are bronze, silver, gold and perfect. That is 45 bytes, with one spare
 * byte so a field that straddles the final byte can be read without special
 * casing. The file is rewritten whole; nobody else reads it.
 */
#define PLUSFILE_PATH "$V/challengeplus.bin"
#define PLUSFILE_MAGIC "CPS1"
#define PLUSFILE_VERSION 1
#define PLUSFILE_MAXAGENTS 8
#define PLUSFILE_ENTRIES (30 * MAX_LOCAL_PLAYERS)
#define PLUSFILE_RECORDS (PLUSFILE_ENTRIES * 3 / 8 + 1)

struct plusfileagent {
	u16 serial;
	u8 fileid;
	u8 pad;
	char name[11];
	u8 records[PLUSFILE_RECORDS];
	u8 reserved[3];
};

struct plusfile {
	char magic[4];
	u8 version;
	u8 count;
	u16 reserved;
	struct plusfileagent agents[PLUSFILE_MAXAGENTS];
};

// The agent whose records are currently in g_MpChallengesPlus. Switching agent
// writes those records back before the next set is loaded, so a star earned in
// a match is not lost if the game never reached an ordinary game save.
static u16 s_LoadedSerial;
static s32 s_LoadedFileid;
static char s_LoadedName[11];
static bool s_Loaded;

static u32 plusReadRecord(u8 *records, s32 entry)
{
	s32 bit = entry * 3;
	s32 byte = bit >> 3;
	s32 shift = bit & 7;

	return ((records[byte] | (records[byte + 1] << 8)) >> shift) & 7;
}

static void plusWriteRecord(u8 *records, s32 entry, u32 value)
{
	s32 bit = entry * 3;
	s32 byte = bit >> 3;
	s32 shift = bit & 7;
	u32 mask = 7u << shift;

	records[byte] = (records[byte] & ~mask) | ((value << shift) & 0xff);

	if (shift > 5) {
		records[byte + 1] = (records[byte + 1] & ~(mask >> 8)) | ((value << shift) >> 8);
	}
}

static void plusPackRecords(u8 *records)
{
	s32 i;
	s32 j;

	memset(records, 0, PLUSFILE_RECORDS);

	for (i = 0; i < 30; i++) {
		for (j = 0; j < MAX_LOCAL_PLAYERS; j++) {
			u8 completion = g_MpChallengesPlus[i].completions[j];
			u32 value = (completion & 1) ? (((completion >> 5) & 7) + 1) : 0;

			plusWriteRecord(records, i * MAX_LOCAL_PLAYERS + j, value);
		}
	}
}

static void plusUnpackRecords(u8 *records)
{
	s32 i;
	s32 j;

	for (i = 0; i < 30; i++) {
		for (j = 0; j < MAX_LOCAL_PLAYERS; j++) {
			u32 value = plusReadRecord(records, i * MAX_LOCAL_PLAYERS + j);

			g_MpChallengesPlus[i].completions[j] = 0;

			if (value > 0) {
				challengeSetCompletedByAnyPlayerIn(g_MpChallengesPlus, i, j + 1, true);
				challengeSetMedalIn(g_MpChallengesPlus, i, j + 1, value - 1);
			}
		}
	}
}

static struct plusfileagent *plusFindAgent(struct plusfile *file, u16 serial, s32 fileid, const char *name, bool create)
{
	struct plusfileagent *freeagent = NULL;
	s32 i;

	for (i = 0; i < file->count && i < PLUSFILE_MAXAGENTS; i++) {
		struct plusfileagent *agent = &file->agents[i];

		if (agent->serial == serial && agent->fileid == fileid
				&& memcmp(agent->name, name, sizeof(agent->name)) == 0) {
			return agent;
		}

		if (!freeagent && agent->serial == 0) {
			freeagent = agent;
		}
	}

	if (!create) {
		return NULL;
	}

	if (!freeagent && file->count < PLUSFILE_MAXAGENTS) {
		freeagent = &file->agents[file->count++];
	}

	if (freeagent) {
		memset(freeagent, 0, sizeof(*freeagent));
		freeagent->serial = serial;
		freeagent->fileid = fileid;
		memcpy(freeagent->name, name, sizeof(freeagent->name));
	}

	return freeagent;
}

static void plusWriteFor(u16 serial, s32 fileid, const char *name)
{
	struct plusfile file;
	struct plusfileagent *agent;
	u32 size = 0;
	void *data;
	FILE *fp;

	memset(&file, 0, sizeof(file));
	data = fsFileLoad(PLUSFILE_PATH, &size);

	if (data && size == sizeof(file)) {
		memcpy(&file, data, sizeof(file));
	}

	if (data) {
		sysMemFree(data);
	}

	if (memcmp(file.magic, PLUSFILE_MAGIC, 4) != 0 || file.version != PLUSFILE_VERSION) {
		memset(&file, 0, sizeof(file));
	}

	agent = plusFindAgent(&file, serial, fileid, name, true);

	if (!agent) {
		sysLogPrintf(LOG_WARNING, "challengeplus: no room for another agent's stars");
		return;
	}

	plusPackRecords(agent->records);
	memcpy(file.magic, PLUSFILE_MAGIC, 4);
	file.version = PLUSFILE_VERSION;

	fp = fsFileOpenWrite(PLUSFILE_PATH);

	if (fp) {
		fwrite(&file, 1, sizeof(file), fp);
		fsFileFree(fp);
	} else {
		sysLogPrintf(LOG_WARNING, "challengeplus: could not write %s", fsFullPath(PLUSFILE_PATH));
	}
}

void challengeplusSave(void)
{
	if (!g_GameFileGuid.deviceserial || !g_GameFileGuid.fileid) {
		return; // no agent selected
	}

	plusWriteFor(g_GameFileGuid.deviceserial, g_GameFileGuid.fileid, g_GameFile.name);

	s_LoadedSerial = g_GameFileGuid.deviceserial;
	s_LoadedFileid = g_GameFileGuid.fileid;
	memcpy(s_LoadedName, g_GameFile.name, sizeof(s_LoadedName));
	s_Loaded = true;
}

void challengeplusLoad(void)
{
	struct plusfile file;
	struct plusfileagent *agent;
	u32 size = 0;
	void *data;
	s32 i;

	if (!g_GameFileGuid.deviceserial || !g_GameFileGuid.fileid) {
		return;
	}

	// Those records still belong to the agent that was loaded before this one.
	if (s_Loaded && (s_LoadedSerial != g_GameFileGuid.deviceserial
			|| s_LoadedFileid != g_GameFileGuid.fileid
			|| memcmp(s_LoadedName, g_GameFile.name, sizeof(s_LoadedName)) != 0)) {
		plusWriteFor(s_LoadedSerial, s_LoadedFileid, s_LoadedName);
	}

	for (i = 0; i < 30; i++) {
		memset(g_MpChallengesPlus[i].completions, 0, sizeof(g_MpChallengesPlus[i].completions));
	}

	memset(&file, 0, sizeof(file));
	data = fsFileLoad(PLUSFILE_PATH, &size);

	if (data && size == sizeof(file)) {
		memcpy(&file, data, sizeof(file));
	}

	if (data) {
		sysMemFree(data);
	}

	if (memcmp(file.magic, PLUSFILE_MAGIC, 4) != 0 || file.version != PLUSFILE_VERSION) {
		memset(&file, 0, sizeof(file));
	}

	agent = plusFindAgent(&file, g_GameFileGuid.deviceserial, g_GameFileGuid.fileid, g_GameFile.name, false);

	if (agent) {
		plusUnpackRecords(agent->records);
	}

	s_LoadedSerial = g_GameFileGuid.deviceserial;
	s_LoadedFileid = g_GameFileGuid.fileid;
	memcpy(s_LoadedName, g_GameFile.name, sizeof(s_LoadedName));
	s_Loaded = true;
}
