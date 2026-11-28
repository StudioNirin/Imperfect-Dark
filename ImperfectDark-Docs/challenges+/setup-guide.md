# Challenges+ setup guide

How to author a challenge in this folder. Each `challenge-NN.md` is the source
of truth; the build regenerates the C datasets from them automatically.

## Converting a challenge

Templates start as `challenge-NN.md`. When a challenge is ready, rename it to
`challenge+-NN.md` and change both `MPCONFIG_CHALLENGE NN` references inside
the file to `MPCONFIG_CHALLENGE+NN`. Only renamed challenges appear in the
in-game Challenges+ list. Then just rebuild: `cmake --build build -j8` runs the
importer as a pre-build step.

## Rules / limits

| Field | Format |
|---|---|
| Time limit | stored value; shown in game = value + 1 minutes, `60` = none |
| Start armed | `none`, `first`, `fiesta`, or `loadout` |
| Score limit (1p/2p/3p/4p) | one value or four comma-separated values, in star order; `400` = no limit |
| Options | `MPOPTION_*` bits; challenge lists use `MPOPTION_TEAMSENABLED` |
| chrslots | `0`; sim slots are filled from the difficulty table at match start |

All challenges are team games. The score row feeds the team score limit; the
player score limit is off. The stock player-count scaling is bypassed for
Plus, so each authored value is the exact limit for that tier.

`Start armed` is applied at match start. It maps to the game's `Mod.StartArmed`
option (`none` = off, `fiesta` = random), which is synced to clients in net
play, so every machine spawns with the same gun. `first` gives everyone the
first weapon in the loadout, and `loadout` gives each player their own saved
loadout.

## Bonus requirement

| Field | Format |
|---|---|
| bonus_requirement | one tag from the table below, or `NONE` |
| silver (1p/2p/3p/4p) | one value or four, in star order |
| gold (1p/2p/3p/4p) | one value or four, in star order |
| perfect (1p/2p/3p/4p) | one value or four; `0` disables the tier |

Bronze is always a win and has no value. Silver, gold and perfect are graded
team-wide at match end using the player count, and the matching star is tinted
bronze, silver, gold, or bright purple for perfect.

Values are integers. `ACCURACY` and `KD_RATIO` are percentages. `FINISH_TIME`,
`TOKEN_HELD_TIME`, `LONGEST_LIFE` and `FASTEST_2KILLS` are seconds in the
template and stored as 60 Hz ticks. For lower-is-better conditions, silver is
the loosest limit and perfect the tightest.

### Requirement tags

| Tag | Team stat | Better |
|---|---|---|
| `NONE` | no bonus tiers | - |
| `HEADSHOTS` | head-region hits | higher |
| `ACCURACY` | combined hits / combined shots | higher |
| `KILLS` | `killcount` | higher |
| `DEATHS` | `numdeaths` | lower |
| `KD_RATIO` | kills x 100 / deaths | higher |
| `FASTEST_2KILLS` | shortest gap between two kills | lower |
| `LONGEST_LIFE` | longest life | higher |
| `MAX_SIMUL_KILLS` | most simultaneous kills | higher |
| `MULTIKILLS` | kills that extended a streak | higher |
| `LONGEST_SPREE` | most kills in one life | higher |
| `TOKEN_HELD_TIME` | objective hold time | higher |
| `BACKSHOTS` | `backshotcount` | higher |
| `DISTANCE` | `distance / 10000` summed | higher |
| `DAM_RECEIVED` | `damreceived` | lower |
| `ITEM_USE` | `cloaktime` | higher |
| `FINISH_TIME` | match mission time | lower |

`LONGEST_LIFE` and `ITEM_USE` read playerstat fields that this build never
fills, so they always grade as zero. Avoid them for now.

The three kill-streak tags count different things. `MAX_SIMUL_KILLS` and
`MULTIKILLS` both come from the two-second kill window: the first is the largest
single streak in the match, so it can never pass 4, and the second adds up every
kill that extended a streak, so a four kill spree is worth 3 and several sprees add
together. `LONGEST_SPREE` ignores the window entirely and counts the kills of one
life, from spawn to respawn, so it is the tag for surviving as much as for shooting
fast; a team is graded on its best single life, not on every life added together.
A player who never chains a kill has 0 of the first two but still has 1 spree per
life it killed in.

