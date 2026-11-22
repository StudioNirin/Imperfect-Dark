/**
 * One Shall Stand...
 *
 * A free-for-all where every player starts with a fixed number of lives
 * (one to nine, three by default). Each death costs one life; a player whose
 * lives are spent stays out of the match and watches the survivors through
 * the spectator cameras (netspec.c online, the LMS follow camera offline).
 * The last player with a life left wins. If the time limit runs out first,
 * the rankings put the player with the most lives left on top, with kills
 * breaking a tie.
 *
 * The mode borrows its elimination rule from GoldenEye Plus's "You Only Live
 * Twice" (gexPlusLivesSpent/gexPlusMatchOver in port/src/gexplus.c), but the
 * death limit is the scenario's own 32-bit save word instead of a hardware
 * constant, so it travels to clients in RULES' scenariobits without a new
 * wire field. Lives themselves are derived from mpchrconfig.numdeaths, which
 * mpstatsRecordDeath() already keeps up to date for players and simulants.
 *
 * Score is deliberately remaining lives, not kills: mpGetPlayerRankings()
 * sorts on the score returned by lmsCalculatePlayerScore(), so the survivor
 * becomes placement 0 on both the local end screen and the net MATCH_END
 * without either path needing a new field.
 */

extern struct menudialogdef g_ExtGameOptionsMenuDialog;

extern s32 g_LmsStartingLives;

#ifndef PLATFORM_N64
// A client's sim runs none of the scenario rules; these are filled from the
// host's NETSCEN block. The host publishes zero lives for an eliminated chr,
// so a separate flag - not a nonzero byte - says whether the block has arrived.
u8 g_LmsNetLives[MAX_MPCHRS];
u8 g_LmsNetOut[MAX_MPCHRS];
bool g_LmsNetBlockSeen = false;
#endif

/**
 * A seat nobody plays (an open seat on a host): out of play, and not a
 * player for the round's sake - no lives, never alive, not a winner. The
 * same seam Pop a Cap and the co-op modes use. The host is the only one
 * where the seat state is known; a client has no local seats and reads the
 * published bytes.
 */
static bool lmsOutOfPlay(s32 mpchrnum)
{
#ifndef PLATFORM_N64
	if (mpchrnum >= 0 && mpchrnum < PLAYERCOUNT() && netPlayerOutOfPlay(mpchrnum)) {
		return true;
	}
#endif
	return false;
}

/**
 * The standard option rows, with the life slider above them. The rows are
 * the same ones Combat and Instagib offer so switching modes does not move
 * the familiar checkboxes around.
 */
MenuItemHandlerResult menuhandlerMpLmsLives(s32 operation, struct menuitem *item, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_GETSLIDER:
		data->slider.value = g_LmsStartingLives - 1;
		break;
	case MENUOP_SET:
		g_LmsStartingLives = data->slider.value + 1;
		break;
	case MENUOP_GETSLIDERLABEL:
		sprintf(data->slider.label, "%d", data->slider.value + 1);
		break;
	}

	return 0;
}

