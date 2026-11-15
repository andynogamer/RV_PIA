"""Drive original city lenses from native signals, leaving source meshes untouched."""
import unreal
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_game_world():raise RuntimeError('Stop PIE first.')
world=editor.get_editor_world()
if world.get_name()!='L_CiudadUrbana':raise RuntimeError('Open the urban map.')
actors=list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
signals=[a for a in actors if isinstance(a,unreal.RVTrafficSignal)]
if len(signals)!=2:raise RuntimeError('Expected two controlled crossings.')
# The north fixtures are at the actual crossing near Y=-4850, rather than mid-block.
north=next(s for s in signals if s.get_actor_label()=='Semaforo_Cruce_Norte')
north.set_actor_location(unreal.Vector(-9090,-4850,0),False,True)
line=next(a for a in actors if a.get_actor_label()=='Urbano_LineaRojoNorte')
line.set_actor_location(unreal.Vector(-9390,-5770,3),False,True)
bindings={s:[] for s in signals}
for actor in actors:
    if not isinstance(actor,unreal.StaticMeshActor):continue
    component=actor.static_mesh_component;mesh=component.static_mesh
    if not mesh:continue
    name=mesh.get_name().lower()
    color=next((c for prefix,c in [('sm_red_light_',unreal.RVSignalColor.RED),('sm_yellow_light_',unreal.RVSignalColor.AMBER),
        ('sm_green_light_',unreal.RVSignalColor.GREEN)] if name.startswith(prefix)),None)
    if color is None:continue
    origin,extent=actor.get_actor_bounds(False)
    signal=min(signals,key=lambda s:(s.get_actor_location().x-origin.x)**2+(s.get_actor_location().y-origin.y)**2)
    p=signal.get_actor_location()
    if (p.x-origin.x)**2+(p.y-origin.y)**2>2200**2:continue
    b=unreal.RVSignalLampBinding()
    b.set_editor_property('component',component)
    b.set_editor_property('x_axis',extent.x<extent.y)
    b.set_editor_property('color',color)
    bindings[signal].append(b)
if any(len(lamps)<12 for lamps in bindings.values()):raise RuntimeError('Not enough imported lenses; inspect before saving.')
for signal,lamps in bindings.items():
    signal.set_editor_property('city_lamps',lamps)
    for component in signal.get_components_by_class(unreal.StaticMeshComponent):component.set_visibility(False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
RV_RESULT={'bound_lenses':{s.get_actor_label():len(lamps) for s,lamps in bindings.items()}}
