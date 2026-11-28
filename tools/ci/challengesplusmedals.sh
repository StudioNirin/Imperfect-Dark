#!/bin/bash
# challengesplusmedals.sh - Challenges+ medal stars, bonus requirements and 16 sims.
#
# The importer check regenerates the Plus C datasets from the 30 template docs
# and diffs them, which validates the bonus field, requirement IDs, value types,
# 8-16 sim rows and every C identifier. The source gates below pin the runtime
# wiring that a doc diff cannot see: the Plus-only sim extension, the
# team-aggregated grader, the save and MATCH_END bit packing, the star tint and
# the end-game panels.
#
# Patterns use grep -F where they contain brackets, so a check cannot pass or
# fail on an escaping accident.
set -u
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC=$ROOT/src
PORT=$ROOT/port
TOOLS=$ROOT/tools
IMPORTER=$TOOLS/ci/challengesplus_import.py
DOCS=$ROOT/ImperfectDark-Docs/challenges+

fail() { echo "challengesplusmedals: $*"; exit 1; }
pass() { echo "challengesplusmedals: ok - $*"; }

count() { grep -cF -- "$2" "$1"; }

python3 "$IMPORTER" --check "$ROOT" || fail 'docs out of sync with generated C'
pass '30 templates parse and match the generated datasets'

CHAL=$SRC/game/challenge.c
CFG=$SRC/game/mpconfigsplus.c
HDR=$SRC/include/game/challenge.h
SETUP=$SRC/game/mplayer/setup.c
MP=$SRC/game/mplayer/mplayer.c
GF=$SRC/game/gamefile.c
NS=$PORT/src/net/netsession.c
INGAME=$SRC/game/mplayer/ingame.c
MPSTATS=$SRC/game/mpstats.c
LV=$SRC/game/lv.c

# --- description panel -------------------------------------------------------
grep -qF 'Bonus - Finish Time' "$CFG" || fail 'tier description not generated from the template'
grep -qF 'Perfect - ' "$CFG" || fail 'perfect tier not described in the panel'
[ "$(count "$CFG" 'char g_MpPlusTierDescriptions[30][4][320]')" -eq 1 ] || fail 'per-tier descriptions not generated'

# The description scrollables are unscrollable, so they must be tall enough to
# show every bonus line without clipping.
[ "$(count "$SETUP" 'PAL ? 0x78 : 0x68')" -eq 4 ] || fail 'challenge description panels not tall enough'
grep -q 'mpChallengesTierTabsMenuHandler' "$SETUP" || fail 'tier tabs handler missing'
pass 'full descriptions with every bonus tier, shown per player count'

# --- requirement registry and generated arrays -------------------------------
grep -q 'enum challengebonusrequirement' "$HDR" || fail 'no requirement registry'
grep -qF 'CHALBONUS_FINISH_TIME' "$HDR" || fail 'registry incomplete'
grep -qF 'g_MpChallengeBonusPlus' "$HDR" || fail 'bonus table not declared'
grep -qF 'g_MpPlusScoreLimits[30][4]' "$HDR" || fail 'per-player-count score goals not declared'
grep -qF 'g_MpPlusExtraSims' "$HDR" || fail 'extra sim array not declared'
grep -qF 'g_MpPlusExtraBotNames' "$HDR" || fail 'extra bot names not declared'
grep -qF 's32 challengeHumanCount(void)' "$HDR" || fail 'no human-count helper for the tiers'
grep -qF 'struct challengebonus g_MpChallengeBonusPlus[30]' "$CFG" || fail 'bonus table not generated'
grep -qF 'struct mpconfigsim g_MpPlusExtraSims[30][8]' "$CFG" || fail 'extra sim array not generated'
grep -qF 'u8 g_MpChallengesPlusConverted[30]' "$CFG" || fail 'converted flags not generated'
grep -qF 'u8 g_MpPlusStartArmed[30]' "$CFG" || fail 'start armed setting not generated'
[ "$(count "$CFG" '{ CHALBONUS')" -eq 30 ] || fail 'not all 30 challenges have a bonus record'
pass 'registry, per-tier goals and generated arrays'

# Only a template renamed to the challenge+-NN format may appear in the Plus
# list, so the author converts challenges one at a time.
[ "$(count "$CHAL" 'return challengePlusConverted(challengeindex);')" -eq 3 ] || fail 'Plus list not gated on converted docs'
pass 'unconverted entries stay out of the Plus lists'

