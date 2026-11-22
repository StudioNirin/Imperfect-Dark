# One Shall Stand... (multiplayer scenario)

A free-for-all Combat Simulator mode. Every player starts with a fixed number
of lives, adjustable from one to nine in the mode's Options (default three).
Each death costs one life. A player whose lives are spent is out: they cannot
respawn, cannot be targeted, and drop into the spectator cameras to watch the
survivors (Z cycles players, A toggles the free camera). The last player with a
life left wins. If the time limit runs out first, the player with the most
lives left wins, with kills breaking a tie.

The mode is id `MPSCENARIO_LASTMANSTANDING` (7) and appears in the free-for-all
section of the scenario menu. Teams are forced off.

## Where the rules live

- `src/game/mplayer/scenarios/lastmanstanding.inc` - options dialog, starting
  lives, save/read, score, elimination helpers, HUD and death messages.
- `src/game/player.c`, `src/game/chraction.c`, `src/game/modbodies.c` - call
  `lmsLivesSpent()` where GE Plus's `gexPlusLivesSpent()` stops respawns.
- `src/game/lv.c` - `lmsMatchOver()` adds a reason to end the match.
- `src/game/mplayer/mplayer.c` - rankings use remaining lives, kills as a tiebreak.
- `port/src/net/netspec.c` - the dead-player watcher, generalized to work for
  eliminated LMS players both online and offline.
- `port/src/net/netscen.c` - the host publishes each chr's lives/out state in
  the spare tail of the SNAP scenario block; no protocol bump is needed.

Starting lives travel in the scenario's 32-bit save word, so saved setups and
net RULES carry it without a new field. Additions are pinned by
`tools/ci/raillaser.sh` and `tools/ci/netscenariotest.sh` (the `lms` case).
