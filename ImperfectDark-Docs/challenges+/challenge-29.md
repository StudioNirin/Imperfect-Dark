# Challenge+ 29

> Source config: `MPCONFIG_CHALLENGE29` (index `0x2a` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 29 is unaffected.

## Identity

- **Display name**: Challenge+ 29 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE29`
- **Arena**: `STAGE_MP_G5BUILDING`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    Standard combat with no shields available. Weapons  the DY357 and the Dragon.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_CYCLONE` |
| 3 | `MPWEAPON_DY357MAGNUM` |
| 4 | `MPWEAPON_CMP150` |
| 5 | `MPWEAPON_DRAGON` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_OFFICEWORKER` | `MPTEAM_1` | Dark | Dark | Dark | Dark | WorkSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_OFFICEWORKER2` | `MPTEAM_1` | Dark | Dark | Dark | Dark | WorkSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Dark | Dark | Dark | GuardSim:1 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Off | Dark | Dark | GuardSim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Off | Off | Dark | GuardSim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:4 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:5 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEM_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:6 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
