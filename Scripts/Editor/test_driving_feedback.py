"""Exercise real driving infractions/contacts through Chaos and Enhanced Input.

Temporarily moves the player, isolates traffic and overrides signal phases. Never
run during a human driving session or with physical force output enabled.
"""
import json
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if not pc:raise RuntimeError('Start urban PIE first.')
pc.start_session()
player=unreal.GameplayStatics.get_player_pawn(world,0)
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
feedback=player.get_component_by_class(unreal.RVDrivingFeedbackComponent)
audio=player.get_component_by_class(unreal.RVVehicleAudioComponent)
movement=player.vehicle_movement_component
assist.reset_practice()
agents=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
signals=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficSignal))
snapshot=[(a,a.distance_along_route) for a in agents]
for a in agents:a.set_actor_tick_enabled(False);a.set_actor_enable_collision(False);a.set_actor_hidden_in_game(True)
light=next(s for s in signals if s.get_actor_label()=='Semaforo_Cruce_Sur')
for s in signals:s.phase_override=0
light.phase_override=3
route=next(a.route for a in agents if a.route.get_actor_label().endswith('_Norte'))
lead=next(a for a in agents if a.route==route)
original_cruise=route.cruise_kmh;original_preference=lead.preferred_speed_factor
scenario=unreal.GameplayStatics.get_actor_of_class(world,unreal.RVUrbanScenario)
original_limit=scenario.posted_speed_kmh;original_reward=feedback.clean_driving_reward_seconds
subsystem=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ['Throttle','Brake','Steering','Handbrake']}
wall=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(-9500,-16000,80),unreal.Vector(-12500,-16000,80),
    unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[player]+agents,unreal.DrawDebugTrace.NONE)
if not wall:raise RuntimeError('Could not find the city collision wall for this test.')
wall_x=wall.to_tuple()[4].x
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'driving_feedback_test.json'
phases=[('settle',.8),('red_ready',.35),('red',8),('bad_stop',6),('return_to_stop',.3),('wait_stop',2),('good_stop',6),
        ('collision',8),('collision_quiet',5),('overspeed',8.5),('wrong_way',8),
        ('lead_prepare',3.5),('following',2.2),('clean_driving',9)]
state={'phase':0,'elapsed':0.0,'enter':True,'checks':{},'records':[],'peak_pitch':audio.engine_pitch,
       'audio_before':audio.feedback_sounds_played}

def pose(x,y,yaw=90,z=25,velocity=None,parked=False):
    player.set_actor_location(unreal.Vector(x,y,z),False,True)
    player.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),True)
    player.mesh.set_physics_linear_velocity(unreal.Vector())
    player.mesh.set_physics_angular_velocity_in_degrees(unreal.Vector())
    movement.reset_vehicle();movement.set_target_gear(1,True);movement.set_handbrake_input(parked)
    feedback.refresh_collision_notifications()
    movement.set_brake_input(0);movement.set_throttle_input(0)
    if abs((player.get_actor_rotation().yaw-yaw+180)%360-180)>1:raise RuntimeError('Unexpected test vehicle heading.')
    if velocity:player.mesh.set_physics_linear_velocity(velocity)

# A long full-throttle manoeuvre can generate more than one valid event on an open road.
# Observe every emitted score change, including simultaneous incidents.
phase_events = []
def observe_event(event):
    phase_events.append((str(event.code),event.score_delta))
feedback.on_feedback.add_callable(observe_event)

def finish(error=None):
    feedback.on_feedback.remove_callable(observe_event)
    unreal.unregister_slate_post_tick_callback(handle)
    for a,d in snapshot:a.teleport_to_distance(d);a.set_actor_enable_collision(True);a.set_actor_tick_enabled(True);a.set_actor_hidden_in_game(False)
    route.cruise_kmh=original_cruise;lead.preferred_speed_factor=original_preference
    scenario.posted_speed_kmh=original_limit;feedback.clean_driving_reward_seconds=original_reward
    for s in signals:s.phase_override=-1
    result={'error':error,'checks':state['checks'],'records':state['records'],'peak_engine_pitch':state['peak_pitch'],
            'limits':'Explicit training rules and thresholds, not legal fault attribution or clinical assessment. No haptic hardware output.'}
    assist.reset_practice();pc.show_start_menu()
    state['checks']['reset_clears_score_events']=feedback.safety_score==100 and feedback.penalty_count==0 and feedback.latest_feedback.haptic_request==0
    feedback.register_incident('collision')
    state['checks']['menu_rejects_incidents']=feedback.penalty_count==0 and feedback.safety_score==100
    result['passed']=error is None and all(state['checks'].values())
    output.write_text(json.dumps(result,indent=2,default=str),encoding='utf-8')

