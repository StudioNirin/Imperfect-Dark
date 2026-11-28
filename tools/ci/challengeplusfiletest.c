// Unit harness for port/src/challengeplus.c: the real file code with a tiny
// fake filesystem, so a save/reload and the per-agent key can be checked
// without running a whole match.
#include "types.h"
#include "constants.h"
#include "bss.h"
#include "data.h"
#include "game/challenge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct gamefile g_GameFile;
struct fileguid g_GameFileGuid;
struct challenge g_MpChallengesPlus[30];

static char s_Path[512];
static s32 s_Writes;

void challengeSetCompletedByAnyPlayerIn(struct challenge *list, s32 index, s32 numplayers, bool completed)
{
	if (completed) {
		list[index].completions[numplayers - 1] |= 1;
	} else {
		list[index].completions[numplayers - 1] &= ~1;
	}
}

void challengeSetMedalIn(struct challenge *list, s32 index, s32 numplayers, u32 medal)
{
	u8 *completion = &list[index].completions[numplayers - 1];

	*completion = (*completion & ~(7 << 5)) | ((medal & 7) << 5);
}

void *fsFileLoad(const char *name, u32 *outSize)
{
	FILE *f = fopen(s_Path, "rb");
	u8 *data;

	(void) name;

	if (!f) {
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	*outSize = (u32) ftell(f);
	rewind(f);
	data = malloc(*outSize + 1);

	if (*outSize) {
		if (fread(data, 1, *outSize, f) != *outSize) {
			free(data);
			fclose(f);
			return NULL;
		}
	}

	data[*outSize] = 0;
	fclose(f);

	return data;
}

FILE *fsFileOpenWrite(const char *name)
{
	(void) name;
	s_Writes++;
	return fopen(s_Path, "wb");
}

void fsFileFree(FILE *f)
{
	fclose(f);
}

const char *fsFullPath(const char *relPath)
{
	return relPath;
}

void sysLogPrintf(s32 level, const char *fmt, ...)
{
	(void) level;
	(void) fmt;
}

void sysMemFree(void *ptr)
{
	free(ptr);
}

void challengeplusLoad(void);
void challengeplusSave(void);

#define CHECK(cond) do { \
	if (!(cond)) { \
		fprintf(stderr, "fail at line %d: %s\n", __LINE__, #cond); \
		exit(1); \
	} \
} while (0)

static u32 record(s32 index, s32 tier)
{
	u8 c = g_MpChallengesPlus[index].completions[tier];

	return (c & 1) ? ((c >> 5) & 7) + 1 : 0;
}

static void clearRecords(void)
{
	s32 i;
	s32 j;

	for (i = 0; i < 30; i++) {
		for (j = 0; j < MAX_LOCAL_PLAYERS; j++) {
			g_MpChallengesPlus[i].completions[j] = 0;
		}
	}
}

static void setRecord(s32 index, s32 tier, u32 value)
{
	if (value > 0) {
		g_MpChallengesPlus[index].completions[tier] |= 1;
		g_MpChallengesPlus[index].completions[tier] |= (value - 1) << 5;
	}
}

int main(int argc, char **argv)
{
	FILE *f;

	CHECK(argc == 2);
	snprintf(s_Path, sizeof(s_Path), "%s/challengeplus.bin", argv[1]);
	remove(s_Path);

	// one agent, four records of 0/1/4/3 spread across challenges and tiers
	g_GameFileGuid.deviceserial = 0x1aba;
	g_GameFileGuid.fileid = 36;
	strcpy(g_GameFile.name, "Agent One");
	clearRecords();
	setRecord(0, 0, 4); // perfect
	setRecord(0, 1, 2); // silver
	setRecord(29, 3, 3); // gold
	challengeplusSave();
	CHECK(s_Writes == 1);

	// a second agent in the same file must not see the first one's stars
	g_GameFileGuid.fileid = 38;
	strcpy(g_GameFile.name, "Agent Two");
	clearRecords();
	setRecord(0, 0, 1); // bronze
	challengeplusSave();

	// back to the first agent: stars reload exactly
	g_GameFileGuid.fileid = 36;
	strcpy(g_GameFile.name, "Agent One");
	clearRecords();
	challengeplusLoad();
	CHECK(record(0, 0) == 4);
	CHECK(record(0, 1) == 2);
	CHECK(record(29, 3) == 3);
	CHECK(record(0, 2) == 0);

	// a reused fileid with a different name inherits nothing
	g_GameFileGuid.fileid = 36;
	strcpy(g_GameFile.name, "Agent Three");
	clearRecords();
	challengeplusLoad();
	CHECK(record(0, 0) == 0);

	// a truncated file reads as blank rather than crashing or shifting
	g_GameFileGuid.fileid = 36;
	strcpy(g_GameFile.name, "Agent One");
	f = fopen(s_Path, "wb");
	CHECK(f != NULL);
	fputc('C', f);
	fclose(f);
	clearRecords();
	challengeplusLoad();
	CHECK(record(0, 0) == 0);

	printf("challengeplusfiletest: ok - stars survive a reload, agents stay separate, bad file reads blank\n");
	return 0;
}
