"""Isolated native traffic tests: pure signal stop, reaction, exit clearance, following."""
import json
from pathlib import Path
import unreal
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if not pc:raise RuntimeError('Start urban PIE first.')
pc.start_session()
player=unreal.GameplayStatics.get_player_pawn(world,0)
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent);assist.reset_practice()
agents=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
signals=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficSignal))
agent=next(a for a in agents if a.route.get_actor_label().endswith('_Norte'))
lead=next(a for a in agents if a!=agent and a.route==agent.route)
route=agent.route;original_cruise=route.cruise_kmh
snapshot=[(a,a.distance_along_route,a.preferred_speed_factor) for a in agents]
for a in agents:a.set_actor_tick_enabled(False);a.set_actor_enable_collision(False)
agent.set_actor_tick_enabled(True);agent.set_actor_enable_collision(True)
light=min(signals,key=lambda a:a.get_actor_location().y)
for s in signals:s.phase_override=0
light.phase_override=3
def place(a,y):
    d=route.spline.get_distance_along_spline_at_location(unreal.Vector(-9500,y,70),unreal.SplineCoordinateSpace.WORLD)
    a.teleport_to_distance(d)
place(agent,-27000)
state={'phase':0,'elapsed':0.0,'enter':True,'checks':{'varied_reactions':len({a.reaction_delay_seconds for a in agents})>1,
    'varied_preferences':len({a.preferred_speed_factor for a in agents})>1},'records':[],'minimum_gap':99999}
phases=[('red',9.0),('reaction',.15),('green',4.5),('blocked_exit',9),('clear_exit',4.5),('following',8),('lead_stop',4)]
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'traffic_refinements_test.json'

def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    route.cruise_kmh=original_cruise
    for a,d,f in snapshot:a.teleport_to_distance(d);a.preferred_speed_factor=f;a.set_actor_enable_collision(True);a.set_actor_tick_enabled(True)
    for s in signals:s.phase_override=-1
    assist.reset_practice()
    output.write_text(json.dumps({'passed':error is None and all(state['checks'].values()),'error':error,'checks':state['checks'],
        'records':state['records'],'minimum_following_gap_cm':state['minimum_gap'],'limits':'Guided kinematic traffic; no connected turns or human-driver model.'},indent=2),encoding='utf-8')

def tick(delta):
    try:
        name,duration=phases[state['phase']]
        if state['enter']:
            if name=='reaction':state['green_start']=agent.distance_along_route;light.phase_override=0
            if name=='blocked_exit':place(agent,-27000);place(lead,-23500);lead.set_actor_enable_collision(True)
            if name=='clear_exit':state['exit_start']=agent.distance_along_route;lead.set_actor_enable_collision(False)
            if name=='following':
                route.cruise_kmh=12;agent.preferred_speed_factor=1;lead.preferred_speed_factor=1
                place(agent,-15000);lead.teleport_to_distance(agent.distance_along_route+1100)
                lead.set_actor_enable_collision(True);lead.set_actor_tick_enabled(True)
            if name=='lead_stop':lead.set_actor_tick_enabled(False);lead.teleport_to_distance(lead.distance_along_route)
            state['enter']=False
        state['elapsed']+=min(delta,.05)
        if name=='following':state['minimum_gap']=min(state['minimum_gap'],lead.distance_along_route-agent.distance_along_route)
        if state['elapsed']<duration:return
        state['records'].append({'phase':name,'speed_kmh':agent.get_speed_kmh(),'reason':str(agent.stop_reason),'brake_lights':agent.get_editor_property('brake_lights_on')})
        checks=state['checks']
        if name=='red':
            checks['pure_red_stop']=agent.get_speed_kmh()<.5 and agent.get_actor_location().y<light.get_actor_location().y-light.stop_line_offset_cm-200
            checks['red_brake_lamps']=agent.get_editor_property('brake_lights_on')
        if name=='reaction':checks['delayed_green_reaction']=agent.distance_along_route-state['green_start']<5
        if name=='green':checks['green_acceleration']=agent.distance_along_route>state['green_start']+300 and agent.get_speed_kmh()>2
        if name=='blocked_exit':checks['waits_for_exit_space']=str(agent.stop_reason)=='intersection' and agent.get_speed_kmh()<.5
        if name=='clear_exit':checks['continues_after_exit_clear']=agent.distance_along_route>state['exit_start']+300
        if name=='following':checks['moving_headway']=state['minimum_gap']>850 and agent.get_speed_kmh()>2 and lead.get_speed_kmh()>2
        if name=='lead_stop':
            checks['stops_for_lead']=agent.get_speed_kmh()<.5 and lead.distance_along_route-agent.distance_along_route>500
            checks['stopped_brake_lamps']=agent.get_editor_property('brake_lights_on')
        state['phase']+=1;state['elapsed']=0;state['enter']=True
        if state['phase']==len(phases):finish()
    except Exception as error:finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output)}
