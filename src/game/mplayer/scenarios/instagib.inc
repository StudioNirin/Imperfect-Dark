/**
 * Instagib
 *
 * A free-for-all or team deathmatch where every player spawns with a
 * Moonrailer (Rail-Laser) with unlimited ammunition and a faster charge
 * time - a tenth of a second instead of the gun's own one and a half. No
 * weapons are placed on the map, and every kill scores one point.
 *
 * The scenario itself stays minimal; the rest of the mode lives where
 * the rules it bends already live:
 *
 * - the Moonrailer in the hands at spawn is playerSpawnWeapons()
 *   (player.c), which asks the scenario rather than the weapon slots,
 *   since the slots are all empty here by design;
 * - the ammunition that never runs out is the per-player tick in
 *   bgunTick() (bondgun.c), not a cheat - the cheat system cannot carry
 *   this across a match start (see instaInit);
 * - the charge time is raillaserCharge60() (port/src/raillaser.c)
 *   reading g_InstagibCharge60 instead of its own constant.
 */

extern struct menudialogdef g_ExtGameOptionsMenuDialog;

/**
 * The charge time in 60ths of a second. Default is TICKS(90), the
 * Rail-Laser's own length; Instagib sets TICKS(6) at instaInit, which is
 * the only hand on it - the mode's charge is its own business, not a
 * setup row. (A setup or a host's rules that carry the scenario bits can
 * still land a saved value on it, via instaReadSave.) Declared in
 * scenarios.c, and what raillaserCharge60() answers with.
 */
extern s32 g_InstagibCharge60;

struct menuitem g_InstaOptionsMenuItems[] = {
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
		L_MPMENU_225, // "Display Team"
		MPOPTION_DISPLAYTEAM,
		menuhandlerMpDisplayTeam,
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
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_228, // "No Player Highlight"
		MPOPTION_NOPLAYERHIGHLIGHT,
		menuhandlerMpCheckboxOption,
	},
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_229, // "No Pickup Highlight"
		MPOPTION_NOPICKUPHIGHLIGHT,
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

struct menudialogdef g_InstaOptionsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	(uintptr_t) "Instagib Options\n",
	g_InstaOptionsMenuItems,
	mpOptionsMenuDialog,
	MENUDIALOGFLAG_MPLOCKABLE | MENUDIALOGFLAG_LITERAL_TEXT,
	&g_ExtGameOptionsMenuDialog,
};

/**
 * Set up the Instagib scenario: on picking the mode, on loading a setup
 * that names it, and on a client taking a host's rules for it.
 *
 * - Clears all six weapon slots, so no weapon spawn is placed anywhere.
 *   The Moonrailer comes from playerSpawnWeapons(), not from a slot.
 * - Sets the charge to its default. A setup or a host's rules that carry
 *   the slider's value overwrite this right after (scenarioReadSave).
 *
 * The ammunition deliberately does NOT go through the cheat system here.
 * Activating CHEAT_UNLIMITEDAMMO sets its bit, and then the level loads and
 * cheatsReset() (lv.c) zeroes the active banks on its way in - the cheat is
 * gone before the first frame of the match. The top-up is ticked per player
 * in bgunTick() (bondgun.c) instead, which is the right context for it
 * anyway.
 */
void instaInit(void)
{
	s32 i;

	for (i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		g_MpSetup.weapons[i] = MPWEAPON_NONE;
	}

	g_InstagibCharge60 = TICKS(6);
}

/**
 * The scenario's own bits, KOH's hill time's shape: 32 of them, in the
 * saved setup after the scenario field (mpsetupfileSaveWad) and in the
 * netplay rules message's scenario-bits word (netrules.c). The charge is
 * the only thing here, and it has to travel: a client that never picked
 * the mode would otherwise charge for a second and a half while its
 * host fires twice as fast.
 */
void instaWriteSave(struct savebuffer *buffer)
{
	savebufferOr(buffer, g_InstagibCharge60, 32);
}

void instaReadSave(struct savebuffer *buffer, u8 version)
{
	g_InstagibCharge60 = savebufferReadBits(buffer, 32);

	// A file or a room that names a charge outside the slider's clamp
	// still has to leave a gun that can charge.
	if (g_InstagibCharge60 < 5 || g_InstagibCharge60 > 240) {
		g_InstagibCharge60 = TICKS(6);
	}
}

/**
 * Score: one point per kill, the way Combat counts them. The kill is
 * already one point's worth on the way in; this is the row that says so
 * rather than any of the scenario's other modes' arithmetic.
 */
void instaCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	*score = mpchr->numpoints + mpchr->killcounts[mpchrnum];
	*deaths = mpchr->numdeaths;
}
