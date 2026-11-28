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
#   joinopen a join-in-progress room reserves four seats for its two humans;
#            the empty seats are out of play and must NOT tick the four-star
#            record. The tier counts joined humans, not reserved seats.
#   sims     a challenge with built-in simulants (entry 14, eight in its ROM
#            config): the host turns on those the difficulty table enables
#            for two humans, so the match plays with allies and enemies - the
#            offline sanity check the net path otherwise never ran
#   plus     Challenges+ (protocol 39): the same challenge 14 on the separate
#            dataset. Assert the generated "Challenge+ N" name, that the host
#            and client both use the Plus records, and that marking a Plus
#            star leaves the standard record for that challenge untouched
#   plain    no --net-test-challenge: MATCH_END's challenge block reads 0,
#            nothing marks (the wire grew three bytes; a match with no
#            challenge must not notice)
#
#   netchallengetest.sh [BIN]   BIN a file name in build/ (pd.x86_64) or a path
#
# Env: OUT (build/netchallenge-out), PORT (27700),
#      CASES (won notwon cheated clamped joinopen sims plus plain)
# Exit status: 0 all good, 1 a check failed, 2 a run failed to start.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/netchallenge-out}; PORT=${PORT:-27700}
CASES=${CASES:-won notwon cheated clamped joinopen sims plus plain}
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
joinopen)
	# A join-in-progress room reserves every seat: two humans sit in a four
	# seat room, the empty two go out of play. They are not players, so the
	# record must tick the two-player tier, not the four-star one.
	run_case joinopen --net-test-challenge 0 --net-test-join-in-progress
	check "$H" "net: match humans 2 of 4 seats" \
		"joinopen: the host counted two humans in four reserved seats"
	check "$H" "net: challenge 0 \(.*\): won at 2 players, completions\[1\] now 3$" \
		"joinopen: the host ticked the two-player tier, not the four-star one"
	check "$C" "net: challenge 0: the host's verdict marked this machine's record at 2 players, completions\[1\] now 3$" \
		"joinopen: the client ticked the same human tier"
	;;
sims)
	# the challenge's sims play for real, so give the host a decisive score to
	# make team 0 (which every player and sim shares) top the ranking - without
	# it a sim can outscore the humans and flip the verdict to "not won"
	run_case sims --net-test-challenge 14 --net-test-givekills 0,30,20
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
plus)
	# Challenges+ on its own dataset. givekills makes team 0 win so the mark
	# is deterministic; challenge 14 has no standard completion in this run.
	run_case plus --net-test-challenge-plus 14 --net-test-givekills 0,30,20
	check "$H" "net: --net-test-challenge-plus: the room plays challenge 14: Challenge\+ 15$" \
		"plus: the host used the generated Challenges+ name"
	check "$C" "net: the room's Challenges\+ challenge: Challenge\+ 15$" \
		"plus: the client named it from RULES as Challenges+"
	check "$H" "net: match 1 starting on stage 0x47," \
		"plus: the challenge's own arena was played"
	check "$H" "net: challenge 14 \(Challenge\+ 15\): won at 2 players, completions\[1\] now 3$" \
		"plus: the host marked the Plus record"
	check "$H" "net: plus: standard challenge 14 completions\[1\] still 0$" \
		"plus: the standard record was left untouched"
	check "$C" "net: challenge 14: the host's verdict marked this machine's record at 2 players, completions\[1\] now 3$" \
		"plus: the client marked the Plus record too"
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

# The description is a UI string (no screenshot on this box, gdb attach is
# blocked), so pin its source: both the online picker and the offline entry
# lists use the stock DESCRIPTION_MPCONFIG scrollable and load the highlighted
# challenge's config into training.mpconfig - never a one-line boilerplate label.
if grep -q 'DESCRIPTION_MPCONFIG' "$ROOT/port/src/netlobbymenu.c" \
		&& grep -q 'training.mpconfig = challengeLoad' "$ROOT/port/src/netlobbymenu.c" \
		&& grep -q 'DESCRIPTION_MPCONFIG' "$ROOT/src/game/mplayer/setup.c" \
		&& grep -q 'training.mpconfig = challengeLoadBySlot' "$ROOT/src/game/mplayer/setup.c"; then
	pass "description: online and offline lists show the challenge's own description"
else
	fail "description: a challenge list does not show the challenge's own description"
fi

