#!/bin/bash
# zombies.sh - Zombies mode and the Falcon Punch alternative melee.
#
# Source gates pin the wiring that is invisible from a single playthrough:
# the scenario id and both tables, the per-round clock replacing the engine's
# match clock, the ceiling-division starter count, the two score rules, the
# body overrides, the NETSCEN overlay (a 768-byte block with only 79 spare),
# and the two doors into the Falcon Punch tickbox with its net key.
# The headless runs then boot the mode at 5 and 9 chrs and check the starter
# counts the user specified (2 and 3) from the scenario's own log line.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

fail() { echo "zombies: $*"; exit 1; }
pass() { echo "zombies: ok - $*"; }
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/zombies-out}
SRC=$ROOT/src
PORT=$ROOT/port

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

# --- scenario plumbing -------------------------------------------------------
grep -q '#define MPSCENARIO_ZOMBIES *9' "$SRC/include/constants.h" || fail 'missing scenario id 9'
grep -q '#define NUM_MPSCENARIOS *10' "$SRC/include/constants.h" || fail 'NUM_MPSCENARIOS not widened'
grep -q 'scenarios/zombies.inc' "$SRC/game/mplayer/scenarios.c" || fail 'zombies.inc not included'
grep -q 'zombiesInitNames' "$SRC/game/mplayer/scenarios.c" || fail 'scenario name not patched'
grep -q 'MPSCENARIO_ZOMBIES,' "$SRC/game/mplayer/scenarios.c" || fail 'missing from menu order'
[ -f "$SRC/game/mplayer/scenarios/zombies.inc" ] || fail 'zombies.inc missing'

# The engine's match timer and score limits must leave the mode alone.
grep -q 'g_MpTimeLimit60 > 0 && g_MpSetup.scenario != MPSCENARIO_ZOMBIES' "$SRC/game/lv.c" || fail 'match clock not suppressed'
[ "$(grep -c 'scenario != MPSCENARIO_LASTMANSTANDING && g_MpSetup.scenario != MPSCENARIO_ZOMBIES' "$SRC/game/lv.c")" -eq 2 ] || fail 'score limits not suppressed'
grep -q 'zombiesMatchOver()' "$SRC/game/lv.c" || fail 'match end not polled'
pass 'scenario plumbing'

# --- rounds, roles and scoring ----------------------------------------------
INC=$SRC/game/mplayer/scenarios/zombies.inc
grep -q '(poolcount + 3) / 4' "$INC" || fail 'starter count is not ceiling division'
grep -q 'score\[i\] += 2' "$INC" || fail 'no +2 for surviving to the round end'
grep -q 'score\[attackerindex\] += 1' "$INC" || fail 'no +1 for infecting'
[ "$(grep -ci 'last survivor' "$INC")" -eq 0 ] || fail 'last-survivor rule not removed'
grep -q 'data->round < g_ZombiesRounds' "$INC" || fail 'no three-round limit'
grep -q 'zombiesHandleDeath' "$SRC/game/mpstats.c" || fail 'death hook not wired'
grep -q 'MPSCENARIO_ZOMBIES' "$SRC/game/mplayer/mplayer.c" || fail 'ranking tie-break not wired'
pass 'rounds, roles and scoring'

# --- bodies -----------------------------------------------------------------
grep -q 'zombiesBodyStaleTake()' "$SRC/game/player.c" || fail 'body rebuild not requested'
# A role flip must rebuild an already-standing human body, on the host as
# well as on a client: sims rebuild directly, humans only through this gate.
grep -q 'zombiesBodyStaleIs(g_Vars.currentplayernum)' "$SRC/game/player.c" \
	|| fail 'an infected human keeps its old body - no stale check in the tick gate'
grep -q 'zombiesBodyStaleIs(g_Vars.currentplayernum) && !g_Vars.currentplayer->isdead' "$SRC/game/player.c" \
	|| fail 'a dead survivor corpse is rebuilt as Skedar mid-death'
