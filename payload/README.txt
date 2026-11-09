This folder is local only. Everything here except this file is gitignored.

Place the files a full package should contain, using the same layout the game
expects at runtime:

  data/pd.ntsc-final.z64              Perfect Dark (USA Rev 1)
                                      md5 e03b088b6ac9e0080440efed07c1e40f

  added-content/<any name>.z64         GoldenEye 007 (US) ROM; .z64 .n64 or .v64
  added-content/Perfect Dark XBLA.7z   the XBLA release, or its folder unpacked
  added-content/GoldenEye_007_XBLA.7z  the GoldenEye XBLA build, or its folder

  mods/                                GE-X and other mods (patch archives)
  texture-packs/                       optional
  model-packs/                         optional

Then run:

  tools/package/package.sh --windows --full

The script copies this tree over the clean package and writes PAYLOAD.txt with
a sha256 for every file, so a given zip can always be traced back.

Deleting this folder's contents and rebuilding with --clean gives a package
with no copyrighted material in it.
