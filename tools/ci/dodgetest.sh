#!/bin/bash
# dodgetest.sh — does a crouch button's double tap still dodge the way it was aimed?
#
# The dodge is a gesture rather than a button, and what is interesting about it
# is arithmetic: which of the four ways the player meant, and whether the shove
# they get is the same length in every direction - a diagonal that threw the
# player further than a straight line would be a dodge the aim of which nobody
# chose. Both are pinned here, out of the lines a --test-dodge run writes
# (lvTestDodgeTick, src/game/lv.c): the push's direction against the way the
# camera looks, and which move the body was given. The seam between the back arc
# and the side arcs deliberately does not sit on 135 degrees, because a keyboard
# diagonal lands exactly there (DODGE_BACK_COS, constants.h) - back-left asking
# for a roll instead of a hop is the failure this caught while it was being
# written, and it is a case of its own below.
#
# What it cannot reach is the stance a dodge ends in while the crouch button is
# still held at the end: that is read off the real buttons, and nothing in the
# joystick layer takes a fake press, so that half of the rule is a hand on a pad.
# The released half is checked, weakly - in hold-to-crouch the button drives the
# stance anyway, and the run has no buttons held.
#
# A run whose player is dead or out of MOVEMODE_WALK by frame 300 means the arena
# interfered with the test rather than the dodge being wrong, and is reported as a
# run that failed (exit 2) rather than as a wrong answer. Being airborne at that
# frame is only noted: with a simulant in the arena the player gets bumped, and
# the sample is one frame taken 300 after the tap. That reading is earned - the
# check used to treat falling as a failure too, and on 2026-11-11 it sent a hunt
# for a physics change that does not exist. With Mod.XblaMeshes on, the release's
# rule about Penny's head makes the male guard head list one shorter
# (bodyReleaseMaleHeads(), src/game/bodyreset.c), the RNG's draws follow that list,
# and every simulant decision in the level is therefore taken from a stream one
# draw along - so by the sampled frame the player has been standing somewhere else
# in the arena for a minute, on a different floor, rather than having moved
# differently. --mpsims 0 makes the two settings byte-identical. The whole reading
# is in CLAUDE-notes/dodge.md.
#
# The run pins its own picture off, and not for the reason first written in this
# paragraph. What Mod.XblaMeshes changes about a run is not the art on the body but
# the length of the male guard head list, which sets the modulus of every
# rngRandom() drawn after it - including chrPlayRollAnimation()'s own draw for the
# roll's variant, and every simulant decision. Ask for the N64's look and the eight
# cases come out byte-identical with the switch either way, package present or
# absent. A fresh install has it on since 2026-11-11; this gate is about the
# arithmetic of a dodge, so it pins the look off anyway.
#
#   dodgetest.sh [BIN]    BIN is a file in build/ or a path (default pd.x86_64)
#
# Env: BUILD (build), OUT (build/dodge-out), STAGE (0x32, the light combat
# simulator), SIMS (1, enough to have a body to put an animation on -
# chrCanPlayOneShotAnim turns away a body nobody is looking at).
#
# Exit status: 0 as expected, 1 a wrong answer, 2 a run that failed.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
OUT=${OUT:-$BUILD/dodge-out}
STAGE=${STAGE:-0x32}
SIMS=${SIMS:-1}
BIN=${1:-pd.x86_64}

case $BIN in /*) ;; */*) BIN=$(realpath "$BIN") ;; *) BIN=$BUILD/$BIN ;; esac
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }

rm -rf "$OUT"; mkdir -p "$OUT"
# The dodge is off by default in a player's pd.ini, and with it off every tap is
# refused before it is looked at - so the run says which option it needs.
mkdir -p "$OUT/save"
# The dodge's arithmetic is what is measured here, so the picture is pinned off.
# Mod.XblaMeshes is the last of the switches xblaSwitchGetEnabled() ANDs, and with
# it on the release's own head/body tables come in (xblatables.c), which changes
# what body a chr is - and where a dodge leaves the player standing. A fresh
# install has it on since 2026-11-11; this run does not want it.
printf '[Mod]\nCombatRoll=2\nXblaMeshes=0\n' > "$OUT/save/pd.ini"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 240 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --boot-stage "$STAGE" --mpsims "$SIMS" \
	--endless --log --rng-seed 7 --fixed-step --test-dodge --exit-frame 1300 \
	> "$OUT/dodgetest.log" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $OUT/dodgetest.log"; exit 2; }

