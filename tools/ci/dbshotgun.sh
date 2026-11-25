#!/bin/bash
# dbshotgun.sh - the DB-Shotgun: alias, two-round double, untouched stock Shotgun, saved-setup width
#
# The fork's fourth own gun, and the first that needs no GoldenEye release: the
# GE shotgun's mesh fitted to the stock PD Shotgun's space, carried in a model
# pack and reached through a port alias over FILE_GSHOTGUN. The alias is a second
# file number, so the stock PD Shotgun keeps its own row and model; the GE
# shotgun, when that mod is installed, keeps its own too.
#
# Its one rule lives in port/src/dbshotgun.c so a player and a simulant cannot
# drift apart: the secondary is a twelve-pellet, two-round blast only while the
# two-round magazine is full, and the ordinary six-pellet single otherwise.
# --test-dbshotgun reads the rule in all three states, the two-round row, the
# alias and model state, the preference row, the untouched stock Shotgun, the
# Combat Simulator's row and both widths of a saved setup.
#
# What a self-test cannot see is the wiring. These source checks pin the four
# places that spend or price a shot (prop.c's pellets, bondgun.c's ready check
# and deduction, and a simulant's gate and deduction in bot.c/chraction.c), the
# clip carried from the deduction to the later shot creation, the curated list's
# two width ternaries and the version they belong to, the alias and its model
# state, and the pack's files. Each is a thing whose absence is silent: a bot
# paying one round for a double, a saved setup read one bit short, a gun drawn
# on the stock mesh with no word said about it. Every source check here has been
# mutation-proved: break it, watch this gate fail with its message, restore it.
#
# --test-dbshotgun writes no files of its own: the run is on its own savedir.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

# An undefined fail() fails open, which is the one thing a gate must never do.
fail() { echo "dbshotgun: $*"; exit 1; }
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/dbstgun-out}
LOG=$OUT/dbstgun.log

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"

( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 120 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-dbshotgun --exit-frame 120 \
	> "$LOG" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "run failed ($rc): see $LOG"; tail -20 "$LOG"; exit 2; }

grep -E "dbshotgun test" "$LOG" | sed 's/^.*dbshotgun test/dbshotgun test/'

FAILED=$(grep -c "dbshotgun test: FAIL" "$LOG")
if [ "$FAILED" -gt 0 ]; then
	echo "dbshotgun: $FAILED check(s) failed"
	exit 1
fi

grep -q "dbshotgun test: all .* checks as expected" "$LOG" || {
	echo "dbshotgun: the test never reported"
	exit 1
}

COUNT=$(grep -oE "dbshotgun test: all [0-9]+ checks" "$LOG" | grep -oE "[0-9]+")
if [ "${COUNT:-0}" -lt 24 ]; then
	echo "dbshotgun: $COUNT checks ran, expected at least 24"
	exit 1
fi

grep -E "^ERROR" "$LOG" && {
	echo "dbshotgun: an error line came out of a passing run"
	exit 1
}

# ---------------------------------------------------------------------------
# The four places that price or count a shot. Missing any one is a silent
# divergence: a bot's double costing one round, or pellets staying at six.
# ---------------------------------------------------------------------------
grep -qF 'dbShotgunPellets(weaponnum, gset.weaponfunc,' "$ROOT/src/game/prop.c" \
	|| fail "prop.c does not ask for the DB-Shotgun's pellet count, so a double still fires six"
[ "$(grep -cF 'dbShotgunAmmoCost(hand->gset.weaponnum, hand->gset.weaponfunc,' "$ROOT/src/game/bondgun.c")" -ge 2 ] \
	|| fail "bondgun.c does not charge two rounds on both the ready check and the shot"
grep -qF 'hand->dbshotgunShotClip = func->ammoindex' "$ROOT/src/game/bondgun.c" \
	|| fail "bondgun.c does not capture the clip before deducting the shot, so prop.c cannot tell a double"
grep -qF 'hand->dbshotgunShotClip);' "$ROOT/src/game/bondgun.c" \
	|| fail "the deduction prices the shot from the spent clip rather than the captured one"
grep -qF 'dbShotgunAmmoCost(aibot->weaponnum, aibot->gunfunc, aibot->loadedammo[i])' "$ROOT/src/game/bot.c" \
	|| fail "a simulant can start a double with a clip too small to pay for it"
