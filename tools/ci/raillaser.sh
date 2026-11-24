#!/bin/bash
# raillaser.sh - the Rail-Laser: its row, its number, its charge, and the width of a saved setup
#
# The fork's own gun (patchnotes 90), which the weapon list calls Moonrailer: a
# Moonraker Laser's look, one shot a
# magazine out of five, a trigger held one and a half seconds to fire it. It has
# two scopes: the aim input's own zoom (kept from the first pass) and an
# AR34-style Use Scope secondary whose saved choice lives in a spare bit of
# gunfuncs[], since its weapon number is past the array's stock range. Its
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

# The pickup message. A type with no case in ammotypeGetPickupName() and
# ammotypeGetDeterminer() appends nothing, so the message for a Rail ammo pickup
# read "Picked up ." - no name, no determiner, and no plural - and its pickup was
# silent, because ammotypePlayPickupSound() had no case either. All three switches
# are over every type the game has now, and the name is a port text like the
# golden bullet's, since no language file has one for a type the ROM never had.
grep -qF 'case AMMOTYPE_RAIL:        textnum = ammotypeRailText();' "$ROOT/src/game/propobj.c" || fail "the Rail ammo pickup has no name again"
grep -qF 'static u16 ammotypeRailText(void)' "$ROOT/src/game/propobj.c" || fail "the Rail ammo pickup's name has no port text helper"
awk '/^void ammotypeGetDeterminer/,/^}/' "$ROOT/src/game/propobj.c" | grep -qF 'case AMMOTYPE_RAIL' || fail "the Rail ammo pickup has no determiner, so the message is 'Picked up .'"
grep -A40 'void ammotypePlayPickupSound' "$ROOT/src/game/propobj.c" | grep -qF 'case AMMOTYPE_RAIL' || fail "a Rail ammo pickup makes no sound"

# An arena's multi-ammo crate has nineteen pairs, one an ammo type minus one, and
# that is the stride the setup stream is walked by: setupGetCmdLength() answers with
# sizeof the record, and the same walk reads GoldenEye's own stage files, so widening
# the record misaligns a stock level from its first crate onwards. A weapon spot whose
# type is past the nineteenth: AMMOTYPE_RAIL is 0x21 and its AMMOTYPE_BOWCASTER 0x22,
# and so is every type added after them. The old `type < 20` guard dropped both
# silently and the crate beside such a spot was drawn holding nothing at all - and, a
# crate with nothing in it being no thing to pick up, was never offered either, which
# reads as a bug in the gun. The writer is bounded by the array; what the record cannot
# carry goes on the crate's prop as a list (struct ammocratepairs, types.h), which both
# halves of the pickup read and the bots see. The gun's own pickup stays the other way in.
grep -qF 'mpweapon->priammotype <= (s32)ARRAYCOUNT(crate->slots)' "$ROOT/src/game/setup.c" || fail "a weapon spot's primary ammo type is not bounded by the crate's pairs"
grep -qF 'mpweapon->secammotype <= (s32)ARRAYCOUNT(crate->slots)' "$ROOT/src/game/setup.c" || fail "a weapon spot's secondary ammo type is not bounded by the crate's pairs"
grep -qF 'mpweapon->priammotype < 20' "$ROOT/src/game/setup.c" && fail "the crate's pair bound is the old hand-typed 20 again"
grep -qF 'setupAddAmmoCratePair(obj->prop, mpweapon->secammotype, mpweapon->secammoqty);' "$ROOT/src/game/setup.c" || fail "a type past the crate's nineteen pairs is not put on the prop, so the crate beside the spot holds nothing"
grep -qF 'prop->extrapairs = NULL;' "$ROOT/src/game/prop.c" || fail "a crate's extra pairs are not cleared on allocation, so a reused prop carries the last crate's ammunition"
awk '/^s32 objTestForPickup/,/^}$/' "$ROOT/src/game/propobj.c" | grep -q 'prop->extrapairs' || fail "the pickup test still sees only the record's nineteen pairs, so a Bowcaster crate is never offered"
awk '/^s32 propPickupByPlayer/,/^}$/' "$ROOT/src/game/propobj.c" | grep -q 'ammoHandlePickup(prop->extrapairs' || fail "the pickup gives only the record's nineteen pairs, so an offered crate hands over no plasma bolts"
# The bots read the same record, and one hand-typed bound there is a bot that will not
# pick up, or will not walk to, a crate whose ammunition is past the record's nineteen.
awk '/case OBJTYPE_MULTIAMMOCRATE:/,/case OBJTYPE_SHIELD:/' "$ROOT/src/game/bot.c" | grep -q 'prop->extrapairs' || fail "a bot taking a multi crate still ignores pairs past the record's nineteen"
grep -qF 'ignore1 && prop->extrapairs != NULL' "$ROOT/src/game/bot.c" || fail "a bot still ignores a crate whose only ammunition is past the record's nineteen"
grep -qE 'for \(i = 0; i != 19; i\+\+\)|for \(i = 0; i < 0x13; i\+\+\)|for \(i = 0; i < 19; i\+\+\)' "$ROOT/src/game/bot.c" && fail "a bot's crate loop is a hand-typed nineteen again"

