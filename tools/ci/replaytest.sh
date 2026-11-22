#!/bin/bash
# replaytest.sh — does a build still play the same game?
#
# Runs seeded, fixed-step headless replays and logs a hash of the simulation's
# state every STEP level frames (--state-hash, port/src/statehash.c). Two runs
# that play the same game print the same lines; the first differing line is the
# frame a divergence reached the state.
#
#   replaytest.sh compare BIN_A BIN_B   both binaries, every case, diff the hashes
#   replaytest.sh record  BIN           write the golden hashes (GOLDEN dir)
#   replaytest.sh check   BIN           compare BIN against the golden hashes
#   replaytest.sh self    BIN           the same binary twice: is a run repeatable at all?
#
# BIN is a file name in build/ (pd.x86_64) or a path. Cases: an 80-sim match on
# 0x32 and a solo mission on 0x34 (the match alone missed the pad2.flags read,
# CLAUDE-notes/performance.md), and GE Plus's own: a 16-sim match on its
# Complex arena (gematch) and GoldenEye's Dam mission (gesolo), found by name
# (--boot-map, --boot-ge-mission) since a converted map's id depends on the
# mods installed. The GE cases run without --moddir (which mounts no map mods)
# and with Mod.MapMods seeded to "GoldenEye Arenas"; they are skipped with a
# note when the GoldenEye ROM has not been converted. Then the mod's own
# options, which are all off in the cases above: optmatch is the 0x32 match
# with jump, combat roll, random start weapons, akimbo, melee combos and
# flinch on for everyone; optsolo is the 0x34 mission with Guards Alerted
# (80, fast spawns, random weapons; the alarm raised at frame 200 by
# --alarm-at, since nobody trips one), akimbo guards and mission respawn; and
# the Randomizer: randrun lands a run on G5 (0x1e, seed 12345, no hops) and
# randmission deals the 0x34 mission again (to frame 1500 at most). Env: FRAMES (default
# 3000), STEP (100), SEED (12345), CASES (all eight), GOLDEN
# (build/replay-golden), OUT (build/replay-out), MODDIR (mod_allinone), EXTRA
# (more arguments for every run, e.g. EXTRA="--simbrain modern" for the
# simulants' modern movement; none by default). Needs the ROM in build/data and an
# offscreen-capable GPU driver (SDL_VIDEODRIVER=offscreen). The mission cases
# pass --skip-cutscenes: Attack Ship's opening runs past frame 1400, and a
# case that hashes only a cutscene tests very little.
#
# Budgets, since rungate.sh asks for measured ones. Measured 2026-11-21 on the dev
# box: `self` is two runs of every case, and the two GoldenEye cases cost ten minutes
# each here for nothing, because "not converted" is only found out by running the
# binary until its own 600s timeout expires - so a box without the converted GoldenEye
# ROM should ask for CASES="match solo optmatch optsolo randrun randmission".
# randmission is the one case near its budget: it is a fixed-step sim so the frames
# always advance, but its wall cost swings with what else the box is doing (1200 of
# its 1500 frames in 450s otherwise idle, the same 1200 in 900s with another heavy
# job on the machine), so treat one randmission timeout with no differing hash line as
# a load reading rather than a divergence - re-run it alone before believing it.
#
# Goldens recorded before merge/f3-0930b (b937e8f6f) are stale for gesolo: Bond's
# body is GoldenEye's 0.97 since 59ec20502, which moves the player's prop y
# and so the hash from frame 600 on Dam. Re-record gesolo (CASES=gesolo record)
# from a build that has it; every other case is unchanged by it.
#
# Goldens recorded before converter 94 (fix/f3-0930c-geclone) are stale for
# gesolo from frame 100: five of Dam's guards now carry GoldenEye's clone flag
# (chr->flags CHRFLAG0_CAN_HEARSPAWN, chrflags CHRCFLAG_CLONEABLE), which the
# hash reads. With those two bits masked out of the hash the run is identical
# to the old golden for all 3000 frames (nothing clones: nobody fires). Re-record
# gesolo once; every other case is unchanged.
#
# Goldens recorded before the fresh-install defaults of 2026-11-11 are stale for
# every case but optmatch, which writes Akimbo itself: a savedir written fresh now
# starts with Akimbo and AkimboTriggers on, the crosshair tinted by health, English
# (UK) text and sound muted while unfocused - and Mod.XblaMeshes on, which is the
# last switch xblaSwitchGetEnabled() ANDs, so the release's table fields (the fog's
# range among them) and its 23 head/body types are in play (xblatables.c). Of those
# only the last reaches the simulation: re-record once, or pin XblaMeshes=0 in
# caseini() to keep the old goldens meaning what they meant.
#
# Two binaries that convert GoldenEye differently (a GECONVERT_VERSION bump
# between them) each run from a folder of their own under OUT, with their own
# mods/GoldenEye Arenas (stage()); the first GE case then converts twice.
#
# Exit status: 0 identical, 1 a divergence, 2 a run that failed.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}   # a worktree with no build/ of its own can point at one
FRAMES=${FRAMES:-3000}; STEP=${STEP:-100}; SEED=${SEED:-12345}
CASES=${CASES:-"match solo gematch gesolo optmatch optsolo randrun randmission"}
GOLDEN=${GOLDEN:-$BUILD/replay-golden}; OUT=${OUT:-$BUILD/replay-out}
MODDIR=${MODDIR:-mod_allinone}
mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)  # runs cd into build/, so no relative paths
case $GOLDEN in /*) ;; *) GOLDEN=$PWD/$GOLDEN ;; esac
EXTRA=${EXTRA:-}
# no controller may reach a replay: the user's DualSense, and the virtual Xbox
# 360 pad the Xenia rigs (tools/gefidelity/xenia) make through /dev/uinput -
# SDL sees every controller on the machine, and one leaked into a headless run
export SDL_VIDEODRIVER=offscreen SDL_GAMECONTROLLER_IGNORE_DEVICES=0x054c/0x0ce6,0x045e/0x028e \
	SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT=0x0000/0x0000 SDL_JOYSTICK_HIDAPI=0

caseargs() {
	case $1 in
	match) echo "--boot-stage 0x32 --mpsims 80 --spectate --endless" ;;
	solo)  echo "--boot-stage 0x34 --skip-cutscenes" ;;
	gematch) echo "--boot-map Complex --mpsims 16 --spectate --endless" ;;
	gesolo)  echo "--boot-ge-mission 0 --skip-cutscenes" ;;
	optmatch) echo "--boot-stage 0x32 --mpsims 80 --spectate --endless" ;;
	optsolo) echo "--boot-stage 0x34 --skip-cutscenes --alarm-at 200" ;;
	randrun) echo "--boot-stage 0x1e --random-run --run-stage 0x1e" ;;
	randmission) echo "--boot-stage 0x34 --random-mission --skip-cutscenes" ;;
	*) echo "unknown case $1" >&2; exit 2 ;;
	esac
}

# caseframes CASE: FRAMES, or less for a case that cannot run that long. A
# Randomizer run with nobody at the controls is over when its player dies
# (frame ~815 on seed 12345), and the level loads again from frame 0.
caseframes() {
	case $1 in
	randrun) echo $((FRAMES < 750 ? FRAMES : 750)) ;;
	# Two things keep this case short and they are easy to mistake for each other.
	# One: the dealt mission's player dies in it, and the endscreen stops the
	# level's frame count there, so --exit-frame past the death is never reached.
	# That was recorded at ~1608 (seed 12345) when the case was written and at
	# ~1210 more recently, on the 2026-11-18 binary as well, so it is not
	# something this tree introduced. Two: the case is simply slow - its 1200th
	# frame arrived at 450s on an otherwise idle box, 900s with another heavy job
	# running, and a 1500s probe with the model server busy got to 1100 having
	# never reached a death at all. So a case that stops short of its cap is
	# either of those, and only the statehash lines say which: a frozen counter
	# leaves no new line, a slow one leaves lines that just run out of time.
	# 1200 sits below both recorded deaths and inside the wall budget, which is
	# what makes it a cap rather than a guess. If the death moves back late,
	# raise it with the death, and raise the budget with it.
	randmission) echo $((FRAMES < 1200 ? FRAMES : 1200)) ;;
	*) echo "$FRAMES" ;;
	esac
}

# caseini CASE: the [Mod] lines a case's pd.ini starts with, if any
caseini() {
	case $1 in
	ge*) printf 'ModDir=\nMapMods=GoldenEye Arenas\n' ;;
	optmatch) printf 'JumpHeight=3\nJumpFor=0\nCombatRoll=1\nStartArmed=2\nStartArmedFor=0\nAkimbo=2\nMeleeCombos=1\nFlinchWhenShot=1\n' ;;
	optsolo) printf 'GuardsAlerted=1\nAlertedGuards=80\nGuardSpawnSpeed=10\nGuardWeapons=1\nAkimbo=3\nMissionRespawn=1\nFlinchWhenShot=1\n' ;;
	rand*) printf 'RandomizerSeed=12345\n' ;;
	esac
}

# run BIN CASE LABEL -> $OUT/LABEL.CASE.hash
run() {
	local bin=$1 c=$2 label=$3 log=$OUT/$3.$2.log frames; frames=$(caseframes "$2")
	case $bin in /*) ;; */*) bin=$(realpath "$bin") ;; *) bin=$BUILD/$bin ;; esac
	local save=$OUT/save-$label-$c moddir="--moddir $MODDIR"
	rm -rf "$save"; mkdir -p "$save"
	case $c in ge*) moddir= ;; esac
	local ini; ini=$(caseini "$c")
	[ -n "$ini" ] && printf '[Mod]\n%s\n' "$ini" > "$save/pd.ini"
	# wall budget, not a frame budget: a fixed-step sim still takes its frames at
	# real speed when the box is busy, and randmission's 1200 frames were measured
	# at 450s idle and 900s with another heavy job running. A run that outruns this
	# is reported as a timeout, which is a load reading, not a divergence.
	( cd "$BUILD" && timeout -k 5 $((frames / 3 + 900)) "$bin" $moddir \
		--savedir "$save" --skip-intro --no-sound $(caseargs "$c") \
		--rng-seed "$SEED" --fixed-step --state-hash "$STEP" --exit-frame "$frames" \
		${EXTRA:-} > "$log" 2>&1 )
	local rc=$?
	if grep -q "boot map .*no such map\|boot GE mission .*not registered" "$log"; then
		echo "skip $label $c: GE Plus is not converted here (see $log)"
		: > "$OUT/$label.$c.hash"
		return 3
	fi
	grep -o 'statehash: frame [0-9]* [0-9a-f]*' "$log" > "$OUT/$label.$c.hash"
	if [ $rc -ne 0 ] || ! grep -q "exit-frame $frames reached" "$log"; then
		echo "FAIL $label $c: exit $rc, no exit-frame line (see $log)"
		return 2
	fi
	echo "ran  $label $c: $(wc -l < "$OUT/$label.$c.hash") hashes to frame $frames"
}

