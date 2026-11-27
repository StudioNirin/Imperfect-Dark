# Challenges+ - challenge data

One file per challenge. These are the **source** for the separate Challenges+ dataset: edit them to change a Plus challenge, then the import step writes the matching entries into `src/game/mpconfigsplus.c` and `src/game/mpstringsplus.c`. Values shown are the current stock challenges (the starting copy) - the Plus mode launched as an exact duplicate, so these are what you see until you edit them.

Every Plus challenge is available (no unlock gating) and a Plus completion ticks only the Plus record for the number of humans in the match (1-4 stars). Plus completions grant no feature unlocks and never touch the standard challenges.

## Converting a challenge

Templates start as `challenge-NN.md`. When you finish rewriting one, rename it
to `challenge+-NN.md` and change `MPCONFIG_CHALLENGE NN` to
`MPCONFIG_CHALLENGE+NN` inside the file (both the Source config line and the
Config index line). A renamed template is the marker that the entry is ready:
the importer sets its flag in `g_MpChallengesPlusConverted`, and the in-game
Plus list shows only converted entries. Unconverted templates still feed their
config data into the build, but their challenges stay hidden in the menus until
you rename them. Re-run `python3 tools/ci/challengesplus_import.py` after each
rename, then rebuild.

**Applying edits:** after changing a `challenge-NN.md`, run `python3 tools/ci/challengesplus_import.py` from anywhere (it finds the repo). It currently verifies all 30 docs and their descriptions are present in the plus dataset; the C entries are seeded copies of the stock ones and are edited by hand until the generator is extended. Keep the config identifier (`MPCONFIG_CHALLENGE01` .. `30`) unchanged unless you also move the matching record in `mpconfigsplus.c`.

