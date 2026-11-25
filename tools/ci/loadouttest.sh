#!/bin/bash
# loadouttest.sh - Start Armed's Loadout mode: two configured guns as every
# player's kit, no map pickup for them, long respawn for the spare slots, and
# the Zombies survivor pair. The port self-tests drive the row resolution;
# source pins hold the spawn hand-out and suppression in place.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
BIN=${BIN:-$BUILD/pd.x86_64}
OUT=${OUT:-$BUILD/loadout-out}

[ -x "$BIN" ] || { echo "no $BIN"; exit 1; }
[ -f "$BUILD/data/pd.ntsc-final.z64" ] || { echo "no $BUILD/data/pd.ntsc-final.z64"; exit 1; }

fail() { echo "loadout: $*"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/save"
printf '[Mod]\n' > "$OUT/save/pd.ini"

# The two startup self-tests: Loadout's row resolution, and Start Armed's
# option list now offering Loadout.
( cd "$BUILD" && SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-offscreen} timeout -k 5 60 "$BIN" \
	--savedir "$OUT/save" --skip-intro --no-sound --test-loadout --test-startarmed --exit-frame 40 \
	> "$OUT/log" 2>&1 )
rc=$?
[ $rc -eq 0 ] || { echo "loadout: self-test run failed ($rc)"; tail -20 "$OUT/log"; exit 2; }

grep -E "loadout test|startarmed test" "$OUT/log" | sed 's/^.*\(loadout test\|startarmed test\)/\1/'

grep -q 'loadout test: FAIL' "$OUT/log" && fail 'a loadout self-test check failed'
grep -q 'loadout test: 5 of 5 checks as expected' "$OUT/log" || fail 'the loadout self-test did not report'
grep -q 'startarmed test: FAIL' "$OUT/log" && fail 'a startarmed check failed'
grep -q 'startarmed test: all 6 checks as expected' "$OUT/log" || fail 'the startarmed self-test did not report'

S=$ROOT/src
P=$ROOT/port

# The enum and both dropdowns must carry the fourth option.
grep -qF '#define SPAWNWEAPON_LOADOUT 3' "$S/include/constants.h" || fail 'no Loadout enum'
grep -qF '"Loadout"' "$S/game/mplayer/setup.c" || fail 'the MP Weapons dropdown has no Loadout row'
grep -qF '"Loadout"' "$P/src/optionsmenu.c" || fail 'the Mods: Player dropdown has no Loadout row'

# The two chosen rows must reach clients before stage load.
grep -qF '{ "Mod.LoadoutWeapon1",          NETKEY_SYNC }' "$P/src/net/netrules.c" || fail 'gun one is not a SYNC rule'
grep -qF '{ "Mod.LoadoutWeapon2",          NETKEY_SYNC }' "$P/src/net/netrules.c" || fail 'gun two is not a SYNC rule'
grep -qF '#define NET_PROTOCOL_VERSION 45' "$P/include/net/netproto.h" || fail 'the protocol was not bumped for the Shock Rifle orb pool'

# Both humans (player.c) and sims (bot.c) get the two guns, in ordinary modes
# and for Zombies survivors; infected keep fists.
grep -qF 'modGetSpawnWeapon() == SPAWNWEAPON_LOADOUT' "$S/game/player.c" || fail 'players do not get the Loadout kit'
grep -qF 'modGetSpawnWeapon() == SPAWNWEAPON_LOADOUT' "$S/game/bot.c" || fail 'simulants do not get the Loadout kit'
grep -qF 'invGiveSingleWeapon(weapon1)' "$S/game/player.c" || fail 'player does not get gun one'
grep -qF 'invGiveSingleWeapon(weapon2)' "$S/game/player.c" || fail 'player does not get gun two'
grep -qF 'botinvGiveSingleWeapon(chr, weapon1)' "$S/game/bot.c" || fail 'sim does not get gun one'
grep -qF 'botinvGiveSingleWeapon(chr, weapon2)' "$S/game/bot.c" || fail 'sim does not get gun two'
grep -qF 'zombiesSeatIsInfected(g_Vars.currentplayernum)' "$S/game/player.c" || fail 'infected player no longer gets fists'
grep -qF 'zombiesIsInfected(chr)' "$S/game/bot.c" || fail 'infected sim no longer gets fists'

# Slots 1-2 never place a pickup; the spare slots respawn slowly.
grep -qF 'mpGetMpWeaponByLocationWithSlot' "$S/game/setup.c" || fail 'placement does not ask which slot a location maps to'
grep -qF 'weaponslot >= 0 && weaponslot < 2' "$S/game/setup.c" || fail 'the first two slots still spawn pickups'
grep -qF 'SPAWNWEAPON_LOADOUT ? TICKS(4800) : TICKS(1200)' "$S/game/prop.c" || fail 'spare pickups do not get a long respawn'

echo "loadout: as expected"
