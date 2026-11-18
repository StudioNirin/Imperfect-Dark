# Challenge+ 4

> Source config: `MPCONFIG_CHALLENGE04` (index `0x11` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 4 is unaffected.

## Identity

- **Display name**: Challenge+ 4 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE04`
- **Arena**: `STAGE_MP_SKEDAR`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    Take control of the hill against a team of standard simulants. Shield technology is available, as well as the K7 Avenger.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 3 | 4 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_KOH_MOBILEHILL | MPOPTION_KOH_HILLONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_CMP150` |
| 3 | `MPWEAPON_DRAGON` |
| 4 | `MPWEAPON_K7AVENGER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Easy | Easy | Easy | Easy | CISim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Meat | Easy | Easy | CISim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Meat | CISim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:8 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
