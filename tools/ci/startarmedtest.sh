#!/bin/bash
# startarmedtest.sh - what Start Armed hands out, in one hand or two, and with how much
#
# Two settings make the four ways to start armed (patchnotes 88): Start Armed says
# which gun - the first slot's, or a fresh roll each spawn - and Akimbo Start? says
# whether it comes twice. The pair could not stay inside Mod.Akimbo, which also
# decides what a player may dual-wield off the floor and which simulants and guards
# are handed a pair, so it is Mod.StartArmedAkimbo, and the row hides when Start
# Armed is Off without the key going with it - a mission's own starting gun is
# doubled by that same key while the row is nowhere to be seen (player.c).
#
# The ammunition is asked of the rule, not of a table read back: asked the lazy way
# - is the answer within the carrying limit - a helper that just returned the
# capacity for every row would pass. So the checks ask that either First option
# comes out under the capacity for some row, and that the three mines come out at
# their hand-out rather than doubled (bondgun.c has the reason mines are the
# exception, and setup.c's mpStartArmedSelfTest its verdicts).
#
# --test-startarmed (main.c, mpStartArmedSelfTest) puts the g_ModOptions values it
# borrowed back afterwards, and touches no files: the run is on its own savedir all
# the same, so a crash cannot reach a player's pd.ini.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/startarmed-out}
LOG=$OUT/startarmedtest.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-startarmed --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "startarmed test" "$LOG" | sed 's/^.*startarmed test/startarmed test/'

FAILED=$(grep -c "startarmed test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "startarmed: $FAILED check(s) failed"
	exit 1
fi

grep -q "startarmed test: all .* checks as expected" "$LOG" || {
	echo "startarmed: the test never reported"
	exit 1
}

# The checks are the feature's whole reach; a run that reported fewer than six of
# them skipped some on the way (the same reason randomgroupstest.sh counts verdicts
# rather than trusting one line)
COUNT=$(grep -oE "startarmed test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 6 ]; then
	echo "startarmed: $COUNT checks ran, expected at least 6"
	exit 1
fi

# Nothing about this feature may write an error line
grep -E "^ERROR" "$LOG" && {
	echo "startarmed: an error line came out of a passing run"
	exit 1
}

echo "startarmed: as expected"
