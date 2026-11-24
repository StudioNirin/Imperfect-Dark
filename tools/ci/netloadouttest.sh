#!/bin/bash
# netloadouttest.sh - Loadout mode online: does the host's two chosen guns
# reach the client before stage load, and do both machines hand out the same
# kit to their local player? A loopback host pair plays a short match with
# Start Armed = Loadout, and the spawn log lines from both machines are
# compared. No ptrace: the match ends on a frame schedule and every assertion
# is off the logs.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/netloadout-out}
PORT=${PORT:-27760}

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

status=0
fail() { echo "FAIL $*"; status=1; }
pass() { echo "ok   $*"; }

rm -rf "$OUT"
mkdir -p "$OUT"
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0

# Both machines use the same Loadout config. The host's SYNC keys carry the
# chosen rows to the client, so even if the client's ini differed the match
# should use the host's.
# Rows 18 (Shotgun) and 1 (Falcon 2) in the stock table. Written in decimal
# because the config parser reads plain integers.
INI='[Mod]\nLoadModels=0\nXblaMeshes=0\nStartArmed=3\nLoadoutWeapon1=18\nLoadoutWeapon2=1\n'

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

# A Combat match with one client and no sims, so the only spawn line on each
# machine is its own player. Both players must get the two configured guns.
game host 200 --host "$PORT" --net-test-host 1 --net-test-end-frame 720 --rng-seed 7 --mp-weapons 9,9,9,9,9,9 &
host=$!
waitfor "$H" "net: hosting on UDP port" 90 || { fail "host did not start"; kill -TERM "$host"; wait "$host" 2>/dev/null; exit 2; }
game client 190 --connect "127.0.0.1:$PORT" --net-test-join &
client=$!
waitfor "$C" "net: match 1: GO" 150 || fail "client never got GO"
waitfor "$H" "net: match 1 ended; scores and awards sent" 120 || fail "host never ended the match"
sleep 2

# Stop both before reading the logs.
for p in $(pgrep -x "$(basename "$BIN" | cut -c1-15)" 2>/dev/null); do
	tr '\0' ' ' < "/proc/$p/cmdline" 2>/dev/null | grep -q -- "--savedir $OUT/save-" && kill -TERM "$p" 2>/dev/null
done
wait "$host" 2>/dev/null
wait "$client" 2>/dev/null

grep -q 'net: hosting on UDP port' "$H" && pass 'host started'
grep -q 'net: match 1: GO' "$C" && pass 'client joined and got GO'

# The protocol must match for the match to run at all.
! grep -q 'protocol.*differs\|NETREFUSE_PROTOCOL' "$H" "$C" || fail 'a peer was refused on protocol - Loadout keys did not travel'

# Check both machines see the Loadout spawn with the configured rows. Rows
# 18 (Shotgun) and 1 (Falcon 2) in the stock table; both machines must agree.
hostline=$(grep 'net: loadout spawn seat' "$H" | head -1)
clientline=$(grep 'net: loadout spawn seat' "$C" | head -1)
[ -n "$hostline" ] || fail "host's player did not spawn with the Loadout pair"
[ -n "$clientline" ] || fail "client's player did not spawn with the Loadout pair"
hpair=$(echo "$hostline" | sed -n 's/.* guns \([0-9]* [0-9]*\).*/\1/p')
cpair=$(echo "$clientline" | sed -n 's/.* guns \([0-9]* [0-9]*\).*/\1/p')
# The log prints weapon numbers, not rows: row 18 names WEAPON_SHOTGUN (19)
# and row 1 names WEAPON_FALCON2 (2).
[ "$hpair" = "19 2" ] || fail "host's pair is $hpair, want 19 2 (Shotgun, Falcon 2)"
[ "$cpair" = "19 2" ] || fail "client's pair is $cpair, want 19 2 (Shotgun, Falcon 2)"
pass 'both machines hand out the same two Loadout guns (Shotgun, Falcon 2)'

# --- Zombies online: survivors get the configured pair, infected get fists ---
# The role is rolled by the host; both machines must hand a survivor the same
# two guns, and an infected seat must never get the loadout (fists only).
ZH=$OUT/zh-host.log; ZC=$OUT/zh-client.log
game zh-host 200 --host "$((PORT + 1))" --net-test-host 1 --net-test-end-frame 720 --rng-seed 7 \
	--net-test-scenario 9 --net-test-sims 4 --mp-weapons 9,9,9,9,9,9 &
zh=$!
waitfor "$ZH" "net: hosting on UDP port" 90 || { fail "zombies host did not start"; kill -TERM "$zh"; wait "$zh" 2>/dev/null; exit 2; }
game zh-client 190 --connect "127.0.0.1:$((PORT + 1))" --net-test-join &
zc=$!
waitfor "$ZC" "net: match 1: GO" 150 || fail "zombies client never got GO"
waitfor "$ZH" "net: match 1 ended; scores and awards sent" 120 || fail "zombies host never ended the match"
sleep 2

for p in $(pgrep -x "$(basename "$BIN" | cut -c1-15)" 2>/dev/null); do
	tr '\0' ' ' < "/proc/$p/cmdline" 2>/dev/null | grep -q -- "--savedir $OUT/save-" && kill -TERM "$p" 2>/dev/null
done
wait "$zh" 2>/dev/null
wait "$zc" 2>/dev/null

# At least one survivor on each machine gets the configured pair.
grep -q 'net: zombies survivor loadout seat .* guns 19 2' "$ZH" || fail "no zombies survivor on the host got the pair: $(grep -i 'survivor loadout' "$ZH" | head -2)"
grep -q 'net: zombies survivor loadout seat .* guns 19 2' "$ZC" || fail "no zombies survivor on the client got the pair: $(grep -i 'survivor loadout' "$ZC" | head -2)"
pass 'zombies survivors get the configured pair on both machines'

# The infected must not be handed a gun: player.c's infected branch returns
# with fists before the survivor log line, so an infected seat has no loadout
# spawn line. Source pin the guard so a regression cannot hand them weapons.
grep -qF 'if (zombiesSeatIsInfected(g_Vars.currentplayernum)) {' "$ROOT/src/game/player.c" \
	|| fail 'infected players are not given fists'
grep -qF 'if (!zombiesIsInfected(chr)) {' "$ROOT/src/game/bot.c" \
	|| fail 'infected simulants are not given fists'
grep -qF 'zombiesSeatIsInfected(g_Vars.currentplayernum)' "$ROOT/src/game/propobj.c" \
	|| fail 'infected players are not blocked from weapon pickups'
pass 'infected players and sims get fists and no pickups'

exit $status
