#!/bin/bash
# bindfamilytest.sh - which of a command's bindings belongs to which device
#
# A command holds INPUT_MAX_BINDS bindings (port/src/input.c's
# binds[player][contkey][slots]) and pd.ini writes them one line comma-separated
# (CK_0100=LEFT_SHIFT, JOY1_LSTICK), so a command bound on both devices is one
# line with two names - which is what lets Sprint answer to Left Shift and to the
# left stick at once. Nothing records which slot is whose, so a page that shows
# one device has to ask, and ask right: taking the family's own slot is what makes
# rebinding a pad button replace it instead of adding a third binding, and taking
# an empty one is what lets a second device be bound without anybody choosing a
# slot number. Answering "slot 0" was the old bug's shape - a command held only on
# the pad read as unbound.
#
# The two functions under test take the four slots as an array, so the test builds
# its own and a player's real bindings are nowhere near it.
#
# --test-bindfamilies (main.c, inputBindsSelfTest) prints one line per check.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/bindfamilies-out}
LOG=$OUT/bindfamilytest.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-bindfamilies --exit-frame 60 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "bindfamilies" "$LOG" | sed 's/^.*bindfamilies/bindfamilies/'

FAILED=$(grep -c "bindfamilies test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "bindfamilies: $FAILED check(s) failed"
	exit 1
fi

grep -q "bindfamilies test: all .* checks as expected" "$LOG" || {
	echo "bindfamilies: the test never reported"
	exit 1
}

echo "bindfamilies: as expected"
