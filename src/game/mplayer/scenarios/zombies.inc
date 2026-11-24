#include <string.h>
#ifndef PLATFORM_N64
#include "system.h"
#endif
#include "game/bot.h"
#include "game/body.h"
#include "game/chr.h"
#include "game/modelmgr.h"
#include "game/player.h"
#include "game/radar.h"
#include "lib/model.h"
#include "data.h"

/**
 * Zombies
 *
 * A free-for-all in which the infected wear Skedar bodies and fight with
 * their fists, while the survivors carry a PD Shotgun and a Falcon 2. The
 * infected chase at half again the walking speed - the survivors' own
 * sprint, permanently, and sprint adds nothing on top of it. About one chr
 * in four starts infected each round (players and simulants alike),
 * and an uninfected chr that dies rises as a zombie. A five-second countdown
 * before each round says who is who; nobody can be hurt during it. The round
 * ends when its clock runs out - every survivor still uninfected gets two
 * points - or when the last uninfected chr falls. Three rounds are played,
 * with a fresh roll of starters each time; the winner is decided on total
 * points afterwards. Zombies are off the radar unless the option says
 * otherwise, and then only in red.
 *
 * Everything is host-authoritative. A net client only renders state published
 * in the NETSCEN block (see netscen.c's ZOM_* offsets), the same way One Shall
 * Stand... publishes lives and eliminations.
 *
 * The scenario's 32-bit save word holds the number of rounds and the radar
 * option.
 */

extern struct menudialogdef g_ExtGameOptionsMenuDialog;
extern s32 g_MpTimeLimit60;

s32 g_ZombiesRounds = 3;
bool g_ZombiesRadarZombies = false;

#define ZOM_PHASE_NONE    0
#define ZOM_PHASE_FIGHT   1
#define ZOM_PHASE_END     2
#define ZOM_PHASE_READY   3
// Seconds, and the phasetime below multiplies by 60 for the clock's own
// unit: TICKS(5) is five sixtyths of a second (the constant.h note beside
// SPRINT_RECOVERY60 spells this trap out the hard way).
#define ZOM_READY_SECONDS 5
#define ZOM_NOTICE_SECONDS 4
#define ZOM_END_SECONDS 3

// A red for textRender, which takes colours as alpha, red, green, blue -
// the same red the paused overlay's flash and the engine's blood use. The
// radar's g_TeamColours array is the other byte order (the radar's dot
// routine reorders it itself), so it must not be handed to textRender: as
// text it is invisible black.
#define ZOM_TEXT_RED      0xff0000ff

#ifndef PLATFORM_N64
// Published by the host, indexed by g_MpAllChrPtrs index (not net slot).
u8 g_ZombiesNetRole[MAX_MPCHRS];
s16 g_ZombiesNetScore[MAX_MPCHRS];
u8 g_ZombiesNetRound;
u8 g_ZombiesNetPhase;
u16 g_ZombiesNetTime60;
bool g_ZombiesNetBlockSeen = false;
#endif

// A seat that the zombies caught wakes to a banner saying so. The pending
// mark is set the moment the conversion is known (on the host at the death,
// on a client when the host's role byte flips); the banner itself starts
// when the seat comes back to life.
static s32 sZombiesNotice60[MAX_LOCAL_PLAYERS];
static bool sZombiesConvertPending[MAX_LOCAL_PLAYERS];
// A client's snapshot of the published roles, to spot the flip to infected.
static u8 sZombiesLastNetRole[MAX_MPCHRS];
static s32 sZombiesLastNetRound = -1;
// The host's fight-phase log line: the last uninfected count and round it
// was printed for, so the line repeats only when something changed.
static s32 sZombiesLastLoggedUninfected = -1;
static s32 sZombiesLastLoggedRound = -1;

/**
 * A seat nobody plays (an open seat on a host): it is real in the player
 * list but out of play, so it takes no part in the round - not the starter
 * pool, not the fight's uninfected count, not the end-of-round points, and
 * not the score table. The same seam Pop a Cap and the co-op modes use.
 */
static bool zombiesOutOfPlay(s32 index)
{
#ifndef PLATFORM_N64
	if (index >= 0 && index < PLAYERCOUNT() && netPlayerOutOfPlay(index)) {
		return true;
	}
#endif
	return false;
}

/**
 * Rounds slider, above the familiar free-for-all rows.
 */
MenuItemHandlerResult menuhandlerMpZombiesRounds(s32 operation, struct menuitem *item, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_GETSLIDER:
		data->slider.value = g_ZombiesRounds - 1;
		break;
	case MENUOP_SET:
		g_ZombiesRounds = data->slider.value + 1;
		break;
	case MENUOP_GETSLIDERLABEL:
		sprintf(data->slider.label, "%d", data->slider.value + 1);
		break;
	}

	return 0;
}

