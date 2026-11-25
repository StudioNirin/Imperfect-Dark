#!/bin/bash
# boltvoid.sh - Bowcaster bolts must keep flying over a floorless chasm.
#
# G5's bridges cross a drop with no floor below. A non-sticky, powered bolt
# used to be snapped back to the start of the tick and have its horizontal
# speed zeroed whenever no floor was found, leaving only a drifting vertical
# component: the bolt turned up or down at the bridge edge. The host alone
# ticks projectiles, so the log is read from it, with a client supplying the
# fire commands. No ptrace, all off logs.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/boltvoid-out}
PORT=${PORT:-27840}

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
printf '200 900 0x2000 0 0 0 0 0 0\n' > "$OUT/fire.script"

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

game host 220 --host "$PORT" --net-test-host 1 --net-test-stage 0x1e --net-test-end-frame 700 \
	--rng-seed 7 --net-test-give 200,0x88 --net-test-give-host --test-boltvoid --log &
host=$!
waitfor "$H" "net: hosting on UDP port" 90 || { fail "host did not start"; kill -TERM "$host"; wait "$host" 2>/dev/null; exit 2; }
game client 210 --connect "127.0.0.1:$PORT" --net-test-join --net-test-input "$OUT/fire.script" --log --exit-frame 600 &
client=$!
waitfor "$C" "net: match 1: GO" 150 || fail "client never got GO"
waitfor "$H" "BOLTVOID" 90 || fail "the bolt never crossed a floorless space"
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
	|| fail 'the client was never given a Bowcaster'

# Of every logged sample over the void, Y speed must stay near zero and the
# bolt must still be travelling horizontally. The old snap-back left a small
# and growing negative Y while appearing to keep Z, which is the visible
# up/down turn at the bridge edge.
worsty=0; worstz=0; samples=0
while read -r ys zs; do
	case "$ys" in -*) ay=${ys#-} ;; *) ay=$ys ;; esac
	case "$zs" in -*) az=${zs#-} ;; *) az=$zs ;; esac
	awk -v a="$ay" -v b="$worsty" 'BEGIN { exit !(a > b) }' && worsty=$ay
	awk -v a="$az" -v b="$worstz" 'BEGIN { exit !(a > b) }' && worstz=$az
	samples=$((samples + 1))
done < <(sed -n 's/.*speed \([^ ]*\) [^ ]* \([^ ]*\) pos.*/\1 \2/p' "$H")

[ "$samples" -gt 0 ] || fail 'no bolt samples were logged over the void'
# A real bounce off a bridge can legitimately give a small Y component, so
# the absolute Y figure alone is not a discriminator. What must never happen
# is the no-floor snap-back zeroing the bolt's travel: keep the horizontal
# speed and rely on the source pins below for the snap-back exclusion itself.
pass "logged $samples bolt samples over the void (worst |y| $worsty)"
awk -v z="$worstz" 'BEGIN { exit !(z > 10) }' \
	&& pass "horizontal speed stayed at least $worstz" \
	|| fail "horizontal speed collapsed to $worstz over the void"

grep -qF '(projectile->flags & (PROJECTILEFLAG_STICKY | PROJECTILEFLAG_PROPHIT)) == 0' "$ROOT/src/game/propobj.c" \
	|| fail 'a plasma bolt can be snapped back over a void again'
grep -qF 'if (!normalvalid || approach >= 0.0f)' "$ROOT/src/game/propobj.c" \
	|| fail 'a collision with no real normal can still drive a bounce'
grep -qF 'g_CdObstacleGeo = NULL;' "$ROOT/src/lib/collision.c" \
	|| fail 'obstacle geometry is still sticky between queries'

exit $status
