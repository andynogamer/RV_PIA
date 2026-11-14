"""Create a separate city level from imported meshes with baked world positions.

Run once through Invoke-Unreal.ps1. Does not reimport or modify source city assets.
The source import baked the scene transforms into mesh vertices; actors stay at origin.
"""
import json
from pathlib import Path
import unreal

MAP = '/Game/RutaVirtual/Maps/L_CiudadPractica'
GROUND = '/Game/RutaVirtual/Environment/SM_SueloConducible'
library = unreal.EditorAssetLibrary
if library.does_asset_exist(MAP):
    raise RuntimeError('Map already exists; inspect and edit it instead of overwriting it.')
if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
    raise RuntimeError('Save the current map before building a separate level.')
library.make_directory('/Game/RutaVirtual/Maps')
library.make_directory('/Game/RutaVirtual/Environment')
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.new_level(MAP):
    raise RuntimeError('Could not create city level')

source_ground = '/Game/Ciudad/Downtown_YongeDundas/StaticMeshes/SM_Ground'
ground = library.load_asset(GROUND) if library.does_asset_exist(GROUND) else library.duplicate_asset(source_ground, GROUND)
ground.get_editor_property('body_setup').set_editor_property(
    'collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
library.save_loaded_asset(ground)

assets = unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(
    '/Game/Ciudad/Downtown_YongeDundas/StaticMeshes', recursive=True)
count = 0
for asset in sorted(assets, key=lambda a: str(a.asset_name)):
    if str(asset.asset_class_path.asset_name) != 'StaticMesh':
        continue
    mesh = ground if str(asset.asset_name) == 'SM_Ground' else asset.get_asset()
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector())
    actor.set_actor_label(str(asset.asset_name))
    actor.set_folder_path('Ciudad')
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_collision_profile_name('BlockAll' if mesh == ground else 'NoCollision')
    component.set_mobility(unreal.ComponentMobility.STATIC)
    count += 1

sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 15000), unreal.Rotator(pitch=-40, yaw=-35, roll=0))
sun.set_actor_label('Luz_Sol')
sun.light_component.set_editor_property('intensity', 4.0)
sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
atmosphere = actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector())
atmosphere.set_actor_label('Cielo_Atmosfera')
sky_class = unreal.load_class(None, '/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere_C')
sky_sphere = actors.spawn_actor_from_class(sky_class, unreal.Vector())
sky_sphere.set_actor_label('Cielo_Esfera')
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
sky.set_actor_label('Luz_Cielo')
sky.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property('intensity', 0.8)
sky.light_component.set_editor_property('real_time_capture', True)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-9550, -16700, 120), unreal.Rotator(pitch=0, yaw=90, roll=0))
start.set_actor_label('Inicio_Practica')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode = library.load_blueprint_class('/Game/VehicleTemplate/Blueprints/BP_VehicleAdvGameMode')
world.get_world_settings().set_editor_property('default_game_mode', mode)
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
    unreal.Vector(0, -45000, 50000), unreal.Rotator(pitch=-55, yaw=90, roll=0))
if not levels.save_current_level():
    raise RuntimeError('City level could not be saved')
RV_RESULT = {'map': MAP, 'city_mesh_actors': count, 'ground_collision': str(
    ground.get_editor_property('body_setup').get_editor_property('collision_trace_flag')),
    'limitation': 'Only ground collision is enabled initially; training route and obstacle collision require validation.'}
folder = Path(unreal.Paths.project_saved_dir()) / 'AgentReports'
folder.mkdir(exist_ok=True, parents=True)
(folder / 'city_build.json').write_text(json.dumps(RV_RESULT, indent=2), encoding='utf-8')