/**
 * Whether the infected show up on the radar. Off by default: only the
 * survivors are blipped, so a zombie can stalk unseen. On: the infected get
 * a red blip instead of the usual green.
 */
MenuItemHandlerResult menuhandlerMpZombiesRadar(s32 operation, struct menuitem *item, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_GET:
		return g_ZombiesRadarZombies;
	case MENUOP_SET:
		g_ZombiesRadarZombies = data->checkbox.value != 0;
		break;
	}

	return 0;
}

struct menuitem g_ZombiesOptionsMenuItems[] = {
	{
		MENUITEMTYPE_SLIDER,
		0,
		MENUITEMFLAG_LOCKABLEMINOR | MENUITEMFLAG_LITERAL_TEXT,
		(uintptr_t) "Rounds\n",
		8,
		menuhandlerMpZombiesRounds,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR | MENUITEMFLAG_LITERAL_TEXT,
		(uintptr_t) "Zombies show on radar\n",
		0,
		menuhandlerMpZombiesRadar,
	},
	{
		MENUITEMTYPE_DROPDOWN,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_223, // "Slow Motion"
		0,
		menuhandlerMpSlowMotion,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_224, // "Fast Movement"
		MPOPTION_FASTMOVEMENT,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_226, // "No Radar"
		MPOPTION_NORADAR,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_227, // "No Auto-Aim"
		MPOPTION_NOAUTOAIM,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_SEPARATOR,
		0,
		0,
		0,
		0,
		NULL,
	},
	{
		MENUITEMTYPE_SELECTABLE,
		0,
		MENUITEMFLAG_SELECTABLE_CLOSESDIALOG,
		L_MPMENU_239, // "Back"
		0,
		NULL,
	},
	{ MENUITEMTYPE_END },
};

struct menudialogdef g_ZombiesOptionsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	(uintptr_t) "Zombies Options\n",
	g_ZombiesOptionsMenuItems,
	mpOptionsMenuDialog,
	MENUDIALOGFLAG_MPLOCKABLE | MENUDIALOGFLAG_LITERAL_TEXT,
	&g_ExtGameOptionsMenuDialog,
};

void zombiesWriteSave(struct savebuffer *buffer)
{
	savebufferOr(buffer, g_ZombiesRounds | (g_ZombiesRadarZombies ? 0x10000 : 0), 32);
}

void zombiesReadSave(struct savebuffer *buffer, u8 version)
{
	s32 value = savebufferReadBits(buffer, 32);

	g_ZombiesRadarZombies = (value & 0x10000) != 0;
	g_ZombiesRounds = value & 0xff;

	if (g_ZombiesRounds < 1 || g_ZombiesRounds > 9) {
		g_ZombiesRounds = 3;
	}
}

/**
 * Find the g_MpAllChrPtrs index a chr occupies, or -1.
 */
static s32 zombiesChrIndex(struct chrdata *chr)
{
	s32 i;

	if (!chr) {
		return -1;
	}

	for (i = 0; i < g_MpNumChrs; i++) {
		if (g_MpAllChrPtrs[i] == chr) {
			return i;
		}
	}

	return -1;
}

/**
 * True if this chr is infected. Clients read the host's published byte.
 */
bool zombiesIsInfected(struct chrdata *chr)
{
	s32 index = zombiesChrIndex(chr);

	if (index < 0) {
		return false;
	}

#ifndef PLATFORM_N64
	if (NET_CLIENT && g_ZombiesNetBlockSeen) {
		return g_ZombiesNetRole[index] != 0;
	}
#endif
	return g_ScenarioData.zombies.infected[index] != 0;
}

/**
 * Take the body-rebuild request for the current local player.
 */
/**
 * Whether a seat's body needs rebuilding after a Zombies role change, without
 * consuming the mark. The normal player tick has to ask this before deciding
 * to rebuild a body that is already standing: a role flip does not by itself
 * remove the old body, and the Take below is only reached once that rebuild
 * starts. Indexed by player number so an already-built human can be converted
 * on the host as well as on a client puppet.
 */
bool zombiesBodyStaleIs(s32 playernum)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return false;
	}

	if (playernum < 0 || playernum >= ARRAYCOUNT(g_ScenarioData.zombies.bodystale)) {
		return false;
	}

	return g_ScenarioData.zombies.bodystale[playernum];
}

/**
 * Take a seat's stale mark, by an explicit player number. The client puppet
 * tick passes the seat it is currently ticking, which need not be
 * g_Vars.currentplayernum; using the global there desynchronises the consumer
 * from the mark the NETSCEN block set.
 */
bool zombiesBodyStaleTakeForPlayer(s32 playernum)
{
	bool stale = zombiesBodyStaleIs(playernum);

	if (stale) {
		g_ScenarioData.zombies.bodystale[playernum] = false;
	}

	return stale;
}

