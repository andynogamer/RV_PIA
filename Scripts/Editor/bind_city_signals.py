"""Bind every imported city signal head to the traffic simulation, using its real lens normals."""
import math
import unreal

editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_editor_world()
if not world or world.get_name() != 'L_CiudadUrbana':
    raise RuntimeError('Open the urban map outside PIE.')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
placed = {a.get_actor_label():a for a in actors.get_all_level_actors()}
# Crossing centres in cm, from the surveyed fixtures. The west arm has a single head.
junctions = [('Sur',-9300,-24480),('Norte',-9090,-4850),('Pinos',-9000,7560),
             ('Mercado',1180,11050),('EsteSur',20700,-24510),('EsteCentro',20700,-5270),
             ('EsteNorte',20700,14480),('Poniente',-31755,7560)]
colors = {'red':unreal.RVSignalColor.RED,'yellow':unreal.RVSignalColor.AMBER,'green':unreal.RVSignalColor.GREEN}
heads = {}
for actor in placed.values():
    if not isinstance(actor,unreal.StaticMeshActor) or not actor.static_mesh_component.static_mesh:
        continue
    words = actor.static_mesh_component.static_mesh.get_name().lower().split('_')
    if len(words) not in (3,4) or words[0]!='sm' or words[2]!='light' or words[1] not in colors:
        continue
    heads.setdefault(tuple(words[3:]),{})[words[1]] = actor

bindings={key:[] for key,_,_ in junctions}
masks={key:0 for key in bindings}
for group in heads.values():
    if set(group)!=set(colors): raise ValueError('Incomplete traffic head')
    red=group['red']
    pos,ext=red.get_actor_bounds(False)
    p_amber=group['yellow'].get_actor_bounds(False)[0]
    p_green=group['green'].get_actor_bounds(False)[0]
    if not (pos.z>p_amber.z>p_green.z and math.hypot(pos.x-p_green.x,pos.y-p_green.y)<10):
        raise ValueError(f'Misplaced lenses at {pos}')
    key,cx,cy=min(junctions,key=lambda row:math.hypot(row[1]-pos.x,row[2]-pos.y))
    if math.hypot(cx-pos.x,cy-pos.y)>2100: raise ValueError(f'Unknown crossing {pos}')
    # The lens is a convex cap. Its mean normal points toward approaching traffic.
    mesh=red.static_mesh_component.static_mesh
    normals=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,0)[2]
    dx=-sum(v.x for v in normals);dy=-sum(v.y for v in normals)
    x_axis=abs(dx)>abs(dy)
    if max(abs(dx),abs(dy))<len(normals)*.12 or x_axis!=(ext.x<ext.y):
        raise ValueError(f'Unresolved head orientation {mesh.get_name()}')
    bit=(0 if dx>0 else 1) if x_axis else (2 if dy>0 else 3)
    masks[key] |= 1<<bit
    for color,actor in group.items():
        binding=unreal.RVSignalLampBinding()
        binding.component=actor.static_mesh_component
        binding.x_axis=x_axis
        binding.color=colors[color]
        bindings[key].append(binding)

for index,(key,x,y) in enumerate(junctions):
    if not bindings[key]: raise ValueError(key)
    label='Semaforo_Cruce_'+key
    signal=placed.get(label) or actors.spawn_actor_from_class(unreal.RVTrafficSignal,unreal.Vector(x,y,0))
    signal.set_actor_label(label)
    signal.set_folder_path('Trafico/Semaforos')
    signal.set_actor_location(unreal.Vector(x,y,0),False,True)
    signal.set_editor_property('cycle_offset',(8*index)%34)
    signal.set_editor_property('approach_mask',masks[key])
    signal.set_editor_property('city_lamps',bindings[key])
    for comp in signal.get_components_by_class(unreal.StaticMeshComponent):comp.set_visibility(False)
if not unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level():raise RuntimeError('Save failed')
RV_RESULT={'heads':len(heads),'lenses':sum(map(len,bindings.values())),'approach_masks':masks}
