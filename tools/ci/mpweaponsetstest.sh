#!/bin/bash
# mpweaponsetstest.sh - the weapon sets a player saves, and the list they go into
#
# The store (port/src/mpweaponsets.c) keeps a saved set as its six GUNS, not as
# the six row numbers the weapons page shows: the rows a mod adds come between
# the stock guns and Disabled, so a row number means another gun on another
# machine (mpWeaponSetsSave). The sets go into the Combat Simulator's list as a
# block that never moves and never changes size, because the sets a mod borrows
# and GoldenEye's record where they put themselves and put nothing after
# themselves; the rows held for sets that were never saved are hidden by the
# same unearnable requirement that hides a gun a player has not earned, so
# nothing after the block shifts when a set is saved or deleted.
#
# --test-mpweaponsets (main.c, mpWeaponSetsSelfTest) runs the checks at startup
# against $S/mpweaponsets-test.bin, so a run never touches a player's own sets.
# The game exits after a second of frames and the log is what is judged.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/mpweaponsets-out}
LOG=$OUT/mpweaponsetstest.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 240 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-mpweaponsets --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "mpweaponsets" "$LOG" | sed 's/^.*mpweaponsets/mpweaponsets/'

FAILED=$(grep -c "mpweaponsets test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "mpweaponsets: $FAILED check(s) failed"
	exit 1
fi

grep -q "mpweaponsets test: all .* checks as expected" "$LOG" || {
	echo "mpweaponsets: the test never reported"
	exit 1
}

ls "$OUT/save" | sed 's/^/     file: /'
test ! -e "$OUT/save/mpweaponsets-test.bin" || {
	echo "mpweaponsets: the scratch file outlived the test"
	exit 1
}
echo "mpweaponsets: as expected"
