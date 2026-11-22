#!/usr/bin/env python3
"""
Make a Blender OBJ export loadable by this game's XBLA mesh loader.

Blender's OBJ round trip preserves the geometry exactly but drops two things
the game reads:

  * OBJ groups. ``g node0``, ``g part1_if1`` and ``g part2`` are not decoration:
    objmesh.c takes the number out of the name and xblamesh.c uses it to index
    the release's group table. With no groups, every draw becomes part 0 and
    the gun's conditional section and working parts stop behaving as such.
    Blender 5 may import these as groups rather than objects, so do not rely on
    renaming objects: put the group lines back here instead.

  * Material picture paths. The dump's MTL uses a path relative to the dump
    folder. Blender resolves it against its own location and writes a wrong
    absolute path. The material NAMES are what bind the release's textures in
    game; the paths only matter for viewing the OBJ in Blender, and pointing
    them at the local PNGs fixes that without touching any name.

The script copies the input's vertices, UVs, normals and faces verbatim. Only
the group lines, the mtllib line and the MTL file are rewritten. It requires
the exported mesh to have the same face count as the original dump, which is
true for shape edits (moving vertices). If you changed the topology, the face
boundaries of the three parts have to be worked out again by hand.

Usage:
    python3 fix-export.py shotgun-output-test.obj
    python3 fix-export.py my-edit.obj --out my-edit-game

Defaults: original=shotgun.obj, materials=shotgun.mtl, textures=shotgun/,
output=<input stem>-game.obj / .mtl. All paths are relative to this folder
unless given as absolute paths.
"""

import argparse
import os
import sys


def read_lines(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read().splitlines()


def group_boundaries(original):
    """Face index at which each g line of the dump starts."""
    boundaries = {}
    faces = 0
    for line in read_lines(original):
        text = line.strip()
        if text.startswith("g "):
            boundaries[faces] = text[2:]
        elif text.startswith("f "):
            faces += 1
    return faces, boundaries


def material_pictures(mtlpath):
    """Material names and picture basenames from the dump's own MTL."""
    result = []
    for line in read_lines(mtlpath):
        text = line.strip()
        if text.startswith("newmtl "):
            result.append([text[7:], None])
        elif text.startswith("map_Kd ") and result and result[-1][1] is None:
            result[-1][1] = os.path.basename(text[7:])
    if not result or any(pic is None for _, pic in result):
        sys.exit("fix-export: the original MTL has a material without a picture")
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("input", help="OBJ exported from Blender")
    p.add_argument("--original", default="shotgun.obj", help="dumped OBJ to take group boundaries from")
    p.add_argument("--mtl", default="shotgun.mtl", help="dumped MTL to take material names and picture names from")
    p.add_argument("--textures", default="shotgun", help="local folder holding the picture PNGs")
    p.add_argument("--out", help="output prefix (default: <input stem>-game)")
    p.add_argument("--export-groups", action="store_true",
                   help="the export already carries the groups: rename them in place and keep "
                        "their face boundaries, for edits that changed the mesh's topology")
    args = p.parse_args()

    here = os.path.dirname(os.path.abspath(args.input))
    def local(path):
        return path if os.path.isabs(path) else os.path.join(here, path)

    lines = read_lines(local(args.input))
    prefix = args.out or (os.path.splitext(os.path.basename(args.input))[0] + "-game")
    pics = material_pictures(local(args.mtl))
    canon_mats = [name for name, _ in pics]
    canon_groups = ["node0", "part1_if1", "part2"]

    def canon_material(name):
        if name in canon_mats:
            return name
        # Blender appends .001 when the same MTL is imported twice. The game
        # binds a release texture by the exact bean_N_0x... name, so a suffix
        # makes the model draw with no picture at all.
        for c in canon_mats:
            if name.startswith(c + "."):
                return c
        return name

    def canon_group(name):
        # objmesh.c's objNameNumber() accepts only prefix+digits with nothing
        # after them: "node0_node0" and "part1_if1_part1_if1" both answer -1
        # and lose their part number, so the doubled name has to go.
        for c in canon_groups:
            if name == c or name.startswith(c):
                return c
        return None

    out = []
    if args.export_groups:
        # Keep the export's own boundaries: the edit changed the face count, so
        # the dump's face numbers no longer describe where these parts begin.
        found = [line[2:].strip() for line in lines if line.startswith("g ")]
        if len(found) != len(canon_groups):
            sys.exit(f"fix-export: {len(found)} groups in the export, expected {len(canon_groups)}")
        names = [canon_group(g) for g in found]
        if names != canon_groups:
            sys.exit(f"fix-export: groups {found} do not map to {canon_groups} in order")
        groups = iter(names)
        for line in lines:
            if line.startswith("g "):
                out.append("g " + next(groups))
            elif line.startswith("usemtl "):
                out.append("usemtl " + canon_material(line[7:]))
            elif line.startswith("mtllib "):
                out.append("mtllib " + os.path.basename(prefix) + ".mtl")
            else:
                out.append(line)
    else:
        orig_faces, boundaries = group_boundaries(local(args.original))
        if not boundaries:
            sys.exit("fix-export: the original OBJ has no g lines to restore")
        out_faces = sum(1 for line in lines if line.startswith("f "))
        if out_faces != orig_faces:
            sys.exit(f"fix-export: {out_faces} faces in the export, {orig_faces} in the dump; "
                     "topology changed, use --export-groups or re-derive the boundaries")
        face = 0
        for line in lines:
            # An export that did carry groups is normalised to the dump's groups.
            if line.startswith("g "):
                continue
            if line.startswith("f ") and face in boundaries:
                out.append("g " + boundaries[face])
            if line.startswith("usemtl "):
                out.append("usemtl " + canon_material(line[7:]))
                continue
            if line.startswith("mtllib "):
                out.append("mtllib " + os.path.basename(prefix) + ".mtl")
                continue
            out.append(line)
            if line.startswith("f "):
                face += 1

    objpath = local(prefix + ".obj")
    with open(objpath, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")

    mtlpath = local(prefix + ".mtl")
    with open(mtlpath, "w", encoding="utf-8") as f:
        f.write("# Material names kept exactly; pictures are local to this folder.\n")
        for name, pic in material_pictures(local(args.mtl)):
            f.write(f"newmtl {name}\nKd 1 1 1\n")
            f.write(f"map_Kd {args.textures}/{pic}\nmap_d {args.textures}/{pic}\n\n")

    ngroups = sum(1 for line in out if line.startswith("g "))
    nfaces = sum(1 for line in out if line.startswith("f "))
    print(f"fix-export: {ngroups} groups, faces {nfaces}, "
          f"materials {len(pics)}")
    print(f"fix-export: {objpath}")
    print(f"fix-export: {mtlpath}")


if __name__ == "__main__":
    main()
