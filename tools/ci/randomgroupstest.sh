#!/bin/bash
# randomgroupstest.sh - the curated random weapon list, the groups it can be kept
# as, the spawn roll they both steer, and the weapon slots that draw one gun from
# a group
#
# Five things run at startup under --test-randomgroups, all of them the parts a
# screen cannot be blamed for:
#
#   mpWeaponFiltersSelfTest (port/src/mpweaponfilters.c) - the store. The check
#   that matters is the moved table: a group keeps the guns it selects as WEAPON_*
#   numbers, so shifting every row's weapon number along one - which is what a
#   mod's import does to g_MpWeapons (moddata.c) - must still select the same
#   guns. Saved as row numbers it would select whatever those rows became.
#
#   mpRandomSetupFileSelfTest (src/game/mplayer/mplayer.c) - the saved setup that
#   carries the list. Versions up to 3 wrote it into a u64, which cannot hold
#   this fork's 81-row table, so the format grew to one bit per row
#   (MPSETUP_VERSION_RANDOMFILTERS): a list with a GoldenEye row taken off must
#   ask for the new layout and read back whole, and a list that touched nothing
#   there must still be written in the old layout - which it also reads back
#   whole, because the rows the u64 cannot name are rows nothing can select off.
#
#   mpRandomRollSelfTest (src/game/mplayer/mplayer.c) - Start Armed's Random. A
#   list of one gun may only roll that gun; a list of none must arm nobody AND
#   take nothing from the stage's rng, because a peer that drew anyway would
#   consume a different amount of a stream every peer has to agree on
#   (netplay.md's split rng). It lives in mplayer.c because the roll's candidate
#   test, mpCanRollSpawnWeapon(), is private there.
#
#   mpRandomSlotSelfTest (src/game/mplayer/mplayer.c) - the weapon slots made
#   random. A slot holds a row of its group and rolls nothing else, never a row the
#   slot's own dropdown would not offer; a group with nothing in it fills nobody and
#   takes no rng; and every way of writing a slot by hand, by the game's Random set,
#   or by the check at mpReset retires the group it claimed - the page lying about a
#   gun is the only failure this design has, and it is invisible in the arena. Also
#   the clamp in mpGetMpWeaponByLocation, which prints one deliberate ERROR line here
#   and nothing else: a marker that escaped into the array the arena reads is memory
#   rather than a gun.
#
#   mpRandomGroupMenuSelfTest (src/game/mplayer/setup.c) - the page. Driven with
#   the operations the menu really sends: MENUOP_SETTEXT on the keyboard row and
#   never MENUOP_SET, and no row that would push a dialog while no menu is up to
#   receive one. Covers Dark against Classic (and that Dark selects no GoldenEye
#   row), RX: on a saved group, Remove dead on a built-in and live on the
#   player's own, the confirm's Yes deleting what the asking row stashed, and a
#   hand-ticked box leaving no group in use.
#
# What this cannot see is the layout: the new page's rows, and the two new rows
# on the weapons pages, are the sort of thing only a screen shows (weapon-sets.md
# says the same of the bind pages).
#
# --test-randomgroups writes $S/mpweaponfilters-test.bin, so a run never touches
# a player's own groups.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/randomgroups-out}
LOG=$OUT/randomgroupstest.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 240 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-randomgroups --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "mpweaponfilters test|randomgroups test" "$LOG" | sed 's/^.*test:/test:/'

FAILED=$(grep -c "test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "randomgroups: $FAILED check(s) failed"
	exit 1
fi

# Five halves, five verdicts: the store, the roll, the slots, the save format, the
# page. A run that reported four of them is missing one, which is how a half that
# never ran - or that took the run down before it got there - would otherwise pass
verdicts=$(grep -cE "test: all [0-9]+( roll| slot| save| page)? checks as expected" "$LOG")
if [ "$verdicts" -ne 5 ]; then
	echo "randomgroups: $verdicts of 5 tests reported, not 5"
	grep -E "test: all|test: FAIL" "$LOG" | tail -5
	exit 1
fi

# Nothing about this feature may write an error line: a store that cannot write
# its file, or a setup that does not fit its block, says so and only so
grep -E "mpweaponfilters:|mpsetup:" "$LOG" | grep -E "^ERROR" && {
	echo "randomgroups: an error line came out of a passing run"
	exit 1
}

ls "$OUT/save" | sed 's/^/     file: /'
test ! -e "$OUT/save/mpweaponfilters-test.bin" || {
	echo "randomgroups: the scratch file outlived the test"
	exit 1
}
echo "randomgroups: as expected"
