# Authoring a custom Challenge (e.g. "Challenge 31")

A challenge is a normal Combat Simulator setup that the game locks as a
Challenge and plays with a per-player-count roster of simulants. Adding one
means touching six places. This file labels every field and gives a worked
example (Challenge 15, "Hold the briefcase in the Grid") you can copy.

There are two ways to ship the config data:

- **In the build (source):** fill in `g_MpConfigs[]` / `mpstrings` and the
  language JSON below, then rebuild. Easiest for development.
- **As a mod (no rebuild):** replace the ROM `mpconfigs` / `mpstrings` segments
  (see `tools/extract`, `tools/importmod`, and the byte-layout section at the
  end). A mod replaces the whole 44-record table, so entry 31 has to fit the
  fixed-size segment - see the caveats at the bottom.

The game has 30 challenges today (`MPCONFIG_CHALLENGE01`..`30`). All 44 config
slots are used (14 setups + 30 challenges), so a 31st needs a new config slot
appended and a new entry in `g_MpChallenges[]`.

---

## The five difficulty values - read this first

A simulant's difficulty is **not one field**. Each of the 8 sim slots carries
**four** difficulties, one per number of human players (1, 2, 3, 4):

```c
{ BOTTYPE_GENERAL, MPHEAD_CHRIST, MPBODY_AREA51GUARD, MPTEAM_1,
  { BOTDIFF_DISABLED, BOTDIFF_EASY, BOTDIFF_EASY, BOTDIFF_EASY } }
  /*  1 player         2 players    3 players    4 players      */
```

`BOTDIFF_DISABLED` means the sim is **absent** at that player count. As you add
players, sims switch on and get harder - that is how the star tiers work. The
star you earn is the same number: a win with N humans ticks `completions[N-1]`
(1 star for 1 player ... 4 stars for 4 players). So tier index and difficulty
index are the same value.

Difficulty values (`constants.h`): `BOTDIFF_MEAT` 0, `EASY` 1, `NORMAL` 2,
`HARD` 3, `PERFECT` 4, `DARK` 5, `DISABLED` 6.

**Everything else (arena, scenario, weapons, limits, options) is fixed for the
whole challenge** - only the sim list changes with player count.

---

## 1. Config index constant - `src/include/constants.h`

After `MPCONFIG_CHALLENGE30 0x2b`:

```c
#define MPCONFIG_CHALLENGE31 0x2c   // next free slot (0x2c = 44 decimal)
```

`0x2c` is one past the current last config (index 43). If you use the mod/ROM
route you do not add C constants - the index is just the record number.

## 2. Language name - `src/assets/<version>/lang/options.json`

Add a row after `L_OPTIONS_435` (Challenge 30). Pick the next free id:

```json
{
    "id": "L_OPTIONS_436",
    "en": "Challenge 31",
    "gb": "Challenge 31",
    "jp": "Challenge 31",
    "it": "Challenge 31",
    "fr": "Challenge 31",
    "de": "Challenge 31",
    "es": "Challenge 31"
},
```

Do this for each version folder (`ntsc-final`, `ntsc-1.0`, `pal-beta`, ...)
that you ship. The id must match the one used in step 3.

## 3. Challenge row - `src/game/challenge.c`, `g_MpChallenges[]`

After the Challenge 30 row:

```c
{ L_OPTIONS_436, MPCONFIG_CHALLENGE31 }, // "Challenge 31"
```

Fields: `name` = the language id from step 2; `confignum` = the config index
from step 1. `struct challenge` is `{ u16 name; s16 confignum; }` plus the
runtime record fields (`availability`, `completions[4]`, `unlockfeatures`)
which are filled in at load and must not be set here.

## 4. Setup record - `src/game/mpconfigs.c`, `g_MpConfigs[]`

Append a `struct mpconfig`. Worked example (Challenge 15's Grid briefcase
setup - copy and edit):