grep -qF 'dbShotgunAmmoCost(gset.weaponnum, gset.weaponfunc,' "$ROOT/src/game/chraction.c" \
	|| fail "a simulant's shot does not spend the two rounds a player pays"

# ---------------------------------------------------------------------------
# The saved setup. One bit per curated row: a fourth appended row shifts every
# field after it, so both readers and the writer must weigh the new version or
# a file comes back one bit short.
# ---------------------------------------------------------------------------
between=$(grep -cF 'version >= MPSETUP_VERSION_GESHOTGUN ? NUM_MPWEAPONS' "$ROOT/src/game/mplayer/mplayer.c")
if [ "$between" -lt 2 ]; then
	fail "only $between of the 2 curated-list widths weigh the DB-Shotgun's version, so a saved set is read shifted"
fi
grep -qF '#define MPSETUP_VERSION_GESHOTGUN 10' "$ROOT/src/include/constants.h" \
	|| fail "there is no saved-setup version for the DB-Shotgun's bit"
grep -qF '#define MPSETUP_VERSION_LATEST MPSETUP_VERSION_GESHOTGUN' "$ROOT/src/include/constants.h" \
	|| fail "a saved setup is not written at the width that holds the DB-Shotgun's row"
grep -qF '#define NUM_MPWEAPONS             (MPWEAPON_GESHOTGUN + 1)' "$ROOT/src/include/constants.h" \
	|| fail "NUM_MPWEAPONS is not derived from the newest Combat Simulator row"
grep -qF '#define NET_PROTOCOL_VERSION 44' "$ROOT/port/include/net/netproto.h" \
	|| fail "the net protocol was not bumped for the widened local-player ammo block"

# ---------------------------------------------------------------------------
# The alias, the model state and the two rows that carry them.
# ---------------------------------------------------------------------------
grep -qF '"PpdDbShotgunZ"' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the DB-Shotgun's alias name is gone, so the pack's mesh resolves to no file"
grep -qF 'romdataRegisterAliasFile(s_PdDbShotgunProp, FILE_GSHOTGUN)' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the viewmodel alias is not over FILE_GSHOTGUN, so the stock Shotgun's file would move"
grep -qF '"PpdDbShotgunChr"' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the held/floor alias name is gone, so a chr's gun has no fitted mesh"
grep -qF 'romdataRegisterAliasFile(s_PdDbShotgunChrProp, FILE_PCHRSHOTGUN)' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the held alias is not over the chr Shotgun file - the hand's rest space would be lost"
grep -qF -- 'state->fileid = (u16) chrfileid;' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the held model state still reads the first-person file - the gun will point up in a chr's hand"
grep -qF -- '->hi_model = (u16) viewfileid;' "$ROOT/port/src/dbshotgun.c" \
	|| fail "the first-person rows no longer use the viewmodel alias"
grep -qF 'MODEL_PD_GESHOTGUN          (MODEL_PD_RAILLASER + 1)' "$ROOT/src/include/constants.h" \
	|| fail "the model state is not the one after the Moonrailer's"
grep -qF '_Static_assert(MODEL_PD_GESHOTGUN < MODEL_REMAKE_FIRST' "$ROOT/port/src/geguns.c" \
	|| fail "nothing keeps the DB-Shotgun's model state below the remake's models"
awk '/case WEAPON_GESHOTGUN:/,/break;/' "$ROOT/src/game/playermgr.c" | grep -qF 'MODEL_PD_GESHOTGUN' \
	|| fail "a body holds the DB-Shotgun in no model at all"
grep -qF '[WEAPON_GESHOTGUN        ] = &invitem_geshotgun' "$ROOT/src/game/invitems.c" \
	|| fail "the weapon table has no DB-Shotgun row"
grep -qF '[WEAPON_GESHOTGUN]' "$ROOT/src/game/botinv.c" \
	|| fail "a simulant has no preference row for the DB-Shotgun"
grep -qF '{ WEAPON_GESHOTGUN,         AMMOTYPE_SHOTGUN' "$ROOT/src/game/mplayer/mplayer.c" \
	|| fail "the Combat Simulator has no DB-Shotgun row"