bool zombiesBodyStaleTake(void)
{
	return zombiesBodyStaleTakeForPlayer(g_Vars.currentplayernum);
}

/**
 * Whether the seat's role is infected, asked by local player number. For
 * callers that cannot lean on a chr pointer, which may be gone mid-respawn.
 * Players hold the head of the g_MpAllChrPtrs pool, so the seat number is
 * the role index.
 */
bool zombiesSeatIsInfected(s32 playernum)
{
	// The role arrays survive a switch to another mode (only this mode's own
	// stage reset clears them), so outside the mode they mean nothing - and
	// the movement hooks that ask are running in every mode.
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return false;
	}

	if (playernum < 0 || playernum >= g_MpNumChrs) {
		return false;
	}

#ifndef PLATFORM_N64
	if (NET_CLIENT && g_ZombiesNetBlockSeen) {
		return g_ZombiesNetRole[playernum] != 0;
	}
#endif
	return g_ScenarioData.zombies.infected[playernum] != 0;
}

/**
 * Called as a local seat comes back to life. One that the zombies caught
 * wakes to a banner saying so, so nobody wonders why a Skedar is looking
 * back at them from the mirror.
 */
void zombiesSeatWokeUp(s32 playernum)
{
	if (playernum < 0 || playernum >= ARRAYCOUNT(sZombiesConvertPending)) {
		return;
	}

	if (!sZombiesConvertPending[playernum]) {
		return;
	}

	sZombiesConvertPending[playernum] = false;
	sZombiesNotice60[playernum] = TICKS(ZOM_NOTICE_SECONDS * 60);
}

/**
 * False during the countdown before a round: nobody may hurt anybody until
 * the fight actually starts. Outside the mode it is always true.
 */
bool zombiesRoundInProgress(void)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return true;
	}

	return g_ScenarioData.zombies.phase == ZOM_PHASE_FIGHT;
}

/**
 * True while the round's countdown is on screen: the seconds between a
 * round starting and the fight opening, when every seat - players and
 * simulants alike - stands still and nobody fires. The mode's damage gate
 * already refused hits in this phase; this is the same hold for bodies.
 */
bool zombiesCountdownActive(void)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return false;
	}

#ifndef PLATFORM_N64
	// A net client follows the host's published phase, the same way the
	// countdown's own display does. Until the first block arrives the
	// client's local phase says (scenarioReset at load started this
	// round, and it is the host's to change), so the fallback is the
	// local read below rather than a guess.
	if (NET_CLIENT && g_ZombiesNetBlockSeen) {
		return g_ZombiesNetPhase == ZOM_PHASE_READY;
	}
#endif

	return g_ScenarioData.zombies.phase == ZOM_PHASE_READY;
}

/**
 * The body override. Called after the player's own choice and the spectator
 * override in playerTickChrBody(), so an infected player is Skedar whichever
 * of those won. The Skedar model carries its own head.
 */
void zombiesNotifyChrSpawn(struct chrdata *chr, s32 *bodynum, s32 *headnum, bool *sp60)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return;
	}

	// A chr is not in the match's list yet on its first build (it registers
	// at the end of the build, after this runs) and on a client the host's
	// published byte is the record of its role, so the seat is the question
	// being asked here: an infected seat is Skedar, whoever the body is
	// being built for.
	if (zombiesIsInfected(chr) || zombiesSeatIsInfected(g_Vars.currentplayernum)) {
		*bodynum = BODY_SKEDAR;
		*headnum = -1;
		if (sp60) {
			*sp60 = false;
		}
	}
}

/**
 * Ask what body a chr should wear in this mode. Returns -1 to leave the
 * caller's own choice alone.
 */
s32 zombiesBodyNumForChr(struct chrdata *chr)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return -1;
	}

	return zombiesIsInfected(chr) ? BODY_SKEDAR : -1;
}

/**
 * Prepare a simulant for a new life. botSpawn() calls this before resetting
 * it, so a chr converted during the last round comes back wearing Skedar.
 */
