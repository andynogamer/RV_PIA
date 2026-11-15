"""Check menu transitions and safe input through real PIE frames.

Starts from the initial menu. Does not quit PIE; quit is tested separately by UI.
Never run during a human driving session or with physical force output active.
"""
import json
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if not pc or not isinstance(pc,unreal.RVPracticePlayerController):raise RuntimeError('Start practice PIE first.')
player=unreal.GameplayStatics.get_player_pawn(world,0)
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
movement=player.vehicle_movement_component
subsystem=next(s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_path_name().startswith('/Engine/Transient.'))
throttle=unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_Throttle')
agents=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
state={'phase':0,'elapsed':0.0,'enter':True,'checks':{},'traffic_start':[a.distance_along_route for a in agents]}
phases=[('start',.4),('controls',.2),('drive',.4),('pause',.4),('resume',.4),('restart',.4),('home',.4),('start_again',.3)]
output=Path(unreal.Paths.project_saved_dir())/'AgentReports'/'session_menu_test.json'

def check(name,value):state['checks'][name]=bool(value)
def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    pc.show_start_menu()
    output.write_text(json.dumps({'passed':error is None and all(state['checks'].values()),'error':error,'checks':state['checks'],
        'limits':'Runtime PIE transitions/input. Mouse capture and visual layout also require UI inspection. No VR/hardware validation.'},indent=2),encoding='utf-8')

def tick(delta):
    try:
        name,duration=phases[state['phase']]
        if state['enter']:
            if name=='controls':pc.session_menu.toggle_controls()
            if name=='drive':
                pc.session_menu.toggle_controls();pc.start_session()
            if name=='pause':
                movement.set_throttle_input(.7);pc.show_pause_menu()
                state['pause_traffic']=[a.distance_along_route for a in agents]
            if name=='resume':pc.start_session()
            if name=='restart':pc.show_pause_menu();pc.restart_session()
            if name=='home':pc.show_pause_menu();pc.show_start_menu()
            if name=='start_again':pc.start_session()
            state['enter']=False
        if name in ['start','pause','resume']:
            subsystem.inject_input_vector_for_action(throttle,unreal.Vector(1,0,0),[],[])
        state['elapsed']+=min(delta,.05)
        if state['elapsed']<duration:return
        if name=='start':
            check('opens_initial_menu',pc.session_screen==unreal.RVSessionScreen.START and pc.session_menu.is_in_viewport())
            check('start_cursor_visible',pc.get_editor_property('show_mouse_cursor'))
            primary=next(b for b in unreal.ObjectIterator(unreal.Button) if b.get_name()=='PrimaryButton' and b.get_outer().get_outer()==pc.session_menu)
            check('start_button_has_keyboard_focus',primary.has_keyboard_focus())
            x,y=pc.get_mouse_position();width,height=pc.get_viewport_size()
            check('cursor_starts_inside_viewport',10<x<width-10 and 10<y<height-10)
            check('start_world_paused',unreal.GameplayStatics.is_game_paused(world))
            check('start_blocks_throttle',movement.get_throttle_input()==0 and assist.get_speed_kmh()<.5)
            check('start_freezes_traffic',all(abs(a.distance_along_route-d)<.01 for a,d in zip(agents,state['traffic_start'])))
        if name=='controls':
            check('controls_page_works',pc.session_menu.is_showing_controls())
            check('controls_keep_world_paused',unreal.GameplayStatics.is_game_paused(world))
        if name=='drive':
            check('start_enters_driving',pc.session_screen==unreal.RVSessionScreen.DRIVING and not unreal.GameplayStatics.is_game_paused(world))
            check('drive_hides_cursor',not pc.get_editor_property('show_mouse_cursor'))
            check('drive_removes_menu',pc.session_menu is None)
        if name=='pause':
            check('pause_menu_visible',pc.session_screen==unreal.RVSessionScreen.PAUSED and pc.session_menu.is_in_viewport())
            check('pause_releases_cursor',pc.get_editor_property('show_mouse_cursor'))
            check('pause_clears_throttle',movement.get_throttle_input()==0)
            check('pause_freezes_traffic',all(abs(a.distance_along_route-d)<.01 for a,d in zip(agents,state['pause_traffic'])))
        if name=='resume':check('held_throttle_cannot_resume_motion',movement.get_throttle_input()==0 and assist.get_speed_kmh()<.5)
        if name=='restart':
            check('restart_selects_forward',not assist.get_editor_property('reverse_mode'))
            check('restart_restores_stage',assist.get_editor_property('practice_stage')==0)
            check('restart_returns_to_driving',pc.session_screen==unreal.RVSessionScreen.DRIVING)
        if name=='home':
            check('return_home_pauses',pc.session_screen==unreal.RVSessionScreen.START and unreal.GameplayStatics.is_game_paused(world))
            check('return_home_clears_progress',assist.get_editor_property('urban_checkpoint_index')==0 and assist.get_editor_property('practice_stage')==0)
        if name=='start_again':check('can_start_repeatedly',pc.session_screen==unreal.RVSessionScreen.DRIVING and not unreal.GameplayStatics.is_game_paused(world))
        state['phase']+=1;state['elapsed']=0;state['enter']=True
        if state['phase']==len(phases):finish()
    except Exception as error:finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(output)}
