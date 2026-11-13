#!/bin/bash
# rungate.sh - run one gate once, and say honestly what it cost
#
# Two things this exists to stop. The first is running a gate twice, once to
# count its "ok" lines and once to count its "FAIL" lines, which doubles the
# wall time of every measurement taken that way: the log goes to a file here and
# both counts come off the file. The second is the outer timeout swallowing a
# verdict - a gate killed at its budget exits 124, which is not a pass, and a run
# reported once as "timed out downloading, not a code failure" was a run that
# never reached a verdict at all. Here 124 is said out loud as DID NOT FINISH.
#
# Budgets should come from the seconds this prints rather than from padding, and
# the padding that used to be here was silly: measured 2026-11-13 on a loaded dev
# box, mpweaponsetstest takes 3s and sprinttest 7s, both of which had been given
# 150 to 300s outer timeouts by whoever ran them last, and netpredicttest's
# failures were being sat through at 600s. CLAUDE-notes/netplay.md records
# netlantest at about 8 minutes, so the two-process LAN and lobby gates are the
# ones that genuinely want a long budget - everything else wants a tight one, so
# a gate that hangs is found out in seconds rather than minutes.
#
#   tools/ci/rungate.sh 120 mpweaponsetstest
#   ONLY="lan direct password" tools/ci/rungate.sh 900 netlantest
# Anything after the gate name is passed to it, and the environment is inherited.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUDGET=${1:?usage: rungate.sh <seconds> <gate> [args...]}; shift
GATE=${1:?usage: rungate.sh <seconds> <gate> [args...]}; shift
[ -x "$ROOT/tools/ci/$GATE.sh" ] || { echo "no gate tools/ci/$GATE.sh"; exit 2; }

LOGDIR=$ROOT/build/gate-logs
mkdir -p "$LOGDIR"
LOG=$LOGDIR/$GATE.log

started=$(date +%s)
timeout -k 10 "$BUDGET" "$ROOT/tools/ci/$GATE.sh" "$@" > "$LOG" 2>&1
rc=$?
secs=$(( $(date +%s) - started ))

echo "$GATE: rc=$rc ${secs}s (budget ${BUDGET}s)"

if [ "$rc" -ne 0 ]; then
	if [ "$rc" -eq 124 ]; then
		echo "$GATE: DID NOT FINISH inside ${BUDGET}s - no verdict, not a pass"
	fi
	grep -E "^FAIL|CHECKS FAILED" "$LOG" | head -8
	exit "$rc"
fi

# The gate's own verdict line if it has one: some print ok per check, some print
# a single summary, and counting "^ok" alone calls the second kind empty.
verdict=$(grep -E "as expected|CHECKS FAILED|^ALL |passed$" "$LOG" | tail -1)
echo "  ok lines: $(grep -cE '^ok' "$LOG")   ${verdict:+verdict: $verdict}"
echo "  log: $LOG"