grep -q 'zombiesBodyStaleTakeForPlayer(playernum)' "$PORT/src/net/netpuppets.c" \
	|| fail 'client puppets take the stale mark of another seat'
grep -q 'bool bodystale\[MAX_PLAYERS\]' "$ROOT/src/include/types.h" \
	|| fail 'bodystale only covers four seats, so humans past four never rebuild'
grep -q 'zombiesBodyStaleTakeForPlayer' "$ROOT/src/include/game/mplayer/scenarios.h" \
	|| fail 'the per-player stale take is not declared for the puppet tick'
grep -q 'zombiesNotifyChrSpawn' "$SRC/game/player.c" || fail 'Skedar override not applied'
grep -q 'bodyGetRace(bodynum)' "$INC" || fail 'sim body race not kept in sync'
grep -q 'varyheight' "$SRC/game/player.c" || fail 'Skedar random scale not fixed'
grep -q 'zombiesPrepareBotSpawn' "$SRC/game/bot.c" || fail 'sim respawn body not updated'
! grep -q 'modelSetScale(newmodel, 1.0f)' "$INC" || fail 'sim Skedar scale reset to ten times life size'
# Free the old body before allocating the new one. Allocating first can hand
# out a slot tied to the old model, which the free then corrupts (crash).
python3 - "$INC" <<'PY'
import sys
s = open(sys.argv[1]).read()
free = s.index("modelmgrFreeModel(oldmodel);")
alloc = s.index("newmodel = bodyAllocateModel", free)
assert free < alloc, 'old body must be freed before the new one is allocated'
PY
pass 'body overrides'

