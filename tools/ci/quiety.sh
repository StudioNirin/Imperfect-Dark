#!/bin/bash
# quiety.sh - run a list of gates alone, on a box that has actually gone quiet.
#
# Several of the netplay gates measure timing: netpredicttest's relay case counts
# corrections a minute and the gap between the client's and the host's level
# clock, and netplayersonlinetest presses a real key at a scheduled frame and
# fails if the page has not taken it. Both are documented in
# CLAUDE-notes/netplay.md as moving with machine load, and both were recorded as
# failures here while a sweep that was supposed to have been stopped was still
# launching three-game matches alongside them. So a result from a loaded box says
# nothing, either way, and running a gate is only worth it once the box is idle.
#
#   tools/ci/quiety.sh GATE [GATE...]      LOGS default to build/quiet-NAME.log
#
# Waits for the 1-minute load average to come under QUIET_LOAD (8, on a box whose
# own inference engine keeps about half the CPU) before the first gate, and again
# between gates, so a gate that spawns games cannot be timed against the previous
# one's leftovers. Gives up waiting after QUIET_WAIT s and says so rather than
# running a gate it cannot trust quietly. Prints the load it saw with each result,
# because a bare exit code from a gate like these is not evidence on its own.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
LOGS=${LOGS:-$ROOT/build}
QUIET_LOAD=${QUIET_LOAD:-8}
QUIET_WAIT=${QUIET_WAIT:-900}

load() { awk '{print $1}' /proc/loadavg; }

waitquiet() {
	local waited=0 l
	while :; do
		l=$(load)
		if awk -v l="$l" -v m="$QUIET_LOAD" 'BEGIN { exit !(l < m) }'; then
			return 0
		fi
		if [ "$waited" -ge "$QUIET_WAIT" ]; then
			echo "gave up waiting for load: still $l after ${QUIET_WAIT}s"
			return 1
		fi
		sleep 15
		waited=$((waited + 15))
	done
}

status=0
for g in "$@"; do
	if [ ! -x "$ROOT/tools/ci/$g.sh" ]; then
		echo "$g: no tools/ci/$g.sh"
		status=1
		continue
	fi
	waitquiet || { echo "$g: SKIPPED, box busy"; status=1; continue; }
	echo "== $g (load $(load))"
	s0=$(date +%s)
	( cd "$ROOT" && SDL_VIDEODRIVER=offscreen "tools/ci/$g.sh" > "$LOGS/quiet-$g.log" 2>&1 )
	rc=$?
	echo "exit $rc ($(( $(date +%s) - s0 )) s, load $(load))"
	[ $rc -eq 0 ] || grep -m 6 -E "^FAIL|: FAILED|^ok " "$LOGS/quiet-$g.log" | sed 's/^/     /'
	[ $rc -eq 0 ] || status=1
done
echo "quiety: $([ $status -eq 0 ] && echo 'all as expected' || echo 'see above')"
exit $status
