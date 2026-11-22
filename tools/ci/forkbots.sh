#!/bin/bash
# forkbots.sh - what a simulant is allowed to make of this fork's guns
#
# A simulant's whole opinion of a weapon number is one row of
# g_AibotWeaponPreferences (src/game/botinv.c), read through AIBOTPREF(), and a
# row of zeroes is not an absence of opinion. botinvScoreWeapon() only reads the
# row's scores when the row's goal flag for that firing function matches what the
# caller asked for, so with the flag clear the scores stay 0 and the gun is never
# wanted, never picked up, never fired. All three of the fork's guns sat at that
# zero row, and nothing about it failed: no log line, no crash, no wrong number. A
# guard handed a Shock Rifle by the Randomizer stood there holding it, which reads
# to anyone watching as a broken gun and as a working one to anyone who only ever
# tests with players in hand.
#
# So most of this gate is --test-forkbots, which reads the table the way the game
# does: that there are rows at all, that they are reachable at these weapon
# numbers rather than past the end of an array that stopped short of them (which
# is what a fourth gun would walk into), that the ammunition goals they set are
# figures a simulant can reach in that ammunition rather than goals it will hunt
# crates towards forever, and that the one gun whose host has opinions of its own
# is not read through them - the crossbow's case scores a bolt that sticks in what
# it hits and can be walked back and pulled out again, and this gun's bolt bounces
# and is spent.
#
# The rest is the charge in a simulant's hands, which lives where a self-test
# cannot reach it: bot.c's firing loop, and the shot that clears the accumulator in
# chraction.c. The Rail-Laser's damage does not scale with its charge, so a
# simulant not held to the wait gets the same kill a second and a half early, and
# the target's only answer to a railgun is that window. The player's half of the
# charge has its own gate (tools/ci/raillaser.sh); this one pins the simulant's
# half: that the loop asks weaponChargeFunction() rather than naming the Mauler,
# that the shot is held until the charge is full, that a reload and a let-go press
# both take the accumulator back to nothing so a charge cannot be carried from one
# fight into the next, and that the simulant's magazine is the magazine rather than
# the 999 the Moonraker's row gives one.
#
# Two of these are the fixes a bot could make and nobody would see until it shot:
# a bot's Bowcaster creating the stock crossbow bolt (which embedded in the wall
# and came back as crossbow ammunition), and a bot's Shock Rifle having no launcher
# arm at all, so its orb was resolved as an instant trace with the orb's damage
# number on it and nothing bouncing.
#
# What none of this reaches is whether a simulant is any good with them: whether a
# guard with a Shock Rifle detonates an orb, whether 80 of them with Bowcasters
# make a mission survivable, and whether a charging simulant should be audible.
# That is eyes, and a match.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Every gate here leaves at the first bad thing, and says what it is. An undefined
# fail() fails open, which is the one thing a gate must never do.
fail() { echo "forkbots: $*"; exit 1; }
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/forkbots-out}
LOG=$OUT/forkbots.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-forkbots --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "forkbot test" "$LOG" | sed 's/^.*forkbot test/forkbot test/'

