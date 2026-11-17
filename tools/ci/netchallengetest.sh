#!/bin/bash
# netchallengetest.sh — challenges in the online lobby (netproto.h protocol 38,
# net.h's struct netchallengesetup): a challenge room's host pair, run through
# the test session's own start (--net-test-challenge N) so the wire is what
# the lobby's Launch would put on it.
#
# What this pins down:
#   RULES carry the challenge (the client names the room's challenge from
#   them, src/game/challenge.c's machine), and the host's end-screen verdict
#   goes out in MATCH_END for both sides: each machine ticks its own
#   profile's record for its own first player (CLAUDE-notes/save-format.md
#   "Challenge records") - the offline marking loop must never run net, its
#   bits live in a byte with four players' room.
#
# No debugger here: the match ends on a schedule (--net-test-end-frame) and
# the verdict's inputs are staged by --net-test-challenge-* boot args, so
# this gate runs wherever a game runs, ptrace settings and all. The record
# bytes come back in the verdict lines both machines log.
#
# Cases (a host with one client each; the challenge is g_MpChallenges entry 0):
#   won      the pair just finishes the match: nobody aborted and team 0 is
#            on top, which is how a finished challenge reads, so the tick
#            lands on the two-player tier's byte 3 (bit 0 any player,
#            bit 1 slot 0) on both machines, and the client marks from the
#            host's verdict
#   notwon   the host's player aborted: the challenge is lost as offline,
#            and the client marks nothing - its copy of the verdict says lost
#   cheated  the host's cheat bank used: lost for everyone - the host's own
#            record and the client's both stay clean
#   clamped  the match counted 6 humans: the host's and the client's ticks both
#            land on the stock four-player tier, completions[3] not [5]; the
#            human count travels in MATCH_END so a challenge's sims (players
#            in PLAYERCOUNT) cannot inflate the client's tier
#   sims     a challenge with built-in simulants (entry 14, eight in its ROM
#            config): the host turns on those the difficulty table enables
#            for two humans, so the match plays with allies and enemies - the
#            offline sanity check the net path otherwise never ran
#   plain    no --net-test-challenge: MATCH_END's challenge block reads 0,
#            nothing marks (the wire grew three bytes; a match with no
#            challenge must not notice)
#
#   netchallengetest.sh [BIN]   BIN a file name in build/ (pd.x86_64) or a path
#
# Env: OUT (build/netchallenge-out), PORT (27700),
#      CASES (won notwon cheated clamped plain)
# Exit status: 0 all good, 1 a check failed, 2 a run failed to start.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/netchallenge-out}; PORT=${PORT:-27700}
CASES=${CASES:-won notwon cheated clamped sims plain}
BIN=${1:-pd.x86_64}
case $BIN in /*) ;; */*) BIN=$(realpath "$BIN") ;; *) BIN=$BUILD/$BIN ;; esac
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
rm -f "$OUT"/*.log
export SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0

status=0
fail() { echo "FAIL $*"; status=1; }
pass() { echo "ok   $*"; }

# game LABEL TIMEOUT ARGS...
game() {
	local label=$1 t=$2; shift 2
	local save=$OUT/save-$label
	rm -rf "$save"; mkdir -p "$save"
	printf '[Mod]\n' > "$save/pd.ini"
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

crashed() { grep -qE "FATAL|Segmentation|Aborted" "$1"; }

# the game's pid for a run label, by its own --savedir (never pgrep -f)
gamepid() {
	local label=$1 p
	for p in $(pgrep -x "$(basename "$BIN" | cut -c1-15)"); do
		tr '\0' ' ' < "/proc/$p/cmdline" 2>/dev/null | grep -q -- "--savedir $OUT/save-$label " && { echo "$p"; return; }
	done
}

# run_case LABEL ARGS... - the pair plays a match that ends 12 s in (the
# barrier keeps that clock at 0 until both are in) and both come out through
# the end screens; the per-case checks come after, off the logs
run_case() {
	local label=$1; shift
	local name=$label H=$OUT/$label-host.log C=$OUT/$label-client.log port=$PORT
	echo "== $name"

	game "$label-host" 300 --host "$port" --net-test-host 1 --rng-seed 7 --net-test-end-frame 720 "$@" &
	local host=$!
	waitfor "$H" "net: hosting on UDP port" 90 || { fail "$name: host did not start"; kill -TERM "$host" 2>/dev/null; wait "$host" 2>/dev/null; return; }
	game "$label-client" 290 --connect "127.0.0.1:$port" --net-test-join &
	local client=$!

	waitfor "$C" "net: match 1: GO" 150 || fail "$name: no GO on the client"
	waitfor "$H" "net: match 1 ended; scores and awards sent" 120 || fail "$name: the host never ended the match"
	waitfor "$C" "net: match 1: the host ended it" 60 || fail "$name: the client never heard the match end"
	sleep 2

	local cp
	cp=$(gamepid "$label-client"); [ -n "$cp" ] && kill -TERM "$cp" 2>/dev/null
	wait "$client" 2>/dev/null; local ccx=$?
	[ "$ccx" = 143 ] && ccx=0
	local hp
	hp=$(gamepid "$label-host"); [ -n "$hp" ] && kill -TERM "$hp" 2>/dev/null
	wait "$host" 2>/dev/null; local chx=$?
	[ "$chx" = 143 ] && chx=0

	crashed "$H" && fail "$name: the host crashed" || { [ "$chx" = 0 ] && pass "$name: host ran to the end" || fail "$name: host exit $chx"; }
	crashed "$C" && fail "$name: the client crashed" || { [ "$ccx" = 0 ] && pass "$name: client ran to the end" || fail "$name: client exit $ccx"; }
}

# check FILE PATTERN LABEL: pass with the matched line, fail quoting the log's tail
check() {
	local line
	line=$(grep -E -- "$2" "$1" | tail -1)
	if [ -n "$line" ]; then pass "$3: $line"; else fail "$3: no match for $2 ($(tail -1 "$1"))"; fi
}

for c in $CASES; do
H=$OUT/$c-host.log
C=$OUT/$c-client.log
case $c in
won)
	run_case won --net-test-challenge 0
	check "$H" "net: --net-test-challenge: the room plays challenge 0: " "won: the host applied the room's challenge"
	check "$C" "net: the room's challenge: " "won: the client named it from RULES"
	check "$H" "net: challenge 0 \(.*\): won at 2 players, completions\[1\] now 3$" "won: the host's verdict and byte"
	check "$C" "net: challenge 0: the host's verdict marked this machine's record at 2 players, completions\[1\] now 3$" "won: the client marked from the verdict"
	;;
notwon)
	run_case notwon --net-test-challenge 0 --net-test-challenge-abort 1
	check "$H" "net: challenge 0 \(.*\): not won at 2 players, completions\[1\] now 0$" "notwon: the host's verdict"
	check "$C" "net: challenge 0: this machine marks nothing: the host's verdict says lost$" "notwon: the client marked nothing"
	;;
cheated)
	run_case cheated --net-test-challenge 0 --net-test-challenge-cheat 1
	check "$H" "net: challenge 0 \(.*\): not won at 2 players, the host cheated, completions\[1\] now 0$" "cheated: the host's verdict"
	check "$C" "net: challenge 0: this machine marks nothing: the host's verdict says lost$" "cheated: the client marked nothing too"
	;;
clamped)
	run_case clamped --net-test-challenge 0 --net-test-challenge-players 6
	check "$H" "net: challenge 0 \(.*\): won at 4 players, completions\[3\] now 3$" "clamped: six humans took the four-player tier"
	check "$C" "net: challenge 0: the host's verdict marked this machine's record at 4 players, completions\[3\] now 3$" "clamped: the client ticked the same human tier, not its sim-inflated player count"
	;;
sims)
	run_case sims --net-test-challenge 14
	check "$H" "net: the room's challenge 14: [1-9][0-9]* sims on for 2 human players$" \
		"sims: the challenge's ROM sims are on for the match"
	# challenge 14's config is MPCONFIG_CHALLENGE15, stage 0x47 - not the
	# 0x32 the --net-test-host arg asked for: the challenge's own arena wins
	check "$H" "net: match 1 starting on stage 0x47," \
		"sims: the challenge's own stage, not the test arg's, was played"
	check "$C" "net: match 1: loading .*stage 0x47" \
		"sims: the client loaded the challenge's own stage"
	check "$H" "net: challenge 14 \(.*\): won at 2 players, completions\[1\] now 3$" \
		"sims: the host won with the challenge's sims present"
	check "$C" "net: challenge 14: the host's verdict marked this machine's record at 2 players, completions\[1\] now 3$" \
		"sims: the client marked the human tier, not its sim-inflated player count"
	;;
plain)
	run_case plain
	if grep -q "net: challenge" "$H"; then
		fail "plain: a plain match talked about challenges"
	else
		pass "plain: no challenge talk on the host"
	fi
	if grep -qE "net: challenge|the host's verdict" "$C"; then
		fail "plain: a plain match talked about verdicts on the client"
	else
		pass "plain: and none on the client"
	fi
	;;
*)
	echo "no such case $c"; exit 2;;
esac
done

# The confirm dialog's text is a UI string (no screenshot on this box, gdb
# attach is blocked), so pin its source: the picker shows the chosen challenge's
# own description, not boilerplate, through challengeLoad + its config.
if grep -q 'challengeGetConfigDescription(cfg)' "$ROOT/port/src/netlobbymenu.c" \
		&& grep -q 'challengeLoad(s_ChallengePickIndex' "$ROOT/port/src/netlobbymenu.c"; then
	pass "picker: the confirm dialog shows the challenge's own description"
else
	fail "picker: the confirm dialog does not show the challenge's description"
fi

exit $status