```c
    // MPCONFIG_CHALLENGE31
    {
        {                                   // struct mpsetup
            "Challenge 31",                 // name[18] (port); ROM stores 12 bytes
            MPOPTION_TEAMSENABLED | MPOPTION_HTB_HIGHLIGHTBRIEFCASE | MPOPTION_HTB_SHOWONRADAR,
                                            // options: MPOPTION_* bitmask (see constants.h)
            MPSCENARIO_HOLDTHEBRIEFCASE,    // scenario: MPSCENARIO_* (0 Combat, 1 HTB,
                                            //   2 Hacker Central, 3 Pop a Cap, 4 KOH,
                                            //   5 CTC, 6 Instagib)
            STAGE_MP_GRID,                  // stagenum: STAGE_MP_* arena
            9,                              // timelimit (minutes, stored as N-1)
            100,                            // scorelimit (stored as N-1)
            9,                              // teamscorelimit (stored as N-1)
            0,                              // chrslots - leave 0 for a challenge;
                                            //   the sim bits are filled from the
                                            //   difficulty table at match start
            {                               // weapons[6]: the fixed loadout
                MPWEAPON_MAGSEC4,
                MPWEAPON_DRAGON,
                MPWEAPON_SHOTGUN,
                MPWEAPON_DEVASTATOR,
                MPWEAPON_SHIELD,
                MPWEAPON_DISABLED,
            },
        }, {                                // simulants[8]
            // type, head, body, team, { difficulties for 1/2/3/4 players }
            { BOTTYPE_GENERAL, MPHEAD_PENNY,  MPBODY_LABTECH,    MPTEAM_1, { BOTDIFF_NORMAL,   BOTDIFF_HARD,     BOTDIFF_HARD,     BOTDIFF_HARD     } },
            { BOTTYPE_GENERAL, MPHEAD_VD,     MPBODY_FEMLABTECH, MPTEAM_1, { BOTDIFF_MEAT,     BOTDIFF_NORMAL,   BOTDIFF_NORMAL,   BOTDIFF_NORMAL   } },
            { BOTTYPE_GENERAL, MPHEAD_CHRIST, MPBODY_DD_LABTECH, MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_MEAT,     BOTDIFF_EASY,     BOTDIFF_EASY     } },
            { BOTTYPE_GENERAL, MPHEAD_GRANT,  MPBODY_LABTECH,    MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_MEAT,     BOTDIFF_EASY     } },
            { BOTTYPE_GENERAL, MPHEAD_VD,     MPBODY_FEMLABTECH, MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_MEAT     } },
            { BOTTYPE_GENERAL, MPHEAD_PENNY,  MPBODY_DD_LABTECH, MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED } },
            { BOTTYPE_GENERAL, MPHEAD_DAVEC,  MPBODY_LABTECH,    MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED } },
            { BOTTYPE_GENERAL, MPHEAD_VD,     MPBODY_FEMLABTECH, MPTEAM_1, { BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED, BOTDIFF_DISABLED } },
        },
    },
```

Field reference:

| Field | Meaning |
|---|---|
| `name` | Setup name. 18 chars in the port struct; only 12 survive in the ROM record. |
| `options` | `MPOPTION_*` bits OR'd together. Team games need `MPOPTION_TEAMSENABLED`; scenario HUD rows need their `*_SHOWONRADAR` / `*_HIGHLIGHT*` bits. |
| `scenario` | `MPSCENARIO_*`. Drives the win condition and the challenge HUD. |
| `stagenum` | `STAGE_MP_*` arena constant. |
| `timelimit` / `scorelimit` / `teamscorelimit` | Stored as the shown value minus 1. |
| `chrslots` | Leave 0 for challenges - the game fills sim slots from the difficulty table. (Player bits `0x000f`, sim bits `0x0ff0`.) |
| `weapons[6]` | Fixed six-slot loadout; `MPWEAPON_DISABLED` for an empty slot. |
| sim `type` | `BOTTYPE_*` behaviour (general, shield, rocket, ...). |
| sim `mpheadnum`/`mpbodynum` | Character head/body `MPHEAD_*` / `MPBODY_*` constants. |
| sim `team` | `MPTEAM_1` .. `MPTEAM_4`. For team scenarios, enemies go on a different team from the player. |
| sim difficulties[4] | One `BOTDIFF_*` per player count 1-4; `DISABLED` = sim absent at that tier. |