# --- fists and zombie damage ------------------------------------------------
grep -q 'MPSCENARIO_ZOMBIES' "$SRC/game/player.c" || fail 'fists-only spawn not wired'
grep -q 'MPSCENARIO_ZOMBIES' "$SRC/game/mplayer/mplayer.c" || fail 'spawn weapon not suppressed'
grep -q 'attackerinfected && victiminfected' "$SRC/game/chraction.c" || fail 'zombie-on-zombie damage not skipped'
grep -q 'damage = 1000.0f' "$SRC/game/chraction.c" || fail 'zombie punch not lethal'
# Starting kit: a PD Shotgun and a Falcon 2 in inventory, single wield (no
# forced akimbo), both with half their spare ammo, for players and sims.
grep -q 'invGiveSingleWeapon(WEAPON_SHOTGUN)' "$SRC/game/player.c" || fail 'player shotgun spawn missing'
grep -q 'invGiveSingleWeapon(WEAPON_FALCON2)' "$SRC/game/player.c" || fail 'player falcon spawn missing'
grep -q 'AMMOTYPE_SHOTGUN, bgunGetAmmoCapacityForWeapon(WEAPON_SHOTGUN, FUNC_PRIMARY) / 2' "$SRC/game/player.c" || fail 'player shotgun ammo not half'
grep -q 'AMMOTYPE_PISTOL, bgunGetAmmoCapacityForWeapon(WEAPON_FALCON2, FUNC_PRIMARY) / 2' "$SRC/game/player.c" || fail 'player falcon ammo not half'
grep -q 'spawnweaponnums\[HAND_LEFT\] = WEAPON_NONE' "$SRC/game/player.c" || fail 'player left hand not empty (forced akimbo?)'
grep -q 'botinvGiveSingleWeapon(chr, WEAPON_SHOTGUN)' "$SRC/game/bot.c" || fail 'bot shotgun spawn missing'
grep -q 'botinvGiveSingleWeapon(chr, WEAPON_FALCON2)' "$SRC/game/bot.c" || fail 'bot falcon spawn missing'
# Mid-round conversion must mark the victim's body stale so the respawn
# rebuilds them as Skedar, not their old human body.
grep -q 'data->bodystale\[victimindex\] = true' "$INC" || fail 'conversion does not rebuild victim body'
# Round clock: g_MpTimeLimit60 is already in 60Hz ticks, so no TICKS() wrap.
grep -q 'roundtime60 = g_MpTimeLimit60 > 0 ? g_MpTimeLimit60 :' "$INC" || fail 'round clock double-scales the time limit'
# The infected are melee only: fists for players and sims, kit for survivors.
grep -q 'if (zombiesSeatIsInfected(g_Vars.currentplayernum)) {' "$SRC/game/player.c" || fail 'player role check missing at spawn'
grep -q 'if (!zombiesIsInfected(chr)) {' "$SRC/game/bot.c" || fail 'simulant role check missing at spawn'
# A fresh roll each round: last round's infected must not carry over.
grep -q 'memset(g_ScenarioData.zombies.infected, 0,' "$INC" || fail 'infected roles carry between rounds'
# The infected chase at sprint's 1.5 as their base, seats and sims alike,
# and the sprint multiplier stands down for them so it never stacks to 2.25.
grep -q '#define ZOMBIE_SPEED_MULT  1.5f' "$SRC/include/constants.h" || fail 'zombie speed multiplier missing'
grep -q 'zombiesSeatIsInfected(g_Vars.currentplayernum)' "$SRC/game/bondwalk.c" || fail 'seat zombie speed not applied'
grep -q 'speedforwards \*= ZOMBIE_SPEED_MULT' "$SRC/game/bondwalk.c" || fail 'seats not sped up'
grep -q 'speed \*= ZOMBIE_SPEED_MULT' "$SRC/game/bot.c" || fail 'simulants not sped up'
grep -A6 'const bool zombieinfected' "$SRC/game/bondwalk.c" | grep -q '} else' || fail 'sprint stacks on the infected base'
# The role readers keep silent outside the mode: the arrays survive a switch.
[ "$(grep -c 'scenario != MPSCENARIO_ZOMBIES' "$INC")" -ge 5 ] || fail 'role readers not gated to the mode'
# Round 11: durations are 60ths of a second - TICKS(x) is x TICKS, not x
# seconds (the constant.h note beside SPRINT_RECOVERY60). A bare TICKS(5)
# countdown ran 1/12 of a second and flickered.
grep -q 'TICKS(ZOM_READY_SECONDS \* 60)' "$INC" || fail 'ready countdown not in real seconds'
grep -q 'TICKS(ZOM_NOTICE_SECONDS \* 60)' "$INC" || fail 'wake notice not in real seconds'
grep -q 'TICKS(ZOM_END_SECONDS \* 60)' "$INC" || fail 'round gap not in real seconds'
grep -q 'TICKS(180 \* 60)' "$INC" || fail 'round clock fallback not in real seconds'
! grep -q 'phasetime60 = TICKS(ZOM_READY_SECONDS);' "$INC" || fail 'ready countdown is 5 ticks, not 5 seconds'
# The opening swirl (no player control, camera only) must hold the countdown.
grep -q 'tickmode != TICKMODE_MPSWIRL' "$INC" || fail 'countdown not held during the opening swirl'
# Round 12: the countdown must actually hold everybody. Humans through the
# no-control pad path, sims through the strafe channel, the attacks and the
# one waypoint-walking choke point (chrGoToRoomPos - magic walk is solo-only,
# so this covers every step a match simulant can take).
grep -q '&& !zombiesCountdownActive()' "$SRC/game/player.c" || fail 'human seats not frozen during countdown'
grep -q '|| zombiesCountdownActive()' "$SRC/game/bot.c" || fail 'simulant strafe not frozen during countdown'
[ "$(grep -c '&& !zombiesCountdownActive()' "$SRC/game/bot.c")" -ge 1 ] || fail 'simulant attacks not gated during countdown'
grep -q 'if (zombiesCountdownActive()) {' "$SRC/game/chraction.c" || fail 'simulant walk not frozen during countdown'
grep -q 'bool zombiesCountdownActive(void);' "$SRC/include/game/mplayer/scenarios.h" || fail 'countdown helper not declared'
# Round 12: both roles are teams - survivor sims must not shoot each other
# either (the same chrCompareTeams blindness as the round-10 huddle).
grep -q 'zombiesIsInfected(chr1) == zombiesIsInfected(chr2)' "$SRC/game/chraction.c" || fail 'survivors not teamed in chrCompareTeams'
# Round 13: the simulant body swap round-trips - the mode keeps the body a
# sim came to the match in and hands it back when the round turns it back
# into a survivor (an early version only went INTO the Skedar body).
grep -q 'homebody\[MAX_MPCHRS\]' "$SRC/include/types.h" || fail 'no home-body slots in the mode state'
grep -q 'homebody\[i\] = -1' "$INC" || fail 'home-body slots not initialised'
[ "$(grep -c 'g_ScenarioData.zombies.homebody\[idx\]' "$INC")" -ge 2 ] || fail 'simulant body not round-tripped'
# Round 10: the infected punch, never disarm - both sources gated (the sim
# weapon scorer's function choice, and both player function readers).
grep -q 'weaponHost(weaponnum) == WEAPON_UNARMED' "$SRC/game/botinv.c" || fail 'sim unarmed function not role gated'
[ "$(grep -c 'zombiesSeatIsInfected(g_Vars.currentplayernum)' "$SRC/game/bondgun.c")" -ge 2 ] || fail 'player function readers not role gated'
# ...and the damage path backstops it: no disarm zeroing, no gun yank.
[ "$(grep -c 'zombiesIsInfected(aprop ? aprop->chr : NULL)' "$SRC/game/chraction.c")" -ge 3 ] || fail 'disarm backstops in chrDamage missing'
# The infected are a team to the sim AI, or they huddle punching each other
# forever (a free-for-all reads every chr as an enemy by default).
grep -q 'return checktype == COMPARE_FRIENDS;' "$SRC/game/chraction.c" || fail 'infected not teamed in chrCompareTeams'
# Pickups split by role: survivors take their dropped gun back, the infected
# never touch one, and the fetch decision agrees with the pick-up.
grep -q 'scenario != MPSCENARIO_ZOMBIES' "$SRC/game/propobj.c" || fail 'survivor floor pickup not re-allowed'
[ "$(grep -c 'obj->type == OBJTYPE_MULTIAMMOCRATE' "$SRC/game/bot.c")" -ge 2 ] || fail 'bot pickup role gate missing'
# A countdown before each round, and nobody can be hurt during it.
grep -q 'ZOM_PHASE_READY' "$INC" || fail 'round countdown phase missing'
grep -q '!zombiesRoundInProgress()' "$SRC/game/chraction.c" || fail 'countdown damage not gated'
# Radar: zombies hidden by default, red when the option is on, wired through
# the scenario's radar hook.
grep -q 'g_ZombiesRadarZombies = false' "$INC" || fail 'radar option not off by default'
grep -q 'g_TeamColours\[0\]' "$INC" || fail 'radar zombies not blipped in red'
grep -q 'zombiesRadarChr,' "$SRC/game/mplayer/scenarios.c" || fail 'radar hook not wired'
# textRender wants AARRGGBB; the radar's g_TeamColours is the other order,
# so the centre notices must use the text red, not the radar array.
grep -q 'infected ? ZOM_TEXT_RED' "$INC" || fail 'round notice not using a visible red'
# Role telling: the round countdown names the teams, and a converted seat
# wakes to the banner.
grep -q 'YOU ARE A SURVIVOR' "$INC" || fail 'survivor round notice missing'
grep -q 'YOU ARE INFECTED' "$INC" || fail 'infected round notice missing'
grep -q 'YOU HAVE BEEN INFECTED' "$INC" || fail 'conversion wake banner missing'
grep -q 'zombiesSeatWokeUp(g_Vars.currentplayernum);' "$SRC/game/player.c" || fail 'wake banner not fired on respawn'
[ "$(grep -c 'playerStartNewLife();' "$INC")" -eq 0 ] || fail 'round restart calls the respawn direct'
grep -q 'dostartnewlife = true' "$INC" || fail 'round restart does not ask the engine'
grep -q 'punchfunc' "$SRC/game/chraction.c" || fail 'simulant punch reads the Falcon damage'
pass 'mode rules'