FAILED=$(grep -c "forkbot test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "forkbots: $FAILED check(s) failed"
	exit 1
fi

grep -q "forkbot test: all .* checks as expected" "$LOG" || {
	echo "forkbots: the test never reported"
	exit 1
}

# The checks are the shell's whole reach; a run that reported fewer than the
# seventeen of them skipped some on the way, the way every other gate counts
# verdicts rather than trusting one line.
COUNT=$(grep -oE "forkbot test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 17 ]; then
	echo "forkbots: $COUNT checks ran, expected at least 17"
	exit 1
fi

# ---------------------------------------------------------------------------
# The charge in a simulant's hands (bot.c), which no self-test can call.
# ---------------------------------------------------------------------------

BOT=$ROOT/src/game/bot.c
CHRACT=$ROOT/src/game/chraction.c
BOTINV=$ROOT/src/game/botinv.c
INVITEMS=$ROOT/src/game/invitems.c

# Comments first: the source says the flag's name in prose to explain why it is
# off, and a grep that reads prose reports the opposite of the row.
strip() { sed 's://.*::' "$1"; }

# The loop has to ask which function charges rather than naming the Mauler: the
# day a third gun charges, a named test is the thing that forgets it.
grep -qF 'weaponChargeFunction(aibot->weaponnum)' "$BOT" \
	|| fail "a simulant's firing loop still names the Mauler rather than asking which gun charges"

# ...and to hold the shot until the charge is full, or the wait is decoration.
grep -qF 'aibot->maulercharge[i] >= raillaserChargeFull()' "$BOT" \
	|| fail "a simulant fires the Rail-Laser without waiting for the charge"

# A charge that survives the press, or the magazine, is a charge carried into the
# next fight: botScheduleReload is where an empty magazine comes through, and the
# firing loop is where a let-go press does.
strip "$BOT" | grep -qF 'chr->aibot->maulercharge[handnum] = 0' \
	|| fail "nothing takes a simulant's charge back on the reload"
strip "$BOT" | grep -qF 'aibot->maulercharge[i] = 0' \
	|| fail "nothing takes a simulant's charge back when the press is let go"

# And the shot that leaves clearing the accumulator, for both guns, by function
# rather than by the Mauler's name.
grep -qF 'weaponChargeFunction(gset.weaponnum) == gset.weaponfunc' "$CHRACT" \
	|| fail "a simulant's charged shot does not spend the charge by function"

# The magazine is the magazine. Read the row rather than the file: the Moonraker's
# row beside this one carries the flag deliberately, and it is what the flag is
# for, so the only failure here is the Rail-Laser's row borrowing it.
RAILROW=$(awk '/^struct weapon invitem_raillaser = \{/,/^\};/' "$INVITEMS")
[ -n "$RAILROW" ] || fail "invitems.c has no invitem_raillaser row to read"
echo "$RAILROW" | sed 's://.*::' | grep -q 'WEAPONFLAG2_BOTLIMITLESS' \
	&& fail "the Rail-Laser's row gives a simulant an unemptying clip"
echo "$RAILROW" | sed 's://.*::' | grep -q 'WEAPONFLAG_AICANUSE' \
	|| fail "the Rail-Laser's row is missing WEAPONFLAG_AICANUSE, so a simulant holds it in bare hands"

# ---------------------------------------------------------------------------
# The preference rows, and the seam that keeps the crossbow out of one of them.
# ---------------------------------------------------------------------------

for row in WEAPON_RAILLASER WEAPON_BOWCASTER WEAPON_SHOCKRIFLE; do
	grep -qF "[$row]" "$BOTINV" || fail "botinv.c has no preference row written for $row"
done

# The switch is on the host, which is right for everything that inherits a host's
# behaviour and wrong for scoring a gun whose host's case is written for a detail
# the copy does not share.
grep -qF 'switch (botScoringWeapon(weaponnum))' "$BOTINV" \
	|| fail "botinvScoreWeapon still scores every gun by its host alone"
grep -qF 'case WEAPON_BOWCASTER:' "$BOTINV" \
	|| fail "nothing scores the Bowcaster apart from the crossbow"

# ---------------------------------------------------------------------------
# What a simulant's shot actually creates (chrTickShoot).
# ---------------------------------------------------------------------------

grep -qF '? WEAPON_BOWCASTERBOLT : WEAPON_BOLT;' "$CHRACT" \
	|| fail "a bot's Bowcaster is left creating a crossbow bolt, which embeds and comes back as crossbow ammo"
grep -qF 'weaponCreateProjectileFromWeaponNum(func->projectilemodelnum, WEAPON_SHOCKORB, chr)' "$CHRACT" \
	|| fail "a bot's Shock Rifle has no launcher arm, so its orb becomes an instant trace"

echo "forkbots: $COUNT runtime checks, the simulant's charge, the rows and the two bot shot arms as expected"
