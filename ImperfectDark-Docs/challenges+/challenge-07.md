# Challenge+ 7

> Source config: `MPCONFIG_CHALLENGE07` (index `0x14` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 7 is unaffected.

## Identity

- **Display name**: Challenge+ 7 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE07`
- **Arena**: `STAGE_MP_WAREHOUSE`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    Take control of the hill against a team of advanced simulants in the Warehouse. All weapons kill with only one hit.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 4 | 5 (400 = none) |
| Options | `MPOPTION_ONEHITKILLS | MPOPTION_TEAMSENABLED | MPOPTION_KOH_HILLONRADAR | MPOPTION_KOH_MOBILEHILL` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2_SILENCER` |
| 2 | `MPWEAPON_MAGSEC4` |
| 3 | `MPWEAPON_CYCLONE` |
| 4 | `MPWEAPON_GRENADE` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Easy | Easy | Easy | Easy | JoSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Meat | Easy | Easy | JoSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:8 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
