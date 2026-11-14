"""Read-only snapshot of the active editor and available vehicle resources."""
import unreal
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
city = registry.get_assets_by_path('/Game/Ciudad', recursive=True)
samples = []
for asset in city:
    if str(asset.asset_class_path.asset_name) == 'StaticMesh' and len(samples) < 4:
        mesh = asset.get_asset()
        samples.append({'name': asset.asset_name, 'bounds': str(mesh.get_bounds()),
                        'body': str(mesh.get_editor_property('body_setup'))})
RV_RESULT = {
    'engine': unreal.SystemLibrary.get_engine_version(),
    'world': world.get_path_name(),
    'actors': len(actors.get_all_level_actors()),
    'dirty_content': [str(p) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()],
    'dirty_maps': [str(p) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()],
    'chaos_available': hasattr(unreal, 'ChaosWheeledVehicleMovementComponent'),
    'city_samples': samples,
    'level_methods': [m for m in dir(unreal.LevelEditorSubsystem) if any(s in m for s in ['new_level','save','play'])],
}