# Late joiners must not change the difficulty: the sim count and the record
# tier both come from the human count captured at H1, never from PLAYERCOUNT(),
# and the sims are only enabled once there (a late join just gets the running
# match's RULES). Pin that shape in source; the clamped case already proves the
# tier follows the human count, not the 2-player PLAYERCOUNT() of the pair.
NS=$ROOT/port/src/net/netsession.c
if grep -q 'tier = netChallengeRecordTier();' "$NS" \
		&& ! grep -q 'tier = s_End.numplayers;' "$NS" \
		&& grep -q 'challengeSetSimulantsForNumPlayers(s_MatchHumans);' "$NS" \
		&& [ "$(grep -c 'challengeSetSimulantsForNumPlayers(s_MatchHumans);' "$NS")" = 1 ]; then
	pass "late join: the sims and tier come from the H1 human count, set once"
else
	fail "late join: the sims or tier are not pinned to the match's start count"
fi

# Open seats are not humans: a join-in-progress/lobby room reserves every seat
# as a player pass, but the empty ones are out of play and must not widen the
# record tier. The tier must be counted from the joined-humans mask, not the
# seat-expanded one; otherwise a 2-human, 4-seat room ticks the 4-star record.
if grep -q 'humanbits = bits;' "$NS" \
		&& grep -q 'if (humanbits & (1 << i))' "$NS" \
		&& grep -q 's_MatchHumans++;' "$NS"; then
	pass "open seats: the record tier counts joined humans, not reserved seats"
else
	fail "open seats: the record tier may count out-of-play seats as humans"
fi

# The picker UI (no screenshot here): selecting a row sets the challenge and
# leaves the picker (no confirm popup), and the list's focus loads the
# highlighted description into the NEWCOLUMN scrollable. The two offline entry
# lists use the same shape. Pin it in source.
NL=$ROOT/port/src/netlobbymenu.c
SU=$ROOT/src/game/mplayer/setup.c
if grep -q 'g_NetChallengeSetup.index = index;' "$NL" \
		&& grep -q 'MENUOP_LISTITEMFOCUS' "$NL" \
		&& grep -q 'challengeLoad(index, s_NetChallengePickCfg' "$NL" \
		&& grep -q 'MENUITEMFLAG_NEWCOLUMN' "$NL" \
		&& grep -q 'DESCRIPTION_MPCONFIG' "$NL" \
		&& ! grep -q 's_NetChallengeConfirmMenuDialog' "$NL"; then
	pass "picker: selecting a row leaves the list and focus drives the right-column description"
else
	fail "picker: direct select or the right-column description is missing"
fi
if grep -q 'mpChallengesEntryListHandler' "$SU" \
		&& grep -q 'challengeSetCurrentBySlot(data->list.value);' "$SU" \
		&& grep -q 'challengeLoadBySlot(data->list.value, s_MpChallengeDescCfg' "$SU" \
		&& grep -q 'MENUITEMFLAG_NEWCOLUMN' "$SU"; then
	pass "offline: Challenge and Challenges+ lists select on click with a live description column"
else
	fail "offline: the entry lists do not match the online picker"
fi

# Challenges+ independence: stock records stay in the GAME note, Plus records
# are read and written by their own per-agent file, and a Plus completion
# grants no shared unlocks.
CH=$ROOT/src/game/challenge.c
GF=$ROOT/src/game/gamefile.c
PLUSFILE=$ROOT/port/src/challengeplus.c
if grep -q 'challengeActiveList()' "$CH" \
		&& grep -q 'challengeSetCompletedByAnyPlayerIn(g_MpChallenges' "$GF" \
		&& grep -q 'challengeIsCompletedByAnyPlayerIn(g_MpChallenges' "$GF" \
		&& grep -q 'challengeplusLoad();' "$GF" \
		&& grep -q 'challengeplusSave();' "$GF" \
		&& grep -q 'challengeSetCompletedByAnyPlayerIn(g_MpChallengesPlus' "$PLUSFILE" \
		&& grep -q 'completion = g_MpChallengesPlus' "$PLUSFILE"; then
	pass "plus: two independent record sets, both read and written by the save"
else
	fail "plus: the standard and Plus records are not both saved independently"
fi
if grep -q 'if (!g_ChallengeMode) {' "$CH" && grep -q 'if (!g_ChallengeMode) {' "$NS"; then
	pass "plus: a Plus completion grants no shared feature unlocks"
else
	fail "plus: a Plus completion feeds the shared unlock table"
fi

exit $status
