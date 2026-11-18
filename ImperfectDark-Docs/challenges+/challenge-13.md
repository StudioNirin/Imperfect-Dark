# Challenge+ 13

> Source config: `MPCONFIG_CHALLENGE13` (index `0x1a` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 13 is unaffected.

## Identity

- **Display name**: Challenge+ 13 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE13`
- **Arena**: `STAGE_MP_G5BUILDING`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    One-hit kills, touch that box, G5, with tranquilizers.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_ONEHITKILLS | MPOPTION_TEAMSENABLED | MPOPTION_00010000` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2_SILENCER` |
| 2 | `MPWEAPON_TRANQUILIZER` |
| 3 | `MPWEAPON_LAPTOPGUN` |
| 4 | `MPWEAPON_GRENADE` |
| 5 | `MPWEAPON_REAPER` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_CIAGUY` | `MPTEAM_1` | Normal | Hard | Hard | Hard | CIASim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Easy | Easy | Easy | FBISim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Easy | Easy | CIASim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Easy | FBISim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Off | Off | CIASim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Off | FBISim:3 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_CIAGUY` | `MPTEAM_1` | Off | Off | Off | Off | CIASim:4 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_FBIGUY` | `MPTEAM_1` | Off | Off | Off | Off | FBISim:4 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
