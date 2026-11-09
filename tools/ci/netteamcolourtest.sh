#!/bin/bash
# netteamcolourtest.sh — the eight team colours as a room plays with them
#
# The fork's Game Lobby change (patch notes 69): a team room's members pick
# any of the eight colours from a picker instead of cycling Red and Yellow.
# Three offscreen clients against a local pdlobbyd (ephemeral ports, open
# auth):
#
#   host  makes "Eight Team Colours", Teams on, and asks for Magenta (3)
#   join  joins and asks for Orange (5)
#   spect joins as a spectator
#
# What is checked: the lobby grants each team ask on the first try (a repeat
# means it refused and asked again), both sides' team pickers are up over
# the Game Lobby and screenshotted (--net-test-lobby-team-dialog), the match
# reaches GO with the spectator watching, and the Game Lobby shots are named
# so a human can check the colours: the roster grouped "Magenta 1 Orange 1",
# the member's row "Team: Orange", the member's picker with a Spectate row
# and the host's without.
#
#   netteamcolourtest.sh [BIN]   BIN a file name in build/ (pd.x86_64) or a path
#
# Env: OUT (build/netteamcolour-out), GPORT (27193).
# Exit status: 0 all good, 1 a check failed, 2 a run failed to start.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/netteamcolour-out}; GPORT=${GPORT:-27193}
ROOM="Eight Team Colours"
BIN=${1:-pd.x86_64}
case $BIN in /*) ;; */*) BIN=$(realpath "$BIN") ;; *) BIN=$BUILD/$BIN ;; esac
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
rm -f "$OUT"/*.log
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

python3 -u "$ROOT/tools/pdlobbyd/pdlobbyd.py" --host 127.0.0.1 --port 0 --udp-host 127.0.0.1 --udp-port 0 --auth open --relay-ports 0 \
	> "$OUT/pdlobbyd.log" 2>&1 &
LOBBY=$!
if ! waitfor "$OUT/pdlobbyd.log" "pdlobbyd listening on" 20; then
	echo "FAIL: pdlobbyd did not start (see $OUT/pdlobbyd.log)"; kill $LOBBY 2>/dev/null; exit 2
fi
LPORT=$(sed -n 's/.*listening on 127.0.0.1:\([0-9]*\).*/\1/p' "$OUT/pdlobbyd.log" | head -1)
echo "     pdlobbyd on 127.0.0.1:$LPORT (pid $LOBBY)"

# game LABEL USER GAMEPORT TIMEOUT ARGS... &
game() {
	local label=$1 user=$2 port=$3 t=$4; shift 4
	local save=$OUT/save-$label run=$OUT/run-$label bin=$OUT/bin-$label
	rm -rf "$save" "$run" "$bin"; mkdir -p "$save" "$run" "$bin"
	ln -s "$BUILD/data" "$run/data"
	# the binary hard-linked into a directory of its own: fsChooseOutputDir
	# puts screenshots under $E (the binary's directory), so sharing build/
	# makes three processes overwrite one another's shots
	cp -l "$BIN" "$bin/pd.x86_64"
	printf '[Mod]\nGhostUser=%s\nGhostPin=1234\n[Net]\nLobbyServer=http://127.0.0.1:%s\nPort=%s\n' "$user" "$LPORT" "$port" > "$save/pd.ini"
	cd "$run" || exit 2
	exec timeout -k 5 "$t" stdbuf -oL -eL "$bin/pd.x86_64" --savedir "$save" --skip-intro --no-sound "$@" > "$OUT/$label.log" 2>&1
}

game host magenta "$GPORT" 240 --net-lobby-script host --net-lobby-room "$ROOM" --net-lobby-teams \
	--net-lobby-side 3 --net-lobby-shots --net-test-lobby-team-dialog --net-lobby-end-frame 300 \
	--net-test-sims 2 --net-test-stage 0x32 --rng-seed 9 &
P1=$!
game join orange "$GPORT" 240 --net-lobby-script join --net-lobby-room "$ROOM" --net-lobby-side 5 \
	--net-lobby-shots --net-test-lobby-team-dialog --net-lobby-leave-frame 0 --net-test-skip-ladder &
P2=$!
game spec spectorguy "$GPORT" 180 --net-lobby-script join --net-lobby-room "$ROOM" --net-lobby-spectate \
	--net-lobby-leave-frame 0 --net-test-skip-ladder &
P3=$!
wait $P1; r1=$?; wait $P2; r2=$?; wait $P3; r3=$?
kill $LOBBY 2>/dev/null; wait $LOBBY 2>/dev/null
echo "     exits: host $r1 join $r2 spec $r3"

H=$OUT/host.log; J=$OUT/join.log; S=$OUT/spec.log
[ $r1 = 0 ] && [ $r2 = 0 ] && [ $r3 = 0 ] && pass "all three ran to their end" || fail "a run exited nonzero: $r1 $r2 $r3"

# Teams 3 and 5 asked for once: the lobby accepted both, so the game never
# fell back to asking again. (This is the server-side test of pinning a
# member to one of the eight.)
[ "$(grep -c 'asking for team 3' "$H")" = 1 ] && pass "host: team Magenta granted on the first ask" || fail "host: team 3 asked $(grep -c 'asking for team 3' "$H") times"
[ "$(grep -c 'asking for team 5' "$J")" = 1 ] && pass "join: team Orange granted on the first ask" || fail "join: team 5 asked $(grep -c 'asking for team 5' "$J") times"

for who in "host:$H" "join:$J"; do
	label=${who%%:*}; log=${who#*:}
	grep -q "team picker up over the Game Lobby" "$log" && pass "$label: the picker opened" || fail "$label: the picker never opened"
	grep -q "shot the team picker" "$log" && pass "$label: the picker screenshotted" || fail "$label: no picker screenshot"
done
grep -q "shot the host's Game Lobby" "$H" && pass "host: Game Lobby screenshotted (check the roster: Magenta 1, Orange 1)" || fail "host: no Game Lobby shot"
grep -q "shot the joiner's Game Lobby (3 members" "$J" && pass "join: Game Lobby screenshotted with all three members" || fail "join: no Game Lobby shot with three members"

grep -q "every machine has loaded; GO" "$H" && grep -q "net: match 1: GO" "$J" && pass "both players passed GO" || fail "no GO on both"
grep -q "join: spectating" "$S" && pass "spectator: in Spectating as asked" || fail "spectator: never got to spectating"

echo "     screenshots (a human checks the eight colours and the Spectate row):"
ls -1 "$OUT/bin-host/screenshots" "$OUT/bin-join/screenshots" 2>/dev/null | sed 's/^/       /'

exit $status
