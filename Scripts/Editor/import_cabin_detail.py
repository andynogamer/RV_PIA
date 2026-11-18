"""Import the player-only trim and create the unlit mirror surface in Unreal."""
from pathlib import Path
import unreal

folder = "/Game/RutaVirtual/Vehicles/Interior/Detalle"
source = Path(unreal.Paths.project_dir()) / "SourceArt/Cockpit/Detalle"
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets.make_directory(folder)
meshes = ["SM_EspejoCentral", "SM_EspejoLateral", "SM_DetalleCabina", "SM_PalancaCabina", "SM_CarroceriaCabina"]
requested = globals().get("RV_ARGS",{}).get("meshes",meshes)
if any(n not in meshes for n in requested):
    raise ValueError("Malla de detalle desconocida")
for name in requested:
    refresh = globals().get("RV_ARGS",{}).get("refresh",False)
    if assets.does_asset_exist(f"{folder}/{name}") and not refresh:
        continue
    options = unreal.FbxImportUI()
    options.automated_import_should_detect_type = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.import_as_skeletal = False
    options.import_materials = not refresh and name != "SM_CarroceriaCabina"
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.force_front_x_axis = True
    options.static_mesh_import_data.convert_scene_unit = True
    task = unreal.AssetImportTask()
    task.filename, task.destination_path, task.destination_name = str((source / (name+".fbx")).resolve()), folder, name
    task.replace_existing_settings = True
    task.replace_existing = refresh
    task.automated, task.save, task.options, task.factory = True, True, options, unreal.FbxFactory()
    # Reimports otherwise dispatch to Interchange and can rotate the existing local basis by 90 degrees.
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    flag = "Interchange.FeatureFlags.Import.FBX"
    previous = unreal.SystemLibrary.get_console_variable_int_value(flag)
    unreal.SystemLibrary.execute_console_command(world,flag + " 0")
    try:
        tools.import_asset_tasks([task])
    finally:
        unreal.SystemLibrary.execute_console_command(world,f"{flag} {previous}")
    if not assets.does_asset_exist(f"{folder}/{name}"):
        raise RuntimeError(name)
    if name == "SM_CarroceriaCabina":
        shell = unreal.load_asset(folder + "/" + name)
        original = unreal.load_asset("/Game/Vehicles/SportsCar/SM_SportsCar")
        material_by_slot = {s.material_slot_name:s.material_interface for s in original.static_materials}
        for i,slot in enumerate(shell.static_materials):
            shell.set_material(i,material_by_slot[slot.material_slot_name])
        assets.save_loaded_asset(shell)
if not assets.does_asset_exist(folder + "/M_ReflejoCabina"):
    mat = tools.create_asset("M_ReflejoCabina",folder,unreal.Material,unreal.MaterialFactoryNew())
    mat.set_editor_property("shading_model",unreal.MaterialShadingModel.MSM_UNLIT)
    mel = unreal.MaterialEditingLibrary
    sample = mel.create_material_expression(mat,unreal.MaterialExpressionTextureSampleParameter2D)
    sample.set_editor_property("parameter_name","Reflejo")
    sample.set_editor_property("texture",unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    mel.connect_material_property(sample,"RGB",unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    assets.save_loaded_asset(mat)
assets.save_directory(folder, only_if_is_dirty=True, recursive=False)
bp = unreal.load_asset("/Game/RutaVirtual/Vehicles/BP_VehiculoPractica")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
for h in sub.k2_gather_subobject_data_for_blueprint(bp):
    obj = lib.get_object_for_blueprint(lib.get_data(h), bp)
    if isinstance(obj,unreal.SpringArmComponent) and obj.get_name().startswith("FrontSpringArm"):
        eye = unreal.Vector(-59,-32,109)
        if (obj.get_editor_property("relative_location")-eye).length()>.01:
            obj.set_editor_property("relative_location",eye)
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
            assets.save_loaded_asset(bp)
        break
else:
    raise RuntimeError("No se encontró el anclaje del asiento")
RV_RESULT = {n:[str(m.material_slot_name) for m in unreal.load_asset(folder+"/"+n).static_materials] for n in meshes}