void zombiesPrepareBotSpawn(struct chrdata *chr)
{
	struct model *newmodel;
	struct model *oldmodel;
	struct prop *prop;
	f32 angle;
	s32 bodynum = zombiesBodyNumForChr(chr);
	s32 headnum = -1; // the Skedar carries its own head
	s32 idx = zombiesChrIndex(chr);

	if (!chr || !chr->prop) {
		return;
	}

	if (bodynum < 0 && chr->bodynum != BODY_SKEDAR) {
		// A survivor in a survivor's body: every ordinary spawn.
		return;
	}

	if (bodynum >= 0 && chr->bodynum == bodynum) {
		return;
	}

	if (bodynum >= 0) {
		// Into the Skedar body - but first, once and only once, keep the
		// body this simulant came to the match in, so a later round that
		// does not turn it infected can hand it back.
		if (idx >= 0 && g_ScenarioData.zombies.homebody[idx] < 0) {
			g_ScenarioData.zombies.homebody[idx] = chr->bodynum;
			g_ScenarioData.zombies.homehead[idx] = chr->headnum;
		}
	} else {
		// A survivor still wearing last round's Skedar body. It goes back
		// to the one it came with where one was kept; botmgr.c's own
		// match-load choice (its config row, the 4MB pool's and the
		// GoldenEye rig pair's) where a rebuild went through without -
		// the same body the sim would have been spawned in fresh.
		if (idx >= 0 && g_ScenarioData.zombies.homebody[idx] >= 0) {
			bodynum = g_ScenarioData.zombies.homebody[idx];
			headnum = g_ScenarioData.zombies.homehead[idx];
		} else if (chr->aibot && chr->aibot->config) {
			headnum = mpGetHeadId(mpHeadNumSafe(chr->aibot->config->base.mpheadnum));
			bodynum = mpGetBodyId(chr->aibot->config->base.mpbodynum);

			if (IS4MB()) {
				headnum = HEAD_DDSHOCK;
				bodynum = BODY_DDSHOCK;
			}

			gebeanOwnRigPair(&bodynum, &headnum);
		} else {
			return;
		}

		if (chr->bodynum == bodynum) {
			return;
		}
	}

	prop = chr->prop;
	oldmodel = chr->model;
	angle = modelGetChrRotY(oldmodel);
	modelFreeVertices(VTXSTORETYPE_CHRVTX, oldmodel);
	chr->model = NULL;
	modelmgrFreeModel(oldmodel);

	// Allocate the replacement after the old model is gone, so it cannot
	// inherit a slot that is still tied up by the old one.
	newmodel = bodyAllocateModel(bodynum, headnum, SPAWNFLAG_FIXEDHEIGHT);

	if (!newmodel) {
		return;
	}

	chr0f020b14(prop, newmodel, &prop->pos, prop->rooms, angle, NULL);
	prop->type = PROPTYPE_CHR;
	chr->bodynum = bodynum;
	chr->headnum = headnum;
	chr->race = bodyGetRace(bodynum);
	// bodyAllocateModel() sets each body's own scale (0.1 for Skedar); the
	// stock 1.0 reset here made an infected simulant ten players tall.
}

/**
 * Clear all of the mode's state. Used when it is selected, a setup is
 * loaded, and at the start of every stage: ticks can run on a stage that is
 * about to be set up again - the boot's catch-up before a match's real
 * first frame - and the first round must begin fresh even against that.
 */
void zombiesStageReset(void)
{
	s32 i;

	memset(&g_ScenarioData.zombies, 0, sizeof(g_ScenarioData.zombies));

	// The remember-a-sim's-body slots say "nothing stored" with -1, which a
	// memset of zeroes would read as a real body number.
	for (i = 0; i < MAX_MPCHRS; i++) {
		g_ScenarioData.zombies.homebody[i] = -1;
		g_ScenarioData.zombies.homehead[i] = -1;
	}

	memset(sZombiesNotice60, 0, sizeof(sZombiesNotice60));
	memset(sZombiesConvertPending, 0, sizeof(sZombiesConvertPending));
	memset(sZombiesLastNetRole, 0, sizeof(sZombiesLastNetRole));
	sZombiesLastNetRound = -1;
	sZombiesLastLoggedUninfected = -1;
	sZombiesLastLoggedRound = -1;
}

/**
 * Clear the mode's state when it is selected or a setup is loaded.
 */
void zombiesInit(void)
{
	s32 i;

	g_MpSetup.options &= ~MPOPTION_TEAMSENABLED;

	// The first two slots are the survivors' two guns, shown in the Weapons
	// page where the player sets them. Seed the stock pair whenever the mode
	// is applied (menu, saved setup or the boot harness); a player can then
	// change either slot for the match. Slots 3-6 are the floor pickups this
	// mode never places, so leave them empty.
	zombiesSeedLoadout();

	for (i = 2; i < NUM_MPWEAPONSLOTS; i++) {
		g_MpSetup.weapons[i] = MPWEAPON_NONE;
	}

	zombiesStageReset();

#ifndef PLATFORM_N64
	memset(g_ZombiesNetRole, 0, sizeof(g_ZombiesNetRole));
	memset(g_ZombiesNetScore, 0, sizeof(g_ZombiesNetScore));
	g_ZombiesNetBlockSeen = false;
#endif
}

/**
 * Roll a fresh set of starters and begin the next round.
 */