python3 - "$OUT/dodgetest.log" <<'PY'
import re, sys

log = open(sys.argv[1], errors="replace").read()
cases = {}
for m in re.finditer(r"dodgetest: (\S+): along (-?[\d.]+) aside (-?[\d.]+) push ([\d.]+) "
		r"anim (0x[0-9a-f]+) rolling (\d) blocked (\d) \(dead (\d) falling (\d) mode (\d+)\)", log):
	cases[m.group(1)] = dict(along=float(m.group(2)), aside=float(m.group(3)), push=float(m.group(4)),
			anim=m.group(5), rolling=m.group(6) == "1", blocked=m.group(7) == "1",
			dead=m.group(8) == "1", falling=m.group(9) == "1", mode=int(m.group(10)))
stances = dict((m.group(1), int(m.group(2))) for m in re.finditer(r"dodgetest: (\S+): after: stance (\d+)", log))

HOP = "0x66"
ROLLS = {"0xb", "0xc", "0xd", "0x27"}
STAND = 2

# name: (expected along, expected aside, the move it must be given)
want = {
	"forward": (1.0, 0.0, "roll"),
	"back": (-1.0, 0.0, "hop"),
	"right": (0.0, 1.0, "roll"),
	"left": (0.0, -1.0, "roll"),
	"forward-right": (0.7071, 0.7071, "roll"),
	"back-left": (-0.7071, -0.7071, "hop"),
	"two-taps-no-direction": None,
	"one-tap-only": None,
}

fails = []
for name, expected in want.items():
	if name not in cases:
		fails.append("%s: never ran" % name); continue
	got = cases[name]
	if got["dead"] or got["mode"] != 0:
		print("run disturbed: %s dead=%d mode=%d" % (name, got["dead"], got["mode"]))
		sys.exit(2)
	if got["falling"]:
		print("note %-24s airborne at the sampled frame - a bump, not a dodge fault" % name)

	if expected is None:
		# Half a gesture is a crouch, and nothing else: no shove, no move.
		if got["push"] > 0.0 or got["rolling"]:
			fails.append("%s: no dodge was asked for and one happened (push %.2f, rolling)" % (name, got["push"]))
		else:
			print("ok   %-24s no dodge, as asked" % name)
		continue

	along, aside, move = expected
	if not got["rolling"]:
		fails.append("%s: no dodge happened (push %.2f)" % (name, got["push"]))
		continue

	bad = []
	if abs(got["along"] - along) > 0.02:
		bad.append("along %.2f, wanted %.2f" % (got["along"], along))
	if abs(got["aside"] - aside) > 0.02:
		bad.append("aside %.2f, wanted %.2f" % (got["aside"], aside))
	if move == "hop" and got["anim"] != HOP:
		bad.append("anim %s, wanted the hop %s" % (got["anim"], HOP))
	if move == "roll" and got["anim"] not in ROLLS:
		bad.append("anim %s, wanted one of the rolls %s" % (got["anim"], "/".join(sorted(ROLLS))))
	if stances.get(name) != STAND:
		bad.append("ends in stance %s, wanted standing (%d): the button was let go" % (stances.get(name), STAND))

	if bad:
		fails.append("%s: %s" % (name, "; ".join(bad)))
	else:
		print("ok   %-24s along %+.2f aside %+.2f push %5.2f  %s" % (
			name, got["along"], got["aside"], got["push"], move))

# The one thing a direction test cannot see: a diagonal that shoved further than
# a straight line would be a dodge nobody aimed. Every dodge got the same shove,
# so every dodge covers the same ground.
shoves = sorted(c["push"] for n, c in cases.items() if want.get(n) and c["rolling"])
if shoves and max(shoves) - min(shoves) > 0.01:
	fails.append("the dodges cover different ground: shoves %s" % ["%.2f" % s for s in shoves])
elif shoves:
	print("ok   %-24s %d dodges, all %.2f of shove" % ("same distance", len(shoves), shoves[0]))

if fails:
	print("\nWRONG:")
	for f in fails:
		print("  " + f)
	sys.exit(1)
print("\nall %d cases as expected" % len(want))
PY