# --- Falcon Punch ------------------------------------------------------------
grep -q 'Mod.FalconPunch' "$PORT/src/main.c" || fail 'config key not registered'
grep -q 'falconpunch' "$SRC/include/game/modoptions.h" || fail 'mod option field missing'
grep -q 'true,.*falconpunch' "$SRC/game/modoptions.c" || fail 'option not ticked by default'
grep -q 'Mod.FalconPunch.*NETKEY_SYNC' "$PORT/src/net/netrules.c" || fail 'net key not synced'
grep -q 'menuhandlerModFalconPunch' "$PORT/src/optionsmenu.c" || fail 'Dab Mod Options row missing'
grep -q 'menuhandlerMpFalconPunch' "$SRC/game/mplayer/setup.c" || fail 'Weapons menu row missing'
grep -q 'modIsFalconPunchEnabled' "$SRC/game/prop.c" || fail 'lock-on not gated'
grep -q 'meleelungeprop' "$SRC/include/types.h" || fail 'lunge state missing'
grep -q 'MVF3(forcespeed)' "$PORT/src/net/netsnap.c" || fail 'lunge movement not networked'
grep -q 'damage = 5.0f' "$SRC/game/game_0b0fd0.c" || fail 'punch damage not raised'
grep -q 'falconangle >' "$SRC/game/chraction.c" || fail 'behind KO not wired'
grep -q 'chr->aibot == NULL' "$SRC/game/chraction.c" || fail 'simulant behind KO not gated off sim-on-sim'
grep -q 'gset->weaponfunc == FUNC_PRIMARY && achr' "$SRC/game/chraction.c" || fail 'behind KO not gated off the disarm function'
grep -q 'modGhostTrialRulesApply' "$SRC/game/modoptions.c" || fail 'trial gate missing'
# One press is one lunge: the two-second lockout, armed at the swing and
# read where the hand enters the melee state machine.
grep -q 'ZOM_FALCON_COOLDOWN' "$SRC/game/prop.c" || fail 'cooldown constant missing'
grep -q 'meleecooldown60 = ZOM_FALCON_COOLDOWN' "$SRC/game/prop.c" || fail 'cooldown not armed at the swing'
grep -q 'meleecooldown60' "$SRC/include/types.h" || fail 'cooldown field missing'
grep -q 'g_Vars.currentplayer->meleecooldown60 > 0' "$SRC/game/bondgun.c" || fail 'cooldown gate missing'
# A match's seats and sims live in the mp chr pool, not the slot list.
grep -q 'g_MpAllChrPtrs' "$SRC/game/prop.c" || fail 'lock-on does not read the mp chr pool'
# The cone must look where the player is looking (bond2.unk1c points
# forward). Negating it lunged at targets behind and dashed backwards.
grep -q 'f32 forwardx = player->bond2.unk1c.x;' "$SRC/game/prop.c" || fail 'cone looks backwards'
grep -q 'f32 forwardz = player->bond2.unk1c.z;' "$SRC/game/prop.c" || fail 'cone looks backwards'
! grep -q 'forward dash' "$SRC/game/prop.c" || fail 'lunge still fires with no target'
# The fist swing is held back and started as the lunge lands, rather than
# playing out before the dash; a punch with no target still animates.
grep -q 'meleeFalconStartPunchAnim' "$SRC/game/prop.c" || fail 'punch anim helper missing'
grep -q 'func->fire_animation && !falconpunch' "$SRC/game/bondgun.c" || fail 'fist anim not held back'
grep -q 'meleefalconanimstarted' "$SRC/include/types.h" || fail 'anim timing state missing'
# Knockback must only touch the unarmed punch's primary function, never a
# disarm or another weapon.
grep -q 'ZOM_FALCON_KNOCKBACK 8.0f' "$SRC/game/chraction.c" || fail 'punch knockback missing'
grep -q 'sp80 = ZOM_FALCON_KNOCKBACK' "$SRC/game/chraction.c" || fail 'knockback not applied'
grep -q 'ZOM_FALCON_LAUNCH_ANGLE 30.0f' "$SRC/game/chraction.c" || fail 'launch angle missing'
grep -q 'BADDEG2RAD(ZOM_FALCON_LAUNCH_ANGLE)' "$SRC/game/chraction.c" || fail 'launch angle not applied'
! grep -q 'ZOM_FALCON_KNOCKBACK_TEST_MULT' "$SRC/game/chraction.c" || fail 'test multiplier still present'
pass 'Falcon Punch'

