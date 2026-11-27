# Challenge+ 14

> Source config: `MPCONFIG_CHALLENGE14` (index `0x1b` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 14 is unaffected.

## Identity

- **Display name**: Challenge+ 14 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE14`
- **Arena**: `STAGE_MP_AREA52`
- **Scenario**: `MPSCENARIO_CAPTURETHECASE`
- **Description**:
    Capture the flag in Area 52 with cloaking.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Start armed | none | - |
| Score limit (1p/2p/3p/4p) | `9, 9, 9, 9` | shown = value + 1 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_CTC_SHOWONRADAR` | see MPOPTION_* in constants.h |
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
| 1 | `MPWEAPON_CYCLONE` |
| 2 | `MPWEAPON_SUPERDRAGON` |
| 3 | `MPWEAPON_K7AVENGER` |
| 4 | `MPWEAPON_FARSIGHT` |
| 5 | `MPWEAPON_CLOAKINGDEVICE` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count, one comma-separated cell per sim (1p/2p/3p/4p). **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims. Up to 16 sims may be listed; the row number is the sim slot.

| # | Type | Head | Body | Team | Difficulties (1p/2p/3p/4p) | Bot name |
|---|------|------|------|------|--------------------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | `Normal, Normal, Hard, Hard` | GuardSim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_A51TROOPER` | `MPTEAM_1` | `Easy, Easy, Normal, Normal` | TroopSim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | `Off, Easy, Easy, Easy` | GuardSim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_A51AIRMAN` | `MPTEAM_1` | `Off, Easy, Easy, Easy` | AirSim:1 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_PENNY` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | `Off, Off, Easy, Easy` | GuardSim:3 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DAVEC` | `MPBODY_A51TROOPER` | `MPTEAM_1` | `Off, Off, Off, Easy` | TroopSim:2 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_KEITH` | `MPBODY_AREA51GUARD` | `MPTEAM_1` | `Off, Off, Off, Off` | GuardSim:4 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_STEVEM` | `MPBODY_A51AIRMAN` | `MPTEAM_1` | `Off, Off, Off, Off` | AirSim:2 |

## Goals / how it is won

- Capture the case and carry it to your spawn point.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep 8 to 16 simulant rows, each with exactly 4 difficulties in the one cell.
- The `bonus_requirement`, `silver`, `gold` and `perfect` fields drive the Challenge+ medal stars. After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
