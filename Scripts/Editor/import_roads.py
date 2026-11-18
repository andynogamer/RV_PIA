"""Import the Blender road kit and the two CC0 terrain maps into the project."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
source = project/'Saved/RoadSource'
folder = '/Game/RutaVirtual/Environment/Vialidad'
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib.make_directory(folder)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if not world:raise RuntimeError('Stop PIE before importing roads')
refresh=globals().get('RV_ARGS',{}).get('refresh',False)
mel=unreal.MaterialEditingLibrary
for name in ('T_Sierra_Diffuse','T_Sierra_nor_dx','T_Sierra_Cielo'):
    if lib.does_asset_exist(folder+'/'+name):continue
    task=unreal.AssetImportTask()
    task.filename=str(project/'SourceArt/Carretera'/(name+'.jpg'))
    task.destination_path=folder
    task.automated=task.save=True
    tools.import_asset_tasks([task])
normal=lib.load_asset(folder+'/T_Sierra_nor_dx')
normal.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_NORMALMAP)
normal.set_editor_property('srgb',False)
lib.save_loaded_asset(normal)
terrain_mat=lib.load_asset(folder+'/M_SueloSierra') if lib.does_asset_exist(folder+'/M_SueloSierra') else None
if not terrain_mat:
    terrain_mat=tools.create_asset('M_SueloSierra',folder,unreal.Material,unreal.MaterialFactoryNew())
    uv=mel.create_material_expression(terrain_mat,unreal.MaterialExpressionTextureCoordinate)
    uv.set_editor_property('u_tiling',.32)
    uv.set_editor_property('v_tiling',.32)
    for name,prop in [('T_Sierra_Diffuse',unreal.MaterialProperty.MP_BASE_COLOR),('T_Sierra_nor_dx',unreal.MaterialProperty.MP_NORMAL)]:
        tex=mel.create_material_expression(terrain_mat,unreal.MaterialExpressionTextureSample)
        tex.texture=lib.load_asset(folder+'/'+name)
        if name.endswith('_dx'):tex.sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
        mel.connect_material_expressions(uv,'',tex,'UVs')
        mel.connect_material_property(tex,'RGB',prop)
    mel.recompile_material(terrain_mat)
    lib.save_loaded_asset(terrain_mat)
sky=lib.load_asset(folder+'/M_CieloSierra') if lib.does_asset_exist(folder+'/M_CieloSierra') else None
if not sky or not mel.get_material_property_input_node(sky,unreal.MaterialProperty.MP_EMISSIVE_COLOR):
    sky=sky or tools.create_asset('M_CieloSierra',folder,unreal.Material,unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(sky)
    sky.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    sky.set_editor_property('two_sided',True)
    sky.set_editor_property('is_sky',True)
    camera=mel.create_material_expression(sky,unreal.MaterialExpressionCameraVectorWS)
    panorama=mel.create_material_expression(sky,unreal.MaterialExpressionCustom)
    panorama.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT2)
    pin=unreal.CustomInput()
    pin.set_editor_property('input_name','ViewDir')
    panorama.set_editor_property('inputs',[pin])
    panorama.set_editor_property('code','float3 d=-normalize(ViewDir); return float2(atan2(d.y,d.x)/6.2831853+0.5,0.5-asin(clamp(d.z,-1,1))/3.14159265);')
    tex=mel.create_material_expression(sky,unreal.MaterialExpressionTextureSample)
    tex.texture=lib.load_asset(folder+'/T_Sierra_Cielo')
    mel.connect_material_expressions(camera,'',panorama,'ViewDir')
    mel.connect_material_expressions(panorama,'',tex,'UVs')
    mel.connect_material_property(tex,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(sky)
    lib.save_loaded_asset(sky)
assets=sorted(source.glob('SM_*.fbx'))
if len(assets)!=17:raise RuntimeError(f'Run the Blender generator first ({len(assets)} meshes)')
selection=globals().get('RV_ARGS',{}).get('meshes')
if selection:assets=[f for f in assets if f.stem in selection]
flag='Interchange.FeatureFlags.Import.FBX'
prior=unreal.SystemLibrary.get_console_variable_int_value(flag)
unreal.SystemLibrary.execute_console_command(world,flag+' 0')
try:
    for file in assets:
        name=file.stem
        if not lib.does_asset_exist(folder+'/'+name) or refresh:
            options=unreal.FbxImportUI()
            options.automated_import_should_detect_type=False
            options.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
            options.import_as_skeletal=False
            options.import_materials=True
            options.import_textures=False
            data=options.static_mesh_import_data
            data.combine_meshes=True
            data.auto_generate_collision=False
            data.generate_lightmap_u_vs=False
            data.force_front_x_axis=True
            data.convert_scene_unit=True
            data.normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
            task=unreal.AssetImportTask()
            task.filename,task.destination_path,task.destination_name=str(file),folder,name
            task.automated=task.save=True
            task.replace_existing=task.replace_existing_settings=refresh
            task.factory=unreal.FbxFactory()
            task.options=options
            tools.import_asset_tasks([task])
        mesh=lib.load_asset(folder+'/'+name)
        for i,slot in enumerate(mesh.static_materials):
            key=str(slot.material_slot_name)
            if key=='M_Via_Asfalto':
                mesh.set_material(i,lib.load_asset('/Game/Ciudad/Downtown_YongeDundas/Materials/M_Asphalt'))
            if name in ('SM_Norte_Naves','SM_Patio_Escuela') and key in ('M_Via_Ladrillo','M_Via_Arena'):
                facade='M_facade_beige_brick' if key=='M_Via_Arena' else 'M_facade_brick_building'
                mesh.set_material(i,lib.load_asset('/Game/Ciudad/Downtown_YongeDundas/Materials/'+facade))
            if key in ('M_Via_Pasto','M_Via_Roca'):
                mesh.set_material(i,terrain_mat)
        if name in ('SM_Norte_Avenida','SM_Norte_Naves','SM_Sierra_Calzada','SM_Sierra_Defensas','SM_Sierra_Relieve','SM_Patio_Escuela'):
            mesh.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        lib.save_loaded_asset(mesh)
finally:
    unreal.SystemLibrary.execute_console_command(world,f'{flag} {prior}')
lib.save_directory(folder,only_if_is_dirty=True,recursive=False)
RV_RESULT=[(f.stem,lib.load_asset(folder+'/'+f.stem).get_bounds().box_extent.to_tuple()) for f in assets]
