"""Read-only Unreal Editor audit. Run with Scripts/Audit-Unreal.ps1.

Writes derived evidence under Saved/AgentReports; never saves packages.
This script runs inside Unreal's Python, not a standalone Python runtime.
"""
import collections
import datetime
import json
import os
import unreal


def audit():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(synchronous_search=True)
    assets = registry.get_assets_by_path('/Game', recursive=True)
    classes = collections.Counter(str(a.asset_class_path.asset_name) for a in assets)
    city = [a for a in assets if str(a.package_name).startswith('/Game/Ciudad/')]
    report = {
        'generated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'engine_version': unreal.SystemLibrary.get_engine_version(),
        'read_only': True,
        'asset_count': len(assets),
        'classes': dict(sorted(classes.items())),
        'city_classes': dict(collections.Counter(str(a.asset_class_path.asset_name) for a in city)),
        'maps': [], 'blueprints': [], 'city_meshes': [], 'errors': [],
    }
    for asset in assets:
        kind = str(asset.asset_class_path.asset_name)
        package = str(asset.package_name)
        if kind == 'Blueprint':
            report['blueprints'].append({
                'package': package,
                'parent': asset.get_tag_value('ParentClass'),
                'native_parent': asset.get_tag_value('NativeParentClass'),
            })
        if kind == 'StaticMesh' and package.startswith('/Game/Ciudad/'):
            try:
                mesh = asset.get_asset()
                body = mesh.get_editor_property('body_setup')
                shapes = 0
                trace = None
                if body:
                    geom = body.get_editor_property('agg_geom')
                    shapes = sum(len(geom.get_editor_property(field)) for field in (
                        'box_elems', 'sphere_elems', 'sphyl_elems', 'convex_elems'))
                    trace = str(body.get_editor_property('collision_trace_flag'))
                report['city_meshes'].append({
                    'package': package, 'simple_shapes': shapes,
                    'collision_trace_flag': trace, 'lods': mesh.get_num_lods(),
                })
            except Exception as exc:
                report['errors'].append({'package': package, 'error': str(exc)})
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for asset in assets:
        if str(asset.asset_class_path.asset_name) != 'World':
            continue
        package = str(asset.package_name)
        try:
            if not levels.load_level(package):
                raise RuntimeError('load_level returned false')
            actors = actors_api.get_all_level_actors()
            actor_classes = collections.Counter(a.get_class().get_name() for a in actors)
            city_actors = []
            starts = []
            for actor in actors:
                if actor.is_a(unreal.PlayerStart):
                    starts.append({'label': actor.get_actor_label(), 'location': str(actor.get_actor_location())})
                for component in actor.get_components_by_class(unreal.StaticMeshComponent):
                    mesh = component.get_editor_property('static_mesh')
                    if mesh and mesh.get_path_name().startswith('/Game/Ciudad/'):
                        city_actors.append({
                            'actor': actor.get_actor_label(), 'mesh': mesh.get_path_name(),
                            'location': str(actor.get_actor_location()),
                            'scale': str(actor.get_actor_scale3d()),
                            'collision_enabled': str(component.get_collision_enabled()),
                            'collision_profile': str(component.get_collision_profile_name()),
                        })
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
            settings = world.get_world_settings()
            mode = settings.get_editor_property('default_game_mode')
            report['maps'].append({
                'package': package, 'actor_count': len(actors),
                'actor_classes': dict(sorted(actor_classes.items())),
                'game_mode_override': str(mode), 'player_starts': starts,
                'city_component_count': len(city_actors), 'city_components': city_actors,
            })
        except Exception as exc:
            report['errors'].append({'map': package, 'error': str(exc)})
    folder = os.path.join(unreal.Paths.project_saved_dir(), 'AgentReports')
    os.makedirs(folder, exist_ok=True)
    output = os.path.abspath(os.path.join(folder, 'project_audit.json'))
    with open(output, 'w', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2, ensure_ascii=False)
    unreal.log('RUTA_AUDIT_RESULT=' + output)
    if report['errors']:
        raise RuntimeError('Audit incomplete; inspect errors in ' + output)


audit()