# Instagib bends three things outside the gun itself, and each is a line a
# later edit can quietly undo. The ammunition: it is ticked per player in
# bgunTick (the cheat system cannot carry a scenario's rule across a level
# load, instagib.inc says why), and the spawn fills it too, so the first
# frame is not bare. The list's order: the menu walks g_ScenarioListOrder
# and the "-Teamwork-" header starts at the teamonly flags, so a scenario
# id never moves and a stored setup never reads wrong - the guards that
# used to name the last ROM scenario are NUM_MPSCENARIOS wide now, which
# is also what lets Instagib be played online at all. And the saved setup:
# the scenario field is three bits until MPSETUP_VERSION_SCENARIO16 and
# four after, at both readers (mpsetupfileLoadWad, mpsetupfileGetOverview)
# - if one grew without the other, the file list and the loaded room would
# read the same bytes as two different modes.
grep -qF 'g_MpSetup.scenario == MPSCENARIO_INSTAGIB' "$ROOT/src/game/bondgun.c" || fail "Instagib stopped ticking its ammunition - the Moonrailer will fire itself empty"
grep -qF 'hand->audiohandle = NULL;' "$ROOT/src/game/bondgun.c" || fail "the charge loop's stop no longer retires the handle - a fast trigger gets the sound once, then silence, because the restart gate finds the old stopped state in the way"
grep -qF 'bgunSetAmmoQuantity(AMMOTYPE_RAIL, bgunGetAmmoCapacityForWeapon(WEAPON_RAILLASER, FUNC_PRIMARY));' "$ROOT/src/game/player.c" || fail "the Instagib spawn stopped filling the Moonrailer"
grep -qF 'cheatActivate(CHEAT_UNLIMITEDAMMO)' "$ROOT/src/game/mplayer/scenarios/instagib.inc" && fail "Instagib reached for the unlimited-ammo cheat again - it dies at cheatsReset() on level load"
grep -qF 'g_ScenarioListOrder[]' "$ROOT/src/game/mplayer/scenarios.c" || fail "the scenario menu walks the id order again, and Instagib is under the wrong header"
grep -qF 'ARRAYCOUNT(g_MpScenarioOverviews) == NUM_MPSCENARIOS' "$ROOT/src/game/mplayer/scenarios.c" || fail "nothing pins the scenario tables' widths to NUM_MPSCENARIOS"
grep -qF 'scenario > MPSCENARIO_CAPTURETHECASE' "$ROOT/port/src/net/netrules.c" && fail "the net rules guard names the last ROM scenario again - it rejects Instagib online"
grep -qF 'scenario > MPSCENARIO_CAPTURETHECASE' "$ROOT/port/src/net/netsession.c" && fail "the host guard names the last ROM scenario again - it refuses to start Instagib online"
between=$(grep -c 'MPSETUP_VERSION_SCENARIO16 ? 4 : 3' "$ROOT/src/game/mplayer/mplayer.c")
if [ "$between" -lt 3 ]; then
	echo "raillaser: the scenario field grew at $between of its 3 widths (load, save, and the overview parse) - one of them is still three bits"
	exit 1
fi
grep -qF 'scenario > 7' "$ROOT/port/src/mpsetups.c" || fail "mpsetupLiveVersion no longer notices a scenario past the 3-bit field - such a setup would be written unreadable"

