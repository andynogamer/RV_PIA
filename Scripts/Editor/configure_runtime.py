"""Attach compiled practice rules and HUD to the existing training Blueprints."""
import unreal
lib = unreal.EditorAssetLibrary
bp = lib.load_asset('/Game/RutaVirtual/Vehicles/BP_VehiculoPractica')
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
data = unreal.SubobjectDataBlueprintFunctionLibrary
handles = sub.k2_gather_subobject_data_for_blueprint(bp)
existing = [data.get_object_for_blueprint(data.get_data(h), bp) for h in handles]
if not any(isinstance(obj, unreal.RVDrivingAssistComponent) for obj in existing if obj):
    handle, reason = sub.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=handles[0], new_class=unreal.RVDrivingAssistComponent, blueprint_context=bp))
    if str(reason):
        raise RuntimeError(str(reason))
    sub.rename_subobject(handle, unreal.Text('DrivingAssist'))
car = unreal.get_default_object(lib.load_blueprint_class(bp.get_path_name()))
movement = car.vehicle_movement_component
movement.set_editor_property('reverse_as_brake', False)
movement.set_editor_property('throttle_as_brake', False)
engine = movement.get_editor_property('engine_setup')
engine.set_editor_property('max_torque', 250.0)
movement.set_editor_property('engine_setup', engine)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
lib.save_loaded_asset(bp)
controller_path = '/Game/RutaVirtual/Blueprints/BP_ControladorPractica'
if not lib.does_asset_exist(controller_path):
    lib.duplicate_asset('/Game/VehicleTemplate/Blueprints/BP_VehicleAdvPlayerController', controller_path)
controller_bp = lib.load_asset(controller_path)
unreal.BlueprintEditorLibrary.reparent_blueprint(controller_bp, unreal.RVPracticePlayerController)
unreal.BlueprintEditorLibrary.compile_blueprint(controller_bp)
lib.save_loaded_asset(controller_bp)
mode_bp = lib.load_asset('/Game/RutaVirtual/Blueprints/BP_ModoPractica')
mode = unreal.get_default_object(lib.load_blueprint_class(mode_bp.get_path_name()))
mode.set_editor_property('hud_class', unreal.RVPracticeHUD)
mode.set_editor_property('player_controller_class', lib.load_blueprint_class(controller_path))
unreal.BlueprintEditorLibrary.compile_blueprint(mode_bp)
lib.save_loaded_asset(mode_bp)
RV_RESULT = {'runtime_component': 'RVDrivingAssistComponent', 'hud': 'RVPracticeHUD',
             'reverse_as_brake': movement.get_editor_property('reverse_as_brake')}