# cmp A B CASE: first differing frame, or identical
cmpcase() {
	local a=$1 b=$2 c=$3
	if [ ! -s "$a" ] && [ ! -s "$b" ]; then
		echo "skip $c"
		return 0
	fi
	if cmp -s "$a" "$b"; then
		echo "same $c"
		return 0
	fi
	paste -d' ' "$a" "$b" | awk -v c="$c" '$4 != $8 || $3 != $7 {
		printf "DIFF %s: first at frame %s (%s vs %s)\n", c, $3, $4, $8; found=1; exit }
		END { if (!found) printf "DIFF %s: runs of different length\n", c }'
	return 1
}

# The GoldenEye conversion a binary makes: its stamp line ("geconvert 97",
# gexplusrom.c's GEXPLUSROM_STAMP_LINE), or nothing for a binary without one.
convstamp() {
	strings -n 6 "$1" 2>/dev/null | grep -m1 -x 'geconvert [0-9][0-9]*'
}

# stage BIN LABEL: the binary in a folder of its own, $OUT/stage-LABEL, that
# holds everything of the binary's own folder (data/, added-content/, the mod
# dirs - each a link) but a mods/ of its own, where the binary converts the
# GoldenEye ROM for itself; the rest of the original mods/ is linked in.
# Prints the staged binary's path. A binary finds its data from its own path
# (/proc/self/exe), so two binaries in one folder share mods/GoldenEye Arenas:
# with a converter bump between them both reconvert it at once, under each
# other, as the two runs of a case go together (2026-10-01: one logged "the
# GoldenEye conversion failed" and the GE cases compared the same data).
stage() {
	local bin src dir e name
	bin=$(realpath "$1"); src=$(dirname "$bin"); dir=$OUT/stage-$2
	rm -rf "$dir"; mkdir -p "$dir/mods"
	for e in "$src"/* "$src"/.[!.]*; do
		[ -e "$e" ] || continue
		name=$(basename "$e")
		case $name in mods|"$(basename "$bin")") continue ;; esac
		ln -s "$e" "$dir/$name"
	done
	if [ -d "$src/mods" ]; then
		for e in "$src"/mods/*; do
			[ -e "$e" ] || continue
			name=$(basename "$e")
			case $name in "GoldenEye Arenas"*) continue ;; esac
			ln -s "$e" "$dir/mods/$name"
		done
	fi
	ln "$bin" "$dir/$(basename "$bin")" 2>/dev/null || cp -p "$bin" "$dir/$(basename "$bin")"
	echo "$dir/$(basename "$bin")"
}

mode=${1:-}; shift || true
mkdir -p "$OUT"
status=0
case $mode in
compare)
	[ $# -eq 2 ] || { echo "compare BIN_A BIN_B"; exit 2; }
	A=$1; B=$2
	case $A in /*) ;; */*) A=$(realpath "$A") ;; *) A=$BUILD/$A ;; esac
	case $B in /*) ;; */*) B=$(realpath "$B") ;; *) B=$BUILD/$B ;; esac
	# a converter bump between the two: each runs from a folder of its own
	# with its own conversion (stage())
	if [ "$(convstamp "$A")" != "$(convstamp "$B")" ]; then
		echo "note: the two convert GoldenEye differently ($(convstamp "$A" || echo none) / $(convstamp "$B" || echo none)): each runs from its own folder with its own conversion"
		A=$(stage "$A" a); B=$(stage "$B" b)
	fi
	set -- "$A" "$B"
	for c in $CASES; do
		run "$1" "$c" a > "$OUT/a.$c.status" & run "$2" "$c" b > "$OUT/b.$c.status" & wait
		cat "$OUT/a.$c.status" "$OUT/b.$c.status"
		if grep -q '^FAIL' "$OUT/a.$c.status" "$OUT/b.$c.status"; then status=2; continue; fi
		if { [ -s "$OUT/a.$c.hash" ] && [ ! -s "$OUT/b.$c.hash" ]; } \
				|| { [ ! -s "$OUT/a.$c.hash" ] && [ -s "$OUT/b.$c.hash" ]; }; then
			echo "FAIL $c: only one binary produced hashes"; status=2; continue
		fi
		cmpcase "$OUT/a.$c.hash" "$OUT/b.$c.hash" "$c" || status=1
	done ;;
