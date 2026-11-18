# Challenge+ 15

> Source config: `MPCONFIG_CHALLENGE15` (index `0x1c` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 15 is unaffected.

## Identity

- **Display name**: Challenge+ 15 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE15`
- **Arena**: `STAGE_MP_GRID`
- **Scenario**: `MPSCENARIO_HOLDTHEBRIEFCASE`
- **Description**:
    Capture the briefcase in the Grid with devastators.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_HTB_HIGHLIGHTBRIEFCASE | MPOPTION_HTB_SHOWONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_DRAGON` |
| 3 | `MPWEAPON_SHOTGUN` |
| 4 | `MPWEAPON_DEVASTATOR` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_LABTECH` | `MPTEAM_1` | Normal | Hard | Hard | Hard | LabSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEMLABTECH` | `MPTEAM_1` | Meat | Normal | Normal | Normal | LabSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_DD_LABTECH` | `MPTEAM_1` | Off | Meat | Easy | Easy | LabSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_LABTECH` | `MPTEAM_1` | Off | Off | Meat | Easy | LabSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEMLABTECH` | `MPTEAM_1` | Off | Off | Off | Meat | LabSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_DD_LABTECH` | `MPTEAM_1` | Off | Off | Off | Off | LabSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_LABTECH` | `MPTEAM_1` | Off | Off | Off | Off | LabSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_FEMLABTECH` | `MPTEAM_1` | Off | Off | Off | Off | LabSim:8 |

## Goals / how it is won

- Hold the briefcase for the required time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
