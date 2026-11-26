#!/bin/bash
# shock.sh - the Shock Rifle: its two firing modes, its orb's row, its ammunition,
# and the width of a saved setup
#
# The fork's third own gun: a plasma beam that lands the instant the trigger is
# pulled (INVENTORYFUNCTYPE_SHOOT_SINGLE, which the engine resolves with one trace
# from the camera, so the beam has no projectile and no damage code of its own) and
# a large slow orb that bounces off walls and detonates when that beam reaches it.
# The detonation itself is stage 3; this gate covers the shell it will hang off, and
# the parts of the shell whose absence is silent.
#
# Three of those are whitelists with no accessor. The beam a player sees is created
# only for the numbers bondgun.c's bgunCreateFx() names and the equip sound only for
# those its equip switch names; the one other players see is created only for the
# numbers chraction.c's makebeam names. A number absent from any of them draws or
# sounds like nothing at all, with no log line and no crash - and the Rail-Laser
# fired invisibly for a week that way, which is why this is a gate and not a note.
#
# A fourth is the ammunition's words. AMMOTYPE_SHOCK is the newest type the game
# has, so no language bank has a name for it, and the pickup message is assembled
# from switches: a type with no case gets neither name nor determiner and the
# player is told "Picked up .". The name half is reachable from --test-shock, so
# the test checks it; the determiner and the plural come out of the ROM's language
# banks, which are not loaded that early in start-up, so this script reads them out
# of the source instead.
#
# A fifth is the flag the orb must NOT carry. WEAPONFLAG2_EXPLODESWHENSHOT is the
# engine's own "any bullet sets me off", carried by the Rocket Launcher's rounds,
# and it is the cheap way to make the combo work. It is not the design: only this
# gun's beam is meant to set an orb off, so a rocket's bullet and every stray round
# in an arena must pass through them. A row cannot assert the absence of a bit to
# the compiler, so the gate reads the row.
#
# The sixth is a bit in a saved file. The curated random weapon list is one bit per
# row, so the row appended for this gun moves every field written after it by one
# bit: a version 8 file is read at 83 bits and a version 9 one at 84, and both have
# to keep reading at the width they were written at (mplayer.c). --test-shock
# round-trips both.
#
# What none of this reaches is the feel: that the beam answers instantly at 1.5,
# that the orb hangs long enough to be shot at and bounces the way a plasma ball
# should, and what the combination looks like when it works. That is eyes, and is
# checked by hand.
#
# --test-shock (main.c, shockSelfTest) puts the curated list back afterwards and
# writes no files of its own: the run is on its own savedir.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# Every gate here leaves at the first bad thing, and says what it is. An undefined
# fail() fails open, which is the one thing a gate must never do.
fail() { echo "shock: $*"; exit 1; }
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/shock-out}
LOG=$OUT/shock.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-shock --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "shock test" "$LOG" | sed 's/^.*shock test/shock test/'