self)
	[ $# -eq 1 ] || { echo "self BIN"; exit 2; }
	for c in $CASES; do
		run "$1" "$c" a > "$OUT/a.$c.status" & run "$1" "$c" b > "$OUT/b.$c.status" & wait
		cat "$OUT/a.$c.status" "$OUT/b.$c.status"
		if grep -q '^FAIL' "$OUT/a.$c.status" "$OUT/b.$c.status"; then status=2; continue; fi
		cmpcase "$OUT/a.$c.hash" "$OUT/b.$c.hash" "$c" || status=1
	done ;;
record)
	[ $# -eq 1 ] || { echo "record BIN"; exit 2; }
	mkdir -p "$GOLDEN"
	for c in $CASES; do
		run "$1" "$c" g; [ $? -eq 2 ] && status=2
		cp "$OUT/g.$c.hash" "$GOLDEN/$c.hash"
	done
	echo "golden hashes in $GOLDEN (seed $SEED, $FRAMES frames, every $STEP)" ;;
check)
	[ $# -eq 1 ] || { echo "check BIN"; exit 2; }
	for c in $CASES; do
		run "$1" "$c" t; [ $? -eq 2 ] && { status=2; continue; }
		cmpcase "$GOLDEN/$c.hash" "$OUT/t.$c.hash" "$c" || status=1
	done ;;
*)
	sed -n '2,24p' "$0" | sed 's/^# \{0,1\}//'; exit 2 ;;
esac
# One thing the cases cannot show from the outside, and which looks exactly like a
# busy box: a run that reaches the end of a mission or a match asks the game to
# save, and if the memory-pak slot has nothing behind it the game draws "Error
# Saving Game ... press R" over the end screen. The end screen holds the level's
# frame count still, so the case does not fail - it stops advancing and dies at the
# wall clock, with a matching hash for every frame it did reach. menutick.c skips
# that page for a run whose video driver says nobody is watching; keep the skip, and
# if a case is ever given a cap above its own death this is what makes the run
# finish rather than time out.
if ! grep -qF '&& !menuSavePromptHasNobody())' "$ROOT/src/game/menutick.c" \
		|| ! grep -qF 'getenv("SDL_VIDEODRIVER")' "$ROOT/src/game/menutick.c"; then
	echo "FAIL menutick.c no longer skips the end-of-match save prompt for an unsupervised run"
	[ $status -eq 0 ] && status=1
fi

exit $status
