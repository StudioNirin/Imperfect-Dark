# Challenge+ 12

> Source config: `MPCONFIG_CHALLENGE12` (index `0x19` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 12 is unaffected.

## Identity

- **Display name**: Challenge+ 12 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE12`
- **Arena**: `STAGE_MP_SKEDAR`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    Slow-motion combat in the Skedar arena.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 2 | 3 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_SLOWMOTION_ON` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2_SCOPE` |
| 2 | `MPWEAPON_SNIPERRIFLE` |
| 3 | `MPWEAPON_SHOTGUN` |
| 4 | `MPWEAPON_SUPERDRAGON` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Hard | Hard | Hard | Hard | JoSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Normal | Normal | Normal | JoSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Easy | Easy | JoSim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Off | Easy | JoSim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_TRENCH` | `MPTEAM_1` | Off | Off | Off | Off | JoSim:8 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
