# Challenge+ 6

> Source config: `MPCONFIG_CHALLENGE06` (index `0x13` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 6 is unaffected.

## Identity

- **Display name**: Challenge+ 6 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE06`
- **Arena**: `STAGE_MP_AREA52`
- **Scenario**: `MPSCENARIO_HOLDTHEBRIEFCASE`
- **Description**:
    Hold the briefcase for as long as possible against a large team of basic simulants.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 5 | 6 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_HTB_HIGHLIGHTBRIEFCASE | MPOPTION_HTB_SHOWONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_CMP150` |
| 2 | `MPWEAPON_DY357MAGNUM` |
| 3 | `MPWEAPON_SHOTGUN` |
| 4 | `MPWEAPON_K7AVENGER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_OFFICEWORKER` | `MPTEAM_1` | Meat | Meat | Easy | Easy | WorkSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_OFFICEWORKER2` | `MPTEAM_1` | Meat | Meat | Meat | Easy | WorkSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_OFFICEWORKER` | `MPTEAM_1` | Meat | Meat | Meat | Meat | WorkSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_OFFICEWORKER2` | `MPTEAM_1` | Meat | Meat | Meat | Meat | WorkSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_OFFICEWORKER` | `MPTEAM_1` | Off | Off | Off | Off | WorkSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_OFFICEWORKER2` | `MPTEAM_1` | Off | Off | Off | Off | WorkSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_OFFICEWORKER` | `MPTEAM_1` | Off | Off | Off | Off | WorkSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_OFFICEWORKER2` | `MPTEAM_1` | Off | Off | Off | Off | WorkSim:8 |

## Goals / how it is won

- Hold the briefcase for the required time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
