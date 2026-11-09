#!/bin/bash
# netlobbysimtest.sh — a live room's Simulants pages still answer
#
# The bug (the user, 2026-11-09): with the room not made yet, Room Setup's
# Simulants page and its Add/Edit pages are the fifth and sixth pages of the
# menu stack and open fine. Once the room is live the Game Lobby stays stacked
# under Room Settings (lobbyNameEntryUp() reads that stack to find its way
# back), so the same chain is one deeper and menuPushDialog() dropped the Add
# Simulant and Edit Simulant pushes with no sound and no word: dead buttons.
#
# One game offscreen on the GPU, no lobby server: a LAN room made by
# --net-lobby-script host with no joiner, which sits in the Game Lobby waiting
# for a READY that never comes. --net-test-lobby-simulants (netlobbymenu.c)
# then walks the chain with the buttons' own handlers and logs the menu depth
# at every page:
#
#   lobby sim test: the Game Lobby is up at depth N
#   lobby sim test: Room Setup is up at depth N
#   lobby sim test: Simulants is up at depth N          (screenshotted)
#   lobby sim test: Add Simulant is up at depth N
#   lobby sim test: Edit Simulant is up at depth N      (screenshotted)
#   lobby sim test: Simulant Character is up at depth N   (screenshotted)
#   lobby sim test: PASS every page of the Simulants chain opened
#
# A page that was refused logs FAIL ... was refused at depth N (and, from the
# menuPushDialog() refusal log on, why).
#
#   netlobbysimtest.sh [BIN]   BIN a file name in build/ (pd.x86_64) or a path
#
# Env: OUT (build/netlobbysim-out), GPORT (27410), MODDIR (mod_allinone).
# Screenshots: build/net-shots/lobby-{simulants,simulant-edit,simulant-character,simulant-last}.png.
# Exit status: 0 all good, 1 a check failed, 2 a run failed to start.
set -u
ROOT=$(cd "$(dirname "$0")"/../.. && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/netlobbysim-out}; GPORT=${GPORT:-27410}
MODDIR=${MODDIR:-mod_allinone}
SHOTDIR=$BUILD/net-shots
BIN=${1:-pd.x86_64}
case $BIN in /*) ;; */*) BIN=$(realpath "$BIN") ;; *) BIN=$BUILD/$BIN ;; esac
mkdir -p "$OUT" "$SHOTDIR" "$BUILD/screenshots"; OUT=$(cd "$OUT" && pwd)
[ -n "$OUT" ] && [ "$OUT" != / ] && rm -f "$OUT"/*.log
rm -f "$SHOTDIR"/lobby-simulant*.png
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0

status=0
fail() { echo "FAIL $*"; status=1; }
pass() { echo "ok   $*"; }

waitfor() {
	local log=$1 pattern=$2 n=$3
	while [ "$n" -gt 0 ]; do
		grep -q -- "$pattern" "$log" 2>/dev/null && return 0
		sleep 1; n=$((n - 1))
	done
	return 1
}

# game LABEL TIMEOUT ARGS... &  (a LAN room of its own: Net.Port, no lobby)
game() {
	local label=$1 t=$2; shift 2
	local save=$OUT/save-$label
	rm -rf "$save"; mkdir -p "$save"
	for f in eeprom.bin mpsetups.bin; do
		[ -f "$HOME/.local/share/perfectdark/$f" ] && cp "$HOME/.local/share/perfectdark/$f" "$save/"
	done
	printf '[Net]\nPort=%s\n' "$GPORT" > "$save/pd.ini"
	cd "$BUILD" || exit 2
	exec timeout -k 5 "$t" stdbuf -oL -eL "$BIN" --savedir "$save" --skip-intro --no-sound --moddir "$MODDIR" "$@" > "$OUT/$label.log" 2>&1
}

# shot LOG PATTERN DEST: the PNG the log's screenshot line names after PATTERN
shot() {
	local f
	f=$(sed -n "/$2/,\$p" "$1" | sed -n 's/.*screenshot: \(.*\.png\).*/\1/p' | head -1)
	[ -n "$f" ] && [ -f "$f" ] && cp "$f" "$3" && { pass "screenshot $3"; return; }
	[ -n "$f" ] && [ -f "$BUILD/$f" ] && cp "$BUILD/$f" "$3" && { pass "screenshot $3"; return; }
	fail "no screenshot after '$2' in $1"
}

H=$OUT/host.log
game host 120 --net-lobby-script host --net-lobby-mode lan --net-lobby-name Hosty \
	--net-lobby-room "Sim Test" --net-test-lobby-simulants --net-test-sims 2 &
P1=$!
if ! waitfor "$H" "lobby sim test: " 60; then
	echo "FAIL: the sim test never started (see $H)"
	kill $P1 2>/dev/null; exit 2
fi
wait $P1; r1=$?
echo "     exits: host $r1"

grep -q "renderD128: Permission denied" "$H" && fail "the run fell back to llvmpipe"

# the whole chain, page by page: a page missing is the bug back again
for what in "the Game Lobby is up at depth" "Room Setup is up at depth" "Simulants is up at depth" \
	"Add Simulant is up at depth" "Edit Simulant is up at depth" "Simulant Character is up at depth"; do
	grep -q -- "$what" "$H" && pass "host: $(grep -m1 -o -- "$what.*" "$H")" || fail "host: no \"$what\" (the page was refused)"
done

grep -q "lobby sim test: FAIL" "$H" && fail "host: $(grep -m1 'lobby sim test: FAIL' "$H")"

# the depths, so the log says how deep the stack got (the cap menuPushDialog
# refuses at is what made these pages dead: MENU_MAX_LAYERS in types.h)
grep -q "lobby sim test: PASS every page of the Simulants chain opened" "$H" \
	&& pass "host: every page of the chain opened" || fail "host: the chain did not finish"
echo "     depths: $(grep -o 'at depth [0-9]*' "$H" | tr '\n' ' ')"
grep -q "menu: push of dialog .* refused" "$H" && fail "host: menuPushDialog refused a page: $(grep -m1 'menu: push of dialog' "$H")"

shot "$H" "shot the room's Simulants page" "$SHOTDIR/lobby-simulants.png"
shot "$H" "shot the simulant's own page" "$SHOTDIR/lobby-simulant-edit.png"
shot "$H" "shot the simulant's character page" "$SHOTDIR/lobby-simulant-character.png"
shot "$H" "shot the page that is up" "$SHOTDIR/lobby-simulant-last.png"

exit $status