# --- NETSCEN ----------------------------------------------------------------
NS=$PORT/src/net/netscen.c
grep -q '#define ZOM_ROLE    (OFF_BODY + 0)' "$NS" || fail 'NETSCEN overlay not defined'
grep -q 'ZOM_TIME + 2 <= NETSCEN_SIZE' "$NS" || fail 'NETSCEN fit assert not extended'
[ "$(grep -c 'MPSCENARIO_ZOMBIES' "$NS")" -eq 2 ] || fail 'NETSCEN host/client cases missing'
grep -q 'NETZOM_PHASE_FIGHT ? d->roundtime60 : d->phasetime60' "$NS" || fail 'countdown clock not published'
grep -q 'g_ZombiesNetRound != oldround' "$NS" || fail 'client seats not respawned at a round turn'
pass 'NETSCEN overlay'

# --- headless ---------------------------------------------------------------
rm -rf "$OUT"
mkdir -p "$OUT"

run_mode() {
	local sims=$1
	local dir=$OUT/sims$sims

	mkdir -p "$dir"
	( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 90 "$BIN" \
		--savedir "$dir" --skip-intro --no-sound --boot-stage 0x32 \
		--mpsims "$sims" --mp-scenario 9 --exit-frame 180 ) > "$dir/log" 2>&1 \
		|| fail "scenario 9 with $sims sims crashed"
	grep -q 'exit-frame 180 reached' "$dir/log" || fail "scenario 9 with $sims sims did not finish"
}