| # | Config | Arena | Scenario | Description |
|---|--------|-------|----------|-------------|
| 1 | `MPCONFIG_CHALLENGE01` | `STAGE_MP_SKEDAR` | `MPSCENARIO_COMBAT` | A challenge against basic simulants using standard weaponry. |
| 2 | `MPCONFIG_CHALLENGE02` | `STAGE_MP_AREA52` | `MPSCENARIO_COMBAT` | A challenge against basic simulants which includes the use of rocket launchers. |
| 3 | `MPCONFIG_CHALLENGE03` | `STAGE_MP_PIPES` | `MPSCENARIO_COMBAT` | A challenge against basic simulants which includes assault rifles and timed mines. |
| 4 | `MPCONFIG_CHALLENGE04` | `STAGE_MP_SKEDAR` | `MPSCENARIO_KINGOFTHEHILL` | Take control of the hill against a team of standard simulants. Shield technology is available, as well as the K7 Avenger. |
| 5 | `MPCONFIG_CHALLENGE05` | `STAGE_MP_COMPLEX` | `MPSCENARIO_COMBAT` | Combat a team of standard simulants using the specialized FarSight weapon in the Complex. |
| 6 | `MPCONFIG_CHALLENGE06` | `STAGE_MP_AREA52` | `MPSCENARIO_HOLDTHEBRIEFCASE` | Hold the briefcase for as long as possible against a large team of basic simulants. |
| 7 | `MPCONFIG_CHALLENGE07` | `STAGE_MP_WAREHOUSE` | `MPSCENARIO_KINGOFTHEHILL` | Take control of the hill against a team of advanced simulants in the Warehouse. All weapons kill with only one hit. |
| 8 | `MPCONFIG_CHALLENGE08` | `STAGE_MP_SKEDAR` | `MPSCENARIO_CAPTURETHECASE` | Capture the enemy's flag while defending your own. Killing the flag carrier means the flag returns to the base. |
| 9 | `MPCONFIG_CHALLENGE09` | `STAGE_MP_RAVINE` | `MPSCENARIO_COMBAT` | A one-hit-kill scenario against expert simulants. Weapons  the FarSight and the laptop gun. |
| 10 | `MPCONFIG_CHALLENGE10` | `STAGE_MP_TEMPLE` | `MPSCENARIO_HACKERCENTRAL` | Fight against a team of simulants for control of the boxes. Touch a box to claim it for your team. |
| 11 | `MPCONFIG_CHALLENGE11` | `STAGE_MP_COMPLEX` | `MPSCENARIO_KINGOFTHEHILL` | King of the hill  against expert simulants. Weapons  the shotgun and tranquilizer. |
| 12 | `MPCONFIG_CHALLENGE12` | `STAGE_MP_SKEDAR` | `MPSCENARIO_COMBAT` | Slow-motion combat in the Skedar arena. |
| 13 | `MPCONFIG_CHALLENGE13` | `STAGE_MP_G5BUILDING` | `MPSCENARIO_COMBAT` | One-hit kills, touch that box, G5, with tranquilizers. |
| 14 | `MPCONFIG_CHALLENGE14` | `STAGE_MP_AREA52` | `MPSCENARIO_CAPTURETHECASE` | Capture the flag in Area 52 with cloaking. |
| 15 | `MPCONFIG_CHALLENGE15` | `STAGE_MP_GRID` | `MPSCENARIO_HOLDTHEBRIEFCASE` | Capture the briefcase in the Grid with devastators. |
| 16 | `MPCONFIG_CHALLENGE16` | `STAGE_MP_FELICITY` | `MPSCENARIO_COMBAT` | Standard combat. Weapons  proximity mines. |
| 17 | `MPCONFIG_CHALLENGE17` | `STAGE_MP_TEMPLE` | `MPSCENARIO_KINGOFTHEHILL` | A king of the hill game. Weapons  fly-by-wire missiles. |
| 18 | `MPCONFIG_CHALLENGE18` | `STAGE_MP_VILLA` | `MPSCENARIO_KINGOFTHEHILL` | A king of the hill game against a team of expert simulants. |
| 19 | `MPCONFIG_CHALLENGE19` | `STAGE_MP_G5BUILDING` | `MPSCENARIO_COMBAT` | Standard combat against a squad of expert simulants. Weapons  the rocket launcher and the FarSight. |
| 20 | `MPCONFIG_CHALLENGE20` | `STAGE_MP_SEWERS` | `MPSCENARIO_COMBAT` | One-hit kill combat with a twist - protect the weaker simulants on your team. Pistols and shields. |
| 21 | `MPCONFIG_CHALLENGE21` | `STAGE_MP_GRID` | `MPSCENARIO_HACKERCENTRAL` | Touch the boxes to claim them for your side. The Cloaking Device is available. |
| 22 | `MPCONFIG_CHALLENGE22` | `STAGE_MP_BASE` | `MPSCENARIO_HOLDTHEBRIEFCASE` | A hold the briefcase game with one-hit kills. Weapons  the crossbow and the sniper rifle. |
| 23 | `MPCONFIG_CHALLENGE23` | `STAGE_MP_COMPLEX` | `MPSCENARIO_COMBAT` | Combat in slow motion. Weapons  the RCP120. |
| 24 | `MPCONFIG_CHALLENGE24` | `STAGE_MP_FORTRESS` | `MPSCENARIO_CAPTURETHECASE` | Capture the flag game against a team of expert simulants. Weapons  the Gold DY357 Magnum and the tranquilizer. No shields. |
| 25 | `MPCONFIG_CHALLENGE25` | `STAGE_MP_RAVINE` | `MPSCENARIO_COMBAT` | A standard combat game with N-Bombs and Cloaking Devices. The simulant team is composed of experts. |
| 26 | `MPCONFIG_CHALLENGE26` | `STAGE_MP_RUINS` | `MPSCENARIO_KINGOFTHEHILL` | A standard king of the hill game with no shields. |
| 27 | `MPCONFIG_CHALLENGE27` | `STAGE_MP_SEWERS` | `MPSCENARIO_HACKERCENTRAL` | A touch the box game featuring rocket launchers and expert opponents. |
| 28 | `MPCONFIG_CHALLENGE28` | `STAGE_MP_VILLA` | `MPSCENARIO_CAPTURETHECASE` | A capture the flag game with no shields and expert opponents. |
| 29 | `MPCONFIG_CHALLENGE29` | `STAGE_MP_G5BUILDING` | `MPSCENARIO_COMBAT` | Standard combat with no shields available. Weapons  the DY357 and the Dragon. |
| 30 | `MPCONFIG_CHALLENGE30` | `STAGE_MP_SKEDAR` | `MPSCENARIO_KINGOFTHEHILL` | A pistols only king of the hill game against elite opposition. |
