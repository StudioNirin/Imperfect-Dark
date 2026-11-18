# Challenge+ 9

> Source config: `MPCONFIG_CHALLENGE09` (index `0x16` in `g_MpConfigs`). Edit this file to change this Challenges+ entry; the standard Challenge 9 is unaffected.

## Identity

- **Display name**: Challenge+ 9 (generated; no separate language string)
- **Config index**: `MPCONFIG_CHALLENGE09`
- **Arena**: `STAGE_MP_RAVINE`
- **Scenario**: `MPSCENARIO_COMBAT`
- **Description**:
    A one-hit-kill scenario against expert simulants. Weapons  the FarSight and the laptop gun.

## Rules / limits

| Field | Stored value | Shown in game |
|-------|--------------|---------------|
| Time limit | 9 | 10 minutes |
| Score limit | 100 | 101 kills |
| Team score limit | 9 | 10 (400 = none) |
| Options | `MPOPTION_ONEHITKILLS | MPOPTION_TEAMSENABLED` | see MPOPTION_* in constants.h |
| chrslots (sim bits) | `0` | filled from difficulty table at match start |

## Weapons (fixed six-slot loadout)

| Slot | Weapon |
|------|--------|
| 1 | `MPWEAPON_FALCON2` |
| 2 | `MPWEAPON_DY357MAGNUM` |
| 3 | `MPWEAPON_TIMEDMINE` |
| 4 | `MPWEAPON_LAPTOPGUN` |
| 5 | `MPWEAPON_FARSIGHT` |
| 6 | `MPWEAPON_DISABLED` |

## Simulants

Difficulty per human-player count. **Off** = simulant absent at that tier; the star you earn is the same player count, so more stars = more/harder sims.

| # | Type | Head | Body | Team | 1p (1 star) | 2p | 3p | 4p (4 stars) | Bot name |
|---|------|------|------|------|-------------|----|----|--------------|----------|
| 1 | `BOTTYPE_GENERAL` | `MPHEAD_TRENT` | `MPBODY_TRENT` | `MPTEAM_1` | Normal | Normal | Normal | Normal | TrentSim |
| 2 | `BOTTYPE_GENERAL` | `MPHEAD_BRIAN` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Meat | Easy | NSASim:1 |
| 3 | `BOTTYPE_GENERAL` | `MPHEAD_ROSS` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:2 |
| 4 | `BOTTYPE_GENERAL` | `MPHEAD_JAMIE` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:3 |
| 5 | `BOTTYPE_GENERAL` | `MPHEAD_DUNCAN2` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:4 |
| 6 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:5 |
| 7 | `BOTTYPE_GENERAL` | `MPHEAD_CHRIST` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:6 |
| 8 | `BOTTYPE_GENERAL` | `MPHEAD_GRANT` | `MPBODY_NSA_LACKEY` | `MPTEAM_1` | Off | Off | Off | Off | NSASim:7 |

## Goals / how it is won

- Reach the score limit (most kills). Win when team 0 finishes on top.
- A challenge completes for the host when no player aborted and team 0 is top of the ranking; cheats used invalidate the completion.
- Completion ticks the record for the number of human players in the match (1-4 stars), independent of the standard challenge record.

## Editing notes

- All values are C identifiers from `src/include/constants.h`; keep them verbatim.
- Keep exactly 8 simulant rows and 4 difficulties per row.
- After editing, regenerate `mpconfigsplus.c` / `mpstringsplus.c` from these files.
