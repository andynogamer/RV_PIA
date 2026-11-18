"""Generate the city hardscape mask from its placed streets and align its grass with the district."""
import math
import struct
import zlib
from pathlib import Path
import unreal

editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world=editor.get_editor_world()
if not world or world.get_name()!='L_CiudadUrbana' or editor.get_game_world():
    raise RuntimeError('Open the saved urban map outside PIE.')
actors={a.get_actor_label():a for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
lib=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
mel=unreal.MaterialEditingLibrary
folder='/Game/RutaVirtual/Environment/Vialidad'
width=256
extent=90000.0
pixel=extent/width
distance=[width*2.]*(width*width)
road=actors['SM_Roads']
mesh=road.static_mesh_component.static_mesh
for section in range(mesh.get_num_sections(0)):
    vertices,indices,*_=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,section)
    tr=road.get_actor_transform()
    points=[((p.x+extent/2)/pixel-.5,(p.y+extent/2)/pixel-.5) for p in (tr.transform_location(v) for v in vertices)]
    for off in range(0,len(indices),3):
        a,b,c=(points[i] for i in indices[off:off+3])
        bx,by=b[0]-a[0],b[1]-a[1]
        cx,cy=c[0]-a[0],c[1]-a[1]
        det=bx*cy-by*cx
        if abs(det)<.001:continue
        for y in range(max(0,math.ceil(min(a[1],b[1],c[1]))),min(width,math.floor(max(a[1],b[1],c[1]))+1)):
            for x in range(max(0,math.ceil(min(a[0],b[0],c[0]))),min(width,math.floor(max(a[0],b[0],c[0]))+1)):
                dx,dy=x-a[0],y-a[1]
                u,v=(dx*cy-dy*cx)/det,(dy*bx-dx*by)/det
                if min(u,v,1-u-v)>=0:distance[y*width+x]=0
for step in (1,-1):
    axis=range(width) if step==1 else range(width-1,-1,-1)
    for y in axis:
        for x in axis:
            i=y*width+x
            for dx,dy,cost in [(-step,0,1),(0,-step,1),(-step,-step,1.4142),(step,-step,1.4142)]:
                nx,ny=x+dx,y+dy
                if 0<=nx<width and 0<=ny<width:
                    distance[i]=min(distance[i],distance[ny*width+nx]+cost)
mask=bytes(round(255*max(0,min(1,(24-d*pixel/100)/12))) for d in distance)
def chunk(kind,data):
    return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
project=Path(unreal.Paths.project_dir()).resolve()
path=project/'Saved/RoadSource/T_BanquetasCiudad.png'
path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,width,8,0,0,0,0))+
    chunk(b'IDAT',zlib.compress(b''.join(b'\0'+mask[y*width:(y+1)*width] for y in range(width))))+chunk(b'IEND',b''))
task=unreal.AssetImportTask()
task.filename=str(path)
task.destination_path=folder
task.automated=task.save=task.replace_existing=True
tools.import_asset_tasks([task])
mask_tex=lib.load_asset(folder+'/T_BanquetasCiudad')
mask_tex.set_editor_property('srgb',False)
mask_tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
mask_tex.set_editor_property('address_x',unreal.TextureAddress.TA_CLAMP)
mask_tex.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP)
lib.save_loaded_asset(mask_tex)

def ground(name,city):
    mat=lib.load_asset(folder+'/'+name) if lib.does_asset_exist(folder+'/'+name) else tools.create_asset(name,folder,unreal.Material,unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    def node(cls):return mel.create_material_expression(mat,cls)
    def wire(a,b,pin,channel=''):mel.connect_material_expressions(a,channel,b,pin)
    def output(a,prop):mel.connect_material_property(a,'',prop)
    def color(r,g,b):
        n=node(unreal.MaterialExpressionConstant3Vector)
        n.set_editor_property('constant',unreal.LinearColor(r,g,b,1))
        return n
    def sample(texture,uv,kind=None):
        n=node(unreal.MaterialExpressionTextureSample)
        n.texture=texture
        if kind:n.sampler_type=kind
        wire(uv,n,'UVs')
        return n
    def uv(code):
        n=node(unreal.MaterialExpressionCustom)
        pin=unreal.CustomInput()
        pin.set_editor_property('input_name','P')
        n.set_editor_property('inputs',[pin])
        n.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        n.set_editor_property('code',code)
        wire(pos,n,'P')
        return n
    def mix(a,b,factor):
        n=node(unreal.MaterialExpressionLinearInterpolate)
        wire(a,n,'A');wire(b,n,'B')
        if isinstance(factor,float):n.set_editor_property('const_alpha',factor)
        else:wire(factor,n,'Alpha','R')
        return n
    pos=node(unreal.MaterialExpressionWorldPosition)
    fine=uv('float2 q=P.xy*0.0015; return q+0.35*float2(sin(q.y*.8),cos(q.x*.61));')
    broad=uv('return mul(float2x2(.73,-.68,.68,.73),P.xy)*.00024;')
    tex=lib.load_asset(folder+'/T_Sierra_Diffuse')
    turf=mix(mix(sample(tex,fine),sample(tex,broad),.47),color(.105,.146,.071),.52)
    result=turf
    if city:
        coverage=sample(mask_tex,uv('return (P.xy+45000.0)/90000.0;'),unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        result=mix(turf,color(.229,.224,.211),coverage)
    output(result,unreal.MaterialProperty.MP_BASE_COLOR)
    for prop,value in [(unreal.MaterialProperty.MP_ROUGHNESS,.95),(unreal.MaterialProperty.MP_SPECULAR,.10)]:
        scalar=node(unreal.MaterialExpressionConstant)
        scalar.set_editor_property('r',value)
        output(scalar,prop)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat
ground('M_SueloSierra',False)
city_material=ground('M_SueloCiudad',True)
actors['SM_Ground'].static_mesh_component.set_material(0,city_material)
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():raise RuntimeError('Save failed')
RV_RESULT={'mask_pixels':len(mask),'paved_pixels':sum(v>128 for v in mask)}
