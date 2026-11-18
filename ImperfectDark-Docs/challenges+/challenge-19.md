# Challenge+ 19

> Source config: `MPCONFIG_CHALLENGE19` (index `0x20` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 19 is unaffected.

## Identity

- **Display name**: Challenge+ 19 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE19`
- **Arena**: `STAGE_MP_G5BUILDING`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    Standard combat against a squad of expert simulants. Weapons  the rocket launcher and the FarSight.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_FASTMOVEMENT` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_CMP150` |
| 2 | `MPWEAPON_SHOTGUN` |
| 3 | `MPWEAPON_ROCKETLAUNCHER` |
| 4 | `MPWEAPON_FARSIGHT` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_COMBATBOOST` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_CASSANDRA` | `MPBODY_CASSANDRA` | `MPTEAM_1` | Hard | Perfect | Perfect | Perfect | CassSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_TRENT` | `MPBODY_TRENT` | `MPTEAM_1` | Easy | Perfect | Hard | Hard | TrentSim |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Normal | Normal | SnipeSim:1 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Off | Easy | SnipeSim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Off | Off | SnipeSim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Off | Off | SnipeSim:4 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Off | Off | SnipeSim:5 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | Off | Off | Off | Off | SnipeSim:6 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
