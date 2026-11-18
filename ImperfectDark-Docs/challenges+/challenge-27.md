# Challenge+ 27

> Source config: `MPCONFIG_CHALLENGE27` (index `0x28` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 27 is unaffected.

## Identity

- **Display name**: Challenge+ 27 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE27`
- **Arena**: `STAGE_MP_SEWERS`
- **Scenario**: `MPSCENARIO_HACKERCENTRAL`
- **Description**:
    A touch the box game featuring rocket launchers and expert opponents.

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
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_MAGSEC4` |
| 3 | `MPWEAPON_CMP150` |
| 4 | `MPWEAPON_ROCKETLAUNCHER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_CIAGUY` | `MPTEAM_1` | Perfect | Perfect | Perfect | Perfect | CIASim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Perfect | Perfect | Perfect | FBISim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Perfect | Perfect | CIASim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Perfect | FBISim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Off | Off | CIASim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Off | FBISim:3 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Off | Off | CIASim:4 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Off | FBISim:4 |

## Goals / how it is won

- Hack the terminals while defending your own.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
