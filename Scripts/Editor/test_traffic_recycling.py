"""Check safe reuse of finite traffic lanes, including opposite-lane endpoints."""
import json
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
controller=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if isinstance(controller,unreal.RVPracticePlayerController):controller.start_session()
if not world: raise RuntimeError('Start urban PIE first.')
agents=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
agent=next(a for a in agents if a.route.get_actor_label().endswith('_Norte'))
opposite=next(a for a in agents if a.route.get_actor_label().endswith('_Sur'))
blocker=next(a for a in agents if a!=agent and a.route==agent.route)
assist=unreal.GameplayStatics.get_player_pawn(world,0).get_component_by_class(unreal.RVDrivingAssistComponent)
assist.reset_practice()
snapshots=[(a,a.distance_along_route) for a in agents]
for a in agents:a.set_actor_tick_enabled(False)
agent.set_actor_tick_enabled(True)
state={'phase':0,'elapsed':0.0,'checks':{}}
length=agent.route.spline.get_spline_length()
for a in agents:
    if a!=agent:a.teleport_to_distance(a.route.spline.get_spline_length()*.5)
opposite.teleport_to_distance(opposite.route.spline.get_spline_length())
agent.teleport_to_distance(length-50)
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'traffic_recycling_test.json'

def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    for a,d in snapshots:a.teleport_to_distance(d);a.set_actor_tick_enabled(True)
    output.write_text(json.dumps({'passed':error is None and all(state['checks'].values()),'error':error,
        'checks':state['checks'],'limits':'Editor traffic prototype; recycling is distance gated, not occlusion tested.'},indent=2),encoding='utf-8')

def tick(delta):
    try:
        state['elapsed']+=min(delta,.05)
        if state['elapsed']<.5:return
        if state['phase']==0:
            state['checks']['opposite_endpoint_does_not_deadlock']=agent.distance_along_route<500
            blocker.teleport_to_distance(300);agent.teleport_to_distance(length-50)
        elif state['phase']==1:
            state['checks']['occupied_start_prevents_recycling']=agent.distance_along_route>length-500
            blocker.teleport_to_distance(5000)
        elif state['phase']==2:
            state['checks']['clear_start_allows_recycling']=agent.distance_along_route<500
            player=unreal.GameplayStatics.get_player_pawn(world,0)
            player.set_actor_location(unreal.Vector(-9210,23900,120),False,True)
            player.mesh.set_physics_linear_velocity(unreal.Vector())
            player.mesh.set_physics_angular_velocity_in_degrees(unreal.Vector())
            player.vehicle_movement_component.reset_vehicle()
            player.vehicle_movement_component.set_handbrake_input(True)
            agent.teleport_to_distance(length-50)
        else:
            state['checks']['nearby_player_prevents_recycling']=agent.distance_along_route>length-500
            assist.reset_practice();finish();return
        state['phase']+=1;state['elapsed']=0.0
    except Exception as error:
        assist.reset_practice();finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output)}