static void zombiesStartRound(s32 round)
{
	s32 pool[MAX_MPCHRS];
	s32 poolcount = 0;
	s32 wanted;
	s32 i;
	s32 j;
	s32 tmp;
	s32 index;
	char text[80];

	g_ScenarioData.zombies.round = round;
	// The round opens on the countdown: who you are, and five seconds before
	// anybody can be hurt.
	g_ScenarioData.zombies.phase = ZOM_PHASE_READY;
	g_ScenarioData.zombies.roundtime60 = g_MpTimeLimit60 > 0 ? g_MpTimeLimit60 : TICKS(180 * 60);
	g_ScenarioData.zombies.phasetime60 = TICKS(ZOM_READY_SECONDS * 60);

	// A fresh roll: the infected of last round go back to being candidates.
	// Everyone whose role changes gets the body rebuild below, and the kit
	// a new life hands out reads the same array.
	memset(g_ScenarioData.zombies.infected, 0, sizeof(g_ScenarioData.zombies.infected));

	// Reuse the role array as the pool marker while rolling. A seat nobody
	// plays (an open seat on a host) is not a candidate: it is hidden, never
	// hurt, and never converts, so it would stay uninfected forever and the
	// round could never end.
	for (i = 0; i < g_MpNumChrs; i++) {
		if (zombiesOutOfPlay(i)) {
			continue;
		}

		pool[poolcount++] = i;
	}

	if (poolcount > 0) {
		wanted = (poolcount + 3) / 4; // ceiling division: 5 -> 2, 9 -> 3
		if (wanted < 1) {
			wanted = 1;
		}
		if (wanted > poolcount) {
			wanted = poolcount;
		}

		// Prefer chrs who have not started a previous round infected.
		for (i = poolcount - 1; i > 0; i--) {
			j = rngRandom() % (i + 1);
			tmp = pool[i];
			pool[i] = pool[j];
			pool[j] = tmp;
		}

		for (i = 0; i < poolcount && wanted > 0; i++) {
			index = pool[i];
			if (!g_ScenarioData.zombies.startedas[index]) {
				g_ScenarioData.zombies.infected[index] = 1;
				g_ScenarioData.zombies.startedas[index] = 1;
				wanted--;
			}
		}

		for (i = 0; i < poolcount && wanted > 0; i++) {
			index = pool[i];
			if (!g_ScenarioData.zombies.infected[index]) {
				g_ScenarioData.zombies.infected[index] = 1;
				g_ScenarioData.zombies.startedas[index] = 1;
				wanted--;
			}
		}
	}

	// Everyone gets a new life at a fresh spawn. Asked of the engine rather
	// than called direct: playerStartNewLife() has one sanctioned call site
	// (lv.c) which reads it from the seat it has installed, and a seat that
	// was down when the round turned is already part way through its own
	// respawn. playerStartNewLife() restocks each seat by role - fists for
	// the infected, the kit for the survivors (playerSpawnWeapons()'s
	// scenario branch); bodystale makes the rebuild pick up the new role.
	for (i = 0; i < PLAYERCOUNT() && i < ARRAYCOUNT(g_ScenarioData.zombies.bodystale); i++) {
		if (zombiesOutOfPlay(i)) {
			continue;
		}

		g_ScenarioData.zombies.bodystale[i] = true;
	}

	for (i = 0; i < PLAYERCOUNT() && i < ARRAYCOUNT(g_Vars.players); i++) {
		if (g_Vars.players[i] && !zombiesOutOfPlay(i)) {
			g_Vars.players[i]->dostartnewlife = true;
		}
	}

	for (i = 0; i < g_BotCount; i++) {
		botSpawn(g_MpBotChrPtrs[i], true);
	}

	sprintf(text, "ROUND %d\n", round);
	hudmsgCreateWithFlags(text, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_NOWRAP);

#ifndef PLATFORM_N64
	{
		s32 infected = 0;
		s32 out = 0;

		for (i = 0; i < g_MpNumChrs; i++) {
			if (zombiesOutOfPlay(i)) {
				out++;
			} else if (g_ScenarioData.zombies.infected[i]) {
				infected++;
			}
		}

		if (out) {
			sysLogPrintf(LOG_NOTE, "zombies: round %d started, %d of %d infected (%d out of play)", round, infected,
					g_MpNumChrs - out, out);
		} else {
			sysLogPrintf(LOG_NOTE, "zombies: round %d started, %d of %d infected", round, infected, g_MpNumChrs);
		}
	}
#endif
}

/**
 * End the current round: every chr still uninfected banks two points.
 */
