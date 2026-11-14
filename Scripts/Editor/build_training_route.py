"""Add a short, clearly marked closed practice corridor to the city level."""
import unreal

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lib = unreal.EditorAssetLibrary
if any(a.get_actor_label().startswith('Practica_') for a in actors.get_all_level_actors()):
    raise RuntimeError('Training markers already exist; edit them instead of duplicating them.')
lib.make_directory('/Game/RutaVirtual/Materials')


def material(name, color):
    path = '/Game/RutaVirtual/Materials/' + name
    if lib.does_asset_exist(path):
        return lib.load_asset(path)
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, '/Game/RutaVirtual/Materials', unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
    node.set_editor_property('constant', unreal.LinearColor(*color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat


orange = material('M_ConosNaranja', (0.9, 0.14, 0.012))
black = material('M_BaseCono', (0.014, 0.017, 0.02))
green = material('M_ZonaDetencion', (0.025, 0.35, 0.21))
white = material('M_SenalBlanca', (0.8, 0.8, 0.8))
cone = lib.load_asset('/Engine/BasicShapes/Cone')
cube = lib.load_asset('/Engine/BasicShapes/Cube')


def mesh_actor(label, mesh, location, scale, mat, block=False):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label('Practica_' + label)
    actor.set_folder_path('Practica')
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_material(0, mat)
    comp.set_collision_profile_name('BlockAll' if block else 'NoCollision')
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


for index, y in enumerate(range(-16900, -12199, 450)):
    for side, x in [('I', -9300), ('D', -9800)]:
        mesh_actor(f'Cono_{index}_{side}', cone, (x, y, 28), (0.32, 0.32, 0.5), orange, True)
        mesh_actor(f'Base_{index}_{side}', cube, (x, y, 3), (0.36, 0.36, 0.04), black)
mesh_actor('ZonaFinal', cube, (-9550, -12700, 2), (4.0, 3.5, 0.012), green)
mesh_actor('LineaFinal', cube, (-9550, -12525, 3), (4.0, 0.15, 0.012), white)
mesh_actor('CartelFinal', cube, (-9150, -12350, 160), (2.5, 0.10, 1.5), white)
mesh_actor('PosteFinal', cube, (-9150, -12350, 75), (0.08, 0.08, 1.5), black)
text_actor = actors.spawn_actor_from_class(unreal.TextRenderActor, unreal.Vector(-9150, -12357, 185),
    unreal.Rotator(pitch=0, yaw=-90, roll=0))
text_actor.set_actor_label('Practica_TextoFinal')
text_actor.set_folder_path('Practica')
text = text_actor.get_component_by_class(unreal.TextRenderComponent)
text.set_text(unreal.Text('ALTO\nFIN DE PRACTICA'))
text.set_world_size(24)
text.set_text_render_color(unreal.Color(15, 20, 25, 255))
text.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
RV_RESULT = {'route_length_m': 40, 'start_cm': [-9550, -16700, 120],
             'stop_zone_cm': [-9550, -12700, 2], 'cones': 22}
