"""Drive the longer guided route with real Chaos physics and controlled input.

Temporarily hides/disables traffic to isolate road and exercise validation. Traffic
interactions are covered separately. Never use during a human driving session.
"""
import json
import math
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
player=unreal.GameplayStatics.get_player_pawn(world,0) if world else None
if not player: raise RuntimeError('Start urban PIE first.')
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
scenario=unreal.GameplayStatics.get_actor_of_class(world,unreal.RVUrbanScenario)
if not scenario or not assist.get_editor_property('urban_mode'): raise RuntimeError('Expected the urban scenario.')
agents=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
signals=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficSignal))
for agent in agents:
    agent.set_actor_tick_enabled(False);agent.set_actor_enable_collision(False);agent.set_actor_hidden_in_game(True)
for signal in signals:signal.phase_override=0
subsystem=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ['Throttle','Brake','Steering','Handbrake']}
assist.reset_practice()
state={'start':unreal.GameplayStatics.get_time_seconds(world),'last':player.get_actor_location(),
       'distance_cm':0.0,'stages':[],'checkpoints':[],'min_z':9999,'peak_kmh':0.0}
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'urban_route_test.json'

def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    for agent in agents:
        agent.set_actor_tick_enabled(True);agent.set_actor_enable_collision(True);agent.set_actor_hidden_in_game(False)
    for signal in signals:signal.phase_override=-1
    checks={'completed':assist.get_editor_property('practice_stage')==3,'visited_all_checkpoints':state['checkpoints']==[0,1,2,3],
        'starts_parked_then_drives':state['stages']==[0,1,2,3],'longer_than_short_course':state['distance_cm']>20000,
        'stopped_at_destination':assist.get_speed_kmh()<.5,'ground_support':state['min_z']>-30}
    report={'passed':error is None and all(checks.values()),'error':error,'checks':checks,'distance_m':state['distance_cm']*.01,
        'elapsed_simulated_s':unreal.GameplayStatics.get_time_seconds(world)-state['start'],'minimum_z_cm':state['min_z'],
        'peak_kmh':state['peak_kmh'],'stages':state['stages'],'checkpoints':state['checkpoints'],
        'limits':'Real player physics; AI disabled and signals green to isolate road traversal. No hardware validation.'}
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    assist.reset_practice()

def tick(delta):
    try:
        elapsed=unreal.GameplayStatics.get_time_seconds(world)-state['start']
        stage=assist.get_editor_property('practice_stage');index=assist.get_editor_property('urban_checkpoint_index')
        if not state['stages'] or state['stages'][-1]!=stage:state['stages'].append(stage)
        if not state['checkpoints'] or state['checkpoints'][-1]!=index:state['checkpoints'].append(index)
        p=player.get_actor_location();last=state['last']
        state['distance_cm']+=math.hypot(p.x-last.x,p.y-last.y);state['last']=p
        state['min_z']=min(state['min_z'],p.z);state['peak_kmh']=max(state['peak_kmh'],assist.get_speed_kmh())
        if stage==3:finish();return
        if elapsed>100:finish('Tour did not finish within 100 simulated seconds.');return
        target=scenario.checkpoints[index]
        bearing=math.degrees(math.atan2(target.y-p.y,target.x-p.x))
        error=(bearing-player.get_actor_rotation().yaw+180)%360-180
        speed=assist.get_speed_kmh()/3.6
        values={}
        if .2<elapsed<.5:values['Handbrake']=1
        if elapsed>.6:
            values={'Throttle':.6 if speed<4.5 else 0,'Steering':max(-.4,min(.4,error/45))}
            if index==len(scenario.checkpoints)-1 and assist.get_distance_to_goal_meters()<max(3,speed*speed/5+1.5):
                values['Throttle']=0;values['Brake']=1
        for name,action in actions.items():subsystem.inject_input_vector_for_action(action,unreal.Vector(values.get(name,0),0,0),[],[])
    except Exception as error:finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output)}