static void zombiesEndRound(void)
{
	s32 i;
	char text[80];

	if (g_ScenarioData.zombies.phase != ZOM_PHASE_FIGHT) {
		return;
	}

	g_ScenarioData.zombies.phase = ZOM_PHASE_END;
	g_ScenarioData.zombies.phasetime60 = TICKS(ZOM_END_SECONDS * 60);

	for (i = 0; i < g_MpNumChrs; i++) {
		if (zombiesOutOfPlay(i)) {
			continue;
		}

		if (!g_ScenarioData.zombies.infected[i]) {
			g_ScenarioData.zombies.score[i] += 2;
		}
	}

	sprintf(text, "ROUND %d OVER\n", g_ScenarioData.zombies.round);
	hudmsgCreateWithFlags(text, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_NOWRAP);

#ifndef PLATFORM_N64
	sysLogPrintf(LOG_NOTE, "zombies: round %d over", g_ScenarioData.zombies.round);
#endif
}

void zombiesTick(void)
{
	struct scenariodata_zombies *data = &g_ScenarioData.zombies;
	s32 i;
	s32 uninfected = 0;

	if (!g_Vars.normmplayerisrunning) {
		return;
	}

	// One tick's worth of the mode's visible clocks. The engine reports real
	// elapsed time, so a load hitch or a lag spike would otherwise skip the
	// countdown and the wake banner whole, on a frame nobody was watching.
	const s32 stepticks60 = g_Vars.lvupdate60 > 4 ? 4 : g_Vars.lvupdate60;

	for (i = 0; i < PLAYERCOUNT() && i < ARRAYCOUNT(sZombiesNotice60); i++) {
		if (sZombiesNotice60[i] > 0) {
			sZombiesNotice60[i] -= stepticks60;
		}
	}

#ifndef PLATFORM_N64
	if (NET_CLIENT) {
		if (!g_ZombiesNetBlockSeen) {
			return;
		}

		// A client notices its own seats turning when the host's role byte
		// flips to infected. A round turn resyncs the snapshot quietly:
		// starting a round as a zombie is not "having been infected".
		if ((s32) g_ZombiesNetRound != sZombiesLastNetRound) {
			sZombiesLastNetRound = g_ZombiesNetRound;
		} else {
			for (i = 0; i < PLAYERCOUNT() && i < g_MpNumChrs && i < ARRAYCOUNT(sZombiesConvertPending); i++) {
				if (g_ZombiesNetRole[i] && !sZombiesLastNetRole[i]) {
					sZombiesConvertPending[i] = true;
				}
			}
		}

		for (i = 0; i < ARRAYCOUNT(sZombiesLastNetRole) && i < g_MpNumChrs; i++) {
			sZombiesLastNetRole[i] = g_ZombiesNetRole[i];
		}

		return;
	}
#endif

	if (!data->inited) {
		if (g_MpNumChrs <= 0) {
			return;
		}

		data->inited = true;
		zombiesStartRound(1);
		return;
	}

	if (data->phase == ZOM_PHASE_READY) {
		// In clock steps of at most four ticks: the engine reports real
		// elapsed time, so a load hitch or lag spike must not be able to
		// skip seconds of a countdown nobody was watching. (A round begun
		// outright before the stage's first real frame is out of the way:
		// lvSetupStage's scenarioReset starts the mode fresh.)
		//
		// The match's opening swirl holds the clock too: it runs ticks
		// behind a camera the players have no hands on, and the first
		// round's countdown was burning away before anyone was in control.
		if (g_Vars.tickmode != TICKMODE_MPSWIRL) {
			data->phasetime60 -= stepticks60;
		}

		if (data->phasetime60 <= 0) {
			data->phase = ZOM_PHASE_FIGHT;
		}
	} else if (data->phase == ZOM_PHASE_FIGHT) {
		data->roundtime60 -= g_Vars.lvupdate60;

		for (i = 0; i < g_MpNumChrs; i++) {
			if (zombiesOutOfPlay(i)) {
				continue;
			}

			if (!data->infected[i]) {
				uninfected++;
			}
		}

#ifndef PLATFORM_N64
		// Log one line per round-flip change, not every tick: phase, round,
		// how many are uninfected and how many are out of play, so a log can
		// show a stalled round as a stall.
		if (uninfected != sZombiesLastLoggedUninfected || sZombiesLastLoggedRound != data->round) {
			sZombiesLastLoggedUninfected = uninfected;
			sZombiesLastLoggedRound = data->round;

			{
				s32 out = 0;
				s32 k;

				for (k = 0; k < g_MpNumChrs; k++) {
					if (zombiesOutOfPlay(k)) {
						out++;
					}
				}

				sysLogPrintf(LOG_NOTE,
					"zombies: tick %u %s round %d: %d uninfected, %d out of play",
					g_NetTick, "fight", data->round, uninfected, out);
			}
		}
#endif

		if (data->roundtime60 <= 0 || uninfected == 0) {
			zombiesEndRound();
		}
	} else if (data->phase == ZOM_PHASE_END) {
		data->phasetime60 -= stepticks60;

		if (data->phasetime60 <= 0 && data->round < g_ZombiesRounds) {
			zombiesStartRound(data->round + 1);
		}
	}
}

