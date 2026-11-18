# Challenge+ 24

> Source config: `MPCONFIG_CHALLENGE24` (index `0x25` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 24 is unaffected.

## Identity

- **Display name**: Challenge+ 24 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE24`
- **Arena**: `STAGE_MP_FORTRESS`
- **Scenario**: `MPSCENARIO_CAPTURETHECASE`
- **Description**:
    Capture the flag game against a team of expert simulants. Weapons  the Gold DY357 Magnum and the tranquilizer. No shields.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 60 | 61 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 8 | 9 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_CTC_SHOWONRADAR` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_CMP150` |
| 2 | `MPWEAPON_TRANQUILIZER` |
| 3 | `MPWEAPON_DEVASTATOR` |
| 4 | `MPWEAPON_SUPERDRAGON` |
| 5 | `MPWEAPON_DY357LX` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | Easy | Normal | Hard | Hard | GuardSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_JON` | `MPBODY_A51TROOPER` | `MPTEAM_1` | Easy | Normal | Normal | Hard | TroopSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_BEAU1` | `MPBODY_A51AIRMAN` | `MPTEAM_1` | Easy | Easy | Normal | Normal | AirSim:1 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DARLING` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | Easy | Easy | Easy | Normal | GuardSim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_A51TROOPER` | `MPTEAM_1` | Off | Off | Easy | Easy | TroopSim:2 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_A51AIRMAN` | `MPTEAM_1` | Off | Off | Perfect | Easy | AirSim:2 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:3 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | Off | Off | Off | Off | GuardSim:4 |

## Goals / how it is won

- Capture the case and carry it to your spawn point.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
