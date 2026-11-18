# Challenge+ 30

> Source config: `MPCONFIG_CHALLENGE30` (index `0x2b` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 30 is unaffected.

## Identity

- **Display name**: Challenge+ 30 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE30`
- **Arena**: `STAGE_MP_SKEDAR`
- **Scenario**: `MPSCENARIO_KINGOFTHEHILL`
- **Description**:
    A pistols only king of the hill game against elite opposition.

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
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_FALCON2_SCOPE` |
| 3 | `MPWEAPON_MAGSEC4` |
| 4 | `MPWEAPON_MAULER` |
| 5 | `MPWEAPON_DY357MAGNUM` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Dark | Dark | Dark | Dark | BlondeSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_CASSANDRA` | `MPBODY_CASSANDRA` | `MPTEAM_1` | Dark | Dark | Dark | Dark | CassSim |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_TRENT` | `MPBODY_TRENT` | `MPTEAM_1` | Off | Dark | Dark | Dark | TrentSim |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Dark | Dark | BlondeSim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_MRBLONDE` | `MPBODY_MRBLONDE` | `MPTEAM_1` | Off | Off | Off | Dark | BlondeSim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_0` | Off | Off | Off | Off | JoSim:3 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_0` | Off | Off | Off | Off | JoSim:2 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DARK_COMBAT` | `MPBODY_DARK_COMBAT` | `MPTEAM_0` | Normal | Normal | Normal | Normal | JoSim:1 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