/**
 * The host ends the match once the final round's short end phase is over.
 */
s32 zombiesMatchOver(void)
{
	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES) {
		return 0;
	}

	return g_ScenarioData.zombies.phase == ZOM_PHASE_END
			&& g_ScenarioData.zombies.round >= g_ZombiesRounds;
}

/**
 * Called from mpstatsRecordDeath() for both suicides and kills. The arguments
 * are g_MpAllChrPtrs indices. Any uninfected death converts the victim; only
 * a kill by an infected attacker earns the infector a point.
 */
void zombiesHandleDeath(s32 attackerindex, s32 victimindex)
{
	struct scenariodata_zombies *data = &g_ScenarioData.zombies;
	char text[80];

	if (!g_Vars.normmplayerisrunning || g_MpSetup.scenario != MPSCENARIO_ZOMBIES
			|| NET_CLIENT || victimindex < 0 || victimindex >= g_MpNumChrs
			|| data->phase != ZOM_PHASE_FIGHT || zombiesOutOfPlay(victimindex)) {
		return;
	}

	if (data->infected[victimindex]) {
		return;
	}

	data->infected[victimindex] = 1;

	// A converted player has to rebuild their body as Skedar when they come
	// back. The body is only rebuilt when the staleness flag is set; the
	// round start sets it for everyone, but a mid-round death and respawn
	// needs it set here or the seat comes back wearing its old human body.
	// The pending mark drives the banner they wake to.
	if (victimindex < PLAYERCOUNT() && victimindex < ARRAYCOUNT(sZombiesConvertPending)) {
		data->bodystale[victimindex] = true;
		sZombiesConvertPending[victimindex] = true;
	}

	if (attackerindex >= 0 && attackerindex < g_MpNumChrs
			&& data->infected[attackerindex]) {
		data->score[attackerindex] += 1;
		data->infections[attackerindex] += 1;
	}

	sprintf(text, "%s has turned\n", g_MpAllChrConfigPtrs[victimindex]->name);
	hudmsgCreateWithFlags(text, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_NOWRAP);
}

/**
 * Mode points, not kills. Clients use the host's published score byte.
 */
void zombiesCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	*score = 0;
	*deaths = mpchr->numdeaths;

	if (mpchrnum < 0 || mpchrnum >= g_MpNumChrs) {
		return;
	}

#ifndef PLATFORM_N64
	if (NET_CLIENT && g_ZombiesNetBlockSeen) {
		*score = g_ZombiesNetScore[mpchrnum];
		return;
	}
#endif
	*score = g_ScenarioData.zombies.score[mpchrnum];
}

/**
 * The radar's say on one chr's blip. Returning true keeps radarRender()
 * from drawing its own. The infected are off the radar by default, so a
 * zombie is only ever announced by what you can see; with the option on
 * they blip in team red, apart from the survivors' green.
 */
bool zombiesRadarChr(Gfx **gdl, struct prop *prop)
{
	struct chrdata *chr;
	struct coord pos;

	if (!prop || !g_Vars.currentplayer || !g_Vars.currentplayer->prop) {
		return false;
	}

	chr = prop->chr;

	if (!chr || !zombiesIsInfected(chr)) {
		return false;
	}

	if (!g_ZombiesRadarZombies) {
		return true;
	}

	pos.x = prop->pos.x - g_Vars.currentplayer->prop->pos.x;
	pos.y = prop->pos.y - g_Vars.currentplayer->prop->pos.y;
	pos.z = prop->pos.z - g_Vars.currentplayer->prop->pos.z;

	*gdl = radarDrawDot(*gdl, prop, &pos, g_TeamColours[0], 0, 0);

	return true;
}

/**
 * One centred line, Handel Gothic Medium, in the view's middle.
 */
static Gfx *zombiesRenderCentreText(Gfx *gdl, char *text, s32 y, u32 colour)
{
	s32 textwidth;
	s32 textheight;
	s32 x = viGetViewLeft() + viGetViewWidth() / 2;

	gdl = text0f153628(gdl);
	textMeasure(&textheight, &textwidth, text, g_CharsHandelGothicMd, g_FontHandelGothicMd, 0);

	x -= textwidth / 2;
	textwidth += x;
	textheight += y;

#if VERSION >= VERSION_NTSC_1_0
	gdl = text0f153990(gdl, x, y, textwidth, textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicMd, g_FontHandelGothicMd, colour, 0xa0, viGetWidth(), viGetHeight(), 0, 0);
#else
	gdl = text0f153858(gdl, &x, &y, &textwidth, &textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicMd, g_FontHandelGothicMd, colour, 0x88, viGetWidth(), viGetHeight(), 0, 0);
#endif

	return text0f153780(gdl);
}

