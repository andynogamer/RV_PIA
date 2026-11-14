"""Derive the training vehicle from Epic's UE 5.6 Chaos vehicle template."""
import unreal

lib = unreal.EditorAssetLibrary
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
data_lib = unreal.SubobjectDataBlueprintFunctionLibrary
folder = '/Game/RutaVirtual/Vehicles/'
lib.make_directory(folder)
CAR = folder + 'BP_VehiculoPractica'
MODE = '/Game/RutaVirtual/Blueprints/BP_ModoPractica'
lib.make_directory('/Game/RutaVirtual/Blueprints')
for source, target in (
    ('/Game/VehicleTemplate/Blueprints/SportsCar/BP_VehicleAdvSportsCar', CAR),
    ('/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode', MODE),
):
    if not lib.does_asset_exist(target):
        lib.duplicate_asset(source, target)

blueprint = lib.load_asset(CAR)
seen = set()
camera_settings = []
for handle in sub.k2_gather_subobject_data_for_blueprint(blueprint):
    component = data_lib.get_object_for_blueprint(data_lib.get_data(handle), blueprint)
    if not component or component.get_path_name() in seen:
        continue
    seen.add(component.get_path_name())
    name = component.get_name()
    if isinstance(component, unreal.CameraComponent):
        is_cabin = name.startswith('FrontCamera')
        component.set_editor_property('auto_activate', is_cabin)
        component.set_editor_property('lock_to_hmd', is_cabin)
        if is_cabin:
            component.set_editor_property('field_of_view', 85.0)
        camera_settings.append({'name': name, 'cabin_active': is_cabin})
    if isinstance(component, unreal.SpringArmComponent) and name.startswith('FrontSpringArm'):
        component.set_editor_property('relative_location', unreal.Vector(-35, -32, 105))
        component.set_editor_property('target_arm_length', 0.0)
        component.set_editor_property('enable_camera_lag', False)
        component.set_editor_property('enable_camera_rotation_lag', False)
        component.set_editor_property('do_collision_test', False)

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
lib.save_loaded_asset(blueprint)
car_class = lib.load_blueprint_class(CAR)
mode_bp = lib.load_asset(MODE)
mode_default = unreal.get_default_object(lib.load_blueprint_class(MODE))
mode_default.set_editor_property('default_pawn_class', car_class)
unreal.BlueprintEditorLibrary.compile_blueprint(mode_bp)
lib.save_loaded_asset(mode_bp)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', lib.load_blueprint_class(MODE))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

mapping = lib.load_asset('/Game/VehicleTemplate/Input/IMC_Vehicle_Default')
keys = [{'action': m.action.get_name(), 'key': str(unreal.InputLibrary.key_get_display_name(m.key))}
        for m in mapping.get_editor_property('mappings') if unreal.InputLibrary.key_is_keyboard_key(m.key)]
RV_RESULT = {'vehicle': CAR, 'mode': MODE, 'cameras': camera_settings, 'keyboard': keys,
             'movement_defaults': str(unreal.get_default_object(car_class).vehicle_movement_component.get_editor_property('engine_setup'))}
