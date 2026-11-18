"""Drive the rural loop or the urban extension using the player's real Chaos pawn and Enhanced Input.
Only run unattended in an isolated editor. The fixture temporarily clears AI collisions.
"""
import json,math
from pathlib import Path
import unreal
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not w:
    raise RuntimeError("Inicia Play en la ciudad o en la carretera antes de probar.")
pc=unreal.GameplayStatics.get_player_controller(w,0)
highway='L_Carretera' in w.get_name()
if not highway and 'L_CiudadUrbana' not in w.get_name():raise RuntimeError('Start open-road PIE')
pc.start_session()
player=unreal.GameplayStatics.get_player_pawn(w,0)
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
feedback=player.get_component_by_class(unreal.RVDrivingFeedbackComponent)
assist.reset_practice()
agents=unreal.GameplayStatics.get_all_actors_of_class(w,unreal.RVTrafficVehicle)
traffic=[(a,a.distance_along_route) for a in agents]
for a in agents:a.set_actor_enable_collision(False);a.set_actor_tick_enabled(False);a.set_actor_hidden_in_game(True)
route_name='Sierra_Carril_0' if highway else 'Trafico_Carril_Norte'
route=next(a.route for a in agents if a.route.get_actor_label()==route_name)
spline=route.spline
world_space=unreal.SplineCoordinateSpace.WORLD
if not highway:
    start_d=spline.get_distance_along_spline_at_location(unreal.Vector(-9100,25700,70),world_space)
    p=spline.get_location_at_distance_along_spline(start_d,world_space)
    rotation=spline.get_rotation_at_distance_along_spline(start_d,world_space)
    player.set_actor_location_and_rotation(p+unreal.Vector(0,0,30),rotation,False,True)
    player.mesh.set_physics_linear_velocity(unreal.Vector())
    player.mesh.set_physics_angular_velocity_in_degrees(unreal.Vector())
    player.vehicle_movement_component.reset_vehicle()
    player.vehicle_movement_component.set_handbrake_input(True)
sub=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ['Throttle','Brake','Steering','Handbrake']}
s={'start':unreal.GameplayStatics.get_time_seconds(w),'peak':0,'max_offset':0,'below_limit_penalty':False,
   'overspeed':False,'distance':0,'last':player.get_actor_location(),'checkpoints':[],'physics_gears':set()}
scenario=unreal.GameplayStatics.get_actor_of_class(w,unreal.RVUrbanScenario)
def done(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    for a,d in traffic:a.teleport_to_distance(d);a.set_actor_enable_collision(True);a.set_actor_tick_enabled(True);a.set_actor_hidden_in_game(False)
    limit=80 if highway else 50
    checks=dict(traffic_population=len(traffic)==(10 if highway else 12),posted=assist.get_posted_speed_kmh()==limit,
        can_exceed_posted=s['peak']>limit+5,appropriate_feedback=s['overspeed'] and not s['below_limit_penalty'])
    if highway:
        checks.update(full_loop=s['distance']>2900,parked_at_end=assist.practice_stage==3,
            visits_all=s['checkpoints']==list(range(len(scenario.checkpoints))))
    else:checks.update(extension_driven=s['distance']>485,stopped_at_turning_head=player.get_actor_location().y>74900 and assist.get_speed_kmh()<.5)
    result=dict(passed=all(checks.values()) and not error,checks=checks,error=error,peak_kmh=s['peak'],
        distance_m=s['distance'],cross_track_max_m=s['max_offset'],gears=sorted(s['physics_gears']),checkpoints=s['checkpoints'])
    Path(unreal.Paths.project_saved_dir(),'AgentReports','carretera_conduccion.json' if highway else 'distrito_conduccion.json').write_text(json.dumps(result,indent=2))
    pc.show_start_menu()

def tick(dt):
    try:
        elapsed=unreal.GameplayStatics.get_time_seconds(w)-s['start']
        p=player.get_actor_location();v=assist.get_speed_kmh();s['peak']=max(v,s['peak'])
        s['distance']+=(p-s['last']).length()*.01;s['last']=p
        s['physics_gears'].add(player.vehicle_movement_component.get_current_gear())
        index=assist.urban_checkpoint_index
        if not s['checkpoints'] or s['checkpoints'][-1]!=index:s['checkpoints'].append(index)
        if str(feedback.latest_feedback.code)=='overspeed':
            if s['peak']<assist.get_posted_speed_kmh()+2:s['below_limit_penalty']=True
            s['overspeed']=True
        if (highway and assist.practice_stage==3) or (not highway and p.y>74900 and v<.5):done();return
        if elapsed>(260 if highway else 95):done('No terminó la vuelta en el tiempo de prueba');return
        d=spline.get_distance_along_spline_at_location(p,world_space)
        lane_point=spline.get_location_at_distance_along_spline(d,world_space)
        s['max_offset']=max(s['max_offset'],(lane_point-p).length()*.01)
        aim=(d+max(850,v*23))%spline.get_spline_length()
        target=spline.get_location_at_distance_along_spline(aim,world_space)
        distance=assist.get_distance_to_goal_meters()
        last=highway and index==len(scenario.checkpoints)-1 and distance<85
        if last and distance<27:target=scenario.checkpoints[index]
        angle=math.degrees(math.atan2(target.y-p.y,target.x-p.x))
        error=(angle-player.get_actor_rotation().yaw+180)%360-180
        # Aim for up to 91 km/h once, then follow the road at a steady cruising speed.
        desired=assist.get_posted_speed_kmh()+11
        if s['overspeed']:desired=71 if highway else 49
        if not highway:
            distance=max(0,(75300-p.y)/100)
            last=distance<85
        if last:desired=min(desired,max(0,(distance-1)*1.4))
        val={'Handbrake':int(.25<elapsed<.55)}
        if elapsed>.72:
            val.update(Throttle=min(.97,max(0,(desired-v)*.15)),Brake=int(v>desired+5),
                       Steering=min(.7,max(-.7,error/30)))
            if last and distance<3.6:val.update(Brake=1,Throttle=0)
        for key,a in actions.items():sub.inject_input_vector_for_action(a,unreal.Vector(val.get(key,0),0,0),[],[])
    except Exception as ex:done(str(ex))
handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT='started'