# One Shall Stand... The life count rides in the scenario's 32 bits (setup and
# RULES), elimination reuses the GE Plus hooks, and the two guards Instagib
# left behind have to accept the eighth mode too.
grep -qF '#define MPSCENARIO_LASTMANSTANDING   7' "$ROOT/src/include/constants.h" || fail "One Shall Stand... has no scenario id"
grep -qF '#define NUM_MPSCENARIOS             10' "$ROOT/src/include/constants.h" || fail "NUM_MPSCENARIOS did not grow for Zombies"
grep -qF 'lmsLivesSpent' "$ROOT/src/game/player.c" || fail "an LMS player can still restart after its lives are spent"
grep -qF 'lmsMatchOver' "$ROOT/src/game/lv.c" || fail "LMS never ends when one player is left"
grep -qF 'scenario <= MPSCENARIO_CAPTURETHECASE' "$ROOT/src/game/challenge.c" && fail "the challenge unlock guard still stops at the last ROM scenario"
grep -qF 'scen <= MPSCENARIO_CAPTURETHECASE' "$ROOT/port/src/net/netsession.c" && fail "--net-test-scenario still cannot start One Shall Stand..."
grep -qF 'LMS_LIVES' "$ROOT/port/src/net/netscen.c" || fail "LMS elimination state is not in the net scenario block"

# The Bowcaster: a crossbow in the hand, a bouncing plasma bolt in the air.
# Its bolt has its own number (or it would stick in walls and be collectible),
# the reload puts ten bolts in per cycle, and its magazine is thirty.
grep -qF 'WEAPON_BOWCASTER,' "$ROOT/src/include/constants.h" || fail "the Bowcaster has no weapon number"
grep -qF '#define AMMOTYPE_BOWCASTER' "$ROOT/src/include/constants.h" || fail "the Bowcaster has no ammo of its own"
grep -qF 'WEAPON_BOWCASTERBOLT' "$ROOT/src/game/bondgun.c" || fail "the Bowcaster still fires a sticking crossbow bolt"
grep -qF 'weaponnum == WEAPON_BOWCASTER ? 10 : 1' "$ROOT/src/game/bondgun.c" || fail "the Bowcaster does not load ten bolts per reload cycle"
grep -qF 'WEAPON_BOWCASTERBOLT' "$ROOT/src/game/propobj.c" || fail "a spent plasma bolt is not stopped from becoming a dropped weapon"
grep -qF 'FUNCFLAG_PROJECTILE_POWERED' "$ROOT/src/game/invitems.c" || fail "the Bowcaster's bolt is not powered - it will arc into the floor"
grep -qF 'ten-round reload cycles need the same bound' "$ROOT/src/game/bondgun.c" || fail "the ten-round reload is not clamped to the clip - it can overfill the magazine"
grep -qF 'MODEL_CHRFLASHBANG' "$ROOT/src/game/invitems.c" || fail "the plasma bolt is not a round glowing ball - it still looks like an arrow"
grep -qF 'boltbeamSetHeadPos(beamnum, &head)' "$ROOT/src/game/propobj.c" || fail "the plasma bolt's trail does not follow it - the boltbeam ends are never updated in flight"
grep -qF 'boltbeamSetGlow(beamnum)' "$ROOT/src/game/bondgun.c" || fail "the Bowcaster's beam is not rendered bright and wide"
grep -qF '#define NUM_BOLTBEAMS 256' "$ROOT/src/include/bss.h" || fail "the boltbeam pool is not large enough for a full room firing continuously"
grep -qF '180, // timer60: three seconds' "$ROOT/src/game/invitems.c" || fail "the plasma bolt's lifetime is not three seconds"
grep -qF 'backwards for knockback and backshot detection' "$ROOT/src/game/propobj.c" || fail "the plasma bolt's knockback uses the hit normal instead of the travel direction"
grep -qF 's32 boltweapon = hand->gset.weaponnum == WEAPON_BOWCASTER' "$ROOT/src/game/bondgun.c" || fail "the crossbow host branch does not pick the plasma bolt itself - branch order is load-bearing again"

