#!/bin/bash
# customsims.sh - pin the wiring of Boomstick, Vulture and Surge, then play a
# short seeded match with one of each. Needs the ROM in build/data and gdb.
#
#   tools/ci/customsims.sh [bin]
#
# BIN=pd.x86_64 (a name in build/, or a path), SEED=7, FRAMES=1800,
# BUILD=<dir> can override the build directory. This gate is standalone: run
# it by hand after touching the simulant roster or the bot AI.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${1:-${BIN:-pd.x86_64}}
SEED=${SEED:-7}
FRAMES=${FRAMES:-1800}
OUT=$BUILD/customsims-out
mkdir -p "$OUT"
rm -rf "$OUT/save"; mkdir -p "$OUT/save"

fail() { echo "FAIL customsims: $1"; exit 1; }
pin() {
	grep -q "$1" "$ROOT/$2" || fail "missing $1 in $2"
}

# The parts a new BOTTYPE does not get for free.
pin '#define BOTTYPE_BOOMSTICK' src/include/constants.h
pin '#define BOTTYPE_VULTURE' src/include/constants.h
pin '#define BOTTYPE_SURGE' src/include/constants.h
pin 'g_BotProfiles\[21\]' src/include/data.h
pin 'BOTTYPE_BOOMSTICK.*MPBODY' src/game/mplayer/mplayer.c
pin 'BOTTYPE_VULTURE.*MPBODY' src/game/mplayer/mplayer.c
pin 'BOTTYPE_SURGE.*MPBODY' src/game/mplayer/mplayer.c
pin 'mpInitSimulantText' src/game/mplayer/mplayer.c
pin 'g_BotProfiles\[g_Menus\[g_MpPlayerNum\].mpsetup.unke24\].description' src/game/mplayer/setup.c
pin 'sim->type > BOTTYPE_SURGE' port/src/net/netrules.c
pin 'BOTTYPE_BOOMSTICK' src/game/botinv.c
[ "$(grep -c 'botPassesVultureCheck(botchr' "$ROOT/src/game/bot.c")" -ge 3 ] \
	|| fail 'vulture target check not wired into all three selection sites'
[ "$(grep -c 'botGetSkillValue(chr' "$ROOT/src/game/bot.c")" -ge 6 ] \
	|| fail 'surge skill values not wired into all aim and reaction reads'

case $BIN in
	/*) ;;
	*/*) BIN=$(realpath "$BIN") ;;
	*) BIN=$BUILD/$BIN ;;
esac

log=$OUT/customsims.log
( cd "$BUILD" && SDL_VIDEODRIVER=offscreen SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0 \
	timeout -k 5 900 gdb -q -batch -x "$ROOT/tools/ci/customsims.gdb" --args "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --boot-stage 0x32 --mpsims 4 \
	--spectate --endless --rng-seed "$SEED" --fixed-step --exit-frame "$FRAMES" ) > "$log" 2>&1

grep -q 'CUSTOMSIM types 13 14 15' "$log" || fail "types not patched (see $log)"
# The exit-frame check happens before lvTick at that frame, so the last
# periodic line is earlier than FRAMES; ask for one comfortably inside it.
grep -q "CUSTOMSIM f$((FRAMES / 3))" "$log" || fail "no periodic simulant log (see $log)"
grep -q "exit-frame $FRAMES reached" "$log" || fail "game did not exit cleanly (see $log)"

echo "PASS customsims: three new types played to frame $FRAMES"
