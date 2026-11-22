/**
 * Plasma Tag
 *
 * A fixed-weapon deathmatch like Instagib, but with the Bowcaster:
 *  - Bowcasters only (no weapon spawns on the map)
 *  - infinite ammunition, ticked per player like Instagib's Moonrailer
 *  - everyone starts at half health
 *  - Fast Movement is offered as an option in the mode's Options page
 *
 * The weapon comes from playerSpawnWeapons(), the ammunition from bgunTick(),
 * and the starting health from playerStartNewLife() - all keyed on this
 * scenario id, the same way Instagib's rules are.
 */

extern struct menudialogdef g_ExtGameOptionsMenuDialog;

struct menuitem g_PlasmaTagOptionsMenuItems[] = {
	{
		MENUITEMTYPE_CHECKBOX,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		L_MPMENU_224, // "Fast Movement"
		MPOPTION_FASTMOVEMENT,
		menuhandlerMpCheckboxOption,
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

struct menudialogdef g_PlasmaTagOptionsMenuDialog = {
	MENUDIALOGTYPE_DEFAULT,
	(uintptr_t) "Plasma Tag Options\n",
	g_PlasmaTagOptionsMenuItems,
	mpOptionsMenuDialog,
	MENUDIALOGFLAG_MPLOCKABLE | MENUDIALOGFLAG_LITERAL_TEXT,
	&g_ExtGameOptionsMenuDialog,
};

/**
 * Clear the weapon slots so no map pickup spawns - the Bowcaster is handed
 * out at spawn by playerSpawnWeapons(), like Instagib's Moonrailer. Fast
 * Movement comes on by default, as the mode's own page calls it a laser-tag
 * speed (and the checkbox stays a checkbox: the owner can turn it off).
 */
void plasmaTagInit(void)
{
	s32 i;

	for (i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		g_MpSetup.weapons[i] = MPWEAPON_NONE;
	}

	g_MpSetup.options |= MPOPTION_FASTMOVEMENT;
}

/**
 * Score: one point per kill, the way Instagib and Combat count them.
 */
void plasmaTagCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	*score = mpchr->numpoints + mpchr->killcounts[mpchrnum];
	*deaths = mpchr->numdeaths;
}
