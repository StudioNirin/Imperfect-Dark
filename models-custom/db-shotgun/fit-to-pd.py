#!/usr/bin/env python3
"""Fit the XBLA shotgun mesh into the PD GshotgunZ model's rest space.

The asset dump in shotgun.obj is the GoldenEye XBLA shotgun in *its own*
units and node structure: its long axis is Z, its node0 group is the muzzle
flash quad, and its groups do not match the Perfect Dark first-person shotgun
(GshotgunZ) model the PpdDbShotgunZ alias replaces.  Loading that dump as a
model pack therefore rendered a giant sidewards gun with the flash always on.

This script drops the flash group, scales and centres the gun body onto the
PD model's rest bbox, joins what remains into its single node0 group, and
writes an OBJ whose MTL paths point at the local PNGs (see fix-export.py for
the material-name rules).  Run with Blender:

  blender -b --python fit-to-pd.py -- PpdDbShotgunZ.obj \
      ../../build/model-dumps/pd-n64/GshotgunZ.obj fitted.obj
"""

import bpy, sys
from mathutils import Vector

src = sys.argv[sys.argv.index('--')+1]
ref = sys.argv[sys.argv.index('--')+2]
out = sys.argv[sys.argv.index('--')+3]

# --- reference: first-person PD shotgun model, imported (Blender Z-up)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
bpy.ops.wm.obj_import(filepath=ref)
refobjs=[o for o in bpy.context.scene.objects if o.type=='MESH']
rp=[o.matrix_world @ v.co for o in refobjs for v in o.data.vertices]
rlo=Vector([min(p[i] for p in rp) for i in range(3)])
rhi=Vector([max(p[i] for p in rp) for i in range(3)])
print("REF bbox", [round(x,2) for x in rlo], [round(x,2) for x in rhi])

# --- source: custom mesh, imported, with its muzzle-flash group removed
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()
bpy.ops.wm.obj_import(filepath=src)
objs=[o for o in bpy.context.scene.objects if o.type=='MESH']
for o in objs:
    print("SRC GROUP", o.name, len(o.data.vertices), "polys", len(o.data.polygons))
    if o.name == 'node0':
        bpy.data.objects.remove(o, do_unlink=True)
objs=[o for o in bpy.context.scene.objects if o.type=='MESH']
sp=[o.matrix_world @ v.co for o in objs for v in o.data.vertices]
slo=Vector([min(p[i] for p in sp) for i in range(3)])
shi=Vector([max(p[i] for p in sp) for i in range(3)])
print("SRC body bbox", [round(x,2) for x in slo], [round(x,2) for x in shi])

# --- similarity transform: uniform scale from the longest axis, then align centers
ssize = shi - slo; rsize = rhi - rlo
scale = max(rsize) / max(ssize)
sctr=(slo+shi)/2; rctr=(rlo+rhi)/2
print("SCALE", round(scale,6))
for o in objs:
    for v in o.data.vertices:
        w = o.matrix_world @ v.co
        v.co = (w - sctr) * scale + rctr
    # the imported objects carry a Z-up conversion rotation; after baking the
    # world-space result into the vertices, clear it so the exporter does not
    # apply it a second time
    o.matrix_world.identity()

# collapse every surviving mesh into one object so the pack loader sees one group
bpy.ops.object.select_all(action='DESELECT')
for o in objs: o.select_set(True)
bpy.context.view_layer.objects.active=objs[0]
bpy.ops.object.join()
joined=bpy.context.view_layer.objects.active
joined.name='node0'
print("JOINED", len(joined.data.vertices), "polys", len(joined.data.polygons))

bpy.ops.object.select_all(action='DESELECT'); joined.select_set(True)
bpy.context.view_layer.objects.active=joined
bpy.ops.wm.obj_export(filepath=out, export_selected_objects=True, export_materials=True,
                     export_uv=True, export_normals=True, apply_modifiers=True)
print("WROTE",out)
