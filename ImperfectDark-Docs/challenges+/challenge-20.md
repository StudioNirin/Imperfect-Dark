# Challenge+ 20

> Source config: `MPCONFIG_CHALLENGE20` (index `0x21` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 20 is unaffected.

## Identity

- **Display name**: Challenge+ 20 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE20`
- **Arena**: `STAGE_MP_SEWERS`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    One-hit kill combat with a twist - protect the weaker simulants on your team. Pistols and shields.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_ONEHITKILLS | MPOPTION_TEAMSENABLED` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAULER` |
| 2 | `MPWEAPON_FALCON2` |
| 3 | `MPWEAPON_MAGSEC4` |
| 4 | `MPWEAPON_DY357MAGNUM` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_PILOTAF1` | `MPTEAM_1` | Hard | Perfect | Perfect | Perfect | PilotSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_PILOTAF1` | `MPTEAM_1` | Off | Hard | Hard | Perfect | PilotSim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_STEWARDESS` | `MPTEAM_0` | Off | Off | Off | Off | AirSim:6 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_STEWARD` | `MPTEAM_0` | Off | Off | Off | Off | AirSim:5 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_STEWARD` | `MPTEAM_0` | Off | Off | Off | Off | AirSim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_STEWARDESS` | `MPTEAM_0` | Off | Off | Off | Normal | AirSim:3 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_STEWARD` | `MPTEAM_0` | Off | Normal | Normal | Normal | AirSim:2 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_STEWARDESS` | `MPTEAM_0` | Normal | Normal | Normal | Normal | AirSim:1 |
| 9 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_STEWARD` | `MPTEAM_0` | Off | Off | Off | Easy |  |
| 10 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_STEWARDESS` | `MPTEAM_0` | Off | Off | Easy | Easy |  |
| 11 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_STEWARD` | `MPTEAM_0` | Off | Normal | Easy | Easy |  |
| 12 | `BOTTYPE_GENERAL` | `MPHEAD_VD` | `MPBODY_STEWARDESS` | `MPTEAM_0` | Normal | Normal | Easy | Easy |  |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
