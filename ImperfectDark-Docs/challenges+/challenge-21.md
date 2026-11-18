# Challenge+ 21

> Source config: `MPCONFIG_CHALLENGE21` (index `0x22` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 21 is unaffected.

## Identity

- **Display name**: Challenge+ 21 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE21`
- **Arena**: `STAGE_MP_GRID`
- **Scenario**: `MPSCENARIO_HACKERCENTRAL`
- **Description**:
    Touch the boxes to claim them for your side. The Cloaking Device is available.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_HTM_HIGHLIGHTTERMINAL | MPOPTION_HTM_SHOWONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAULER` |
| 2 | `MPWEAPON_REAPER` |
| 3 | `MPWEAPON_SHOTGUN` |
| 4 | `MPWEAPON_CALLISTO` |
| 5 | `MPWEAPON_CLOAKINGDEVICE` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARKSNOW` | `MPTEAM_1` | Hard | Hard | Hard | Hard | JoSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Normal | Normal | Normal | SnowSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Easy | Normal | SnowSim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Off | Easy | SnowSim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | SnowSim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | SnowSim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | SnowSim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_ALASKAN_GUARD` | `MPTEAM_1` | Off | Off | Off | Off | SnowSim:7 |

## Goals / how it is won

- Hack the terminals while defending your own.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