struct menuitem g_LmsOptionsMenuItems[] = {
	{
		MENUITEMTYPE_SLIDER,
		0,
		MENUITEMFLAG_LOCKABLEMINOR | MENUITEMFLAG_LITERAL_TEXT,
		(uintptr_t) "Lives\n",
		8,
		menuhandlerMpLmsLives,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_222, // "One-Hit Kills"
		MPOPTION_ONEHITKILLS,
		menuhandlerMpOneHitKills,
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

struct menudialogdef g_LmsOptionsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	(uintptr_t) "One Shall Stand... Options\n",
	g_LmsOptionsMenuItems,
	mpOptionsMenuDialog,
	MENUDIALOGFLAG_MPLOCKABLE | MENUDIALOGFLAG_LITERAL_TEXT,
	&g_ExtGameOptionsMenuDialog,
};

/**
 * Set up One Shall Stand... when the mode is picked, a setup naming it loads,
 * or a client takes the host's rules. Teams are forced off: the winner is a
 * single player, so a team rule would contradict the mode. Score limits are
 * ignored for this mode in lv.c, because its score column is remaining lives.
 */
void lmsInit(void)
{
	g_MpSetup.options &= ~MPOPTION_TEAMSENABLED;
	g_LmsNetBlockSeen = false;
}

/**
 * The whole 32-bit scenario word is the life count. Instagib uses all 32 bits
 * for its charge time; a single u8 is plenty here, but reading the full word
 * keeps the read and write symmetric with every other scenario.
 */
void lmsWriteSave(struct savebuffer *buffer)
{
	savebufferOr(buffer, g_LmsStartingLives, 32);
}

void lmsReadSave(struct savebuffer *buffer, u8 version)
{
	g_LmsStartingLives = savebufferReadBits(buffer, 32);

	// A hand-edited setup or a host's rules can carry anything. A mode with
	// no lives is not playable, and nine is the slider's top.
	if (g_LmsStartingLives < 1 || g_LmsStartingLives > 9) {
		g_LmsStartingLives = 3;
	}
}

/**
 * How many lives this mpchr still has, as a nonnegative number.
 */
static s32 lmsLivesLeft(s32 mpchrnum)
{
	s32 left = g_LmsStartingLives - MPCHR(mpchrnum)->numdeaths;

#ifndef PLATFORM_N64
	// The net arrays are written by chr index, not by net slot. The score
	// callback can be called with either (the net scenario log uses slots),
	// so only a chr index may read the array; anything else falls back to
	// the local death count.
	if (NET_CLIENT && mpchrnum >= 0 && mpchrnum < g_MpNumChrs && g_LmsNetBlockSeen) {
		left = g_LmsNetLives[mpchrnum];
	}
#endif
	if (lmsOutOfPlay(mpchrnum)) {
		return 0;
	}

	return left > 0 ? left : 0;
}

/**
 * Find the mpchr slot a chr belongs to. Humans occupy the low slots and
 * simulants the rest, so the slot index is also what killcounts and the net
 * scenario block are keyed by.
 */
static s32 lmsChrIndex(struct chrdata *chr)
{
	s32 i;

	for (i = 0; i < g_MpNumChrs; i++) {
		if (g_MpAllChrPtrs[i] == chr) {
			return i;
		}
	}

	return -1;
}

/**
 * True once this chr has used up all of its lives. player.c, chraction.c and
 * modbodies.c ask this in place of gexPlusLivesSpent() when LMS is selected,
 * so an eliminated player never respawns and an eliminated simulant stays
 * down. The chr is looked up by pointer, the same way the GE Plus rule does.
 */
bool lmsLivesSpent(struct chrdata *chr)
{
	s32 index;

	if (!chr || g_MpSetup.scenario != MPSCENARIO_LASTMANSTANDING) {
		return false;
	}

	index = lmsChrIndex(chr);
	if (index < 0) {
		return false;
	}

	if (lmsOutOfPlay(index)) {
		return true;
	}

#ifndef PLATFORM_N64
	// A net client has no local numdeaths during the match; the host publishes
	// each chr's elimination in its scenario block.
	if (NET_CLIENT) {
		return g_LmsNetOut[index] != 0;
	}
#endif
	return g_MpAllChrConfigPtrs[index]->numdeaths >= g_LmsStartingLives;
}

/**
 * True when one or fewer chrs still have a life left. lv.c adds a reason to
 * end the match on this, just as it does for You Only Live Twice.
 */
s32 lmsMatchOver(void)
{
	s32 alive = 0;
	s32 i;

	if (g_MpSetup.scenario != MPSCENARIO_LASTMANSTANDING || g_MpNumChrs < 2) {
		return 0;
	}

	for (i = 0; i < g_MpNumChrs; i++) {
		if (lmsOutOfPlay(i)) {
			continue;
		}

		if (g_MpAllChrConfigPtrs[i]->numdeaths < g_LmsStartingLives) {
			alive++;
		}
	}

	return alive <= 1;
}

/**
 * Score is remaining lives. mpGetPlayerRankings() breaks a tie on the score
 * with kills for LMS (see mplayer.c), so a player with the same number of
 * lives but more kills places higher when the timer runs out.
 */
void lmsCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	s32 left = g_LmsStartingLives - mpchr->numdeaths;

#ifndef PLATFORM_N64
	// The net arrays are written by chr index, not by net slot. The score
	// callback can be called with either (the net scenario log uses slots),
	// so only a chr index may read the array; anything else falls back to
	// the local death count.
	if (NET_CLIENT && mpchrnum >= 0 && mpchrnum < g_MpNumChrs && g_LmsNetBlockSeen) {
		left = g_LmsNetLives[mpchrnum];
	}
#endif

