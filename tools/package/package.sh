#!/usr/bin/env bash
#
# Builds the downloadable packages, in two kinds.
#
#   --clean  what the release job makes: the program, its DLLs, README,
#            LICENSE, and empty data/ and added-content/ with a note in each
#            saying what goes there. Nothing copyrighted.
#
#   --full   the clean package plus everything in payload/, which is the ROM
#            and the third-party content, laid out as the game expects to find
#            them. Sendable only to people who own the games; see
#            payload/README.txt.
#
# payload/ is in .gitignore and is never part of the build: deleting its
# contents and rebuilding with --clean gives the clean package with no residue,
# which is the point of keeping them apart. --full prints the exact list of
# files it added, so a tree can be put back.
#
# A Windows package can be made here with the cross toolchain (see
# docs/windows-build.md and cmake/mingw-w64.cmake) or taken from a CI run with
# --from-artifact, which is what to use when the toolchain is not set up:
#
#   gh run download <run-id> -R StudioNirin/Imperfect-Dark -D /tmp/art
#   tools/package/package.sh --windows --full --from-artifact /tmp/art
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST="$ROOT/dist"
PAYLOAD="$ROOT/payload"
ROMID="ntsc-final"
CHANNEL="dev"
ROM="data/pd.${ROMID}.z64"
ROM_MD5="e03b088b6ac9e0080440efed07c1e40f"

platforms=()
mode=""
artifact=""

die() { printf '%s\n' "package: $*" >&2; exit 1; }
note() { printf '%s\n' "package: $*" >&2; }

usage() { sed -n '3,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
	case "$1" in
	--windows) platforms+=("windows") ;;
	--linux) platforms+=("linux") ;;
	--clean) [ -z "$mode" ] || die "choose one of --clean and --full"; mode="clean" ;;
	--full) [ -z "$mode" ] || die "choose one of --clean and --full"; mode="full" ;;
	--from-artifact)
		[ $# -ge 2 ] || die "--from-artifact wants a directory"
		[ -d "$2" ] || die "$2 is not a directory"
		artifact="$2"
		shift
		;;
	--out) [ $# -ge 2 ] || die "--out wants a directory"; DIST="$2"; shift ;;
	-h | --help) usage; exit 0 ;;
	*) die "unknown option $1 (--help)" ;;
	esac
	shift
done

[ ${#platforms[@]} -gt 0 ] || platforms=(windows linux)
[ -n "$mode" ] || die "say --clean or --full"

COMMIT="$(git -C "$ROOT" rev-parse --short HEAD)"
JOBS="$(nproc 2>/dev/null || echo 4)"

# The prefix cmake/mingw-w64.cmake looks in for SDL2 and a static zlib.
MINGW_PREFIX="$HOME/.local/mingw64"

# The clean package's contents, from a build directory. The release job does
# the same in .github/workflows/dabs-mod.yml; keep the two in step.
stage_windows_from_build() { # <build dir> <staging>
	local build="$1" stage="$2" exe dll name found sysroot

	exe="$build/pd.x86_64.exe"
	[ -f "$exe" ] || die "$exe is not there; build it or use --from-artifact"
	cp "$exe" "$stage/"

	# The DLL closure, read from the binary rather than written out by hand:
	# a list kept by hand is one DLL short the first time a dependency
	# changes, and that is a game that will not start with no message to
	# act on. winhttp and the rest come with Windows. winpthreads and
	# libgcc must be linked in rather than shipped, because Check for
	# Updates replaces only the exe and would leave an old copy beside it.
	if objdump -p "$exe" | grep -qi -E 'DLL Name: (libwinpthread|libgcc_s)'; then
		die "pd.x86_64.exe imports winpthreads or libgcc from a DLL; see docs/windows-build.md"
	fi
	for sysroot in "$MINGW_PREFIX/bin" /usr/x86_64-w64-mingw32/bin; do
		[ -d "$sysroot" ] || continue
		while read -r dll; do
			name="$(basename "$dll")"
			found="$(find "$sysroot" -maxdepth 1 -iname "$name" -print -quit 2>/dev/null || true)"
			if [ -n "$found" ]; then
				cp -n "$found" "$stage/"
			fi
		done < <(objdump -p "$exe" | awk '/DLL Name:/ { print $3 }')
	done

	# The two the game cannot run without. zlib's DLL is zlib1.dll through
	# 1.3 and libz-1.dll from 1.4, so accept whichever the binary imports.
	[ -e "$stage/SDL2.dll" ] || die "SDL2.dll is missing from the package"
	if ! objdump -p "$exe" | grep -qi -E 'DLL Name: (zlib1|libz-1)\.dll'; then
		die "pd.x86_64.exe does not import zlib at all"
	fi
	if [ ! -e "$stage/zlib1.dll" ] && [ ! -e "$stage/libz-1.dll" ]; then
		die "the zlib DLL is missing from the package"
	fi
}

stage_linux_from_build() { # <build dir> <staging>
	local build="$1" stage="$2"

	[ -f "$build/pd.x86_64" ] || die "$build/pd.x86_64 is not there"
	cp "$build/pd.x86_64" "$stage/"
	# Linux carries its libraries in the system, so there is no closure to
	# copy; ldd is only the same sanity check the release job makes.
	if command -v ldd >/dev/null && ! ldd "$stage/pd.x86_64" 2>/dev/null | grep -q libcurl; then
		note "this build has no libcurl: Ghost Trials will not reach a server"
	fi
}

strip_one() { # <platform> <stage>: the debug info beside the package rather
              # than inside it, as the release job does. The binary keeps its
              # symbol table; a crash report's offsets read the same off the
              # .debug as off the whole binary.
	local platform="$1" stage="$2" exe objcopy="objcopy" debug

	case "$platform" in
	windows)
		exe="pd.x86_64.exe"
		command -v x86_64-w64-mingw32-objcopy >/dev/null && objcopy=x86_64-w64-mingw32-objcopy
		;;
	*) exe="pd.x86_64" ;;
	esac

	command -v "$objcopy" >/dev/null || {
		note "no $objcopy: the package keeps its debug info"
		return 0
	}

	mkdir -p "$DIST/debug"
	debug="$DIST/debug/$exe.debug"
	"$objcopy" --only-keep-debug "$stage/$exe" "$debug"
	# objcopy looks for the named file relative to the working directory, so
	# it runs from where that path is true; the name written into the binary is
	# the plain file name, which is what to put beside it to symbolise a crash.
	(cd "$DIST" && "$objcopy" --strip-debug --add-gnu-debuglink="debug/$exe.debug" "$stage/$exe")
	note "debug info for this package in $debug"
}

