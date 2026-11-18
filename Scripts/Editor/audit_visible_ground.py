"""Validate the visible LOD0 surface, including the ground outside the traffic splines."""
import collections
import json
import math
from pathlib import Path
import unreal

sub=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world=sub.get_editor_world()
if not world or sub.get_game_world() or world.get_name() not in ('L_Carretera','L_CiudadUrbana'):
    raise RuntimeError('Open a road map outside PIE.')
highway=world.get_name()=='L_Carretera'
labels={'Sierra_Calzada','Sierra_Relieve'} if highway else {'DistritoNorte_Avenida'}
grid=collections.defaultdict(list)
cell=1200 if highway else 3000
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
for actor in actors:
    label=actor.get_actor_label()
    if label not in labels:continue
    mesh=actor.static_mesh_component.static_mesh
    tr=actor.get_actor_transform()
    subsystem=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for slot in range(mesh.get_num_sections(0)):
        material=mesh.static_materials[subsystem.get_lod_material_slot(mesh,0,slot)]
        key=str(material.material_slot_name)
        if key not in ('M_Via_Asfalto','M_Via_Pasto','M_Via_Arcilla','M_Via_Hormigon'):continue
        vertices,indices,*_=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,slot)
        vertices=[tr.transform_location(v).to_tuple() for v in vertices]
        for i in range(0,len(indices),3):
            a,b,c=(vertices[k] for k in indices[i:i+3])
            bx,by,bz=(b[j]-a[j] for j in range(3))
            cx,cy,cz=(c[j]-a[j] for j in range(3))
            det=bx*cy-by*cx
            if abs(det)<.01:continue
            item=(a,bx,by,bz,cx,cy,cz,det,key)
            for ix in range(math.floor(min(a[0],b[0],c[0])/cell),math.floor(max(a[0],b[0],c[0])/cell)+1):
                for iy in range(math.floor(min(a[1],b[1],c[1])/cell),math.floor(max(a[1],b[1],c[1])/cell)+1):
                    grid[ix,iy].append(item)

def surface(x,y):
    tops={}
    for a,bx,by,bz,cx,cy,cz,det,key in grid[math.floor(x/cell),math.floor(y/cell)]:
        dx,dy=x-a[0],y-a[1]
        u,v=(dx*cy-dy*cx)/det,(dy*bx-dx*by)/det
        if min(u,v,1-u-v)<-.00001:continue
        z=a[2]+u*bz+v*cz
        tops[key]=max(tops.get(key,-1e9),z)
    return tops

issues=[]
samples=0
if highway:
    route=next(a for a in actors if a.get_actor_label()=='Sierra_Carril_0')
    s=route.spline
    for d in range(0,int(s.get_spline_length()),200):
        p=s.get_location_at_distance_along_spline(d,unreal.SplineCoordinateSpace.WORLD)
        v=s.get_direction_at_distance_along_spline(d,unreal.SplineCoordinateSpace.WORLD)
        for offset in [-645,-480,-340,-182,0,180,290]:
            x,y=p.x-v.y*offset,p.y+v.x*offset
            hits=surface(x,y)
            pavement=hits.get('M_Via_Asfalto',-1e9)
            if pavement< -1e8 or hits.get('M_Via_Pasto',-1e10)>pavement-5:
                issues.append([round(d/100,1),round(x),round(y),{k:round(z,1) for k,z in hits.items()}])
            samples+=1
else:
    # Rectangle straddles both triangular holes seen from the user's overhead camera.
    for x in range(-80800,61800,475):
        for y in range(40500,113000,475):
            if not surface(x,y):issues.append([x,y])
            samples+=1
result={'map':world.get_name(),'pass':not issues,'samples':samples,'defects':len(issues),'examples':issues[:8]}
name=globals().get('RV_ARGS',{}).get('tag','surface')
Path(unreal.Paths.project_saved_dir(),'AgentReports',world.get_name()+'_'+name+'.json').write_text(json.dumps(result))
RV_RESULT=result
