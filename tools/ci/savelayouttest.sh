#!/bin/bash
# savelayouttest.sh - saves move into saves/ once; --savedir stays flat.
#
# The game used to write eeprom, setups and weapon files next to pd.ini. They
# now live in $V, which defaults to $S/saves but is the explicit --savedir
# folder itself, so gates and players choosing a folder keep their layout.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=$BUILD/pd.x86_64
OUT=${OUT:-/tmp/opencode/savelayouttest}

fail() { echo "savelayouttest: $*"; exit 1; }

[ -x "$BIN" ] || fail "build first ($BIN missing)"
[ -d "$BUILD/data" ] || fail "no data dir in $BUILD"

rm -rf "$OUT"
mkdir -p "$OUT/exe" "$OUT/flat"
cp "$BIN" "$OUT/exe/pd.x86_64"
ln -s "$BUILD/data" "$OUT/exe/data"

# an old, flat save: config and save files in one directory
printf '[config]\n' > "$OUT/exe/pd.ini"
for f in eeprom.bin mpsetups.bin mpweaponsets.bin mpweaponfilters.bin; do
	: > "$OUT/exe/$f"
done
mkdir -p "$OUT/exe/ghosts" "$OUT/exe/modsaves/testmod"

(cd "$OUT/exe" && SDL_VIDEODRIVER=offscreen timeout 120 ./pd.x86_64 \
	--skip-intro --no-sound --exit-frame 2 > "$OUT/first.log" 2>&1) || true

grep -q 'saves: moved' "$OUT/first.log" || fail "no migration on a flat save: $(tail -3 "$OUT/first.log")"
grep -q 'save files: .*/saves' "$OUT/first.log" || fail '$V is not the saves folder'
[ -f "$OUT/exe/saves/eeprom.bin" ] || fail 'eeprom not moved into saves/'
[ -f "$OUT/exe/saves/mpsetups.bin" ] || fail 'setups not moved into saves/'
[ -f "$OUT/exe/saves/mpweaponsets.bin" ] || fail 'weapon sets not moved into saves/'
[ -d "$OUT/exe/saves/ghosts" ] || fail 'ghosts not moved into saves/'
[ -d "$OUT/exe/saves/modsaves/testmod" ] || fail 'mod saves not moved into saves/'
[ -f "$OUT/exe/pd.ini" ] || fail 'config moved out of the save root'
[ ! -e "$OUT/exe/eeprom.bin" ] || fail 'flat eeprom left behind'

# a second start must not move anything again or overwrite the new save
(cd "$OUT/exe" && SDL_VIDEODRIVER=offscreen timeout 120 ./pd.x86_64 \
	--skip-intro --no-sound --exit-frame 2 > "$OUT/second.log" 2>&1) || true
! grep -q 'saves: moved' "$OUT/second.log" || fail 'migration ran twice'
[ -f "$OUT/exe/saves/eeprom.bin" ] || fail 'save lost on the second start'

# an explicit --savedir is the save-files folder, not a parent of one
: > "$OUT/flat/eeprom.bin"
: > "$OUT/flat/mpsetups.bin"
mkdir -p "$OUT/flat/ghosts"
(cd "$OUT/exe" && SDL_VIDEODRIVER=offscreen timeout 120 ./pd.x86_64 \
	--savedir "$OUT/flat" --skip-intro --no-sound --exit-frame 2 > "$OUT/explicit.log" 2>&1) || true
[ -f "$OUT/flat/eeprom.bin" ] || fail 'explicit --savedir lost its eeprom'
[ ! -d "$OUT/flat/saves" ] || fail 'explicit --savedir was given a saves/ folder'
[ -d "$OUT/flat/ghosts" ] || fail 'explicit --savedir lost ghosts/'

echo 'savelayouttest: ok - flat saves migrate once, explicit --savedir stays flat'