build_one() { # <platform> -> the build directory on stdout
	local platform="$1" build

	case "$platform" in
	windows) build="$ROOT/build-windows" ;;
	*) build="$ROOT/build" ;;
	esac

	if [ "$platform" = windows ] && [ ! -d "$MINGW_PREFIX" ]; then
		die "$MINGW_PREFIX is not set up; see docs/windows-build.md, or pass --from-artifact"
	fi

	note "configuring $platform"
	if [ "$platform" = windows ]; then
		cmake -G"Unix Makefiles" -B"$build" -DROMID="$ROMID" -DUPDATE_CHANNEL="$CHANNEL" \
			-DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64.cmake" "$ROOT" >/dev/null
	else
		cmake -G"Unix Makefiles" -B"$build" -DROMID="$ROMID" -DUPDATE_CHANNEL="$CHANNEL" "$ROOT" >/dev/null
	fi

	note "building $platform ($JOBS jobs)"
	cmake --build "$build" -j"$JOBS" >/dev/null
	printf '%s' "$build"
}

for platform in "${platforms[@]}"; do
	name="pd-dabs-mod-x86_64-$platform"
	stage="$DIST/stage/$name"

	rm -rf "$stage"
	mkdir -p "$stage/data" "$stage/added-content"

	if [ -n "$artifact" ] && [ -d "$artifact/$name" ]; then
		note "$platform taken from $artifact/$name"
		cp -a "$artifact/$name/." "$stage/"
	elif [ -n "$artifact" ]; then
		die "$artifact has no $name in it"
	else
		build="$(build_one "$platform")"
		"stage_${platform}_from_build" "$build" "$stage"
		strip_one "$platform" "$stage"
		cp "$ROOT/README.md" "$ROOT/LICENSE" "$stage/"
		: >"$stage/data/put_your_rom_here.txt"
		: >"$stage/added-content/put_goldeneye_rom_and_xbla_releases_here.txt"
	fi

	[ -f "$stage/README.md" ] || die "$stage lost README.md"
	[ -f "$stage/LICENSE" ] || die "$stage lost LICENSE"
	chmod +x "$stage/pd.x86_64" 2>/dev/null || true

	base="$name-$mode-$COMMIT"

	# Windows in a zip and Linux in a tarball, as the release job ships them:
	# a zip does not have to keep the executable bit, and tar keeps it.
	case "$platform" in
	windows) archive="$DIST/$base.zip" ;;
	*) archive="$DIST/$base.tar.gz" ;;
	esac

	if [ "$mode" = full ]; then
		[ -d "$PAYLOAD" ] || die "there is no payload/ to make a full package from"
		[ -f "$PAYLOAD/$ROM" ] || die "payload/$ROM is not there; see payload/README.txt"
		got="$(md5sum "$PAYLOAD/$ROM" | cut -d' ' -f1)"
		[ "$got" = "$ROM_MD5" ] || die "payload/$ROM is md5 $got, not $ROM_MD5: the game would refuse it too"

		cp -a "$PAYLOAD/." "$stage/"
		rm -f "$stage/README.txt"

		# What went in, so this zip can be traced back file by file, and so
		# the list below is the list of what to delete to be clean again.
		(
			cd "$PAYLOAD"
			find . -type f ! -name README.txt | sed 's|^\./||' | sort | while read -r f; do
				printf '%s  %s  %s\n' "$(sha256sum "$f" | cut -d' ' -f1)" "$(stat -c%s "$f")" "$f"
			done
		) >"$stage/PAYLOAD.txt"
		note "over the clean package, from payload/:"
		sed 's/^[^ ]*  [^ ]*  /  /' "$stage/PAYLOAD.txt"
	fi

	rm -f "$archive" "$archive.sha256"
	if [ "${archive##*.}" = zip ]; then
		if command -v zip >/dev/null; then
			(cd "$DIST/stage" && zip -qr "$archive" "$name")
		else
			# zip is not everywhere; cmake, which this script already needs,
			# writes the same archive.
			(cd "$DIST/stage" && cmake -E tar cf "$archive" --format=zip "$name")
		fi
	else
		tar czf "$archive" -C "$DIST/stage" "$name"
	fi
	(cd "$DIST" && sha256sum "$(basename "$archive")" >"$(basename "$archive").sha256")
	note "wrote $archive"
done

rm -rf "$DIST/stage"
note "zips and sha256 files are in $DIST"
