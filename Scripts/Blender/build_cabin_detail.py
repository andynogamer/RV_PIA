"""Player-only cabin trim, moving shift lever and planar mirror glass. Dimensions in cm."""
from math import cos, pi, sin
from pathlib import Path

import bpy
from mathutils import Vector

out = Path(__file__).resolve().parents[2] / "SourceArt" / "Cockpit" / "Detalle"
out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete()
bpy.context.scene.unit_settings.system = "METRIC"


def material(name, rgb, metal=0, rough=.65):
    m = bpy.data.materials.new("M_RV_" + name)
    m.diffuse_color = (*rgb, 1)
    m.use_nodes = True
    shader = m.node_tree.nodes["Principled BSDF"]
    shader.inputs["Base Color"].default_value = m.diffuse_color
    shader.inputs["Metallic"].default_value = metal
    shader.inputs["Roughness"].default_value = rough
    return m


leather = material("Grafito", (.047, .053, .063), rough=.51)
fabric = material("Tejido", (.105, .12, .138), rough=.92)
metal = material("Satinado", (.39, .43, .46), .70, .31)
seam = material("Marfil", (.67, .64, .55))
glass = material("Cristal", (.17, .23, .26))
parts = []


def xyz(p):
    return Vector((p[0]/100, -p[1]/100, p[2]/100))


def box(pos, size, mat, radius):
    bpy.ops.mesh.primitive_cube_add(size=1, location=xyz(pos))
    o = bpy.context.object
    o.dimensions = tuple(n/100 for n in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.data.materials.append(mat)
    if radius:
        mod = o.modifiers.new("Canto", "BEVEL")
        mod.width, mod.segments = radius/100, 3
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod = o.modifiers.new("Normales", "WEIGHTED_NORMAL")
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(o)
    return o


def rod(a, b, r, mat):
    a, b = xyz(a), xyz(b)
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=r/100, depth=(b-a).length, location=(a+b)/2)
    o = bpy.context.object
    o.rotation_euler = (b-a).to_track_quat("Z", "Y").to_euler()
    for face in o.data.polygons:
        face.use_smooth = len(face.vertices) == 4
    o.data.materials.append(mat)
    parts.append(o)


def export(name):
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    o = bpy.context.object
    o.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.export_scene.fbx(filepath=str(out / (name+".fbx")), use_selection=True,
        object_types={"MESH"}, axis_forward="X", axis_up="Z", apply_unit_scale=True,
        bake_space_transform=True, mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False)
    bpy.data.objects.remove(o, do_unlink=True)
    parts.clear()


# Pivoted mirror faces share a tangent frame. +X points toward the driver.
for name, width, height in [("SM_EspejoCentral", 25, 7.8), ("SM_EspejoLateral", 19.5, 11.8)]:
    box((-1.5,0,0), (3,width+1.6,height+1.6), leather, 1.45)
    half_w, half_h, r = width/2, height/2, 1.25
    rim = []
    for sy, sz, angle in [(1,1,0),(-1,1,90),(-1,-1,180),(1,-1,270)]:
        for i in range(6):
            a = (angle+90*i/5)*pi/180
            rim.append((.10, sy*(half_w-r)+r*cos(a), sz*(half_h-r)+r*sin(a)))
    mesh = bpy.data.meshes.new("Cristal")
    mesh.from_pydata([xyz(p) for p in rim], [], [tuple(reversed(range(len(rim))))])
    mesh.materials.append(glass)
    uv = mesh.uv_layers.new()
    for loop in mesh.loops:
        _, y, z = rim[loop.vertex_index]
        uv.data[loop.index].uv = (.5+y/width, .5+z/height)
    face = bpy.data.objects.new("Cristal",mesh)
    bpy.context.collection.objects.link(face)
    parts.append(face)
    export(name)

# Fixed brackets join the mirror housings to the windscreen and the two door sills.
rod((-3,0,136),(13,0,132),.85,leather)
rod((13,0,132),(10.4,0,126),.85,leather)
box((1,0,135),(9,10,2.4),leather,.9)
for side in [-1,1]:
    rod((13,79*side,86),(15,98*side,88),1.3,leather)
    rod((15,98*side,88),(16,103*side,94),1.25,leather)
    box((13,79*side,86),(12,3.5,7),leather,1.2)
# Carpet carries into the footwells under the dashboard and into a closed firewall.
box((34,0,25),(64,153,3),fabric,1.0)
box((59,0,43),(4,155,35),leather,1.7)
for side in (-1,1):
    box((40,77*side,51),(43,4,52),leather,1.2)
for y,width,height in [(-49,6.1,7.1),(-35,6.7,7.2),(-21,4.8,10.0)]:
    rod((34,y,50),(26,y,36),.65,metal)
    box((25,y,36),(1.6,width,height),leather,.55)
    for z in (-1.9,0,1.9):
        box((24,y,36+z),(.55,width-.9,.45),metal,.12)
# Seat cushions, side bolsters, headrests and stitched piping.
for y in (-32,33):
    box((-44,y,42),(47,48,9),fabric,3.8)
    for side in (-1,1):
        box((-44,y+side*21,46),(44,7,10),leather,3)
        rod((-61,y+side*19,50),(-27,y+side*19,50),.27,seam)
    back = box((-68,y,67),(10,46,49),fabric,4.4)
    back.rotation_euler.y = -.13
    for offset in (-7,7):
        rod((-72,y+offset,86),(-74,y+offset,101),.75,metal)
    box((-74,y,103),(11,25,15),leather,4.0)
# Stitched boots and console rim around the manual shift lever.
box((-17,-5,55.5),(18,15,2.2),metal,1.0)
for index in range(4):
    box((-17,-5,56.5+index*.75),(15-index*2.4,12-index*1.9,1.6),leather,.7)
box((-43,-5,57),(23,16,6),leather,2.5)
for side in (-1,1):
    rod((-51,-5+side*5.8,60),(-35,-5+side*5.8,60),.2,seam)
# Inner door releases and speakers follow the door geometry.
for side in (-1,1):
    box((-17,75.3*side,73),(15,2,5.4),leather,.9)
    rod((-22,74*side,73),(-12,73*side,74),.8,metal)
    for i in range(7):
        box((-12+i*1.45,74.8*side,47),(0.55,1.1,6+2*sin(i*pi/6)),fabric,.2)
export("SM_DetalleCabina")
rod((0,0,0),(0,0,10),.8,metal)
box((0,0,10.5),(4.4,4.2,5.5),leather,1.8)
box((0,0,13.04),(3.1,3.0,.32),metal,.14)
for y in (-.93,0,.93):
    rod((-.87,y,13.25),(.87,y,13.25),.095,leather)
rod((0,-.93,13.25),(0,.93,13.25),.095,leather)
export("SM_PalancaCabina")