def tick(delta):
    try:
        name,duration=phases[state['phase']]
        if state['enter']:
            phase_events.clear()
            state['penalties_before']=feedback.penalty_count;state['rewards_before']=feedback.reward_count
            if name=='red_ready':pose(-9610,-26200)
            if name=='bad_stop':pose(-9850,-18300)
            if name=='return_to_stop':pose(-9850,-19000,parked=True)
            if name=='wait_stop':pose(-9850,-18000,parked=True)
            if name=='good_stop':movement.set_handbrake_input(False)
            if name=='collision':pose(wall_x+1000,-16000,180)
            if name=='overspeed':scenario.posted_speed_kmh=8;pose(-9460,-14000)
            if name=='wrong_way':scenario.posted_speed_kmh=original_limit;pose(-9460,-14000,-90)
            if name=='lead_prepare':
                pose(-9850,-18200,parked=True)
                route.cruise_kmh=10;lead.preferred_speed_factor=1
                lead.teleport_to_distance(route.spline.get_distance_along_spline_at_location(unreal.Vector(-9460,-13000,70),unreal.SplineCoordinateSpace.WORLD))
                lead.set_actor_enable_collision(True);lead.set_actor_tick_enabled(True);lead.set_actor_hidden_in_game(False)
            if name=='following':
                p=route.spline.get_location_at_distance_along_spline(lead.distance_along_route-760,unreal.SplineCoordinateSpace.WORLD)
                pose(p.x,p.y,velocity=unreal.Vector(0,300,0))
            if name=='clean_driving':
                lead.set_actor_tick_enabled(False);lead.set_actor_enable_collision(False)
                route.cruise_kmh=original_cruise;feedback.clean_driving_reward_seconds=5
                pose(2000,-24180,0)
            state['enter']=False
        values={}
        if name in ['red','bad_stop','good_stop','collision','overspeed','wrong_way']:values={'Throttle':1}
        if name in ['return_to_stop','wait_stop','collision_quiet','lead_prepare']:values={'Handbrake':1}
        if name=='following':values={'Throttle':.2}
        if name=='clean_driving':values={'Throttle':.5}
        for n,a in actions.items():subsystem.inject_input_vector_for_action(a,unreal.Vector(values.get(n,0),0,0),[],[])
        state['peak_pitch']=max(state['peak_pitch'],audio.engine_pitch)
        state['elapsed']+=min(delta,.05)
        if state['elapsed']<duration:return
        event=feedback.latest_feedback
        state['records'].append({'phase':name,'score':feedback.safety_score,'penalties':feedback.penalty_count,'rewards':feedback.reward_count,
            'collisions':feedback.collision_count,'event':str(event.code),'phase_events':list(phase_events),'delta':event.score_delta,'haptic_request':event.haptic_request,
            'haptic_duration_s':event.haptic_duration_seconds,'speed_kmh':assist.get_speed_kmh(),
            'position_cm':str(player.get_actor_location()),'feedback_sounds':audio.feedback_sounds_played})
        checks=state['checks']
        if name=='red':checks['red_light_penalty']=feedback.penalty_count>state['penalties_before'] and ('red_light',-12) in phase_events
        if name=='bad_stop':checks['stop_without_stopping_penalty']=feedback.penalty_count>state['penalties_before'] and str(event.code)=='stop_sign'
        if name=='good_stop':checks['proper_stop_reward']=feedback.reward_count>state['rewards_before'] and feedback.penalty_count==state['penalties_before']
        if name=='collision':
            checks['physical_contact_penalty']=feedback.collision_count>0 and str(event.code)=='collision'
            checks['future_haptic_is_bounded_pulse']=0<event.haptic_request<=1 and 0<event.haptic_duration_seconds<=.25
        if name=='collision_quiet':checks['stationary_contact_not_repeated']=feedback.penalty_count==state['penalties_before']
        if name=='overspeed':checks['sustained_overspeed_penalty']=feedback.penalty_count>state['penalties_before'] and str(event.code)=='overspeed'
        if name=='wrong_way':checks['sustained_wrong_way_penalty']=feedback.penalty_count>state['penalties_before'] and ('wrong_way',-8) in phase_events
        if name=='following':checks['unsafe_headway_penalty']=feedback.penalty_count>state['penalties_before'] and str(event.code)=='following'
        if name=='clean_driving':
            checks['clean_driving_reward']=feedback.reward_count>state['rewards_before']
            checks['score_stays_in_range']=0<=feedback.safety_score<=100
            checks['audio_feedback_events']=audio.feedback_sounds_played>state['audio_before']
            checks['engine_pitch_changes_with_driving']=state['peak_pitch']>1
        state['phase']+=1;state['elapsed']=0;state['enter']=True
        if state['phase']==len(phases):finish()
    except Exception as error:finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output),'collision_wall_x_cm':wall_x}