# --- 16 configurable sims ----------------------------------------------------
grep -q 'void challengeApplyPlusSims' "$CHAL" || fail 'no Plus sim apply'
grep -qF 'challengeApplyPlusSims();' "$CHAL" || fail 'Plus sims not applied at match start'
grep -qF 'g_ChallengeMode ? MAX_BOTS_CONFIG + 8 : MAX_BOTS_CONFIG' "$CHAL" || fail 'sim count not widened for Plus'
grep -qF '&g_MpPlusExtraSims[challengeindex][i - MAX_BOTS_CONFIG]' "$CHAL" || fail 'extra sims not checked for unlocks'
grep -qF 'sizeof(struct mpconfig) == 116' "$MP" || fail 'ROM config layout assert removed'
grep -qF 'sizeof(struct mpstrings) == 320' "$MP" || fail 'ROM strings layout assert removed'
pass '16 sims wired without growing the ROM structs'

# --- team-aggregated grader --------------------------------------------------
grep -q 'void challengeEvaluateBonus' "$CHAL" || fail 'no grader'
grep -qF 'challengeEvaluateBonus(' "$MP" || fail 'grader not called at match end'
grep -q 'netChallengeRecordTier' "$MP" || fail 'grader does not use the room tier'
grep -qF 'challengeSetMedal(g_MpChallengeIndex, tier, g_ChallengeMedalLevel)' "$NS" || fail 'host keeps bronze while the client gets the graded star'
grep -qF 'g_ChallengeMedalLevel = (s_End.challenge >> 2) & 3' "$NS" || fail 'client does not show the host verdict'
grep -qF 'mpchr->team != 0' "$CHAL" || fail 'grader does not filter team 0'
grep -qF 'setCurrentPlayerNum(i)' "$CHAL" || fail 'grader does not switch current player'
grep -qF 'm.hits * 100 / m.shots' "$CHAL" || fail 'accuracy not combined from raw counters'
grep -qF 'CHALBONUS_FINISH_TIME:' "$CHAL" || fail 'finish-time condition missing'
grep -qF 's32 tier = challengeHumanCount() - 1;' "$CHAL" || fail 'thresholds not picked by player count'
grep -qF 'bonus->perfect[tier] > 0' "$CHAL" || fail 'perfect tier not gated on a set threshold'
pass 'grader aggregates team 0 with per-tier goals and thresholds'

# --- storage and net ---------------------------------------------------------
NETPLUS=$ROOT/port/src/challengeplus.c
grep -qF 'challengeSetMedalIn(g_MpChallengesPlus, i, j + 1, value - 1)' "$NETPLUS" || fail 'stars not read from the star file'
grep -qF 'u32 value = (completion & 1)' "$NETPLUS" || fail 'stars not written to the star file'
grep -qF '(7 << 5)' "$CHAL" || fail 'medal bits not in spare byte bits'
grep -qF '(g_ChallengeMedalLevel & 3) << 2' "$NS" || fail 'medal not packed into MATCH_END'
grep -qF 'challengeSetMedal(s_End.challengeindex, tier' "$NS" || fail 'client does not store host medal'
grep -qF 'NETMSG_MATCH_END' "$NS" || fail 'MATCH_END missing'
pass 'medal bits round-trip through save and MATCH_END'

# --- star tint ---------------------------------------------------------------
grep -q 'mpChallengeStarColour' "$SETUP" || fail 'no star tint helper'
grep -q 'challengeGetMedalBySlot' "$CHAL" || fail 'no slot medal accessor'
grep -qF '0xc0c0c000' "$SETUP" || fail 'silver star colour missing'
grep -qF '0x9a5a1e00' "$SETUP" || fail 'bronze star colour missing'
grep -qF 'menuGetSinOscFrac' "$SETUP" || fail 'perfect star does not shimmer'
pass 'stars tinted bronze/silver/gold/perfect for Plus entries'

# --- end-game panels ---------------------------------------------------------
grep -q 'mpMenuTextChallengeResult' "$INGAME" || fail 'challenge result text missing'
[ "$(count "$INGAME" 'g_MpChallengeResultMenuItems,')" -eq 3 ] || fail 'result line not on all three challenge screens'
grep -q 'g_MpEndscreenExtendedStatsMenuDialog' "$INGAME" || fail 'extended stats panel missing'
[ "$(count "$INGAME" '&g_MpEndscreenExtendedStatsMenuDialog')" -eq 1 ] || fail 'extended stats panel not in end-game chain'
grep -qF '&g_MpEndscreenPlayerStatsMenuDialog' "$INGAME" || fail 'stats panel dropped from the chain'
grep -q 'mpMenuTextExtendedStatValue' "$INGAME" || fail 'extended stats rows missing'
# The success dialog's alternative label colour is checkbox_checked_unfocused,
# which that palette sets to 0x00000000: an alt-colour label on Challenge
# Completed is invisible except mid-swipe while two palettes blend.
if awk '/g_MpChallengeResultMenuItems\[\] = \{/,/MENUITEMTYPE_END/' "$INGAME" | grep -q 'ALTCOLOUR'; then
	fail 'challenge result line uses the transparent success colour'
