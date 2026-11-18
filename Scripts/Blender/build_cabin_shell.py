"""Retain the original hood and rear deck for the view between the A-pillars."""
import json
from pathlib import Path

import bpy
from mathutils import Vector

root = Path(__file__).resolve().parents[2]
sections = json.loads((root/"Saved/CabinSource/exterior.json").read_text())
vertices, faces, normals, texcoords, slots = [], [], [], [], []
vertex_map = {}


def clip_polygon(polygon, edge, side):
    result = []
    for a, b in zip(polygon[-1:] + polygon[:-1], polygon):
        inside_a = side*(a[0].x-edge) >= 0
        inside_b = side*(b[0].x-edge) >= 0
        if inside_a != inside_b:
            t = (edge-a[0].x)/(b[0].x-a[0].x)
            result.append(tuple(x.lerp(y,t) for x,y in zip(a,b)))
        if inside_b:
            result.append(b)
    return result


bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete()
for section_id, section in enumerate(sections):
    points = [(Vector(p),Vector(n),Vector(uv)) for p,n,uv in
              zip(section["positions"],section["normals"],section["uv"])]
    indices = section["triangles"]
    for i in range(0,len(indices),3):
        tri = [points[k] for k in indices[i:i+3]]
        for edge, side in [(63,1),(-126,-1)]:
            polygon = clip_polygon(tri,edge,side)
            for j in range(1,len(polygon)-1):
                a,b,c = polygon[0],polygon[j],polygon[j+1]
                if (b[0]-a[0]).cross(c[0]-a[0]).length_squared < 1e-10:
                    continue
                face = []
                for p,n,uv in (a,b,c):
                    pos = (p.x/100,-p.y/100,p.z/100)
                    key = tuple(round(v,7) for v in pos)
                    if key not in vertex_map:
                        vertex_map[key] = len(vertices)
                        vertices.append(pos)
                    face.append(vertex_map[key])
                    normals.append(Vector((n.x,-n.y,n.z)).normalized())
                    texcoords.append((uv.x,1-uv.y))
                faces.append(face)
                slots.append(section_id)

mesh = bpy.data.meshes.new("SM_CarroceriaCabina")
mesh.from_pydata(vertices,[],faces)
assert len(mesh.loops) == len(normals)
for s in sections:
    mesh.materials.append(bpy.data.materials.new(s["slot"]))
layer = mesh.uv_layers.new()
for loop in mesh.loops:
    layer.data[loop.index].uv = texcoords[loop.index]
for face, material in zip(mesh.polygons, slots):
    face.material_index = material
    face.use_smooth = True
mesh.use_auto_smooth = True
mesh.normals_split_custom_set(normals)
obj = bpy.data.objects.new(mesh.name,mesh)
bpy.context.collection.objects.link(obj)
obj.select_set(True)
bpy.context.view_layer.objects.active=obj
bpy.ops.export_scene.fbx(filepath=str(root/"SourceArt/Cockpit/Detalle/SM_CarroceriaCabina.fbx"),
    use_selection=True, object_types={"MESH"}, axis_forward="X", axis_up="Z", bake_space_transform=True,
    mesh_smooth_type="FACE", add_leaf_bones=False, bake_anim=False)
print("Cabin shell triangles:",len(faces))
