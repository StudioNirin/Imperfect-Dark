# Challenge+ 23

> Source config: `MPCONFIG_CHALLENGE23` (index `0x24` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 23 is unaffected.

## Identity

- **Display name**: Challenge+ 23 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE23`
- **Arena**: `STAGE_MP_COMPLEX`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    Combat in slow motion. Weapons  the RCP120.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 1 | 2 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_SLOWMOTION_ON` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_GRENADE` |
| 3 | `MPWEAPON_LAPTOPGUN` |
| 4 | `MPWEAPON_RCP120` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_COMBATBOOST` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_PRESIDENT` | `MPBODY_PRESIDENT_CLONE2` | `MPTEAM_1` | Hard | Perfect | Perfect | Perfect | CloneSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_STRIPES` | `MPTEAM_1` | Hard | Perfect | Perfect | Perfect | StripeSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Hard | Normal | StripeSim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Off | Normal | StripeSim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Off | Off | StripeSim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Off | Off | StripeSim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Off | Off | StripeSim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_STRIPES` | `MPTEAM_1` | Off | Off | Off | Off | StripeSim:7 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
