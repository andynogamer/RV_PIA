"""Add physical sign boards and parking markings to the new urban map only."""
import unreal

editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_game_world(): raise RuntimeError('Stop PIE before editing the scene.')
world=editor.get_editor_world()
if world.get_name()!='L_CiudadUrbana': raise RuntimeError('Open L_CiudadUrbana first.')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lib=unreal.EditorAssetLibrary
cube=lib.load_asset('/Engine/BasicShapes/Cube')
dark=lib.load_asset('/Game/RutaVirtual/Materials/M_BaseCono')
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

for name,location,width,height,text_size in [
    ('Inicio',(-10150,-17800,245),245,88,18),
    ('Limite',(-10100,-14500,245),105,100,24),
    ('Estacionamiento',(-9950,6900,245),270,88,18),
    ('AltoLateral',(-10080,-17500,225),100,65,28),
]:
    actor=labels['Urbano_Senal_'+name]
    actor.set_actor_location(unreal.Vector(*location),False,True)
    text=actor.get_component_by_class(unreal.TextRenderComponent)
    text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    text.set_vertical_alignment(unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
    text.set_world_size(text_size)
    x,y,z=location
    box('Tablero_'+name,(x,y+6,z),(width/100,.08,height/100),dark)
    box('Poste_'+name,(x,y+10,(z-height/2)/2),(.07,.07,(z-height/2)/100),dark)

for name,y in [('Inicio',-18200),('Destino',6500)]:
    x=-9850 if name=='Inicio' else -9650
    for side in [-1,1]:box('Bahia_'+name+'_Borde'+str(side),(x+side*150,y,3),(.12,6,.01),white)
    for end in [-1,1]:box('Bahia_'+name+'_Extremo'+str(end),(x,y+end*300,3),(3,.12,.01),white)

lib.save_directory('/Game/RutaVirtual/',only_if_is_dirty=True,recursive=True)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
RV_RESULT={'map':world.get_path_name(),'boards':4,'parking_bays':2}
