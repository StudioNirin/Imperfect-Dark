# Challenge+ 11

> Source config: `MPCONFIG_CHALLENGE11` (index `0x18` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 11 is unaffected.

## Identity

- **Display name**: Challenge+ 11 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE11`
- **Arena**: `STAGE_MP_COMPLEX`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    King of the hill  against expert simulants. Weapons  the shotgun and tranquilizer.

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
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_TRANQUILIZER` |
| 3 | `MPWEAPON_SHOTGUN` |
| 4 | `MPWEAPON_K7AVENGER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Normal | Normal | Hard | Hard | BlondeSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Normal | Meat | Normal | BlondeSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Meat | Easy | BlondeSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Meat | Meat | BlondeSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Off | Off | BlondeSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Off | Off | BlondeSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Off | Off | BlondeSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Off | Off | BlondeSim:8 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
