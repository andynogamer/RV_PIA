"""Drive from PlayerStart into A1 and reverse into B2, using the actual car and parking lot."""
import json
import math
from pathlib import Path
import unreal

s=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
w=s.get_game_world()
if not w or 'L_Estacionamiento' not in w.get_name() or '-RVNoWheelInput' not in unreal.SystemLibrary.get_command_line():
    raise RuntimeError('Start isolated parking PIE.')
pc=unreal.GameplayStatics.get_player_controller(w,0)
p=unreal.GameplayStatics.get_player_pawn(w,0)
assist=p.get_component_by_class(unreal.RVDrivingAssistComponent)
lot=unreal.GameplayStatics.get_actor_of_class(w,unreal.RVParkingScenario)
sub=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
actions={n:unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_'+n) for n in ('Throttle','Brake','Steering','Handbrake')}
pc.start_session()
assist.reset_practice()
def controls(throttle=0,brake=0,steering=0,hand=0):
    for name,value in zip(actions,(throttle,brake,steering,hand)):
        sub.inject_input_vector_for_action(actions[name],unreal.Vector(value,0,0),[],[])
def curve(points):
    path=[]
    for i in range(121):
        t=i/120
        weights=((1-t)**3,3*t*(1-t)**2,3*t*t*(1-t),t**3)
        path.append(tuple(sum(w*p[k] for w,p in zip(weights,points)) for k in (0,1)))
    return path
paths=[curve([(0,-1300),(133,-891),(171,-340),(937,-340)]),
       curve([(937,-340),(465,-340),(10,340),(-485,340)])+curve([(-485,340),(-640,340),(-800,340),(-938,340)])[1:]]
run={'leg':0,'time':0.,'hold':0.,'peak':0.,'meters':0.,'last':p.get_actor_location(),'ends':[],'samples':[]}
def finish(error):
    unreal.unregister_slate_post_tick_callback(handle)
    controls()
    result={'pass':not error and lot.completed_bays==2,'error':error,'meters':round(run['meters'],2),
            'peak_kmh':round(run['peak'],2),'ends':run['ends'],'samples':run['samples']}
    Path(unreal.Paths.project_saved_dir(),'AgentReports','patio_recorrido.json').write_text(json.dumps(result,indent=2))
    pc.show_start_menu()
def tick(dt):
    try:
        run['time']+=dt
        leg=run['leg']
        pos=p.get_actor_location()
        yaw=p.get_actor_rotation().yaw
        speed=assist.get_speed_kmh()
        run['peak']=max(run['peak'],speed)
        run['meters']+=math.hypot(pos.x-run['last'].x,pos.y-run['last'].y)/100
        run['last']=pos
        if run['time']>60:finish('Tiempo agotado: '+assist.get_instruction());return
        if run['time']<1.1:
            controls(hand=int(.25<run['time']<.65))
            return
        if leg and not assist.reverse_mode:
            controls(brake=1)
            if speed<.2:assist.try_toggle_reverse()
            return
        path=paths[leg]
        end=path[-1]
        longitudinal=(end[0]-pos.x)*(-1 if leg else 1)
        distance=math.hypot(end[0]-pos.x,end[1]-pos.y)
        if len(run['samples'])<int(run['time']/2):
            run['samples'].append([round(pos.x),round(pos.y),round(yaw),round(speed,1),lot.completed_bays])
        if longitudinal<15 and distance<110:
            controls(brake=1,hand=int(speed<.45))
            if lot.completed_bays>leg:
                run['ends'].append([round(pos.x,1),round(pos.y,1),round(yaw,1)])
                if leg:finish('');return
                run['leg']=1
                controls(brake=1)
            return
        idx=min(range(len(path)),key=lambda i:math.hypot(path[i][0]-pos.x,path[i][1]-pos.y))
        lookahead=260 if leg else 170
        j=idx
        while j<len(path)-1 and math.hypot(path[j][0]-pos.x,path[j][1]-pos.y)<lookahead:j+=1
        tx,ty=path[j]
        if j==len(path)-1 and distance<lookahead:tx+=(lookahead-distance)*(-1 if leg else 1)
        error=math.radians((math.degrees(math.atan2(ty-pos.y,tx-pos.x))-yaw-(180 if leg else 0)+180)%360-180)
        curvature=2*math.sin(error)/max(90,math.hypot(tx-pos.x,ty-pos.y))
        steer=math.atan(278*curvature)/math.radians(40)*(-1 if leg else 1)
        if leg and pos.x < -485:
            # Near the stop line, track lateral position and align with the painted sides.
            target_yaw=max(-14,min(14,(pos.y-end[1])*.11)) if longitudinal>155 else 0
            steer=max(-1,min(1,(yaw-target_yaw)/14))
        desired=min(4.2 if leg else 7.0,max(1.6,distance*.019))
        controls(throttle=max(.08,min(.65,(desired-speed)*.35)),brake=int(speed>desired+1.1),steering=max(-1,min(1,steer)))
    except Exception as ex:finish(str(ex))
handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT='running'
