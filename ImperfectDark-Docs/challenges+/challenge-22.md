# Challenge+ 22

> Source config: `MPCONFIG_CHALLENGE22` (index `0x23` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 22 is unaffected.

## Identity

- **Display name**: Challenge+ 22 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE22`
- **Arena**: `STAGE_MP_BASE`
- **Scenario**: `MPSCENARIO_HOLDTHEBRIEFCASE`
- **Description**:
    A hold the briefcase game with one-hit kills. Weapons  the crossbow and the sniper rifle.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_ONEHITKILLS | MPOPTION_TEAMSENABLED | MPOPTION_HTB_HIGHLIGHTBRIEFCASE | MPOPTION_HTB_SHOWONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_SNIPERRIFLE` |
| 3 | `MPWEAPON_CROSSBOW` |
| 4 | `MPWEAPON_K7AVENGER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_PRESIDENT` | `MPBODY_PRESIDENT` | `MPTEAM_1` | Perfect | Perfect | Perfect | Perfect | PresSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Normal | Normal | Hard | Normal | GuardSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Easy | Normal | Normal | GuardSim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Off | Off | Easy | GuardSim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_PRES_SECURITY` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:7 |

## Goals / how it is won

- Hold the briefcase for the required time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
