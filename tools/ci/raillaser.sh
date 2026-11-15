#!/bin/bash
# raillaser.sh - the Rail-Laser: its row, its number, its charge, and the width of a saved setup
#
# The fork's own gun (patchnotes 90), which the weapon list calls Moonrailer: a
# Moonraker Laser's look, one shot a
# magazine out of five, a trigger held one and a half seconds to fire it, and
# the AR34's zoom on the aim input (invitems.c, port/src/raillaser.c). Its
# charge is the Mauler's machine - bgunTickMaulerCharge() widened to the new
# number by weaponChargesLikeMauler() - because the rising loop, the red build
# over the model and the shot's pitched-down voice all read the one field that
# machine fills, hand->matmot1.
#
# The first two are invisible from the row. GoldenEye's block used to run to the end of the weapon enum, so a number
# appended to it was read as a 42nd GoldenEye gun - g_GeWeaponHosts[] indexed
# one past itself, and a segfault in langAddPortText() on the way out of a
# constructor (constants.h anchors NUM_GE_WEAPONS, data.h bounds WEAPON_IS_GE).
# And the curated random weapon list is one bit per row in a saved setup, so a
# row appended to g_MpWeapons[] moves every field written after it by one bit,
# which is why MPSETUP_VERSION_RAILLASER exists and why a version 5 file is
# still read at 81 bits (mplayer.c). Both are pinned here by a literal, along
# with the charge at its edges - a hair short of full the gun holds, at full it
# fires itself - and with the Combat Simulator's rows found where a saved setup
# names them.
#
# A third is that hand->matmot1 is per-hand scratch, and two other writers come
# near this gun - the Moonraker's stream and
# the shotgun's starburst. Neither is asked for by this row, and either would
# fight the charge invisibly, so both are asserted (bondgun.c).
#
# A fourth is a whitelist: the beam a player sees is created only
# for the numbers bgunCreateFx() names, the one other players see only for those
# chraction.c's makebeam names, and a number absent from either draws nothing -
# no log line, no crash. Those two lists have no accessor, so the script looks at
# the source for them, and says so; the body's held model has an accessor and is
# checked by the test, which is where the same omission left a player holding
# nothing at all (playermgrGetModelOfWeapon answers -1 past WEAPON_PSYCHOSISGUN).
#
# What it does not reach is the feel. That the charge loop stops on a release
# and starts again at the bottom of its pitch, that the colour and the shot's
# voice land with the Mauler's full charge, that one and a half seconds is the
# right wait, and what the laser model looks like held up while it glows red:
# all of that is eyes and ears, and is checked by hand.
#
# --test-raillaser (main.c, raillaserSelfTest) puts the curated list back
# afterwards and writes no files of its own: the run is on its own savedir.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# The other gates accumulate a status and leave at the end; this file has always left at
# the first bad thing, so fail() leaves the same way. Written down because the first five
# checks written against it here reported nothing whatever: a fail() that was never
# defined prints "command not found", the script goes on, and the gate says "as
# expected". An undefined helper fails open, which is the one thing a gate must never do
# - the checks looked like coverage and were none. Every other gate defines fail().
fail() { echo "raillaser: $*"; exit 1; }
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/raillaser-out}
LOG=$OUT/raillaser.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-raillaser --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "raillaser test" "$LOG" | sed 's/^.*raillaser test/raillaser test/'

