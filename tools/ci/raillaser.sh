#!/bin/bash
# raillaser.sh - the Rail-Laser: its row, its number, its charge, and the width of a saved setup
#
# The fork's own gun (patchnotes 90): a Moonraker Laser's look, one shot a
# magazine out of five, a trigger held one and a half seconds to fire it, and
# the AR34's zoom on the aim input (invitems.c, port/src/raillaser.c).
#
# Two of its three traps are invisible from the row, which is what the gate is
# for. GoldenEye's block used to run to the end of the weapon enum, so a number
# appended to it was read as a 42nd GoldenEye gun - g_GeWeaponHosts[] indexed
# one past itself, and a segfault in langAddPortText() on the way out of a
# constructor (constants.h anchors NUM_GE_WEAPONS, data.h bounds WEAPON_IS_GE).
# And the curated random weapon list is one bit per row in a saved setup, so a
# row appended to g_MpWeapons[] moves every field written after it by one bit,
# which is why MPSETUP_VERSION_RAILLASER exists and why a version 5 file is
# still read at 81 bits (mplayer.c). Both are pinned here by a literal, along
# with the charge at its edges - one tick short of it the gun holds - and with
# the Combat Simulator's rows found where a saved setup names them.
#
# What it does not reach is the feel: that the gun does not fire while it fills,
# that letting the trigger go costs only the holding, and that a press fires one
# shot and waits for another, are the gate's checks on raillaserCharging(), the
# helper bondgun.c asks. The look of it is checked by hand.
#
# --test-raillaser (main.c, raillaserSelfTest) puts the curated list back
# afterwards and writes no files of its own: the run is on its own savedir.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/raillaser-out}
LOG=$OUT/raillaser.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-raillaser --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "raillaser test" "$LOG" | sed 's/^.*raillaser test/raillaser test/'

FAILED=$(grep -c "raillaser test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "raillaser: $FAILED check(s) failed"
	exit 1
fi

grep -q "raillaser test: all .* checks as expected" "$LOG" || {
	echo "raillaser: the test never reported"
	exit 1
}

# The checks are the feature's whole reach; a run that reported fewer than the
# thirty of them skipped some on the way (the same reason the other gates
# count verdicts rather than trusting one line)
COUNT=$(grep -oE "raillaser test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 28 ]; then
	echo "raillaser: $COUNT checks ran, expected at least 30"
	exit 1
fi

# Nothing about this feature may write an error line
grep -E "^ERROR" "$LOG" && {
	echo "raillaser: an error line came out of a passing run"
	exit 1
}

echo "raillaser: as expected"