# The bolt is NOT sticky: it uses the prop-aware collision for chrs and surfaces,
# but a surface bounces and only the chr hit spends it. The old objLand special
# case (a WEAPON_BOWCASTERBOLT test in the flight loop) is the workaround this
# replaces and must be gone.
grep -qF '#define PROJECTILEFLAG_PROPHIT' "$ROOT/src/include/constants.h" || fail "there is no non-sticky prop-hit projectile flag"
grep -qF 'obj->projectile->flags |= PROJECTILEFLAG_PROPHIT' "$ROOT/src/game/bondgun.c" || fail "the plasma bolt is still launched sticky"
awk '/void bgun0f09ebcc/,/^}/' "$ROOT/src/game/bondgun.c" | grep -q 'projectileSetSticky' || fail "the shared launch no longer sticks the other projectiles"
grep -cF 'projectile->flags & (PROJECTILEFLAG_STICKY | PROJECTILEFLAG_PROPHIT)' "$ROOT/src/game/propobj.c" | grep -qx 3 || fail "the prop-aware collision and damage block are not shared by sticky and prop-hit"
grep -qF 'if ((projectile->flags & PROJECTILEFLAG_STICKY) || handled)' "$ROOT/src/game/propobj.c" || fail "a surface contact still lands the plasma bolt"
grep -qF 'w->weaponnum != WEAPON_BOWCASTERBOLT' "$ROOT/src/game/propobj.c" && fail "the old objLand bolt special case is still in the flight loop"

# Its ammunition has to be named in each of the pickup message's switches, and in
# its own words: the crossbow's word is "bolt", and what the Bowcaster fires is a
# plasma bolt (propobj.c's three switches, plus the PAL and JPN name table, which
# cannot carry port text where it is written - see ammotypePortNames()).
awk '/^void ammotypeGetDeterminer/,/^}/' "$ROOT/src/game/propobj.c" | grep -q 'AMMOTYPE_BOWCASTER' || fail "the Bowcaster's ammunition has no determiner - the pickup message reads \"Picked up .\""
awk '/^void ammotypeGetPickupName/,/^}/' "$ROOT/src/game/propobj.c" | grep -q 'ammotypeBowcasterText' || fail "the Bowcaster's ammunition is named with the crossbow's word - the crate says \"bolts\""
awk '/^void ammotypePlayPickupSound/,/^}/' "$ROOT/src/game/propobj.c" | grep -q 'AMMOTYPE_BOWCASTER' || fail "a Bowcaster ammunition pickup is silent - the one pickup class with no sound"
grep -qF 'info->id == AMMOTYPE_BOWCASTER' "$ROOT/src/game/propobj.c" || fail "the PAL and JPN ammo name table still gives the Bowcaster the crossbow's word"

# Plasma Tag: Bowcasters only, infinite ammo, half health, fast-motion option.
grep -qF 'MPSCENARIO_PLASMATEST' "$ROOT/src/include/constants.h" || fail "Plasma Tag has no scenario id"
grep -qF 'MPSCENARIO_PLASMATEST' "$ROOT/src/game/player.c" || fail "Plasma Tag does not hand out a Bowcaster or set half health"
grep -qF 'MPSCENARIO_PLASMATEST' "$ROOT/src/game/bondgun.c" || fail "Plasma Tag does not keep the Bowcaster's ammunition full"
grep -qF 'weapon->weaponnum == WEAPON_BOWCASTERBOLT' "$ROOT/src/game/propobj.c" || fail "the plasma bolt does not register damage on characters"
grep -qF 'g_Vars.normmplayerisrunning && g_MpSetup.scenario == MPSCENARIO_INSTAGIB' "$ROOT/src/game/player.c" || fail "Instagib's spawn gun leaks into single-player missions"
grep -qF 'g_Vars.normmplayerisrunning && g_MpSetup.scenario == MPSCENARIO_PLASMATEST' "$ROOT/src/game/player.c" || fail "Plasma Tag's spawn gun leaks into single-player missions"
grep -qF 'g_MpSetup.scenario == MPSCENARIO_INSTAGIB ? g_InstagibCharge60 : TICKS(90)' "$ROOT/port/src/raillaser.c" || fail "the Instagib charge time leaks into other scenarios"
grep -qF 'g_LmsNetBlockSeen' "$ROOT/port/src/net/netscen.c" || fail "the LMS net block's arrival is not marked - zero lives reads as no block"
grep -qF 'g_LmsNetBlockSeen' "$ROOT/src/game/mplayer/scenarios/lastmanstanding.inc" || fail "the LMS lives display ignores the net block marker"

# A fixed-loadout mode must keep its Weapons tab out of reach, and must not
# leave its guns and options behind when the player picks a normal mode.
grep -qF 'if (item->flags & MENUITEMFLAG_ALWAYSDISABLED) {' "$ROOT/src/game/menuitem.c" \
	|| fail "disabled menu rows can still be selected - the Weapons tab opens in fixed-loadout modes"
