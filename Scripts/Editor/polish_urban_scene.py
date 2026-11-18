"""Add parking markings to the new urban map only."""
import unreal

editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_game_world(): raise RuntimeError('Stop PIE before editing the scene.')
world=editor.get_editor_world()
if world.get_name()!='L_CiudadUrbana': raise RuntimeError('Open L_CiudadUrbana first.')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lib=unreal.EditorAssetLibrary
cube=lib.load_asset('/Engine/BasicShapes/Cube')
white=lib.load_asset('/Game/RutaVirtual/Materials/M_SenalBlanca')
labels={a.get_actor_label():a for a in actors.get_all_level_actors()}

def box(label,position,scale,material):
    label='Urbano_'+label
    a=labels.get(label) or actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*position))
    a.set_actor_label(label); a.set_folder_path('Urbano/Senalizacion')
    a.set_actor_location(unreal.Vector(*position),False,True)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_material(0,material)
    a.static_mesh_component.set_collision_profile_name('NoCollision')
    a.set_actor_scale3d(unreal.Vector(*scale))
    return a

for name,y in [('Inicio',-18200),('Destino',5750)]:
    x=-9850 if name=='Inicio' else -9650
    for side in [-1,1]:box('Bahia_'+name+'_Borde'+str(side),(x+side*150,y,3),(.12,6,.01),white)
    for end in [-1,1]:box('Bahia_'+name+'_Extremo'+str(end),(x,y+end*300,3),(3,.12,.01),white)

lib.save_directory('/Game/RutaVirtual/',only_if_is_dirty=True,recursive=True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
RV_RESULT={'map':world.get_path_name(),'parking_bays':2}