FAILED=$(grep -c "raillaser test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "raillaser: $FAILED check(s) failed"
	exit 1
fi

grep -q "raillaser test: all .* checks as expected" "$LOG" || {
	echo "raillaser: the test never reported"
	exit 1
}

# The checks are the feature's whole reach; a run that reported fewer than the
# forty of them skipped some on the way (the same reason the other gates
# count verdicts rather than trusting one line)
COUNT=$(grep -oE "raillaser test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 40 ]; then
	echo "raillaser: $COUNT checks ran, expected at least 40"
	exit 1
fi

# Nothing about this feature may write an error line
grep -E "^ERROR" "$LOG" && {
	echo "raillaser: an error line came out of a passing run"
	exit 1
}

# The beam whitelists, by adjacency: the Rail-Laser has to sit beside the
# Moonraker in each, and dropping out of one is what makes the bolt vanish.
between=$(grep -A1 "case WEAPON_LASER:" "$ROOT/src/game/bondgun.c" | grep -c "case WEAPON_RAILLASER:")
if [ "$between" -lt 2 ]; then
	echo "raillaser: bondgun.c names the Rail-Laser in $between of the 2 lists it must (bgunCreateFx's beam, and the equip sound)"
	exit 1
fi
between=$(grep -A1 "case WEAPON_LASER:" "$ROOT/src/game/chraction.c" | grep -c "case WEAPON_RAILLASER:")
if [ "$between" -lt 1 ]; then
	echo "raillaser: chraction.c's makebeam no longer names the Rail-Laser, so other players see no tracer"
	exit 1
fi

# The first-person row's wiring (gebean.c). A row that is not there, or an alias
# name that is not its own, means the gun quietly keeps Perfect Dark's Laser - the
# stand-in is a correct answer, so nothing complains when it is the wrong one. And
# the two watch arrays must be sized by the rows: the loops that read them run to
# ARRAYCOUNT(fpRows), and past GoldenEye's block fpWatchSlot[] abuts a live flag.
between=$(grep -c 'FPROW("GpdMoonrailerZ", "gun/laser")' "$ROOT/port/src/gebean.c")
if [ "$between" -lt 1 ]; then
	echo "raillaser: gebean.c has no first-person row of the Moonrailer's own over gun/laser"
	exit 1
fi
for guard in 'fpWatchSlot\[ARRAYCOUNT(fpRows)\]' 'fpWatchRows\[ARRAYCOUNT(fpRows)\]'; do
	grep -q "$guard" "$ROOT/port/src/gebean.c" || {
		echo "raillaser: a watch array is sized by GoldenEye's block again, and the loops over the rows are not"
		exit 1
	}
done
between=$(grep -c 'fp < NUM_GE_GUNS && gegunsIsBorrowed(fp)' "$ROOT/port/src/gebean.c")
if [ "$between" -lt 2 ]; then
	echo "raillaser: a gegunsIsBorrowed() call lost its guard ($between of 2); the extra row is inside borrowed[]'s reach, where it is the Covert Modem's"
	exit 1
fi

# What a stage says when the release is installed - the positive path the run
# above cannot reach, because the guns are refreshed on a stage load and on F6,
# not at boot. The mesh's own fit logs next to it as "fpfit: row 25 ... scale".
#	  gebean: the Moonrailer's first person is GpdMoonrailerZ (file N)
# The floor and held copy. It is a model state (MODEL_PD_RAILLASER) with an alias
# name of its own over the Laser's pickup file, mapped to the Moonraker's pickup row
# so the mesh and its measured ground fit are borrowed by reference - and that borrow
# is only valid because the Moonraker's row's host is the same FILE_PCHRLASER this
# aliases (g_GeWeaponHosts[WEAPON_GE_MOONRAKER] is WEAPON_LASER), so a row that drifted
# off that host would make the fit wrong and stay silent about it.
# Once, not twice: it is one array whose address is the key, and a second literal
# would be the bug - the pointer compare in gebeanPoolRowForFile() would then be
# comparing against a different string. Asked for two at first, which the check
# answered by failing on its own first run.
grep -q '"PpdMoonrailerZ"' "$ROOT/port/src/gebean.c" || {
	echo "raillaser: the Moonrailer's pickup alias name is gone"; exit 1; }
grep -q 'name == s_PdMoonrailerProp' "$ROOT/port/src/gebean.c" || {
	echo "raillaser: nothing maps the Moonrailer's pickup to a row, so its mesh is never built"; exit 1; }
grep -q '+ (WEAPON_GE_MOONRAKER - WEAPON_GE_FIRST);' "$ROOT/port/src/gebean.c" || {
	echo "raillaser: the Moonrailer's pickup no longer borrows the Moonraker's row, so its ground fit is gone"; exit 1; }
grep -q '#define MODEL_PD_RAILLASER          (MODEL_GE_FIRST + NUM_GE_WEAPONS)' "$ROOT/src/include/constants.h" || {
	echo "raillaser: MODEL_PD_RAILLASER is not derived from MODEL_GE_FIRST, and it moves with the version"; exit 1; }
grep -q '_Static_assert(MODEL_PD_RAILLASER < MODEL_REMAKE_FIRST' "$ROOT/port/src/geguns.c" || {
	echo "raillaser: nothing asserts the fork's model state is inside the range before the remake's"; exit 1; }
# Both paths that put a gun on the floor from the static g_MpWeapons[] rows, and
# neither of them is the one playermgrGetModelOfWeapon() answers for.
between=$(grep -rc 'raillaserFloorModel(weapon->weaponnum' "$ROOT/src/game/setup.c" "$ROOT/src/game/modrandom.c" | awk -F: '{s+=$2} END {print s}')
if [ "$between" -lt 2 ]; then
	echo "raillaser: a weapon spot or a Randomizer crate stopped asking for the floor model ($between of 2)"
	exit 1
fi

# botFindPickup() indexes two stack arrays by ammunition type, and they were sized by a
# literal 33 - the count from before the Rail-Laser - so a simulant scanning a dropped
# Moonrailer wrote one past both and the stack canary aborted the process
# (__stack_chk_fail). It crashed at stage start with a simulant present and never alone,
# because only a bot runs that scan. Sized from NUM_AMMOTYPES now, and the write is
# bounded as well, because the index comes from a function that answers with any type
# the game has. types.h's ammoheldarr carries the same note from the commit that added
# the type: that sweep stopped at the arrays with names, and a stack array has none.
grep -qF 'struct prop *ammoproplist[NUM_AMMOTYPES]' "$ROOT/src/game/bot.c" || fail "botFindPickup's per-ammo-type array is not NUM_AMMOTYPES wide"
grep -qF 'f32 ammodistlist[NUM_AMMOTYPES]' "$ROOT/src/game/bot.c" || fail "botFindPickup's ammo distances are not NUM_AMMOTYPES wide"
grep -qF 'ammoproplist[33]' "$ROOT/src/game/bot.c" && fail "a bot's ammo-type array is sized by the pre-Rail-Laser count again"
grep -qF 'ammotype < ARRAYCOUNT(ammoproplist)' "$ROOT/src/game/bot.c" || fail "a bot's ammo type indexes those arrays unbounded"
grep -qF 'ammoheldarr[NUM_AMMOTYPES]' "$ROOT/src/include/types.h" || fail "a player's held-ammo array is not NUM_AMMOTYPES wide"

# What a stage says, both halves of what the gun is wearing. The pickup line is the
# only report a state whose file is 0 gets - such a state is legal and draws nothing,
# so an invisible pickup has no other line to look for.
#	  gebean: the Moonrailer's first person is GpdMoonrailerZ (file N)
#	  gebean: the Moonrailer's pickup is PpdMoonrailerZ (file N)
echo "raillaser: as expected"
