"""Import original Blender cockpit assets and attach them to the training Blueprint."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
destination = '/Game/RutaVirtual/Vehicles/Interior'
library = unreal.EditorAssetLibrary
library.make_directory(destination)
tasks = []
reimport = globals().get('RV_ARGS', {}).get('reimport', False)
for name in ['SM_Cabina', 'SM_Volante']:
    if library.does_asset_exist(destination + '/' + name) and not reimport:
        continue
    options = unreal.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property('import_as_skeletal', False)
    options.set_editor_property('import_materials', True)
    options.set_editor_property('import_textures', False)
    data = options.static_mesh_import_data
    data.set_editor_property('combine_meshes', True)
    data.set_editor_property('auto_generate_collision', False)
    data.set_editor_property('convert_scene_unit', True)
    data.set_editor_property('force_front_x_axis', True)
    task = unreal.AssetImportTask()
    task.filename = str(root / 'SourceArt' / 'Cockpit' / (name + '.fbx'))
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.save = True
    task.replace_existing = reimport
    task.options = options
    task.factory = unreal.FbxFactory()
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

blueprint = library.load_asset('/Game/RutaVirtual/Vehicles/BP_VehiculoPractica')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary
handles = sub.k2_gather_subobject_data_for_blueprint(blueprint)
parent = None
existing = {}
for handle in handles:
    obj = lib.get_object_for_blueprint(lib.get_data(handle), blueprint)
    if obj:
        existing[obj.get_name()] = obj
        if obj.get_name() == 'VehicleMesh':
            parent = handle
if parent is None:
    raise RuntimeError('Vehicle mesh attachment point not found')
bounds = {}
for name in ['SM_Cabina', 'SM_Volante']:
    mesh = library.load_asset(destination + '/' + name)
    if not mesh:
        raise RuntimeError('Import did not produce ' + name)
    bounds[name] = str(mesh.get_bounds())
    component_name = 'InteriorCabina' if name == 'SM_Cabina' else 'VolanteVisual'
    matching = next((o for n, o in existing.items() if n.startswith(component_name)), None)
    if matching is None:
        params = unreal.AddNewSubobjectParams(parent_handle=parent,
            new_class=unreal.StaticMeshComponent, blueprint_context=blueprint)
        handle, reason = sub.add_new_subobject(params)
        if str(reason):
            raise RuntimeError(str(reason))
        sub.rename_subobject(handle, unreal.Text(component_name))
        matching = lib.get_object_for_blueprint(lib.get_data(handle), blueprint)
    matching.set_editor_property('static_mesh', mesh)
    matching.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    matching.set_editor_property('relative_rotation', unreal.Rotator(pitch=0, yaw=90, roll=0))
    matching.set_editor_property('relative_location', unreal.Vector(-8, -32, 79) if name == 'SM_Volante' else unreal.Vector())
    matching.set_collision_profile_name('NoCollision')
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
library.save_loaded_asset(blueprint)
library.save_directory(destination, only_if_is_dirty=True, recursive=True)
RV_RESULT = {'bounds': bounds, 'vehicle': blueprint.get_path_name()}
