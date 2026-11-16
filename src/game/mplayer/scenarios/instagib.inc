/**
 * Instagib
 *
 * A free-for-all or team deathmatch where every player spawns with a
 * Moonrailer (Rail-Laser) with unlimited ammo and a faster charge time
 * (0.5s instead of 1.5s). No weapons are placed on the map. Each kill
 * scores one point.
 *
 * The scenario is intentionally minimal: it reuses Combat's scoring and
 * team logic, and only overrides the weapon loadout, ammo, and charge
 * rate.
 */

#include "game/cheats.h"

extern struct menudialogdef g_ExtGameOptionsMenuDialog;

/**
 * The charge time in 60ths of a second. Default is TICKS(90) (1.5s),
 * which is the Rail-Laser's normal charge. Instagib sets this to
 * TICKS(30) (0.5s). Declared in scenarios.c and checked by
 * raillaserCharge60() in port/src/raillaser.c.
 */
extern s32 g_InstagibCharge60;

MenuItemHandlerResult menuhandlerInstagibCharge(s32 operation, struct menuitem *item, union handlerdata *data)
{
	switch (operation) {
	case MENUOP_GETSLIDER:
		data->slider.value = g_InstagibCharge60;
		break;
	case MENUOP_SET:
		g_InstagibCharge60 = (u8)data->slider.value;
		break;
	case MENUOP_GETSLIDERLABEL:
		sprintf(data->slider.label, langGet(L_MPWEAPONS_023), data->slider.value / 60); // "%ds/Point"
		break;
	}

	return 0;
}

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
		MENUITEMTYPE_SLIDER,
		0,
		MENUITEMFLAG_LOCKABLEMINOR,
		(uintptr_t) "Charge Time\n",
		0,
		menuhandlerInstagibCharge,
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
 * Set up the Instagib scenario. Called once at the start of a match.
 *
 * - Forces the Moonrailer as the starting weapon (g_DefaultWeapons[0]).
 * - Disables all map weapon spawns (g_MpSetup.weapons[] = MPWEAPON_NONE).
 * - Activates CHEAT_UNLIMITEDAMMO so the Rail-Laser never runs dry.
 * - Sets g_InstagibCharge60 to TICKS(30) (0.5s) for the faster charge.
 */
void instaInit(void)
{
	s32 i;

	// No weapons on the map: clear all six weapon slots.
	for (i = 0; i < NUM_MPWEAPONSLOTS; i++) {
		g_MpSetup.weapons[i] = MPWEAPON_NONE;
	}

	// Equip every player with a Moonrailer at spawn.
	g_DefaultWeapons[HAND_RIGHT] = WEAPON_RAILLASER;
	g_DefaultWeapons[HAND_LEFT] = WEAPON_NONE;

	// Unlimited ammo.
	cheatActivate(CHEAT_UNLIMITEDAMMO);

	// Faster charge.
	g_InstagibCharge60 = TICKS(30);
}

/**
 * Called each tick while the Instagib scenario is active. Currently a no-op:
 * the charge time is enforced by raillaserCharge60() reading
 * g_InstagibCharge60, and the unlimited ammo is handled by the cheat system.
 */
void instaTick(void)
{
}

/**
 * Score calculation: one point per kill. Reuses the standard Combat scoring
 * (kill count minus deaths).
 */
void instaCalculatePlayerScore(struct mpchrconfig *mpchr, s32 mpchrnum, s32 *score, s32 *deaths)
{
	*score = mpchr->numpoints + mpchr->killcounts[mpchrnum];
	*deaths = mpchr->numdeaths;
}