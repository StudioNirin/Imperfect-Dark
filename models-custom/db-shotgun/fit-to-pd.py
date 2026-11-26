#!/usr/bin/env python3
"""Fit a Blender-exported gun mesh into a Perfect Dark model's rest space.

The source is a mesh exported from Blender (the GE shotgun dump opened,
edited and saved again). Its own units, origin and axes have nothing to do
with the PD model it replaces: its long axis is +Z, its muzzle-flash quad is
the ``node0`` group, and it is Y-up. The two PD shotgun models are in two
different rest spaces:

  GshotgunZ    a viewmodel gun. Long axis +Z, up +Y (its GUNDL list loads the
               matrix the first-person gun is drawn under).
  PchrshotgunZ a held/floor gun. Long axis -X, up +Z, relative to the chr's
               hand node (chr0f022214 installs that node's matrix as the
               weapon model's root).

One OBJ cannot be correct in both spaces. This script takes the target PD
model dump as its reference and rotates, scales and centres the Blender mesh
into that model's rest space, so the same source edit can be fitted to either
slot without touching game code. The rotation is derived from the mesh's own
muzzle marker and the target space's forward/up convention, not hand-tuned
per gun.

Run with Blender, once per target model:

  blender -b --python fit-to-pd.py -- db-shotgun-game.obj \
      ../../build/model-dumps/pd-n64/GshotgunZ.obj PpdDbShotgunZ.obj
  blender -b --python fit-to-pd.py -- db-shotgun-game.obj \
      ../../build/model-dumps/pd-n64/PchrshotgunZ.obj PpdDbShotgunChr.obj

The space is inferred from the reference name (G* = viewmodel, Pchr* = held)
or can be given explicitly with ``--space view|held``. The output OBJ is
collapsed into one ``node0`` group, which is what both of these PD models
have. Its MTL keeps the bean_N material names and points the pictures at
``--textures``, a folder of PNGs anywhere, defaulting to the one canonical set
in the pack (``model-packs/dbshotgun/n64/shotgun``, which CMakeLists globs into
the embed); each path written into the MTL is made relative to that MTL, so the
run is correct whether the pair lands in this working area or in the pack. What
``--textures`` is not is a second copy of the PNGs: one set of bytes to keep
true. The script ends by checking every picture it named resolves, because an
MTL with a broken path loads the mesh untextured in Blender and says nothing.
"""

import bpy
import os
import sys
from mathutils import Vector, Matrix


args = sys.argv[sys.argv.index('--') + 1:]
src = args[0]
ref = args[1]
out = args[2]
space = args[args.index('--space') + 1] if '--space' in args else None
# models-custom/db-shotgun/fit-to-pd.py -> models-custom/ -> the repository root.
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
textures = (args[args.index('--textures') + 1] if '--textures' in args
            else os.path.join(REPO, 'model-packs', 'dbshotgun', 'n64', 'shotgun'))


def bbox(objs):
    pts = [o.matrix_world @ v.co for o in objs for v in o.data.vertices]
    lo = Vector([min(p[i] for p in pts) for i in range(3)])
    hi = Vector([max(p[i] for p in pts) for i in range(3)])
    return lo, hi


def principal_axis(lo, hi):
    size = hi - lo
    return max(range(3), key=lambda i: size[i]), size


def orthogonal(up, forward):
    """Up with the forward component removed, then normalised."""
    return (up - forward * up.dot(forward)).normalized()


def basis(forward, up):
    """Matrix whose columns are right, up and forward (right = up x forward)."""
    right = up.cross(forward).normalized()
    return Matrix((right, up, forward)).transposed()


def import_obj(path):
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    bpy.ops.wm.obj_import(filepath=path)
    return [o for o in bpy.context.scene.objects if o.type == 'MESH']


# --- reference: the PD model to fit into (imported into Blender Z-up) ---
refobjs = import_obj(os.path.abspath(ref))
rlo, rhi = bbox(refobjs)
raxis, rsize = principal_axis(rlo, rhi)

# The gun grows away from the model root (the hand or gun-list origin), so the
# end of the long axis furthest from 0 is the muzzle. This answers forward for
# both GshotgunZ (+Z) and PchrshotgunZ (-X) without naming either model.
rforward = Vector((0, 0, 0))
rforward[raxis] = 1 if abs(rhi[raxis]) > abs(rlo[raxis]) else -1

# Up is a property of the rest space, not of this particular gun. Viewmodel
# dumps are authored Y-up, held dumps Z-up. Blender's OBJ importer maps dump
# +Y to world +Z and dump +Z to world -Y.
if space is None:
    space = 'view' if os.path.basename(ref).lower().startswith('g') else 'held'
if space not in ('view', 'held'):
    sys.exit('fit-to-pd: --space must be view or held')
rup = Vector((0, 0, 1)) if space == 'view' else Vector((0, -1, 0))
rup = orthogonal(rup, rforward)