## 5. Description and bot names - `src/assets/<version>/mpstrings/mpstrings<lang>.c`

One `struct mpstrings` per config, same index as `g_MpConfigs`. English is
`mpstringsF.c`. Add after the Challenge 30 entry:

```c
    // MPCONFIG_CHALLENGE31
    {
        "Capture the briefcase in the Grid with devastators.",  // description[200]
        {                                                       // aibotnames[8][15]
            "LabSim:1\n",
            "LabSim:2\n",
            "LabSim:3\n",
            "LabSim:4\n",
            "LabSim:5\n",
            "LabSim:6\n",
            "LabSim:7\n",
            "LabSim:8\n",
        },
    },
```

The description is what the room's confirm page shows when the host picks the
challenge. Do this for each language file you ship (`mpstringsE/J/P/G/F/S/I`).
The array index must line up with the config index - a missing or shifted entry
shows the wrong description for every later challenge.

---

## ROM byte layout (for the mod/importmod route)

`challengeDecodeRomConfig()` in `src/game/challenge.c:416` reads each config as
a **104-byte big-endian** record (`MPCONFIG_ROMSIZE`). If you edit
`mpconfigs.bin` extracted by `tools/extract`, this is the layout of record 31
(byte 31 × 104):

| Offset | Size | Field |
|---|---|---|
| `0x00` | 12 | name (ASCII, NUL-padded) |
| `0x0c` | 4  | options (u32) |
| `0x10` | 1  | scenario |
| `0x11` | 1  | stagenum |
| `0x12` | 1  | timelimit (shown - 1) |
| `0x13` | 1  | scorelimit (shown - 1) |
| `0x14` | 2  | teamscorelimit (u16, shown - 1) |
| `0x16` | 2  | chrslots (u16) - 0 for challenges |
| `0x18` | 6  | weapons[6] - ROM weapon ids, remapped by `modDataMpWeaponSlot()` |
| `0x1e` | 1  | paused |
| `0x1f` | 1  | padding |
| `0x20` | 4  | fileguid.fileid |
| `0x24` | 2  | fileguid.deviceserial |
| `0x26` | 2  | padding |
| `0x28` | 64 | 8 sims × 8 bytes: type, mpheadnum, mpbodynum, team, difficulties[4] |

Descriptions are in a separate `mpstrings` segment: 44 records of 0x3700 bytes
each, `description[200]` then 8 × 15-byte bot names.

`tools/importmod` treats `mpconfigs` (and the `mpstrings*`) as fixed-size
segments - it will not grow them. A 31st challenge must therefore be a source
build (new config + array entries) unless you repurpose an unused slot.

---

## Checklist

1. `constants.h` - `MPCONFIG_CHALLENGE31` (source builds only).
2. `lang/options.json` (each version) - `L_OPTIONS_436` name.
3. `challenge.c` - `g_MpChallenges[]` row.
4. `mpconfigs.c` - `g_MpConfigs[]` setup record.
5. `mpstrings<lang>.c` (each language) - description + bot names.
6. Rebuild; run `tools/ci/netchallengetest.sh` and check the game loads the new
   entry with `--moddata-trace` (logs every config's stage/scenario/weapons/sims).

## Caveats

- **Records**: completion bytes are indexed by challenge and player count. An
  appended entry is safe for existing saves, but the array length is hard-coded
  in a few `ARRAYCOUNT(g_MpChallenges)` loops - check the save-format and
  unlock logic (`challengeDetermineUnlockedFeatures`) if you care about old
  saves and star gifting.
- **Online**: RULES/MATCH_END carry the challenge index as `u16` and guard it
  with `>= ARRAYCOUNT(g_MpChallenges)`, so online play accepts a 31st
  automatically once the array grows - no protocol bump needed for that part.
- **Feature unlocks**: some sim types/difficulties are behind
  `MPFEATURE_*` bits; `challengeForceUnlockConfigFeatures()` grants the config's
  declared features, so set `unlockfeatures` if your config uses gated content.