run_mode 4
grep -q 'zombies: round 1 started, 2 of 5 infected' "$OUT/sims4/log" || fail '5 chrs did not start 2 infected'
pass 'headless 5 chrs'

run_mode 8
grep -q 'zombies: round 1 started, 3 of 9 infected' "$OUT/sims8/log" || fail '9 chrs did not start 3 infected'
pass 'headless 9 chrs'

# Falcon, headless: --falcon-debug holds the trigger in (bondmove.c). The
# arena's simulants do not normally line up in front of a stationary seat,
# so a clean run must show no dash at all: no target means a plain punch.
# If a lock does occur, consecutive dashes must still be ~121 frames apart
# (120 cooldown + the swing); a missing lockout would fire every ~21.
rm -rf "$OUT/falcon"
mkdir -p "$OUT/falcon"
( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 90 "$BIN" \
	--savedir "$OUT/falcon" --skip-intro --no-sound --boot-stage 0x32 \
	--mpsims 3 --mp-scenario 9 --rng-seed 7 --falcon-debug 1 --exit-frame 1300 ) \
	> "$OUT/falcon/log" 2>&1 || fail 'falcon headless run crashed'
grep -q 'exit-frame 1300 reached' "$OUT/falcon/log" || fail 'falcon headless run did not finish'
! grep -q 'forward dash' "$OUT/falcon/log" || fail 'lunge fired with no target in front'
grep -q 'fist anim' "$OUT/falcon/log" || fail 'a punch with no target did not animate'
gaps=$(grep -oE 'falcon: locked at frame [0-9]+' "$OUT/falcon/log" | grep -oE '[0-9]+$' \
	| awk 'NR > 1 {d = $1 - p; if (d < 100 || d > 140) bad++} {p = $1} END {print bad + 0}')
[ "$gaps" -eq 0 ] || fail "$gaps dash starts landed inside the two-second lockout"
pass 'falcon lock-only + cooldown headless'

echo "zombies: all checks passed"