fi
grep -qF 'MENUITEMFLAG_LABEL_CUSTOMCOLOUR' "$INGAME" || fail 'challenge result line not medal tinted'
# A custom render handler in a dialog row has crashed the port twice (returning
# 0 as the Gfx pointer, and a width read as a lang id), so the panel stays a
# plain column of label rows that the menu lays out itself.
if awk '/g_MpExtendedStatsMenuItems\[\] = \{/,/MENUITEMTYPE_END/' "$INGAME" | grep -q 'CUSTOMRENDER'; then
	fail 'extended stats panel uses a custom renderer'
fi
pass 'challenge result and extended stats panels wired'

# --- kill-streak stats -------------------------------------------------------
# maxsimulkills cannot express these: four kills inside the two second window is
# one largest streak but three multi-kills, and a spree counts a whole life
# rather than a window. All three read fields this build used to leave empty.
grep -qF 'multikillcount += simulkills - 1' "$MPSTATS" || fail 'multi-kills not counted per streak'
grep -qF 'maxkills = g_Vars.currentplayer->killsthislife' "$MPSTATS" || fail 'longest spree not tracked per life'
grep -qF 's32 multikillcount;' "$SRC/include/types.h" || fail 'multi-kill field not named'
grep -qF 'playerstats[i].multikillcount = 0' "$LV" || fail 'multi-kills not reset between matches'
grep -qF 'playerstats[i].maxkills = 0' "$LV" || fail 'spree not reset between matches'
grep -q 'case CHALBONUS_MULTIKILLS' "$CHAL" || fail 'multi-kills not gradable'
grep -q 'case CHALBONUS_LONGEST_SPREE' "$CHAL" || fail 'longest spree not gradable'
grep -qF '"MULTIKILLS"' "$IMPORTER" || fail 'MULTIKILLS tag not in the importer registry'
grep -qF '"LONGEST_SPREE"' "$IMPORTER" || fail 'LONGEST_SPREE tag not in the importer registry'
grep -qF '"Multi-kills\n"' "$INGAME" || fail 'multi-kills row missing from the stats panel'
grep -qF '"Max multi-kills\n"' "$INGAME" || fail 'best-streak row missing from the stats panel'
grep -qF '"Longest spree\n"' "$INGAME" || fail 'longest spree row missing from the stats panel'
grep -qF 'stats->multikillcount' "$INGAME" || fail 'multi-kills row does not read the stat'
grep -qF 'stats->maxkills' "$INGAME" || fail 'longest spree row does not read the stat'
TPLS=("$DOCS"/challenge-[0-9][0-9].md "$DOCS"/challenge+-[0-9][0-9].md)
[ "${#TPLS[@]}" -eq 30 ] || fail "expected 30 challenge templates, found ${#TPLS[@]}"
for f in "${TPLS[@]}"; do
	grep -qF '`MULTIKILLS`' "$f" || fail "$(basename "$f") does not list the MULTIKILLS tag"
	grep -qF '`LONGEST_SPREE`' "$f" || fail "$(basename "$f") does not list the LONGEST_SPREE tag"
done
pass 'streak stats tracked, gradable, shown and listed in all 30 templates'

# --- save layout and star records -------------------------------------------
# The GAME note body is 0xa0 and stock used all but three bits of it, so stars
# go in a per-agent file beside the setups rather than being appended there.
# Save files moved from $S to $V to keep them apart from config and recordings.
GATEFS=$ROOT/port/src/fs.c
GATELIB=$ROOT/port/src/libultra.c
GATEPLUS=$ROOT/port/src/challengeplus.c
GATEGFILE=$SRC/game/gamefile.c
grep -qF 'expStr = saveFilesDir' "$GATEFS" || fail 'no $V placeholder'
grep -q 'fsMoveSaveFilesIntoSubdir' "$GATEFS" || fail 'flat saves are not migrated into saves/'
grep -qF '"$V/" EEPROM_FNAME' "$GATELIB" || fail 'eeprom not in the save-files folder'
grep -qF '"$V/challengeplus.bin"' "$GATEPLUS" || fail 'star file not in the save-files folder'
grep -qF 'challengeplusLoad();' "$GATEGFILE" || fail 'stars not loaded with the agent'
grep -qF 'challengeplusSave();' "$GATEGFILE" || fail 'stars not saved with the agent'
if grep -qF 'savebufferOr(&buffer, challengeGetMedalIn' "$GATEGFILE"; then
	fail 'plus medals still appended to the truncated GAME note'
fi
pass 'save files in their own folder, stars in a per-agent file'

echo "challengesplusmedals: all gates passed"
