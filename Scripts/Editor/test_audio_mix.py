"""Record the real game master submix, including horn, mute and pause.

Writes WAVs to Saved/AgentReports for signal inspection; never run during a user session.
"""
import json
from pathlib import Path
import unreal
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc=unreal.GameplayStatics.get_player_controller(world,0) if world else None
if not pc:raise RuntimeError('Start PIE first.')
pc.start_session()
player=unreal.GameplayStatics.get_player_pawn(world,0)
assist=player.get_component_by_class(unreal.RVDrivingAssistComponent)
audio=player.get_component_by_class(unreal.RVVehicleAudioComponent)
if not audio:raise RuntimeError('Vehicle audio component missing.')
original_mute=audio.get_editor_property('muted')
if original_mute:audio.toggle_mute()
assist.reset_practice()
folder=Path(unreal.Paths.project_saved_dir()).resolve()/'AgentReports'
state={'phase':0,'elapsed':0.0,'enter':True,'horn':False,'checks':{},'recording':False}
phases=[('warm',.8),('audible',2.0),('mute_warm',.4),('muted',1.0),('pause_warm',.4),('paused',1.0),('flush',.5)]

def record(name):
    unreal.AudioMixerLibrary.start_recording_output(world,3.0)
    state['recording']=True;state['record_name']=name

def stop():
    unreal.AudioMixerLibrary.stop_recording_output(world,unreal.AudioRecordingExportType.WAV_FILE,'audio_'+state['record_name'],str(folder))
    state['recording']=False

def finish(error=None):
    if state['recording']:stop()
    unreal.unregister_slate_post_tick_callback(handle)
    if audio.get_editor_property('muted')!=original_mute:audio.toggle_mute()
    pc.show_start_menu()
    (folder/'audio_mix_test.json').write_text(json.dumps({'passed':error is None and all(state['checks'].values()),'error':error,
        'checks':state['checks'],'wavs':['audio_audible.wav','audio_muted.wav','audio_paused.wav'],
        'limits':'Game master submix capture; speaker/headphone perception requires human listening.'},indent=2),encoding='utf-8')

def tick(delta):
    try:
        name,duration=phases[state['phase']]
        if state['enter']:
            if name=='audible':record(name);state['horn_before']=audio.horn_sounds_played
            if name=='mute_warm':audio.toggle_mute()
            if name=='muted':record(name)
            if name=='pause_warm':audio.toggle_mute();pc.show_pause_menu()
            if name=='paused':record(name)
            state['enter']=False
        state['elapsed']+=min(delta,.05)
        if name=='audible' and state['elapsed']>.5 and not state['horn']:
            assist.request_horn();state['horn']=True
        if state['elapsed']<duration:return
        if name in ['audible','muted','paused']:stop()
        if name=='audible':
            state['checks']['engine_loop_active']=audio.engine_audio.is_playing()
            state['checks']['city_loop_active']=audio.city_audio.is_playing()
            state['checks']['manual_horn_rendered']=audio.horn_sounds_played>state['horn_before']
        if name=='muted':state['checks']['mute_state']=audio.get_editor_property('muted')
        if name=='paused':state['checks']['pause_state']=unreal.GameplayStatics.is_game_paused(world)
        state['phase']+=1;state['elapsed']=0;state['enter']=True
        if state['phase']==len(phases):finish()
    except Exception as error:finish(str(error))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={'started':True,'report':str(folder/'audio_mix_test.json')}
