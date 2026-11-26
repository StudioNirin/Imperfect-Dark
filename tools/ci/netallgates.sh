#!/bin/bash
# netallgates.sh — every netplay gate, one at a time, then the replay gate.
#
#   netallgates.sh [LOG]   LOG defaults to build/netallgates.log
#
# Builds Linux (build/) and mingw (build-win/, re-running cmake first so new
# source files link), runs each tools/ci/net*test.sh alone (several are load
# sensitive: CLAUDE-notes/netplay.md lists them), the replay gate against
# build/pd-base.x86_64 (the pre-netplay baseline), pdlobbyd's tests and
# pd-nettest. Read the log: a gate prints "exit N" and its last lines. A Windows
# build that cannot be configured is reported too, so a missing
# build-win/pd.x86_64.exe cannot pass for a silent build.
#
# A gate that fails is run once more (RERUN=0 turns that off): passing the
# second time it counts as FLAKY, not failed, and its first run's log is kept
# as build/gate-NAME.try1.log; failing twice it is FAIL. The log ends with a
# summary: "== summary", then "pass N, flaky N, FAIL N" and one line per
# gate that was not a plain pass. The replay gate is not rerun: it is
# deterministic, and a DIFF is a real one. The script exits 0 when nothing
# failed (a FLAKY gate passed), 1 when anything did, replay, pdlobbyd and
# pd-nettest included.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT" || exit 2
LOG=${1:-$ROOT/build/netallgates.log}
RERUN=${RERUN:-1}
GATES=${GATES:-"netclocktest netsessiontest netplayertest netsnaptest netammotest netpuppettest neteventtest netlobbytest netlobbyuitest netlantest netlobbysimtest netchallengetest netplayersonlinetest netlobbywinetest netnattest netpredicttest netlagcomptest netscenariotest netcontenttest netjointest netchattest netwidetest nettwelvetest netcooptest netmigratetest netteamtest netteamcolourtest netantitest"}

rungate() {
	local t=$1 s0 rc
	s0=$(date +%s)
	SDL_VIDEODRIVER=offscreen timeout 3600 tools/ci/$t.sh > build/gate-$t.log 2>&1
	rc=$?
	echo "exit $rc ($(( $(date +%s) - s0 )) s)"
	return $rc
}

{
cmake -Bbuild . >/dev/null && cmake --build build -j8 2>&1 | grep -E "error|Built target pd$"
npass=0; nflaky=0; nfail=0; notes=""

# The Windows build, and a report on it. This used to be
# `cmake -Bbuild-win . >/dev/null 2>&1 && cmake --build build-win -j8`, which
# threw the configure's output away and short-circuited on failure: a machine
# without mingw SDL2 and zlib therefore ran no Windows build at all, printed
# nothing, and the sweep still reported its gates - while its own header said it
# builds mingw. netlobbywinetest's bare "no pd.x86_64.exe" was the first anyone
# saw of it, with no cause. Keep the logs and fail the run on the missing exe.
# And pass the toolchain file: without it cmake configures build-win for the
# host, so the step built a Linux binary under build-win, printed "Built target
# pd", and still left netlobbywinetest looking for a pd.x86_64.exe that the
# configuration was never going to make.
echo "== mingw (build-win)"
cmake -Bbuild-win . -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64.cmake > build/mingw-configure.log 2>&1
cmake --build build-win -j8 > build/mingw-build.log 2>&1
grep -E " error|Built target pd$" build/mingw-build.log 2>/dev/null
if [ ! -f build-win/pd.x86_64.exe ]; then
	echo "     no build-win/pd.x86_64.exe; the last of the configure:"
	tail -4 build/mingw-configure.log | sed 's/^/     /'
	nfail=$((nfail + 1))
	notes="$notes\nFAIL   mingw: no build-win/pd.x86_64.exe, so netlobbywinetest has no WinHTTP client to run (build/mingw-configure.log)"
fi
for t in $GATES; do
	[ -x tools/ci/$t.sh ] || { echo "MISSING $t"; nfail=$((nfail + 1)); notes="$notes\nFAIL   $t: no tools/ci/$t.sh"; continue; }
	echo "== $t"
	rm -f build/gate-$t.try1.log
	if rungate "$t"; then
		tail -3 build/gate-$t.log
		npass=$((npass + 1))
		continue
	fi
	tail -3 build/gate-$t.log
	if [ "$RERUN" = 0 ]; then
		nfail=$((nfail + 1)); notes="$notes\nFAIL   $t (not rerun)"
		continue
	fi
	mv -f build/gate-$t.log build/gate-$t.try1.log
	echo "-- $t: failed, run once more (the first run's log: build/gate-$t.try1.log)"
	if rungate "$t"; then
		tail -3 build/gate-$t.log
		echo "-- $t: FLAKY (failed once, passed on the rerun)"
		nflaky=$((nflaky + 1)); notes="$notes\nFLAKY  $t: first run in build/gate-$t.try1.log"
	else
		tail -3 build/gate-$t.log
		echo "-- $t: FAIL (failed twice)"
		nfail=$((nfail + 1)); notes="$notes\nFAIL   $t: build/gate-$t.try1.log and build/gate-$t.log"
	fi
done
echo "== replay"; SDL_VIDEODRIVER=offscreen tools/ci/replaytest.sh compare pd-base.x86_64 pd.x86_64 2>&1 | grep -E "^(same|DIFF|fail)"
[ "${PIPESTATUS[0]}" = 0 ] || { nfail=$((nfail + 1)); notes="$notes\nFAIL   replay: tools/ci/replaytest.sh compare pd-base.x86_64 pd.x86_64"; }
python3 tools/pdlobbyd/test_pdlobbyd.py 2>&1 | tail -2
[ "${PIPESTATUS[0]}" = 0 ] || { nfail=$((nfail + 1)); notes="$notes\nFAIL   pdlobbyd: python3 tools/pdlobbyd/test_pdlobbyd.py"; }
./build/pd-nettest 2>&1 | tail -1
[ "${PIPESTATUS[0]}" = 0 ] || { nfail=$((nfail + 1)); notes="$notes\nFAIL   pd-nettest: build/pd-nettest"; }
echo "== summary"
# Cases a gate could not set up because a fixture this box does not have is
# absent. They are not evidence either way, so they are listed here rather than
# left in a per-gate log where a green summary would hide them: a skip must never
# read as a pass. A gate's own exit status stays 0 for a skip, so that a box
# without a download is not red; the count is what makes the gap visible. The
# try1 logs are a rerun's first attempt, so they would count twice.
nskip=0; skiplines=""
for f in build/gate-*.log; do
	[ -e "$f" ] || continue
	case $f in *.try1.log) continue ;; esac
	while IFS= read -r l; do
		nskip=$((nskip + 1))
		skiplines="$skiplines\n     $(basename "$f" .log | sed 's/^gate-//'): ${l#skip }"
	done < <(grep -h "^skip " "$f" 2>/dev/null)
done
echo "gates: pass $npass, flaky $nflaky, FAIL $nfail (FAIL counts replay, pdlobbyd and pd-nettest too), cases skipped for a fixture this box does not have $nskip"
[ "$nskip" = 0 ] || { echo "     skipped, so not evidence either way:"; printf "%b\n" "${skiplines#\\n}"; }
[ -z "$notes" ] || printf "%b\n" "${notes#\\n}"
} > "$LOG" 2>&1
[ "$nfail" = 0 ]