/**
 * Round number and phase at the top of the view, in the same place and the
 * same numeric font as One Shall Stand...'s lives. In the middle: during the
 * countdown before a round, what the viewer is in it and the seconds left;
 * and after a conversion, the banner saying the viewer turned.
 */
Gfx *zombiesRenderHud(Gfx *gdl)
{
	s32 textwidth;
	s32 textheight;
	s32 x;
	s32 y;
	s32 round;
	s32 phase;
	s32 phasetime60;
	char text[32];
	struct scenariodata_zombies *data = &g_ScenarioData.zombies;

	if (!g_Vars.currentplayer || !g_Vars.currentplayer->prop || !g_Vars.currentplayer->prop->chr) {
		return gdl;
	}

#ifndef PLATFORM_N64
	if (NET_CLIENT && g_ZombiesNetBlockSeen) {
		round = g_ZombiesNetRound;
		phase = g_ZombiesNetPhase;
		phasetime60 = g_ZombiesNetTime60;
	} else
#endif
	{
		round = data->round;
		phase = data->phase;
		phasetime60 = data->phasetime60;
	}

	if (phase == ZOM_PHASE_END) {
		sprintf(text, "ROUND OVER");
	} else {
		sprintf(text, "ROUND %d / %d", round, g_ZombiesRounds);
	}

	x = viGetViewLeft() + viGetViewWidth() / 2;
	y = viGetViewTop() + 10;

#ifndef PLATFORM_N64
	const s32 playercount = PLAYERCOUNT();
	if (playercount < 2 || (playercount == 2 && optionsGetScreenSplit() == SCREENSPLIT_HORIZONTAL)) {
		gSPSetExtraGeometryModeEXT(gdl++, G_ASPECT_CENTER_EXT);
	}
#endif

	gdl = text0f153628(gdl);
	textMeasure(&textheight, &textwidth, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, 0);

	x -= textwidth / 2;
	textwidth += x;
	textheight += y;

#if VERSION >= VERSION_NTSC_1_0
	gdl = text0f153990(gdl, x, y, textwidth, textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, g_ModColours[MODCOLOUR_KOHHUD], 0xa0, viGetWidth(), viGetHeight(), 0, 0);
#else
	gdl = text0f153858(gdl, &x, &y, &textwidth, &textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsHandelGothicXs, g_FontHandelGothicXs, g_ModColours[MODCOLOUR_KOHHUD], 0x88, viGetWidth(), viGetHeight(), 0, 0);
#endif
	gdl = text0f153780(gdl);

	if (phase == ZOM_PHASE_READY) {
		s32 seconds = (phasetime60 + TICKS(60) - 1) / TICKS(60);
		bool infected = zombiesIsInfected(g_Vars.currentplayer->prop->chr);

		if (seconds < 0) {
			seconds = 0;
		}

		gdl = zombiesRenderCentreText(gdl,
				infected ? "YOU ARE INFECTED" : "YOU ARE A SURVIVOR",
				viGetViewTop() + viGetViewHeight() / 2 - 16,
				infected ? ZOM_TEXT_RED : g_ModColours[MODCOLOUR_KOHHUD]);

		sprintf(text, "%d", seconds);
		gdl = zombiesRenderCentreText(gdl, text,
				viGetViewTop() + viGetViewHeight() / 2 + 14, g_ModColours[MODCOLOUR_KOHHUD]);
	} else if (g_Vars.currentplayernum < ARRAYCOUNT(sZombiesNotice60)
			&& sZombiesNotice60[g_Vars.currentplayernum] > 0) {
		// Woken up as one of them: say so while the new life settles in.
		gdl = zombiesRenderCentreText(gdl, "YOU HAVE BEEN INFECTED",
				viGetViewTop() + viGetViewHeight() / 2, ZOM_TEXT_RED);
	}

#ifndef PLATFORM_N64
	gSPClearExtraGeometryModeEXT(gdl++, G_ASPECT_MODE_EXT);
#endif

	return gdl;
}

/**
 * Put the stock survivor pair in the first two weapon slots when Zombies is
 * selected. Called at the menu transition, after the player's own loadout has
 * been snapshotted, so leaving the mode hands it back. The player can then
 * change either slot in the Weapons page to suit the host's match.
 */
void zombiesSeedLoadout(void)
{
	g_MpSetup.weapons[0] = (u8) mpLoadoutRowForWeapon(WEAPON_SHOTGUN, 0);
	g_MpSetup.weapons[1] = (u8) mpLoadoutRowForWeapon(WEAPON_FALCON2, 0);
}
