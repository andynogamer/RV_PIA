"""Generate original cockpit reference geometry using Blender 3.6 (meters)."""
from pathlib import Path
import bpy
from math import pi, sin, cos
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'SourceArt' / 'Cockpit'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0
bpy.context.preferences.filepaths.save_version = 0


def material(name, color, metal=0.0, rough=0.65):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1)
    mat.use_nodes = True
    shader = mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value = (*color, 1)
    shader.inputs['Metallic'].default_value = metal
    shader.inputs['Roughness'].default_value = rough
    return mat


plastic = material('M_Cabina_Plastic', (0.024, 0.029, 0.034))
rubber = material('M_Volante_Rubber', (0.009, 0.011, 0.013), rough=0.82)
trim = material('M_Cabina_Trim', (0.16, 0.18, 0.2), metal=0.7, rough=0.35)
screen = material('M_Cabina_Display', (0.006, 0.022, 0.03), rough=0.2)
letters = material('M_Cabina_Letters', (0.52, 0.74, 0.7), rough=0.4)


def box(name, loc, size, mat, bevel=0.015):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    if bevel:
        mod = obj.modifiers.new('Soft manufactured edges', 'BEVEL')
        mod.width = bevel
        mod.segments = 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
        obj.data.use_auto_smooth = True
        normal_mod = obj.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
        bpy.ops.object.modifier_apply(modifier=normal_mod.name)
    return obj


def bar(name, a, b, radius, mat):
    midpoint = (Vector(a) + Vector(b)) / 2
    delta = Vector(b) - Vector(a)
    bpy.ops.mesh.primitive_cylinder_add(vertices=20, radius=radius, depth=delta.length, location=midpoint)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = delta.to_track_quat('Z', 'Y').to_euler()
    obj.data.materials.append(mat)
    for face in obj.data.polygons:
        face.use_smooth = True
    return obj


def text(name, body, loc, size):
    bpy.ops.object.text_add(location=loc, rotation=(pi / 2, 0, -pi / 2))
    obj = bpy.context.object
    obj.name = name
    obj.data.body = body
    obj.data.size = size
    obj.data.align_x = 'CENTER'
    obj.data.extrude = 0.0002
    obj.data.materials.append(letters)
    bpy.ops.object.convert(target='MESH')
    return obj


box('Dashboard', (0.33, 0, 0.72), (0.55, 1.57, 0.27), plastic, 0.055)
box('Dashboard lower', (0.20, 0, 0.53), (0.35, 1.48, 0.25), plastic, 0.04)
box('Instrument hood', (0.025, 0.32, 0.84), (0.12, 0.56, 0.17), plastic, 0.035)
box('Instrument inset', (-0.042, 0.32, 0.825), (0.008, 0.49, 0.105), screen, 0.008)
box('Central display surround', (0.02, -0.25, 0.75), (0.07, 0.36, 0.24), trim, 0.015)
box('Central display', (-0.02, -0.25, 0.75), (0.008, 0.32, 0.20), screen, 0.008)
text('Brand', 'RUTA VIRTUAL', (-0.027, -0.25, 0.78), 0.027)
text('Mode', 'PRACTICA', (-0.027, -0.25, 0.72), 0.020)
for y in [-0.65, -0.02, 0.64]:
    box('Vent surround', (0.022, y, 0.79), (0.05, 0.15, 0.09), trim, 0.008)
    for z in [0.765, 0.785, 0.805]:
        box('Vent slat', (-0.009, y, z), (0.012, 0.13, 0.006), rubber, 0.001)
for y in [-0.79, 0.79]:
    box('Door interior', (-0.32, y, 0.58), (1.08, 0.075, 0.46), plastic, 0.025)
    box('Door grip', (-0.25, y * 0.95, 0.65), (0.25, 0.055, 0.055), trim, 0.012)
    bar('Windscreen pillar', (-0.12, y, 0.85), (0.53, y * 0.86, 1.40), 0.035, plastic)
bar('Windscreen upper rail', (0.53, -0.68, 1.4), (0.53, 0.68, 1.4), 0.035, plastic)
box('Floor', (-0.15, 0, 0.24), (1.4, 1.5, 0.04), rubber)
box('Console', (-0.12, 0.11, 0.40), (0.65, 0.23, 0.30), plastic, 0.03)


def join_named(name):
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    return obj


cab = join_named('SM_Cabina')
cab.hide_set(True)
cab.hide_render = True
center = Vector((-0.08, 0.32, 0.79))
bpy.ops.mesh.primitive_torus_add(major_radius=0.175, minor_radius=0.017, major_segments=64,
                               minor_segments=12, location=center, rotation=(0, pi / 2, 0))
rim = bpy.context.object
rim.data.materials.append(rubber)
for face in rim.data.polygons:
    face.use_smooth = True
box('Hub', center, (0.06, 0.105, 0.085), rubber, 0.02)
for angle in [0, pi, 1.5 * pi]:
    bar('Wheel spoke', center, center + Vector((0, 0.157 * cos(angle), 0.157 * sin(angle))), 0.012, trim)
# Join only wheel geometry; keep dashboard excluded.
bpy.ops.object.select_all(action='DESELECT')
wheel_objects = [o for o in bpy.context.scene.objects if o.type == 'MESH' and o != cab]
for obj in wheel_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = rim
bpy.ops.object.join()
wheel = bpy.context.object
wheel.name = 'SM_Volante'
bpy.context.scene.cursor.location = center
bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
cab.hide_set(False)
cab.hide_render = False

for obj in [cab, wheel]:
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    saved_location = obj.location.copy()
    if obj == wheel:
        obj.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=str(OUT / (obj.name + '.fbx')), use_selection=True,
        object_types={'MESH'}, axis_forward='X', axis_up='Z', apply_unit_scale=True,
        bake_space_transform=True, mesh_smooth_type='FACE', use_mesh_modifiers=True,
        add_leaf_bones=False, bake_anim=False)
    obj.location = saved_location
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Cabina.blend'))
print('RUTA_COCKPIT_CREATED=' + str(OUT))