print('REF bbox', [round(x, 2) for x in rlo], [round(x, 2) for x in rhi])
print('REF forward', tuple(rforward), 'up', tuple(rup), 'space', space)


# --- source: Blender's export, with its muzzle-flash group removed ---
objs = import_obj(os.path.abspath(src))
flash = [o for o in objs if o.name == 'node0']
if flash:
    fpts = [o.matrix_world @ v.co for o in flash for v in o.data.vertices]
    flashctr = sum(fpts, Vector()) / len(fpts)
for o in flash:
    bpy.data.objects.remove(o, do_unlink=True)
objs = [o for o in bpy.context.scene.objects if o.type == 'MESH']

slo, shi = bbox(objs)
saxis, ssize = principal_axis(slo, shi)
sctr = (slo + shi) / 2

# The flash sits slightly off the barrel's centreline after a shape edit, so
# use it only for the sign: the axis itself is the mesh's longest bbox axis.
if flash:
    direction = flashctr - sctr
else:
    direction = Vector((0, 0, 0))
    direction[saxis] = 1
sforward = Vector((0, 0, 0))
sforward[saxis] = 1 if direction[saxis] >= 0 else -1
# The GE dump is Y-up, which Blender's importer turns into world +Z.
sup = orthogonal(Vector((0, 0, 1)), sforward)

print('SRC body bbox', [round(x, 2) for x in slo], [round(x, 2) for x in shi])
print('SRC forward', tuple(round(x, 3) for x in sforward),
      'up', tuple(round(x, 3) for x in sup))

# --- one similarity transform: rotate, uniform scale, align box centres ---
rotation = basis(rforward, rup) @ basis(sforward, sup).transposed()
scale = max(rsize) / max(ssize)
rctr = (rlo + rhi) / 2
transform = (Matrix.Translation(rctr)
             @ Matrix.Diagonal((scale, scale, scale, 1))
             @ rotation.to_4x4()
             @ Matrix.Translation(-sctr))
print('SCALE', round(scale, 6))

for o in objs:
    for v in o.data.vertices:
        v.co = transform @ (o.matrix_world @ v.co)
    # Bake the world matrix into the vertices so the exporter does not apply
    # the importer's axis conversion a second time.
    o.matrix_world.identity()
    o.data.update()

# Collapse every surviving mesh into the pack's single node0 group.
bpy.ops.object.select_all(action='DESELECT')
for o in objs:
    o.select_set(True)
bpy.context.view_layer.objects.active = objs[0]
bpy.ops.object.join()
joined = bpy.context.view_layer.objects.active
joined.name = 'node0'
print('JOINED', len(joined.data.vertices), 'polys', len(joined.data.polygons))

out = os.path.abspath(out)
bpy.ops.object.select_all(action='DESELECT')
joined.select_set(True)
bpy.context.view_layer.objects.active = joined
bpy.ops.wm.obj_export(filepath=out, export_selected_objects=True, export_materials=True,
                      export_uv=True, export_normals=True, apply_modifiers=True)
print('WROTE', out)

# Blender writes the object line but does not always write a group line for a
# single joined object. objmesh.c numbers the pack's draw lists from g lines,
# so put the node0 group back explicitly (the group is the one this PD model
# has, and the joined object is its whole mesh).
objlines = open(out, encoding='utf-8').read().splitlines()
if not any(line.startswith('g ') for line in objlines):
    at = next((i for i, line in enumerate(objlines) if line.startswith('o ')), -1)
    objlines.insert(at + 1, 'g node0')
    with open(out, 'w', encoding='utf-8') as f:
        f.write('\n'.join(objlines) + '\n')

# Blender resolves the dump's relative picture paths against its own location
# and writes those absolute paths into the new MTL. Point them at the pack's one
# canonical set, made relative to the MTL being written; the material names are
# what bind in game, and the pack keeps its pictures beside its own MTLs.
mtl = os.path.splitext(out)[0] + '.mtl'
texdir = os.path.abspath(textures)
lines = open(mtl, encoding='utf-8').read().splitlines()
named = []
with open(mtl, 'w', encoding='utf-8') as f:
    for line in lines:
        if line.startswith('map_Kd ') or line.startswith('map_d '):
            key, _, value = line.partition(' ')
            rel = os.path.relpath(os.path.join(texdir, os.path.basename(value.strip())),
                                  os.path.dirname(os.path.abspath(mtl)))
            rel = rel.replace(os.sep, '/')
            named.append(rel)
            f.write(f'{key} {rel}\n')
        else:
            f.write(line + '\n')
# A picture path that does not resolve is not something Blender or the viewer
# reports: the mesh simply loads untextured. So it is reported here.
missing = [r for r in dict.fromkeys(named)
           if not os.path.isfile(os.path.join(os.path.dirname(os.path.abspath(mtl)), r))]
if missing:
    sys.exit(f'fit-to-pd: {len(missing)} picture path(s) in {mtl} do not resolve, '
             f'first {missing[0]}; pass --textures pointing at the PNGs '
             f'(default is the pack\'s set)')
print('WROTE', mtl)