FAILED=$(grep -c "shock test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "shock: $FAILED check(s) failed"
	exit 1
fi

grep -q "shock test: all .* checks as expected" "$LOG" || {
	echo "shock: the test never reported"
	exit 1
}

# The checks are the shell's whole reach; a run that reported fewer than the
# forty-two of them skipped some on the way, the way every other gate counts
# verdicts rather than trusting one line.
COUNT=$(grep -oE "shock test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 48 ]; then
	echo "shock: $COUNT checks ran, expected at least 48"
	exit 1
fi

# Nothing about this feature may write an error line
grep -E "^ERROR" "$LOG" && {
	echo "shock: an error line came out of a passing run"
	exit 1
}

# The whitelists, by adjacency: the Shock Rifle has to sit beside the Moonraker in
# each, and dropping out of one is what makes the shot invisible or silent.
between=$(grep -A2 "case WEAPON_LASER:" "$ROOT/src/game/bondgun.c" | grep -c "case WEAPON_SHOCKRIFLE:")
if [ "$between" -lt 2 ]; then
	fail "bondgun.c names the Shock Rifle in $between of the 2 lists it must (bgunCreateFx's beam, and the equip sound)"
fi
between=$(grep -A2 "case WEAPON_LASER:" "$ROOT/src/game/chraction.c" | grep -c "case WEAPON_SHOCKRIFLE:")
if [ "$between" -lt 1 ]; then
	fail "chraction.c's makebeam no longer names the Shock Rifle, so other players see no beam"
fi

# The body's held model, and the floor copy, which have to be the same gun.
# playermgrGetModelOfWeapon answers -1 for anything past WEAPON_PSYCHOSISGUN
# unless the number is named beside a model, which is the whole fork block.
grep -A4 "case WEAPON_SHOCKRIFLE:" "$ROOT/src/game/playermgr.c" | grep -q "MODEL_CHRAR34" \
	|| fail "a body holding the Shock Rifle holds nothing, or a different gun from the one on the floor"

# The shot has to be created under the orb's own number. bondgun.c's chain of
# branches decides a projectile's number per host, and its last branch makes the
# shot from the hand's gset - so with no branch of its own the orb is born with the
# RIFLE's number, meaning the flags of the row written to draw a hitscan beam are
# what flies, and every check after it that asks which orb this is gets the wrong
# answer. Same reason the Bowcaster's bolt has its own number.
grep -qF 'weaponCreateProjectileFromWeaponNum(funcdef->projectilemodelnum, WEAPON_SHOCKORB' "$ROOT/src/game/bondgun.c" \
	|| fail "the orb is created with the rifle's number, so it wears the beam's flags and nothing downstream can identify it"

# And a spent orb has to be deleted rather than landed: it is a projectile
# identity with no firing function now, but a landed weapon prop is still a
# pickup the floor would hand out.
grep -qF 'weapon->weaponnum == WEAPON_BOWCASTERBOLT || weapon->weaponnum == WEAPON_SHOCKORB' "$ROOT/src/game/propobj.c" \
	|| fail "a spent orb lands instead of fizzling, which makes it a pickup the floor hands out"

# The ammunition's words, the three switches and the PAL/JPN name table. AMMOTYPE_SHOCK
# missing from any of them is a pickup message with no name in it, which is a name the
# player never sees twice and never complains about clearly.
awk '/^void ammotypeGetDeterminer/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCK:" \
	|| fail "ammotypeGetDeterminer has no case for the Shock Rifle's ammunition: \"Picked up .\""
awk '/^void ammotypeGetPickupName/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCK:" \
	|| fail "ammotypeGetPickupName has no case for the Shock Rifle's ammunition: no name in the message"
awk '/^void ammotypePlayPickupSound/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCK:" \
	|| fail "the Shock Rifle's ammunition picks up in silence"
grep -q "^	{ AMMOTYPE_SHOCK," "$ROOT/src/game/propobj.c" \
	|| fail "the PAL and JPN ammo name table has no row for the Shock Rifle's ammunition"
# The loop that rewrites that table's fork rows must not stop at the first match:
# the Bowcaster's row is the one before this, and a break after it would leave the
# Shock Rifle's reading the crossbow's "bolt" on those builds, silently.
# A word match, not a substring one: grepping this slice for AMMOTYPE_SHOCK also
# matches a typo like AMMOTYPE_SHOCKX, and the gate said "as expected" for one.
awk '/^static void ammotypePortNames/,/^}/' "$ROOT/src/game/propobj.c" | grep -qw "AMMOTYPE_SHOCK" \
	|| fail "ammotypePortNames never rewrites the Shock Rifle's row, so it keeps the crossbow's \"bolt\""
if awk '/^static void ammotypePortNames/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "break;"; then
	fail "ammotypePortNames stops at the first fork row again, so the rows after it keep the crossbow's name"
fi
# The quantities, all three tables: a crate, and the gun picked up in each mode.
between=$(grep -Ec "case AMMOTYPE_SHOCK *: *qty = " "$ROOT/src/game/propobj.c")
if [ "$between" -lt 3 ]; then
	fail "the Shock Rifle's ammunition hand-out is missing from $((3 - between)) of its 3 quantity tables"
fi

# The orb pool is a separate ammunition type with the same full set of words:
# without any one of these a pickup says nothing, or says "cell" for an orb.
awk '/^void ammotypeGetDeterminer/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCKORB:" \
	|| fail "ammotypeGetDeterminer has no case for the orb pool"
awk '/^void ammotypeGetPickupName/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCKORB:" \
	|| fail "ammotypeGetPickupName has no case for the orb pool"
awk '/^void ammotypePlayPickupSound/,/^}/' "$ROOT/src/game/propobj.c" | grep -q "case AMMOTYPE_SHOCKORB:" \
	|| fail "the orb pool picks up in silence"
grep -q "^	{ AMMOTYPE_SHOCKORB," "$ROOT/src/game/propobj.c" \
	|| fail "the PAL and JPN ammo name table has no row for the orb pool"
awk '/^static void ammotypePortNames/,/^}/' "$ROOT/src/game/propobj.c" | grep -qw "AMMOTYPE_SHOCKORB" \
	|| fail "ammotypePortNames never rewrites the orb pool's row"
between=$(grep -Ec "case AMMOTYPE_SHOCKORB *: *qty = " "$ROOT/src/game/propobj.c")
if [ "$between" -lt 3 ]; then
	fail "the orb pool's hand-out is missing from $((3 - between)) of its 3 quantity tables"
fi
grep -qF '(weaponnum) != WEAPON_SHOCKORB && (weaponnum) != WEAPON_BOWCASTERBOLT' "$ROOT/src/include/constants.h" \
	|| fail "a projectile number can still be cycled to as a weapon"
grep -qF 'weaponnum == WEAPON_SHOCKORB || weaponnum == WEAPON_BOWCASTERBOLT' "$ROOT/src/game/inv.c" \
	|| fail "an orb or bolt can still become an inventory item"
grep -qF '!WEAPON_IS_FORK(weapon1)' "$ROOT/src/game/inv.c" \
	|| fail "a second fork gun can still become an Akimbo pair"

# The orb's row: the bits it must not carry. Neither is visible as a wrong number in
# a test - with STICKSTOWALL it sticks instead of bouncing, and with the explosive-
# when-shot flag any bullet in the arena pops every orb, which is the design's own
# prohibition rather than a crash. Comments are stripped before the search: the row's
# own note explains why the flag is absent, and a gate that reads its own explanation
# fails on a correct file, which is exactly what happened on this gate's first run.
awk '/^struct weapon invitem_shockorb = {/,/^};/' "$ROOT/src/game/invitems.c" | sed 's://.*::' \
		| grep -q "WEAPONFLAG2_EXPLODESWHENSHOT" \
	&& fail "the orb carries the any-bullet-sets-it-off flag: only the Shock Rifle's beam is meant to"
awk '/^struct weapon invitem_shockorb = {/,/^};/' "$ROOT/src/game/invitems.c" | sed 's://.*::' \
		| grep -q "WEAPONFLAG2_STICKSTOWALL" \
	&& fail "the orb carries STICKSTOWALL: it would stick in the wall instead of bouncing"

# The counts that make the new numbers legal. Each is derived from the newest id,
# not written as a literal, which is what lets the next gun append without a sweep.
grep -q "#define NUM_AMMOTYPES         (AMMOTYPE_SHOCKORB + 1)" "$ROOT/src/include/constants.h" \
	|| fail "NUM_AMMOTYPES is not derived from the newest ammunition type"
grep -q "#define NUM_MPWEAPONS             (MPWEAPON_GESHOTGUN + 1)" "$ROOT/src/include/constants.h" \
	|| fail "NUM_MPWEAPONS is not derived from the newest Combat Simulator row"
grep -q "#define MPSETUP_VERSION_LATEST MPSETUP_VERSION_GESHOTGUN" "$ROOT/src/include/constants.h" \
	|| fail "a saved setup does not write the curated list at the width that holds this row"

# Both sides of that list, load and save, have to weigh the new version: one of the
# two is enough to write a file the other misreads by one bit, and everything after
# the list in the block is then shifted.
between=$(grep -c "version >= MPSETUP_VERSION_GESHOTGUN ? NUM_MPWEAPONS" "$ROOT/src/game/mplayer/mplayer.c")
if [ "$between" -lt 2 ]; then
	fail "only $between of the 2 curated-list widths weigh the Shock Rifle's version, so a saved set is read shifted"
fi

# The newest ammunition type is now the bound on the arrays bots index by type, which
# is the same class as the crash the Rail-Laser's type found (bot.c sizes two stack
# arrays from NUM_AMMOTYPES after writing past them by a literal 33). Repeated here
# because the type that overruns them is a different one each time this is added to.
grep -qF 'struct prop *ammoproplist[NUM_AMMOTYPES]' "$ROOT/src/game/bot.c" || fail "botFindPickup's per-ammo-type array is not NUM_AMMOTYPES wide"
grep -qF 'f32 ammodistlist[NUM_AMMOTYPES]' "$ROOT/src/game/bot.c" || fail "botFindPickup's ammo distances are not NUM_AMMOTYPES wide"
grep -qF 'ammoproplist[33]' "$ROOT/src/game/bot.c" && fail "a bot's ammo-type array is sized by a literal count again"

# And the third of them: the simulant preference table, indexed by weapon number by
# AIBOTPREF() (data.h), botinvScoreWeapon() and the mod importer. It was a literal
# 128, which is below every number this fork has appended, so a bot scoring a dropped
# fork gun read past the end of it - a read the ROM's own source had a @dangerous
# comment about. It follows the enum now, and both the definition and the declaration
# have to say so: a short declaration alone would let the reads run short again while
# the definition kept a longer table.
grep -qE '#define BOTINV_MAX_WEAPONPREFS \(NUM_WEAPONS > 128 \? NUM_WEAPONS : 128\)' "$ROOT/src/include/constants.h" \
	|| fail "the simulant preference table's length is not derived from the weapon enum"
grep -qF 'g_AibotWeaponPreferences[BOTINV_MAX_WEAPONPREFS]' "$ROOT/src/include/data.h" \
	|| fail "data.h declares the simulant preference table shorter than botinv.c defines it"
grep -qF 'BOTINV_MAX_WEAPONPREFS >= NUM_WEAPONS' "$ROOT/src/game/botinv.c" \
	|| fail "nothing keeps the simulant preference table as wide as the weapon enum"

# The beam's cadence, which is a row of four numbers and easy to tune back to
# the pistol pace it had: 8 + 16 + 20 = 44 ticks (0.73s), and unk27 = -1 takes
# away the quick re-press, so a tap and a held trigger repeat at the same rate.
# The first shot of a fresh press is still immediate, as it is for the Magnum
# whose timing this is.
ROW=$(awk '/^struct weaponfunc_shootsingle invfunc_shockrifle_beam = \{/,/^\};/' "$ROOT/src/game/invitems.c")
[ -n "$ROW" ] || fail "invfunc_shockrifle_beam is missing from invitems.c"
echo "$ROW" | sed 's://.*::' | grep -qE '^[[:space:]]*20,' \
	|| fail "the beam's recoverytime60 is not the Magnum's 20"
echo "$ROW" | sed 's://.*::' | grep -qF '8, 16, 0, -1,' \
	|| fail "the beam's shot gap is not 8 + 16 + 20 = 44, with the tap shortcut off"

# One round per shot from either pool, as with any other two-mode gun, and
# read in all three places that spend ammunition: the player's readiness and
# deduction, and a simulant's firing loop. The old five-cell price was only a
# workaround for both modes sharing one reserve.
grep -qF 'return 1;' "$ROOT/port/src/shock.c" \
	|| fail "the orb's one-round price is not the one rule"
! grep -qF 'FUNC_SECONDARY ? 5' "$ROOT/port/src/shock.c" \
	|| fail "the old five-cell workaround is still in the ammo cost"
[ "$(grep -cF 'shockAmmoCostForFunc(hand->gset.weaponnum, hand->gset.weaponfunc)' "$ROOT/src/game/bondgun.c")" -ge 2 ] \
	|| fail "bondgun.c does not charge five cells on both the ready check and the shot"
grep -qF 'shockAmmoCostForFunc(gset.weaponnum, gset.weaponfunc)' "$ROOT/src/game/chraction.c" \
	|| fail "a simulant's shot does not spend the five cells"
grep -qF 'shockAmmoCostForFunc(aibot->weaponnum, aibot->gunfunc)' "$ROOT/src/game/bot.c" \
	|| fail "a simulant can start firing an orb with a clip too small to pay for it"

# The four feel tweaks: the lance is a thick bright beam with the Farsight's
# sound, the orb starts at the muzzle, is a glowing ball, and only this gun's
# beam can set it off.
awk '/^struct weaponfunc_shootsingle invfunc_shockrifle_beam = {/,/^};/' "$ROOT/src/game/invitems.c" \
		| grep -qF 'SFX_813E' \
	|| fail "the beam no longer fires with the Farsight's sound"
awk '/^struct weaponfunc_shootprojectile invfunc_shockrifle_ball = {/,/^};/' "$ROOT/src/game/invitems.c" \
		| grep -qF 'MODEL_CHRFLASHBANG' \
	|| fail "the orb is back to a grenade-shaped projectile"
grep -qF 'weaponHost(beam->weaponnum) == WEAPON_SHOCKRIFLE' "$ROOT/src/game/gunfx.c" \
	|| fail "the Shock Rifle's beam has no wider lance of its own"
grep -qF 'pos.x = newpos->x;' "$ROOT/src/game/bondgun.c" \
	|| fail "the orb still starts from the player's body root"
grep -qF 'weaponnum == WEAPON_SHOCKORB' "$ROOT/src/game/bondgun.c" \
	|| fail "the orb has no glow beam at launch"
grep -qF 'weapon->timer240 == 0' "$ROOT/src/game/propobj.c" \
	|| fail "nothing detonates an orb when the beam hits it"
grep -qF 'propExplode(prop, EXPLOSIONTYPE_SHOCKFLASH)' "$ROOT/src/game/propobj.c" \
	|| fail "a detonating orb makes no burst"
grep -qF 'weaponnum == WEAPON_SHOCKORB' "$ROOT/port/src/net/netpuppets.c" \
	|| fail "a client's orb has no glow trail"
grep -qF 'boltbeamSetBlue(beamnum)' "$ROOT/src/game/bondgun.c" \
	|| fail "the orb's glow is not tinted blue"
grep -qF 'boltbeamSetBlue(beamnum)' "$ROOT/port/src/net/netpuppets.c" \
	|| fail "a client's orb glow is not tinted blue"
# The glow and the hit target are both at the projectile's own position: the
# borrowed held-item model is hidden (its rest offset put the ball beside the
# light) and objTestHit() uses a sphere rather than the tiny borrowed mesh.
grep -qF 'mtx4TransformVec(camGetWorldToScreenMtxf(), &prop->pos, &centre);' "$ROOT/src/game/propobj.c" \
	|| fail "the orb's hit sphere is still placed by the camera-to-world matrix"
grep -qF 'dist2 <= 45.0f * 45.0f' "$ROOT/src/game/propobj.c" \
	|| fail "a beam can no longer hit the orb's light reliably"
grep -qF 'static Gfx *orbRender' "$ROOT/src/game/gunfx.c" \
	|| fail "the orb is still a flat beam ribbon rather than a round sprite"
grep -qF 'g_TexLightGlareConfigs[0], 4, 0, 2, true, NULL' "$ROOT/src/game/gunfx.c" \
	|| fail "the orb uses the shield's cube texture instead of a round glare"
awk '/^static Gfx \*orbRender/,/^}/' "$ROOT/src/game/gunfx.c" | grep -qF 'G_RM_AA_ZB_XLU_SURF' \
	|| fail "the orb's glow does not depth-test against scenery"
awk '/exp->type == EXPLOSIONTYPE_SHOCKFLASH/,/return gdl;/' "$ROOT/src/game/explosions.c" | grep -qF 'g_TexLightGlareConfigs[0]' \
	|| fail "the flash still uses a cube texture"
grep -qF '/*26*/ { 110, 110, 0,   0,    110,   90,   140,  6' "$ROOT/src/game/explosions.c" \
	|| fail "the flash does no damage or has no blast radius"
grep -qF 'g_BoltBeams[i].unk04 == 2' "$ROOT/src/game/gunfx.c" \
	|| fail "the blue orb is not routed to its round renderer"
grep -qF 'propExplode(prop, EXPLOSIONTYPE_SHOCKFLASH)' "$ROOT/src/game/propobj.c" \
	|| fail "an orb still detonates as a flame explosion"
grep -qF 'EXPLOSIONTYPE_SHOCKFLASH    26' "$ROOT/src/include/constants.h" \
	|| fail "the flash explosion type is missing"

echo "shock: as expected"