## Simulants

8 to 16 rows; the row number is the sim slot. Slots 9-16 live in Plus-only
arrays, so the ROM-resident config stays at 8 sims. The four difficulties are
one comma-separated cell in 1p/2p/3p/4p order.

```
| # | Type | Head | Body | Team | Difficulties (1p/2p/3p/4p) | Bot name |
| 1 | `BOTTYPE_TURTLE` | `MPHEAD_GRANT` | `MPBODY_CARRINGTON` | `MPTEAM_1` | `Normal, Normal, Hard, Hard` | TurtleSim:1 |
```

### Behaviour types (Type column)

| Identifier | Behaviour |
|---|---|
| `BOTTYPE_GENERAL` | standard AI |
| `BOTTYPE_PEACE` | collects weapons, won't engage |
| `BOTTYPE_SHIELD` | prioritises getting a full shield |
| `BOTTYPE_ROCKET` | prefers explosives |
| `BOTTYPE_KAZE` | closes distance, never backs off |
| `BOTTYPE_FIST` | fists only |
| `BOTTYPE_PREY` | hunts freshly spawned, weak or poorly armed players |
| `BOTTYPE_COWARD` | runs away, attacks only with a better weapon |
| `BOTTYPE_JUDGE` | targets the current leader |
| `BOTTYPE_FEUD` | fixes on one player for the whole match |
| `BOTTYPE_SPEED` | moves faster |
| `BOTTYPE_TURTLE` | slower, carries a double-strength shield |
| `BOTTYPE_VENGE` | targets whoever last killed it |

The bot-name cell is just a label, so change it to match the type. Any feature
unlock a type needs is granted automatically by the challenge config.

### Difficulty values (Difficulties column)

`Meat`, `Easy`, `Normal`, `Hard`, `Perfect`, `Dark`, or `Off` (absent at that
tier). The importer maps these to `BOTDIFF_*`.

## Description panel

Do not type bonuses into the `Description` field. The importer generates one
complete description per tier from the template values, and a row of tabs
labelled `1p / 2p / 3p / 4p+` on the selection screen picks which is shown.
Only the bonus lines change; the description stays fixed:

```
A challenge against normal simulants using standard weaponry.

Bonus - Finish Time
Bronze - Win
Silver - 120s
Gold - 90s
Perfect - 75s
```

Times get an `s` suffix, ACCURACY / KD_RATIO a `%`, other tags a plain integer.
`NONE` adds no bonus lines. The tabs are a preview; the tier actually applied
is the real player count at match start.

## End-game panels

On the Challenge Completed / Failed / Cheated screens, a result line shows the
bonus requirement and the team's achieved value for it. It only appears for
converted Plus challenges that actually set a requirement. A finished challenge
names the star the thresholds give (Bronze, Silver, Gold or Perfect); an ended or
abandoned one says `Unfinished`, and a run with cheats active says `Not awarded`,
because only a finished challenge stores a medal. The thresholds are still graded
either way, so the value shown is the real one. When a star is earned the line is
tinted with that tier's colour, matching the star in the challenge list; the
palette's alternative label colour cannot be used, because the success dialog sets
it fully transparent.

A second end-game panel, reached by swiping after Player Ranking, shows the
hidden per-player stats in two columns of seven: accuracy, K/D ratio, headshots,
back shots, longest life, fastest two kills, multi-kills, max multi-kills, longest
spree, distance, damage taken and dealt, objective time and cloak time.
Rows are small-font labels whose text comes from the item's `param`, so a row is
one `EXTSTAT_ROW(n)` line and no drawing code. Swipe navigation reaches it in every multiplayer
mode. The player is chosen with the same dropdown the standard stats panel uses,
and the panel shares that choice with the standard panel. Fastest two kills shows
`-` until the player gets two kills in a row, as the stat is otherwise unused.

## Checks

Stars are kept in `challengeplus.bin` beside the other save files, one block
per agent so two players using different agent saves on one computer do not
share progress. A missing file starts blank. On an N64 build there is no side
file and Plus stars do not survive a reload.

`tools/ci/challengesplusmedals.sh` verifies all 30 templates parse, the
generated C matches them, and the runtime wiring is intact.