grep -qF 'return scenario == MPSCENARIO_INSTAGIB || scenario == MPSCENARIO_PLASMATEST;' "$ROOT/src/game/mplayer/scenarios.c" \
	|| fail "the fixed-loadout predicate does not cover both modes"
grep -qF 'fixedLoadoutRestore();' "$ROOT/src/game/mplayer/scenarios.c" \
	|| fail "a normal mode does not get the player's own loadout back"
grep -qF 'mpUpdateSetupWeaponLocks();' "$ROOT/src/game/mplayer/scenarios.c" \
	|| fail "the Weapons tab is not relocked the moment the scenario changes"
grep -qF 'mpScenarioHasFixedLoadout(g_MpSetup.scenario)' "$ROOT/src/game/setup.c" \
	|| fail "a fixed-loadout mode can still place weapon pickups on the map"

# The Moonrailer's scope is the AR34's pattern: a full firing-function clone on
# FUNC_SECONDARY, with bondmove winding the view in when that function is active.
# The aim input itself stays a plain aim (no MANUALZOOM), and both functions carry
# their own port text names.
grep -qF '&invfunc_raillaser_charge, &invfunc_raillaser_usescope' "$ROOT/src/game/invitems.c" || fail "the Moonrailer has no Use Scope secondary function"
grep -qF 'INVAIMFLAG_MANUALZOOM | INVAIMFLAG_ACCURATESINGLESHOT' "$ROOT/src/game/invitems.c" || fail "the Moonrailer's aim-input scope was lost - it should keep the zoom it always had"
grep -qF 'raillaserOwn(bgunGetWeaponNum(HAND_RIGHT))' "$ROOT/src/game/bondmove.c" || fail "the Moonrailer's secondary does not wind the view in like the AR34's"
grep -qF 'gunfuncBitIndex' "$ROOT/src/include/constants.h" || fail "the function-toggle macros still index gunfuncs by weapon number - the Moonrailer's alt button is a no-op"
grep -qF 'GUNFUNC_BIT_RAILLASER' "$ROOT/src/include/data.h" || fail "the Moonrailer has no saved function-choice bit"
grep -qF 'return weaponnum;' "$ROOT/src/include/data.h" || fail "the function choice no longer gives each weapon number its own bit - the Bowcaster would share the crossbow's"
grep -qF 'u8 gunfuncs[GUNFUNCS_BYTES]' "$ROOT/src/include/types.h" || fail "the function-choice bitmap is not sized from the weapon numbers it must cover"
grep -qF '#define GUNFUNC_SAVEBITS (GUNFUNCS_BYTES * 8)' "$ROOT/src/game/mplayer/mplayer.c" || fail "the player-file save width does not cover the per-weapon function bits - the alt choice is lost on reload"
grep -qF 'Rail Shot' "$ROOT/port/src/raillaser.c" || fail "the Moonrailer's primary has no Rail Shot name"
grep -qF 'Use Scope' "$ROOT/port/src/raillaser.c" || fail "the Moonrailer's secondary has no Use Scope name"

# What a stage says, both halves of what the gun is wearing. The pickup line is the
# only report a state whose file is 0 gets - such a state is legal and draws nothing,
# so an invisible pickup has no other line to look for.
#	  gebean: the Moonrailer's first person is GpdMoonrailerZ (file N)
#	  gebean: the Moonrailer's pickup is PpdMoonrailerZ (file N)

# The fixed-loadout modes' own state machine: their Weapons tab must be
# unenterable, and leaving them must restore the player's guns and options.
FIXLOG=$OUT/fixedloadout.log
( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 60 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-fixed-loadout --exit-frame 30 \
	> "$FIXLOG" 2>&1 )
[ $? -eq 0 ] || { echo "fixed-loadout run failed: see $FIXLOG"; tail -20 "$FIXLOG"; exit 2; }
grep -E "fixed-loadout test" "$FIXLOG" | sed 's/^.*fixed-loadout test/fixed-loadout test/'
grep -q "fixed-loadout test: FAIL" "$FIXLOG" && { echo "raillaser: a fixed-loadout check failed"; exit 1; }
grep -q "fixed-loadout test: 12 of 12 checks as expected" "$FIXLOG" || {
	echo "raillaser: the fixed-loadout test did not report all checks"
	exit 1
}

echo "raillaser: as expected"
