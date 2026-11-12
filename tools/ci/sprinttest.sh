#!/bin/bash
# sprinttest.sh — does a sprint run, hold its gun down, and keep its promise about firing?
#
# Sprint's rule is three sentences and every one of them has been broken at least
# once, so all three are pinned here out of the lines a --test-sprint run writes
# (lvTestSprintTick, src/game/lv.c): the first press runs, the gun is held down for
# the run by the arm that holds it and not switched out of the hand, and a trigger
# asked of the run is refused - refused through the lockout afterwards, and granted
# once it is spent, since a gate that never lets go is the same bug wearing a beard.
#
# Two of these are the author's own bug reports, both from the same playtest
# (2026-11-12): the first press was eaten by the sprint's own gate, because putting
# the gun away had been implemented as a switch to WEAPON_UNARMED and a switch ends
# a run; and a trigger fired the frame after a run ended, because SPRINT_RECOVERY60
# was written TICKS(1) believing TICKS() took seconds. It takes 60ths, so the
# lockout was one tick. That is the failure mode this whole script exists for, and
# it is why the lockout is checked both held and released rather than once.
#
# What it cannot reach is what the run looks like: how far down the arm goes and
# whether the lift at the end reads as a gun coming back up. That is a hand on a
# pad. The numbers here say the arm is being written at all, which is the part that
# can silently stop being true.
#
#   sprinttest.sh [BIN]    BIN is a file in build/ or a path (default pd.x86_64)
#
# Env: BUILD (build), OUT (build/sprint-out), STAGE (0x32, the light combat
# simulator - the player spawns holding the Falcon 2, which is the gun the run has
# to keep), SIMS (0, so nothing bumps the player about mid-sample).
#
# Exit status: 0 as expected, 1 a wrong answer, 2 a run that failed.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/sprint-out}
STAGE=${STAGE:-0x32}
SIMS=${SIMS:-0}
BIN=${1:-pd.x86_64}

case $BIN in /*) ;; */*) BIN=$(realpath "$BIN") ;; *) BIN=$BUILD/$BIN ;; esac
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }

rm -rf "$OUT"; mkdir -p "$OUT/save"
# Sprint is off in a pd.ini written before it existed, and with it off the button
# is refused before it is looked at - so the run says which option it needs. The
# picture is pinned off for the reason dodgetest.sh gives at length: Mod.XblaMeshes
# moves the RNG, and with a simulant in the arena that moves where the player is
# standing by the frame these are sampled at.
printf '[Mod]\nSprint=1\nXblaMeshes=0\n' > "$OUT/save/pd.ini"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 240 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --boot-stage "$STAGE" --mpsims "$SIMS" \
	--endless --log --rng-seed 7 --fixed-step --test-sprint --exit-frame 700 \
	> "$OUT/sprinttest.log" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $OUT/sprinttest.log"; exit 2; }

python3 - "$OUT/sprinttest.log" <<'PY'
import re, sys

log = open(sys.argv[1], errors="replace").read()

# name -> the numbers that line logged
got = {}
for m in re.finditer(r"sprinttest: (\w+): fired (\d+) sprinting (\d+) weapon (\d+)", log):
	got[m.group(1)] = dict(fired=int(m.group(2)), sprinting=int(m.group(3)), weapon=int(m.group(4)))
for m in re.finditer(r"sprinttest: (\w+): sprinting (\d+) weapon (\d+) \(dead (\d+) mode (\d+)\)", log):
	got[m.group(1)] = dict(sprinting=int(m.group(2)), weapon=int(m.group(3)),
			dead=int(m.group(4)) == 1, mode=int(m.group(5)))
for m in re.finditer(r"sprinttest: (\w+): sprinting (\d+) arm (\d+) drop (-?[\d.]+) weapon (\d+)", log):
	got[m.group(1)] = dict(sprinting=int(m.group(2)), arm=int(m.group(3)),
			drop=float(m.group(4)), weapon=int(m.group(5)))
for m in re.finditer(r"sprinttest: (\w+): fired (\d+) weapon (\d+)", log):
	got.setdefault(m.group(1), dict(fired=int(m.group(2)), weapon=int(m.group(3))))
for m in re.finditer(r"sprinttest: (\w+): sprinting (\d+) weapon (\d+)", log):
	got.setdefault(m.group(1), dict(sprinting=int(m.group(2)), weapon=int(m.group(3))))

if "no gun in hand" in log:
	print("run disturbed: the player was not holding a gun - the arena interfered")
	sys.exit(2)

start = got.get("start")
if start is None:
	print("run failed: --test-sprint wrote nothing")
	sys.exit(2)
if start.get("dead") or start.get("mode") not in (0, None):
	print("run disturbed: dead=%s mode=%s" % (start.get("dead"), start.get("mode")))
	sys.exit(2)

UNARMED = 1  # WEAPON_UNARMED: what the run used to switch the hand to

fails = []
passed = [0]
def check(label, ok, detail):
	if ok:
		passed[0] += 1
		print("ok   %-24s %s" % (label, detail))
	else:
		fails.append("%s: %s" % (label, detail))

# The press. The first version's own gate ended the run in the same frame it
# started it, so "sprint does not actually start, and the second press does" was
# one bug and not two.
s = got.get("start")
check("first press runs", s and s["sprinting"] == 1,
	"sprinting %s" % (s and s["sprinting"]))

# The gun. It is held down by the arm, so the hand keeps its weapon: no fist in
# the HUD, no pick-ups, and no punch when the trigger is asked for.
arms = [got.get(k) for k in ("during", "pull", "lockout", "rising", "free", "switch")]
if any(a is None for a in arms) or s is None:
	fails.append("the run did not report all of its steps")
else:
	weapons = set([s["weapon"]] + [a["weapon"] for a in arms])
	check("weapon kept", len(weapons) == 1 and s["weapon"] != UNARMED,
			"weapon %s through every step" % "/".join(str(w) for w in sorted(weapons)))

d = got.get("during")
check("gun held down for the run", d and d["sprinting"] == 1 and d["arm"] == 1 and d["drop"] < -20.0,
		d and "arm %d, drop %.1f" % (d["arm"], d["drop"]))

# The lockout, both halves.
p = got.get("pull")
check("a trigger ends the run", p and p["sprinting"] == 0, p and "sprinting %d" % p["sprinting"])
check("and is not a shot", p and p["fired"] == 0, p and "fired %d" % p["fired"])

l = got.get("lockout")
check("lockout holds", l and l["fired"] == 0, l and "fired %d, ten ticks after the pull" % l["fired"])

f = got.get("free")
check("lockout lets go", f and f["fired"] == 1, f and "fired %d, past the lockout" % f["fired"])

r = got.get("rising")
check("the gun comes back up", r and r["sprinting"] == 0 and r["arm"] == 0 and abs(r["drop"]) < 1.0,
		r and "arm %d, drop %.1f" % (r["arm"], r["drop"]))

w = got.get("switch")
check("a switch ends the run", w and w["sprinting"] == 0, w and "sprinting %d" % w["sprinting"])

if fails:
	print("\nWRONG:")
	for x in fails:
		print("  " + x)
	sys.exit(1)
print("\nall %d checks as expected" % passed[0])
PY
