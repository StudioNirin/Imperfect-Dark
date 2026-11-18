# Challenge+ 26

> Source config: `MPCONFIG_CHALLENGE26` (index `0x27` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 26 is unaffected.

## Identity

- **Display name**: Challenge+ 26 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE26`
- **Arena**: `STAGE_MP_RUINS`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    A standard king of the hill game with no shields.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_KOH_HILLONRADAR | MPOPTION_KOH_MOBILEHILL` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_MAULER` |
| 3 | `MPWEAPON_CYCLONE` |
| 4 | `MPWEAPON_LAPTOPGUN` |
| 5 | `MPWEAPON_REAPER` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_CILABTECH` | `MPTEAM_1` | Perfect | Perfect | Perfect | Perfect | TechSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_CIFEMTECH` | `MPTEAM_1` | Hard | Perfect | Perfect | Perfect | TechSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Hard | Perfect | Perfect | CISim:1 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Easy | Perfect | Perfect | CISim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Perfect | CISim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:4 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:5 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:6 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
