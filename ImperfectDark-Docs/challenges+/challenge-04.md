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
| Start armed | none | - |
| Score limit (1p/2p/3p/4p) | `3, 7, 9, 11` | shown = value + 1 (400 = none) |
| Options | `MPOPTION_TEAMSENABLED | MPOPTION_KOH_MOBILEHILL | MPOPTION_KOH_HILLONRADAR` | see MPOPTION_* in constants.h |
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
| 1 | `MPWEAPON_MAGSEC4` |
| 2 | `MPWEAPON_CMP150` |
| 3 | `MPWEAPON_DRAGON` |
| 4 | `MPWEAPON_K7AVENGER` |
| 5 | `MPWEAPON_SHIELD` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count, one comma-separated cell per sim (1p/2p/3p/4p). **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims. Up to 16 sims may be listed; the row number is the sim slot.

| # | Type | Head | Body | Team | Difficulties (1p/2p/3p/4p) | Bot name |
|---|------|------|------|------|--------------------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_MARK2` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Easy, Easy, Easy, Easy` | CISim:1 |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_RUSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Meat, Easy, Easy` | CISim:2 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Meat` | CISim:3 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Off` | CISim:4 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Off` | CISim:5 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Off` | CISim:6 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Off` | CISim:7 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_CISOLDIER` | `MPTEAM_1` | `Off, Off, Off, Off` | CISim:8 |

## Goals / how it is won

- Hold the hill for the hill-time to score.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep 8 to 16 simulant rows, each with exactly 4 difficulties in the one cell.
- The `bonus_requirement`, `silver`, `gold` and `perfect` fields drive the Challenge+ medal stars. After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