	if (lmsOutOfPlay(mpchrnum)) {
		left = 0;
	}

	if (left < 0) {
		left = 0;
	}

	*score = left;
	*deaths = mpchr->numdeaths;
}

/**
 * The local player's remaining lives, in the same numeric font and position
 * as King of the Hill's hill timer. The spectator camera's own line along the
 * foot of the view says who is being watched; this one is about the match.
 */
Gfx *lmsRenderHud(Gfx *gdl)
{
	s32 textwidth;
	s32 textheight;
	s32 x;
	s32 y;
	s32 lives;
	s32 index;
	char text[32];
	struct player *player = g_Vars.currentplayer;

	if (!player || !player->prop || !player->prop->chr) {
		return gdl;
	}

	index = lmsChrIndex(player->prop->chr);
	lives = index >= 0 ? lmsLivesLeft(index) : 0;

	if (lives == 0) {
		sprintf(text, "OUT");
	} else {
		sprintf(text, "%d %s", lives, lives == 1 ? "LIFE" : "LIVES");
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
	gdl = textRender(gdl, &x, &y, text, g_CharsNumeric, g_FontNumeric, g_ModColours[MODCOLOUR_KOHHUD], 0xa0, viGetWidth(), viGetHeight(), 0, 0);
#else
	gdl = text0f153858(gdl, &x, &y, &textwidth, &textheight);
	gdl = textRender(gdl, &x, &y, text, g_CharsNumeric, g_FontNumeric, g_ModColours[MODCOLOUR_KOHHUD], 0x88, viGetWidth(), viGetHeight(), 0, 0);
#endif
	gdl = text0f153780(gdl);

#ifndef PLATFORM_N64
	gSPClearExtraGeometryModeEXT(gdl++, G_ASPECT_MODE_EXT);
#endif

	return gdl;
}

/**
 * Announce eliminations in the feed. mpstatsRecordDeath() calls this for
 * every death, human or simulant, so the message is built from the mpchr that
 * just lost its last life. Kept here rather than as a scenario death callback
 * because the callback table has no death hook; Pop a Cap uses the same
 * hard-coded dispatch from mpstats.c.
 */
void lmsHandleDeath(s32 attackerindex, s32 victimindex)
{
	struct mpchrconfig *victim;
	char text[80];

	if (g_MpSetup.scenario != MPSCENARIO_LASTMANSTANDING || victimindex < 0 || victimindex >= g_MpNumChrs) {
		return;
	}

	victim = g_MpAllChrConfigPtrs[victimindex];

	if (victim->numdeaths < g_LmsStartingLives) {
		s32 left = g_LmsStartingLives - victim->numdeaths;
		sprintf(text, "%s has %d %s left\n", victim->name, left, left == 1 ? "life" : "lives");
		hudmsgCreateWithFlags(text, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_NOWRAP);
		return;
	}

	sprintf(text, "%s is out of lives\n", victim->name);
	hudmsgCreateWithFlags(text, HUDMSGTYPE_MPSCENARIO, HUDMSGFLAG_NOWRAP);
}
