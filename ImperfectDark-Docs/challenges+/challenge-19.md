# Challenge+ 19

> Source config: `MPCONFIG_CHALLENGE19` (index `0x20` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 19 is unaffected.

## Identity

- **Display name**: Challenge+ 19 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE19`
- **Arena**: `STAGE_MP_G5BUILDING`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    Standard combat against a squad of expert simulants. Weapons  the rocket launcher and the FarSight.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Start armed | none | - |
| Score limit (1p/2p/3p/4p) | `9, 19, 24, 29` | shown = value + 1 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_FASTMOVEMENT` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Bonus requirement

| Field | Value |
|-------|-------|
| bonus_requirement | `NONE` |
| silver (1p/2p/3p/4p) | `0, 0, 0, 0` |
| gold (1p/2p/3p/4p) | `0, 0, 0, 0` |
| perfect (1p/2p/3p/4p) | `0, 0, 0, 0` |

Valid `bonus_requirement` tags (all 17, no others):
`NONE`, `HEADSHOTS`, `ACCURACY`, `KILLS`, `DEATHS`, `KD_RATIO`, `FASTEST_2KILLS`,
`LONGEST_LIFE`, `MAX_SIMUL_KILLS`, `MULTIKILLS`, `LONGEST_SPREE`, `TOKEN_HELD_TIME`,
`BACKSHOTS`, `DISTANCE`, `DAM_RECEIVED`, `ITEM_USE`, `FINISH_TIME`.

Each tag maps to a `CHALBONUS_*` enum in `src/include/game/challenge.h`. See
`setup-guide.md` for each tag's team stat, whether a higher or lower
value wins, and its value format. `NONE` means bronze only. Values are integers;
`ACCURACY` is a percent 0-100; times and durations are seconds. Silver and gold
are the thresholds for the chosen requirement, one per player-count star (1p/2p/3p/4p); a single value is copied to all four. Bronze is always a win and needs no value; perfect is the hardest tier and earns a purple star. The selection screen's 1p/2p/3p/4p+ tabs show each tier's values in the description panel, which otherwise stays the same. `LONGEST_LIFE` and `ITEM_USE`
read playerstat fields that this build never fills, so avoid them for now.

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_CMP150` |
| 2 | `MPWEAPON_SHOTGUN` |
| 3 | `MPWEAPON_ROCKETLAUNCHER` |
| 4 | `MPWEAPON_FARSIGHT` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_COMBATBOOST` |

## Simulants

Difficulty per human-player count, one comma-separated cell per sim (1p/2p/3p/4p). **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims. Up to 16 sims may be listed; the row number is the sim slot.

| # | Type | Head | Body | Team | Difficulties (1p/2p/3p/4p) | Bot name |
|---|------|------|------|------|--------------------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_CASSANDRA` | `MPBODY_CASSANDRA` | `MPTEAM_1` | `Hard, Perfect, Perfect, Perfect` | CassSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_TRENT` | `MPBODY_TRENT` | `MPTEAM_1` | `Easy, Perfect, Hard, Hard` | TrentSim |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Normal, Normal` | SnipeSim:1 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Off, Easy` | SnipeSim:2 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Off, Off` | SnipeSim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Off, Off` | SnipeSim:4 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Off, Off` | SnipeSim:5 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_DDSNIPER` | `MPBODY_DDSNIPER` | `MPTEAM_1` | `Off, Off, Off, Off` | SnipeSim:6 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep 8 to 16 simulant rows, each with exactly 4 difficulties in the one cell.
- The `bonus_requirement`, `silver`, `gold` and `perfect` fields drive the Challenge+ medal stars. After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
