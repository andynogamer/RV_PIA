"""Integration checks in the urban PIE world. Keep Unreal in the foreground.

Moves the player and a selected traffic agent; do not run during a human session.
No physical peripheral is exercised. Sound may play; this test asserts horn events,
not the mixer signal or perceived output (see test_audio_mix.py for mixer capture).
"""
import json
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
controller=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if isinstance(controller,unreal.RVPracticePlayerController):controller.start_session()
player=unreal.GameplayStatics.get_player_pawn(world,0) if world else None
if not player: raise RuntimeError('Start urban PIE first.')
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
scenario=unreal.GameplayStatics.get_actor_of_class(world,unreal.RVUrbanScenario)
agents=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle)
signals=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficSignal)
if not scenario or not agents or not signals: raise RuntimeError('Urban traffic was not spawned.')
light=next(s for s in signals if s.get_actor_label()=='Semaforo_Cruce_Sur')
route=next(a.route for a in agents if 'Carril_Norte' in a.route.get_actor_label())
agent=next(a for a in agents if a.route==route)
# Previous route tests leave traffic at arbitrary distances. Keep this signal/
# player-obstacle fixture isolated so another agent cannot spawn against or push
# the parked player. Population is still checked before isolating the actors.
traffic_snapshot=[(a,a.distance_along_route,a.is_actor_tick_enabled(),a.get_actor_enable_collision()) for a in agents]
for a in agents:
    if a!=agent:a.set_actor_tick_enabled(False);a.set_actor_enable_collision(False)
subsystem=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ['Throttle','Brake','Steering','Handbrake']}
checks={'urban_mode':assist.get_editor_property('urban_mode'),'population':len(agents)==12,'signal_count':len(signals)==8}
expected=[(unreal.RVSignalColor.GREEN,unreal.RVSignalColor.RED),
          (unreal.RVSignalColor.AMBER,unreal.RVSignalColor.RED),
          (unreal.RVSignalColor.RED,unreal.RVSignalColor.RED),
          (unreal.RVSignalColor.RED,unreal.RVSignalColor.GREEN),
          (unreal.RVSignalColor.RED,unreal.RVSignalColor.AMBER),
          (unreal.RVSignalColor.RED,unreal.RVSignalColor.RED)]
checks['exclusive_signal_phases']=True
checks['city_lenses_bound']=sum(len(s.city_lamps) for s in signals)==150 and all(s.city_lamps for s in signals)
checks['visible_lamps_match_cycle']=True
for phase,pair in enumerate(expected):
    light.phase_override=phase
    checks['exclusive_signal_phases'] &= (light.get_signal_for_axis(False),light.get_signal_for_axis(True))==pair
    for signal in signals:
        signal.phase_override=phase;signal.refresh_lights()
        for binding in signal.city_lamps:
            color=binding.component.get_material(0).get_vector_parameter_value('LampColor')
            lit=max(color.r,color.g,color.b)>.1
            checks['visible_lamps_match_cycle'] &= lit==(signal.get_signal_for_axis(binding.x_axis)==binding.color)
light.phase_override=-1
original_offset=light.cycle_offset
checks['automatic_cycle_order']=True
for seconds,phase in [(0.1,0),(12.1,1),(15.1,2),(17.1,3),(29.1,4),(32.1,5)]:
    light.cycle_offset=seconds-unreal.GameplayStatics.get_time_seconds(world)
    checks['automatic_cycle_order'] &= light.get_phase()==phase
light.cycle_offset=original_offset
for s in signals:s.phase_override=0
light.phase_override=3
state={'phase':0,'elapsed':0.0,'enter':True,'records':[],'initial_horns':agent.horn_event_count}
phases=[('settle',1.5),('parked_throttle',1.2),('release_handbrake',0.3),('released',0.5),
        ('red',9.0),('green',5.0),('player_block',10.0),('cleared',3.0),('paused',.5),('resumed',.5)]
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'urban_traffic_test.json'


def inject(values):
    for name,action in actions.items():subsystem.inject_input_vector_for_action(action,unreal.Vector(values.get(name,0),0,0),[],[])


def move_player(position):
    player.set_actor_location(position,False,True)
    player.mesh.set_physics_linear_velocity(unreal.Vector())
    player.mesh.set_physics_angular_velocity_in_degrees(unreal.Vector())
    player.vehicle_movement_component.reset_vehicle()
    player.vehicle_movement_component.set_target_gear(1,True)
    player.vehicle_movement_component.set_handbrake_input(True)


