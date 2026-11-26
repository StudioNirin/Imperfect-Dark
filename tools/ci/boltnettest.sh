#!/bin/bash
# boltnettest.sh - Bowcaster bolts online: a client fires and its beam is drawn.
#
# A loopback host gives both seats a Bowcaster (0x88) and the client holds fire
# for a stretch. The bolt is host-authoritative; the client draws it from
# snapshot poses. Its whole visible shot is the glow beam, so the puppet log's
# "bolt beams" count must climb: with the old six-frame throttle and short
# trail the shot stuttered, and with a camera/matrix mismatch the host's bolt
# kicked vertically on the first tick in a passage. No ptrace, all off logs.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/boltnet-out}
PORT=${PORT:-27810}

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
printf '200 1100 0x2000 0 0 0 0 0 0\n' > "$OUT/client.script"

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

game host 220 --host "$PORT" --net-test-host 1 --net-test-stage 0x32 --net-test-end-frame 1200 \
	--rng-seed 7 --net-test-give 200,0x88 --net-test-give-host --log &
host=$!
waitfor "$H" "net: hosting on UDP port" 90 || { fail "host did not start"; kill -TERM "$host"; wait "$host" 2>/dev/null; exit 2; }
game client 210 --connect "127.0.0.1:$PORT" --net-test-join --net-test-input "$OUT/client.script" --log &
client=$!
waitfor "$C" "net: match 1: GO" 150 || fail "client never got GO"
waitfor "$C" "bolt beams" 90 || fail "client never logged a Bowcaster beam update"
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

grep -q 'net: --net-test-give: slot 1 given guns 0x88/0x00' "$H" \
	&& pass 'the host gave the client a Bowcaster' \
	|| fail "the host's give never happened: $(grep -i -- '--net-test-give' "$H" | head -2)"

line=$(grep 'bolt beams' "$C" | tail -1)
count=$(echo "$line" | sed -n 's/.*bolt beams \([0-9]*\).*/\1/p')
if [ -n "$count" ] && [ "$count" -gt 0 ]; then
	pass "the client drew $count Bowcaster beam updates"
else
	fail "the client drew no Bowcaster beams: ${line:-no puppet log}"
fi

# The host's collision normal must come from the camera that posed each model,
# not from whichever seat was rendered last.
grep -qF 'projectileHitCamera(obj->model, &w2s, &c2w);' "$ROOT/src/game/propobj.c" \
	|| fail 'prop collisions still use the current camera for another view matrices'
grep -qF 'projectileHitCamera(chr->model, &w2s, &c2w);' "$ROOT/src/game/propobj.c" \
	|| fail 'chr collisions still use the current camera for another view matrices'
grep -qF 'cam0f0b5050((u8 *) model->matrices)' "$ROOT/src/game/propobj.c" \
	|| fail 'the helper does not recover the camera that posed the model'
grep -qF '(g_Vars.lvframenum - obj->projectile->unk0d8) > 0' "$ROOT/src/game/propobj.c" \
	|| fail 'a bolt can bounce off a passage frame on its first tick'
grep -qF 'const f32 trail = prop->weapon->weaponnum == WEAPON_SHOCKORB ? 10.0f : 180.0f;' "$ROOT/port/src/net/netpuppets.c" \
	|| fail 'the client beam no longer matches the host trail'

exit $status
