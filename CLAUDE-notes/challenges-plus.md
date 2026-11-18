# Challenges+

A second, independent 30-challenge mode that starts as an exact copy of the
stock Combat Challenges but is edited separately. See
[challenges+ docs](../ImperfectDark-Docs/challenges+/) for the per-challenge
data and [custom-challenge-template.md](../docs/custom-challenge-template.md)
for the field reference.

## The three parallel datasets

- `g_MpChallenges[]` / `g_MpChallengesPlus[]` - the 30-entry registries (name,
  confignum, availability, completions, unlockfeatures). Plus is defined in
  `src/game/challengeplus.c`.
- `g_MpConfigs[]` / `g_MpConfigsPlus[]` - the 44 setups; Plus in
  `src/game/mpconfigsplus.c`. The 30 challenge records are indices `0x0e..0x2b`.
- `mpstrings` / `g_MpStringsPlus[]` - descriptions and bot names; Plus in
  `src/game/mpstringsplus.c`, seeded from the English `mpstringsF`.

`g_ChallengeMode` (0 stock, 1 Plus) selects the active dataset. Set by the
Combat Simulator's "Challenges+" menu row (setup.c) and carried in net RULES
(protocol 39). `challengeActiveList()` returns the active registry; all
completion get/set APIs route through it. Names for Plus are generated
("Challenge+ N"), so no new language strings were added.

## Save format

`gamefile.c` writes the stock completion bits as before, then appends a second
block of `30 * 4` bits for the Plus records at the END of the bitstream. Old
saves read the new block as zeros (blank Plus progress); a save from this build
writes both. Do not reorder the stock block - existing saves depend on it.

## Rules

- Every Plus challenge is visible - there is no unlock gating in Plus yet.
- A Plus completion ticks the Plus record for the number of human players
  (1-4 stars) and grants NO shared feature unlocks: `challengeDetermineUnlockedFeatures()`
  is skipped for Plus in both the offline (`challengeConsiderMarkingComplete`)
  and net (`netChallengeMatchEnded`) marking paths. Availability stays driven
  by the stock list.
- Future, not implemented: gate the Challenges+ menu behind completing all
  stock challenges with at least one star (any tier), and make custom weapons
  the Plus unlock tiers.

## Network

RULES and MATCH_END carry `u8 challengemode` (protocol 39). Clients set the
mode from RULES and mark into that dataset from the verdict, then restore their
own menu mode. The lobby's Game dropdown has a "Challenge+" option. The gate
`tools/ci/netchallengetest.sh` has a `plus` case asserting the generated name,
the challenge's own arena, both machines marking the Plus record, and that the
standard record is untouched.