def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    if unreal.GameplayStatics.is_game_paused(world):assist.toggle_practice_pause()
    for s in signals:s.phase_override=-1
    for a,d,ticking,collision in traffic_snapshot:
        a.teleport_to_distance(d);a.set_actor_enable_collision(collision);a.set_actor_tick_enabled(ticking)
    assist.reset_practice()
    report={'passed':error is None and all(checks.values()),'error':error,'checks':checks,
            'records':state['records'],'traffic_agents':len(agents),
            'method':'PIE physics, native traffic, controlled signal phases and Enhanced Input',
            'limits':'Prototype guided traffic; horn events only, no mixer/perceived audio, headset or G920 validation.'}
    output.write_text(json.dumps(report,indent=2,default=str),encoding='utf-8')


def tick(delta):
    try:
        name,duration=phases[state['phase']]
        if state['enter']:
            if name=='red':
                agent.route=route
                distance=route.spline.get_distance_along_spline_at_location(unreal.Vector(-9630,-26700,70),unreal.SplineCoordinateSpace.WORLD)
                agent.teleport_to_distance(distance)
                move_player(unreal.Vector(-9620,-25300,120))
                state['red_start']=agent.distance_along_route
            if name=='green':
                state['green_start']=agent.distance_along_route
                assist.reset_practice();light.phase_override=0
            if name=='player_block':
                point=route.spline.get_location_at_distance_along_spline(agent.distance_along_route+1100,unreal.SplineCoordinateSpace.WORLD)
                move_player(unreal.Vector(point.x,point.y,120))
                state['block_horns']=agent.horn_event_count
            if name=='cleared':
                state['cleared_start']=agent.distance_along_route
                assist.reset_practice()
            if name=='paused':
                state['pause_distance']=agent.distance_along_route
                state['pause_time']=unreal.GameplayStatics.get_time_seconds(world)
                assist.toggle_practice_pause()
            if name=='resumed':
                assist.toggle_practice_pause()
                state['resume_distance']=agent.distance_along_route
            state['enter']=False
        inject({'Throttle':1} if name=='parked_throttle' else ({'Handbrake':1} if name=='release_handbrake' else ({'Brake':1} if name=='player_block' else {})))
        state['elapsed']+=min(delta,.05)
        if state['elapsed']<duration:return
        state['records'].append({'phase':name,'agent_speed_kmh':agent.get_speed_kmh(),'distance_cm':agent.distance_along_route,
            'reason':str(agent.stop_reason),'horns':agent.horn_event_count,'player_speed_kmh':assist.get_speed_kmh()})
        if name=='settle':checks['starts_parked']=player.vehicle_movement_component.get_handbrake_input() and assist.get_speed_kmh()<.5
        if name=='parked_throttle':checks['parking_brake_holds']=assist.get_speed_kmh()<.5
        if name=='released':checks['handbrake_releases']=not player.vehicle_movement_component.get_handbrake_input()
        if name=='red':
            checks['stops_before_red']=agent.get_speed_kmh()<.5 and agent.get_actor_location().y<light.get_actor_location().y-light.stop_line_offset_cm
            checks['no_horn_at_red']=agent.horn_event_count==state['initial_horns']
        if name=='green':checks['resumes_on_green']=agent.distance_along_route>state['green_start']+300 and agent.get_speed_kmh()>2
        if name=='player_block':
            a=agent.get_actor_location();p=player.get_actor_location()
            checks['avoids_player']=agent.get_speed_kmh()<.5 and ((a.x-p.x)**2+(a.y-p.y)**2)**.5>350
            checks['contextual_horn']=agent.horn_event_count>state['block_horns']
            checks['horn_cooldown']=agent.horn_event_count-state['block_horns']<=1
        if name=='cleared':
            checks['resumes_after_obstacle']=agent.distance_along_route>state['cleared_start']+150
        if name=='paused':
            checks['pause_applies_to_world']=unreal.GameplayStatics.is_game_paused(world)
            checks['traffic_frozen_during_pause']=abs(agent.distance_along_route-state['pause_distance'])<.01
            checks['signal_clock_frozen_during_pause']=abs(unreal.GameplayStatics.get_time_seconds(world)-state['pause_time'])<.01
        if name=='resumed':checks['traffic_moves_after_pause']=agent.distance_along_route>state['resume_distance']+20
        state['phase']+=1;state['elapsed']=0;state['enter']=True
        if state['phase']==len(phases):finish()
    except Exception as error:finish(str(error))


handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output),'agents':len(agents)}
