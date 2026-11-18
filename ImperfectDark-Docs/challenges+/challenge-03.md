# Challenge+ 3

> Source config: `MPCONFIG_CHALLENGE03` (index `0x10` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 3 is unaffected.

## Identity

- **Display name**: Challenge+ 3 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE03`
- **Arena**: `STAGE_MP_PIPES`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    A challenge against basic simulants which includes assault rifles and timed mines.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 4 | 5 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 7 | 8 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_CMP150` |
| 3 | `MPWEAPON_TIMEDMINE` |
| 4 | `MPWEAPON_DRAGON` |
| 5 | `MPWEAPON_AR34` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_CILABTECH` | `MPTEAM_1` | Meat | Meat | Meat | Meat | TechSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_CIFEMTECH` | `MPTEAM_1` | Meat | Meat | Meat | Meat | TechSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_CILABTECH` | `MPTEAM_1` | Off | Off | Meat | Meat | TechSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_CIFEMTECH` | `MPTEAM_1` | Off | Off | Off | Meat | TechSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_CILABTECH` | `MPTEAM_1` | Off | Off | Off | Off | TechSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_CIFEMTECH` | `MPTEAM_1` | Off | Off | Off | Off | TechSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_CILABTECH` | `MPTEAM_1` | Off | Off | Off | Off | TechSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_CIFEMTECH` | `MPTEAM_1` | Off | Off | Off | Off | TechSim:8 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
