# Challenge+ 18

> Source config: `MPCONFIG_CHALLENGE18` (index `0x1f` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 18 is unaffected.

## Identity

- **Display name**: Challenge+ 18 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE18`
- **Arena**: `STAGE_MP_VILLA`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    A king of the hill game against a team of expert simulants.

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
| 2 | `MPWEAPON_PHOENIX` |
| 3 | `MPWEAPON_TRANQUILIZER` |
| 4 | `MPWEAPON_LAPTOPGUN` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_CLOAKINGDEVICE` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_THEKING` | `MPTEAM_1` | Perfect | Perfect | Perfect | Normal | ElvisSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Meat | Hard | Hard | Normal | MaianSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Meat | Easy | MaianSim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Off | Easy | MaianSim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Off | Meat | MaianSim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Off | Meat | MaianSim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Off | Off | MaianSim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_ELVIS` | `MPBODY_ELVIS1` | `MPTEAM_1` | Off | Off | Off | Off | MaianSim:7 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
