"""Import original WAVs into the project's own folder; preserve unrelated content."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
lib=unreal.EditorAssetLibrary
folder='/Game/RutaVirtual/Audio'
names=['S_EngineLoop','S_RoadLoop','S_CityLoop','S_Horn','S_Impact','S_Reward']
tasks=[]
reimport=globals().get('RV_ARGS',{}).get('reimport',False)
for name in names:
    if lib.does_asset_exist(folder+'/'+name) and not reimport:continue
    task=unreal.AssetImportTask()
    task.filename=str(root/'SourceAudio'/(name+'.wav'));task.destination_path=folder
    task.destination_name=name;task.automated=True;task.save=True;tasks.append(task)
    task.replace_existing=reimport
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
    sound=lib.load_asset(folder+'/'+name)
    if not sound:raise RuntimeError('Audio import missing: '+name)
    sound.set_editor_property('looping',name.endswith('Loop'))
    if name.endswith('Loop'):sound.set_editor_property('virtualization_mode',unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    lib.save_loaded_asset(sound)
RV_RESULT={'audio_folder':folder,'sounds':names,'source':'Original synthesized WAV files in SourceAudio'}
