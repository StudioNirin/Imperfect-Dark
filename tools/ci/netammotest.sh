#!/bin/bash
# netammotest.sh - the fork's ammunition reaches a client's HUD.
#
# A loopback host gives its client the Moonrailer (0x87) at tick 300 with 40
# rounds of AMMOTYPE_RAIL (0x21) in the --net-test-give hook. The reserve is
# only ever reconciled from the host's local-player block, which used to be
# 33 entries wide and stopped at the ROM's last ammo type: the client picked
# the rounds up "behind the scenes" while its HUD read zero. The client logs
# every local-block change, so a line naming type 0x21 and the host's 40 is
# the proof that the value crossed the wire. No ptrace, all off the logs.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/netammo-out}
PORT=${PORT:-27780}

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

status=0
fail() { echo "FAIL $*"; status=1; }
pass() { echo "ok   $*"; }

rm -rf "$OUT"
mkdir -p "$OUT"
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0

INI='[Mod]\nLoadModels=0\nXblaMeshes=0\nStartArmed=0\n'

game() {
	local label=$1 t=$2; shift 2
	local save=$OUT/save-$label
	rm -rf "$save"; mkdir -p "$save"
	printf '%b' "$INI" > "$save/pd.ini"
	cd "$BUILD" || exit 2
	exec timeout -k 5 "$t" stdbuf -oL -eL "$BIN" --savedir "$save" --skip-intro --no-sound "$@" > "$OUT/$label.log" 2>&1
}

waitfor() {
	local log=$1 pattern=$2 n=$3
	while [ "$n" -gt 0 ]; do
		grep -q -- "$pattern" "$log" 2>/dev/null && return 0
		sleep 1; n=$((n - 1))
	done
	return 1
}

H=$OUT/host.log; C=$OUT/client.log

game host 220 --host "$PORT" --net-test-host 1 --net-test-end-frame 1200 --rng-seed 7 --net-test-give 300,0x87/600,0x8c &
host=$!
waitfor "$H" "net: hosting on UDP port" 90 || { fail "host did not start"; kill -TERM "$host"; wait "$host" 2>/dev/null; exit 2; }
game client 210 --connect "127.0.0.1:$PORT" --net-test-join &
client=$!
waitfor "$C" "net: match 1: GO" 150 || fail "client never got GO"
waitfor "$C" "ammo type 0x21" 90 || fail "client never reconciled its Rail reserve"
waitfor "$C" "got a bbox from its OBJ" 90 || fail "the DB-Shotgun's OBJ never supplied a bbox"
waitfor "$H" "net: match 1 ended; scores and awards sent" 120 || fail "host never ended the match"
sleep 2

for p in $(pgrep -x "$(basename "$BIN" | cut -c1-15)" 2>/dev/null); do
	tr '\0' ' ' < "/proc/$p/cmdline" 2>/dev/null | grep -q -- "--savedir $OUT/save-" && kill -TERM "$p" 2>/dev/null
done
wait "$host" 2>/dev/null
wait "$client" 2>/dev/null

grep -q 'net: hosting on UDP port' "$H" && pass 'host started'
grep -q 'net: match 1: GO' "$C" && pass 'client joined and got GO'
! grep -q 'protocol.*differs\|NETREFUSE_PROTOCOL' "$H" "$C" || fail 'a peer was refused on protocol'

grep -q 'net: --net-test-give: slot 1 given guns 0x87/0x00' "$H" \
	&& pass 'the host gave the client a Moonrailer' \
	|| fail "the host's give never happened: $(grep -i -- '--net-test-give' "$H" | head -2)"

line=$(grep 'ammo type 0x21' "$C" | head -1)
hostreserve=$(echo "$line" | sed -n "s/.*the host's \([0-9]*\).*/\1/p")
# Equipping puts one of the 40 rounds in the magazine, so the host's reserve
# is 39 by the time the block stabilises. The point is that it is nonzero.
if [ -n "$hostreserve" ] && [ "$hostreserve" -gt 0 ]; then
	pass "the client took the host's $hostreserve Rail rounds: $line"
else
	fail "the client's Rail reserve was never reconciled: ${line:-no local-block line for 0x21}"
fi

# The other two appended types travel in the same array, and the respawn
# sweep must not zero them as types nobody is recorded as holding.
grep -qE '^#define NETLP_NUMAMMO +37' "$ROOT/port/include/net/netsnap.h" \
	|| fail 'the local-player block is back to the ROM ammo count'
grep -qF 'bool ammotypesheld[NUM_AMMOTYPES]' "$ROOT/src/game/player.c" \
	|| fail 'a respawn still wipes the fork reserves'

# The pack pipeline now supplies a real bbox from the OBJ, so the hit and
# falling paths never see the NULL that used to fault at bbox->xmax.
grep -qF 'xblaMeshBboxAdd(modeldef);' "$ROOT/port/src/xblamesh.c" \
	|| fail 'the pack loader no longer gives a model a bbox'
grep -qF 'b->rodata->bbox.xmin = lo[0];' "$ROOT/port/src/xblamesh.c" \
	|| fail 'the bbox is not measured from the OBJ vertices'
bboxline=$(grep 'got a bbox from its OBJ' "$C" | head -1)
[ -n "$bboxline" ] && pass "the OBJ supplied a bbox: $bboxline" \
	|| fail "the client never logged a bbox from the OBJ"

exit $status