# The secondary must not be the engine's two-frame burst: that would be two
# shots of six over two frames, not one shot of twelve. Read the row, with its
# own explanation stripped first.
ROW=$(awk '/^struct weaponfunc_shootsingle invfunc_geshotgun_double = \{/,/^\};/' "$ROOT/src/game/invitems.c")
[ -n "$ROW" ] || fail "invfunc_geshotgun_double is missing from invitems.c"
echo "$ROW" | sed 's://.*::' | grep -qF 'FUNCFLAG_BURST2' \
	&& fail "the secondary is a two-frame burst, so a double is two shots of six rather than one of twelve"

# The stock PD Shotgun's row and its two-round clone must stay distinct: the
# clone's magazine is 2 and the stock's is still 9.
grep -qF '2, // clip size: the DB-Shotgun' "$ROOT/src/game/invitems.c" \
	|| fail "the DB-Shotgun's magazine is no longer two rounds"
grep -qF '9, // clip size' "$ROOT/src/game/invitems.c" \
	|| fail "the stock PD Shotgun's nine-round magazine was changed"

# ---------------------------------------------------------------------------
# The pack itself. Not enabled by shipped config; pd.ini's Mod.LoadModels=1
# and Mod.ModelPack=dbshotgun (or the Options dropdown) turn it on.
# ---------------------------------------------------------------------------
PACK=$BUILD/model-packs/dbshotgun/n64
[ -f "$PACK/PpdDbShotgunZ.obj" ] || fail "the fitted viewmodel mesh is missing from $PACK"
[ -f "$PACK/PpdDbShotgunZ.mtl" ] || fail "the fitted viewmodel's material file is missing"
[ -f "$PACK/PpdDbShotgunChr.obj" ] || fail "the fitted held mesh is missing from $PACK - a chr's gun would use the view fit"
[ -f "$PACK/PpdDbShotgunChr.mtl" ] || fail "the fitted held mesh's material file is missing"

# The pack folder above is the source the build embeds, not something a
# player has to install. CMake compiles it into modelpacks.c, startup mounts
# it in memory, and the self-test asks for both meshes with no pack selected.
grep -qF 'tools/modelpack/embed.py' "$ROOT/CMakeLists.txt" \
	|| fail "the build no longer compiles the fitted meshes into the executable"
grep -qF 'modelpackInitBuiltin();' "$ROOT/port/src/main.c" \
	|| fail "the built-in meshes are never mounted at startup"
grep -q 'both fitted meshes are built in and resolve without a model pack' "$ROOT/port/src/dbshotgun.c" \
	|| fail "nothing checks that the meshes work with model packs off"
grep -qF 'dbshotgun test: ok   both fitted meshes are built in and resolve without a model pack' "$LOG" \
	|| fail "the built-in meshes did not resolve with model packs off"

# A player killed by the shotgun drops it, and the falling-weapon tick asks
# the model for a bbox. The embedded mesh has no bbox node, so that lookup is
# NULL: reading bbox->xmax faulted at 0x8 on both host and client.
grep -qF 'faulted at bbox->xmax (fault address 0x8' "$ROOT/src/game/propobj.c" \
	|| fail "a dropped DB-Shotgun can fault on a missing model bbox again"

for png in 00_0x098EA1A5 01_0x098E51A5 02_0x09BEA1A5 03_0x09BE51A5 04_0x0D0689B5 05_0x051D5125; do
	[ -f "$PACK/shotgun/$png.png" ] || fail "the pack is missing texture $png.png"
done

# A first-person gun's mesh must be posed under the matrix its GUNDL list
# loads (gebeanListLoadedMatrix(), matrix 33), the same one the stock gun body
# and the hands use. The model pack's replacement reaches this through the
# frompack GUNDL branch; without it the mesh sits still while the hands recoil
# and the muzzle flash (which follows the animated muzzle node) comes adrift.
grep -qF 'frompack && node && (node->type & 0xff) == MODELNODETYPE_GUNDL' "$ROOT/port/src/xblamesh.c" \
	|| fail "the pack's first-person gun is not posed under the list-loaded gun matrix"
grep -qF 'xblaMeshIsViewmodel(model)' "$ROOT/port/src/xblamesh.c" \
	|| fail "the pack's gun list matrix is not limited to first-person hands - a world pickup draws as a grey box"

echo "dbshotgun: $COUNT runtime checks, the shot's four call sites, both setup widths, the alias, the rows, the pack and the first-person pose as expected"
