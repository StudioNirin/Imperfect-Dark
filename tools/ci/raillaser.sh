#!/bin/bash
# raillaser.sh - the Rail-Laser: its row, its number, its charge, and the width of a saved setup
#
# The fork's own gun (patchnotes 90): a Moonraker Laser's look, one shot a
# magazine out of five, a trigger held one and a half seconds to fire it, and
# the AR34's zoom on the aim input (invitems.c, port/src/raillaser.c). Its
# charge is the Mauler's machine - bgunTickMaulerCharge() widened to the new
# number by weaponChargesLikeMauler() - because the rising loop, the red build
# over the model and the shot's pitched-down voice all read the one field that
# machine fills, hand->matmot1.
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
# with the charge at its edges - a hair short of full the gun holds, at full it
# fires itself - and with the Combat Simulator's rows found where a saved setup
# names them.
#
# A third trap is worth a check for the same reason: hand->matmot1 is per-hand
# scratch, and two other writers come near this gun - the Moonraker's stream and
# the shotgun's starburst. Neither is asked for by this row, and either would
# fight the charge invisibly, so both are asserted (bondgun.c).
#
# What it does not reach is the feel. That the charge loop stops on a release
# and starts again at the bottom of its pitch, that the colour and the shot's
# voice land with the Mauler's full charge, that one and a half seconds is the
# right wait, and what the laser model looks like held up while it glows red:
# all of that is eyes and ears, and is checked by hand.
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
# forty of them skipped some on the way (the same reason the other gates
# count verdicts rather than trusting one line)
COUNT=$(grep -oE "raillaser test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 40 ]; then
	echo "raillaser: $COUNT checks ran, expected at least 40"
	exit 1
fi

# Nothing about this feature may write an error line
grep -E "^ERROR" "$LOG" && {
	echo "raillaser: an error line came out of a passing run"
	exit 1
}

echo "raillaser: as expected"
