# Challenge+ 17

> Source config: `MPCONFIG_CHALLENGE17` (index `0x1e` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 17 is unaffected.

## Identity

- **Display name**: Challenge+ 17 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE17`
- **Arena**: `STAGE_MP_TEMPLE`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    A king of the hill game. Weapons  fly-by-wire missiles.

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
| 1 | `MPWEAPON_DY357MAGNUM` |
| 2 | `MPWEAPON_AR34` |
| 3 | `MPWEAPON_REAPER` |
| 4 | `MPWEAPON_SLAYER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_CARRINGTON` | `MPBODY_CARRINGTON` | `MPTEAM_1` | Perfect | Hard | Perfect | Perfect | DanSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Easy | Normal | Normal | CISim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Meat | Easy | Easy | CISim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Meat | Off | Meat | CISim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_CISOLDIER` | `MPTEAM_1` | Off | Off | Off | Off | CISim:7 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
