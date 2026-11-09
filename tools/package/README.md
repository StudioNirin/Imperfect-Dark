# Packaging

`package.sh` makes the archives you hand to somebody else, in two kinds.

```sh
tools/package/package.sh --linux --clean
tools/package/package.sh --windows --clean
tools/package/package.sh --windows --full
tools/package/package.sh --linux --full --from-artifact /tmp/art
```

| Option | |
| --- | --- |
| `--linux`, `--windows` | which to build; both if neither is named |
| `--clean` | the program, its DLLs, `README.md`, `LICENSE`, and empty `data/` and `added-content/` each with a note saying what goes there. Nothing copyrighted |
| `--full` | the same, plus everything in `payload/`. See `payload/README.txt` for what belongs there |
| `--from-artifact DIR` | take the binaries from `gh run download` output instead of building them here |
| `--out DIR` | where to write; `dist/` by default |

Both kinds write `<name>-<clean|full>-<commit>.zip` (Windows) or `.tar.gz`
(Linux) and a `.sha256` beside it, and `--full` adds `PAYLOAD.txt` inside the
package: sha256, size and path of every file it put in over the clean one.
That file is the record of who was sent what, and the list to delete to get a
clean tree back; the script prints it too.

## Why the two kinds are kept apart

`payload/` is in `.gitignore`, is never read by CMake, and is never part of a
build. Deleting its contents and rebuilding with `--clean` therefore gives a
package with no residue of anything copyrighted, which is the point: the same
tree makes the private build for people who own the games and the shareable
build for everybody else, and the difference is one folder rather than two
checkouts.

`--full` refuses to run unless `payload/data/pd.ntsc-final.z64` is there with
the md5 the game itself expects, because a package with the wrong ROM in it is
a package that will not start.

## Building the Windows package here

Needs the cross toolchain the project already documents:
`docs/windows-build.md` and `cmake/mingw-w64.cmake`, which looks for SDL2 and a
static zlib in `~/.local/mingw64`. Without it the script says so rather than
configuring a build that cannot work.

The alternative is to let CI build it and take the binaries from the run:

```sh
gh run list -R StudioNirin/Imperfect-Dark --limit 3
gh run download <run-id> -R StudioNirin/Imperfect-Dark -D /tmp/art
tools/package/package.sh --windows --full --from-artifact /tmp/art
```

`--from-artifact` also skips the strip step, since CI's binaries arrive
stripped with their debug info already beside the release.

## The DLL closure

For a Windows build made here, the script reads the imports out of
`pd.x86_64.exe` with `objdump -p` and copies whatever the mingw sysroot has,
then refuses to ship the package unless `SDL2.dll` and zlib's DLL came with it.
The list is read from the binary rather than written out by hand because a list
kept by hand is one DLL short the first time a dependency changes, and that is
a game that will not start with nothing to act on. The release job
(`.github/workflows/dabs-mod.yml`) checks the same things; keep the two in step.
