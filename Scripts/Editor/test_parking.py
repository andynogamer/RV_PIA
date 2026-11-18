"""Drive the Chaos car into the two bays with Enhanced Input in isolated desktop PIE.
The fixture places the car in the aisle before each approach, then only applies pedals.
"""
import json,math,time
from pathlib import Path
import unreal

if '-RVNoWheelInput' not in unreal.SystemLibrary.get_command_line():
    raise RuntimeError('Open an isolated editor with -DisableWheelForTests first.')
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not w or 'L_Estacionamiento' not in w.get_name():raise RuntimeError('Start parking PIE first.')
pc=unreal.GameplayStatics.get_player_controller(w,0)
p=unreal.GameplayStatics.get_player_pawn(w,0)
assist=p.get_component_by_class(unreal.RVDrivingAssistComponent)
lot=unreal.GameplayStatics.get_actor_of_class(w,unreal.RVParkingScenario)
move=p.vehicle_movement_component
sub=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ('Throttle','Brake','Handbrake')}
pc.start_session()
assist.reset_practice()
cases=[('corner_outside',0,77,False),('angled',0,0,False),('front_in',0,0,False),('wrong_way',1,0,False),('reverse_in',1,0,True)]
state={'i':0,'checks':{},'runs':[],'start':time.monotonic()}
obstacles=[actor for actor in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)
           if actor.get_actor_label().startswith('Patio_Coche') and actor.get_actor_label().endswith('_Carroceria')]
hits=[]
for actor in obstacles:
    pos=actor.get_actor_location()
    hit=unreal.SystemLibrary.line_trace_single(w,pos+unreal.Vector(0,0,350),pos+unreal.Vector(0,0,25),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE)
    hits.append(bool(hit and hit.to_tuple()[9]==actor))
state['checks']['parked_car_collisions']=len(hits)==5 and all(hits)
wheels=[actor.get_actor_bounds(False) for actor in unreal.GameplayStatics.get_all_actors_of_class(w,unreal.StaticMeshActor)
        if actor.get_actor_label().startswith('Patio_Coche') and '_Rueda' in actor.get_actor_label()]
state['checks']['parked_tires_on_ground']=len(wheels)==20 and all(abs(center.z-extent.z)<1 for center,extent in wheels)
def inputs(throttle=0,brake=0,hand=0):
    for a,value in zip(actions.values(),(throttle,brake,hand)):
        sub.inject_input_vector_for_action(a,unreal.Vector(value,0,0),[],[])
def prepare():
    name,idx,offset,reverse=cases[state['i']]
    goal=lot.get_editor_property('bays')[idx].translation
    side=1 if idx==0 else -1
    yaw=0 if (side==1) != reverse else 180
    if name=='angled':
        yaw+=12
        offset-=880*math.tan(math.radians(12))
    p.set_actor_location_and_rotation(unreal.Vector(goal.x-side*880,goal.y+offset,68),unreal.Rotator(yaw=yaw),False,True)
    p.mesh.set_physics_linear_velocity(unreal.Vector())
    p.mesh.set_physics_angular_velocity_in_degrees(unreal.Vector())
    move.reset_vehicle()
    state.update(goal=goal,side=side,initial_count=lot.completed_bays,phase='release',sec=0.,peak=0.,messages=set(),expected_reverse=reverse)
def finish(error=''):
    unreal.unregister_slate_post_tick_callback(handle)
    inputs()
    saved={'pass':not error and all(state['checks'].values()),'error':error,'checks':state['checks'],'runs':state['runs']}
    Path(unreal.Paths.project_saved_dir(),'AgentReports','estacionamiento_pie.json').write_text(json.dumps(saved,ensure_ascii=False,indent=2),encoding='utf-8')
    pc.show_start_menu()
def tick(dt):
    try:
        if time.monotonic()-state['start']>130:raise RuntimeError('Timed out '+str(state['phase']))
        state['sec']+=dt
        name,idx,offset,rev=cases[state['i']]
        phase=state['phase']
        pos=p.get_actor_location()
        dist=(state['goal'].x-pos.x)*state['side']
        speed=assist.get_speed_kmh()
        state['peak']=max(state['peak'],speed)
        state['messages'].add(assist.get_instruction())
        if phase=='release':
            inputs(hand=int(.18<state['sec']<.55))
            if state['sec']>.95:
                if rev!=assist.reverse_mode and not assist.try_toggle_reverse():raise RuntimeError('Gear selection failed')
                state.update(phase='drive',sec=0.)
        elif phase=='drive':
            desired=min(5.5 if not rev else 4.3,max(1.5,dist*.018))
            if dist<=22:
                state.update(phase='brake',sec=0.)
                inputs(brake=1)
            else:inputs(throttle=min(.55,max(.08,(desired-speed)*.32)),brake=int(speed>desired+1))
        elif phase=='brake':
            inputs(brake=1)
            if state['sec']>2.0:
                if name in ('front_in','reverse_in'):
                    state['checks'][name+'_requires_handbrake']=lot.completed_bays==idx
                state.update(phase='hold',sec=0.)
        else:
            inputs(brake=1,hand=1)
            if .45<state['sec']<.85 and name in ('front_in','reverse_in'):
                state['checks'][name+'_brief_stop']=lot.completed_bays==idx
            if state['sec']<2.25:return
            expected=idx+(name in ('front_in','reverse_in'))
            state['checks'][name]=lot.completed_bays==expected and state['peak']>1 and abs(dist)<80
            if name=='angled':
                state['checks']['alignment_feedback']=any('Endereza' in msg for msg in state['messages'])
            state['runs'].append({'name':name,'end':pos.to_tuple(),'peak':state['peak'],'completed':lot.completed_bays,'hints':sorted(state['messages'])})
            if not state['checks'][name]:raise RuntimeError(name+' failed')
            state['i']+=1
            if state['i']==len(cases):
                state['checks']['completed']=assist.practice_stage==3
                assist.reset_practice()
                state['checks']['restart']=lot.completed_bays==0 and assist.practice_stage==0
                finish()
            else:
                inputs()
                prepare()
    except Exception as ex:finish(str(ex))
prepare()
handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT='running'
