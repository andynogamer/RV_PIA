"""Create the urban sandbox without editing the short practice map or city originals."""
import json
from pathlib import Path
import unreal

lib=unreal.EditorAssetLibrary
MAP='/Game/RutaVirtual/Maps/L_CiudadUrbana'
if lib.does_asset_exist(MAP):
    raise RuntimeError('Urban map already exists; inspect it instead of overwriting it.')
if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages():
    raise RuntimeError('Save the current level before creating the urban map.')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not levels.new_level_from_template(MAP,'/Game/RutaVirtual/Maps/L_CiudadPractica'):
    raise RuntimeError('Could not create urban map from the practice template')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
collision_folder='/Game/RutaVirtual/Environment/UrbanCollision'
lib.make_directory(collision_folder)
removed=0; collisions=0
all_actors=list(actors.get_all_level_actors())
# These street-level facades are low or thin enough to miss the general building filter.
sidewalk_facades={'SM_element_026','SM_element_042','SM_slc_bottom','SM_element_084'}
with unreal.ScopedSlowTask(len(all_actors),'Preparando ciudad y colisiones') as progress:
    progress.make_dialog(False)
    for actor in all_actors:
        progress.enter_progress_frame(1)
        name=actor.get_actor_label()
        if isinstance(actor,unreal.SkyLight) and name=='Luz_Cielo':
            # Daylight is fixed. Capture it on load, outside the stereo eye render pass.
            actor.light_component.set_editor_property('real_time_capture',False)
        if name.startswith('Practica_'):
            actors.destroy_actor(actor); continue
        if not isinstance(actor,unreal.StaticMeshActor) or str(actor.get_folder_path())!='Ciudad': continue
        component=actor.static_mesh_component
        mesh=component.static_mesh
        mesh_name=mesh.get_name()
        car_prefixes=('SM_Body_','SM_Interior_','SM_Wiper_','SM_FrontLeftWheel','SM_FrontRightWheel','SM_RearLeftWheel','SM_RearRightWheel')
        if mesh_name.startswith(car_prefixes):
            actors.destroy_actor(actor); removed+=1; continue
        bounds=mesh.get_bounds(); extent=bounds.box_extent
        # Exact static triangle collision on substantial buildings. Avoid pooled cables/props.
        large_building=(extent.z>250 and min(extent.x,extent.y)>180
                        and max(extent.x,extent.y)<10000 and mesh_name!='SM_Ground')
        if mesh_name in sidewalk_facades or large_building:
            target=collision_folder+'/'+mesh_name
            duplicate=lib.load_asset(target) if lib.does_asset_exist(target) else lib.duplicate_asset(mesh.get_path_name().split('.')[0],target)
            body=duplicate.get_editor_property('body_setup')
            body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            if mesh_name in sidewalk_facades:
                body.set_editor_property('double_sided_geometry',True)
            lib.save_loaded_asset(duplicate)
            component.set_static_mesh(duplicate);component.set_collision_profile_name('BlockAll')
            collisions+=1

for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.PlayerStart):
        actor.set_actor_location(unreal.Vector(-9850,-18200,120),False,True)
        actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)

scenario=actors.spawn_actor_from_class(unreal.RVUrbanScenario,unreal.Vector())
scenario.set_actor_label('Urbano_Escenario')
scenario.set_editor_property('checkpoints',[
    unreal.Vector(-9500,-17000,0),unreal.Vector(-9460,-11000,0),
    unreal.Vector(-9390,-2500,0),unreal.Vector(-9650,6500,0)])

routes=[
 ('Norte',[(-9680,-32000,70),(-9560,-18000,70),(-9460,-8000,70),(-9350,2000,70),(-9200,24000,70)],4,20),
 ('Sur',[(-8600,24000,70),(-8750,2000,70),(-8860,-8000,70),(-8960,-18000,70),(-9080,-32000,70)],4,18),
 ('Este',[(-17500,-24180,70),(-9500,-24180,70),(0,-24080,70),(10000,-23920,70),(19000,-24020,70),(33000,-24200,70)],2,18),
 ('Oeste',[(33000,-24800,70),(19000,-24620,70),(10000,-24520,70),(0,-24680,70),(-9500,-24780,70),(-17500,-24780,70)],2,18),
]
for name,points,count,speed in routes:
    route=actors.spawn_actor_from_class(unreal.RVTrafficRoute,unreal.Vector())
    route.set_actor_label('Trafico_Carril_'+name);route.set_folder_path('Trafico/Carriles')
    route.set_editor_property('vehicle_count',count);route.set_editor_property('cruise_kmh',speed)
    spline=route.get_editor_property('spline');spline.clear_spline_points(False)
    for point in points: spline.add_spline_point(unreal.Vector(*point),unreal.SplineCoordinateSpace.WORLD,False)
    for index in range(len(points)): spline.set_spline_point_type(index,unreal.SplinePointType.LINEAR,False)
    spline.update_spline()
for name,position,offset in [('Sur',(-9300,-24480,0),0),('Norte',(-9090,-4850,0),8)]:
    signal=actors.spawn_actor_from_class(unreal.RVTrafficSignal,unreal.Vector(*position))
    signal.set_actor_label('Semaforo_Cruce_'+name);signal.set_folder_path('Trafico/Semaforos')
    signal.set_editor_property('cycle_offset',offset);signal.refresh_lights()

cube=lib.load_asset('/Engine/BasicShapes/Cube')
white=lib.load_asset('/Game/RutaVirtual/Materials/M_SenalBlanca')
green=lib.load_asset('/Game/RutaVirtual/Materials/M_ZonaDetencion')
for name,location,scale,material in [
 ('BahiaInicio',(-9850,-18200,2),(2.8,6,0.01),green),
 ('BahiaDestino',(-9650,6500,2),(2.8,6,0.01),green),
 ('LineaRojoSurNorte',(-9500,-25400,3),(3.4,.18,.012),white),
 ('LineaRojoSurSur',(-8950,-23550,3),(3.4,.18,.012),white),
 ('LineaRojoNorte',(-9390,-5770,3),(3.4,.18,.012),white),
 ('LineaRojoEste',(-10200,-24180,3),(.18,3.4,.012),white),
 ('LineaRojoOeste',(-8400,-24780,3),(.18,3.4,.012),white),
]:
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*location))
    actor.set_actor_label('Urbano_'+name);actor.set_folder_path('Urbano/Senalizacion')
    actor.static_mesh_component.set_static_mesh(cube);actor.static_mesh_component.set_material(0,material)
    actor.static_mesh_component.set_collision_profile_name('NoCollision');actor.set_actor_scale3d(unreal.Vector(*scale))
# Physical road signs and the northern district are placed by build_road_maps.py.
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
    unreal.Vector(-9000,-21100,2000),unreal.Rotator(pitch=-20,yaw=90,roll=0))
levels.save_current_level()
RV_RESULT={'map':MAP,'routes':len(routes),'vehicles_configured':12,'signals':2,'building_collision_meshes':collisions,
           'removed_decorative_car_parts':removed,'playground_dimensions_m':[770,760],'start':[-9850,-18200,120]}
Path(unreal.Paths.project_saved_dir(),'AgentReports','urban_build.json').write_text(json.dumps(RV_RESULT,indent=2),encoding='utf-8')
