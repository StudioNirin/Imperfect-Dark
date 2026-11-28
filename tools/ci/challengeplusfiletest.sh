#!/bin/bash
# challengeplusfiletest.sh - the star file round trips, one block per agent.
#
# The GAME note's body was too short for the medals, so port/src/challengeplus.c
# keeps them in a versioned file beside the save files. This compiles that one
# file against a fake filesystem (challengeplusfiletest.c), so a reload, a
# second agent and a damaged file can be checked without playing a match.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/build}
CC=${CC:-cc}
OUT=${OUT:-/tmp/opencode/challengeplusfiletest}

[ -f "$BUILD/port/include/versioninfo.h" ] || { echo "challengeplusfiletest: build first"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT/bin" "$OUT/save"

"$CC" -std=c11 -D_LANGUAGE_C=1 -DVERSION=2 -DPAL=0 -DPLATFORM_64BIT=1 \
	-DMATCHING=0 -DAVOID_UB=1 -DPIRACYCHECKS=0 -DROM_SIZE=32 -DPD_HAVE_CURL \
	-I"$ROOT/include" -I"$ROOT/include/PR" -I"$ROOT/src/include" \
	-I"$BUILD/src/generated/ntsc-final" -I"$ROOT/port/include" \
	-I"$BUILD/port/include" -I"$BUILD" -include versioninfo.h \
	"$ROOT/tools/ci/challengeplusfiletest.c" "$ROOT/port/src/challengeplus.c" \
	-o "$OUT/bin/test"

"$OUT/bin/test" "$OUT/save"
