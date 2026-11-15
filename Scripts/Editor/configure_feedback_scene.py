"""Add one explicit stop-rule marker at the training bay exit, in the urban map only."""
import unreal
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
if editor.get_game_world():raise RuntimeError('Stop PIE before editing markers.')
world=editor.get_editor_world()
if world.get_name()!='L_CiudadUrbana':raise RuntimeError('Open the urban map first.')
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing={a.get_actor_label():a for a in actors.get_all_level_actors()}
rule=existing.get('Feedback_ALTO_Salida') or actors.spawn_actor_from_class(unreal.RVStopRule,unreal.Vector(-9850,-17550,0),unreal.Rotator(pitch=0,yaw=90,roll=0))
rule.set_actor_rotation(unreal.Rotator(pitch=0,yaw=90,roll=0),False)
rule.set_actor_label('Feedback_ALTO_Salida');rule.set_folder_path('Urbano/Reglas')
rule.set_editor_property('half_width_cm',200);rule.set_editor_property('approach_cm',350)
line=existing.get('Urbano_LineaALTO_Salida') or actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(-9850,-17550,3))
line.set_actor_label('Urbano_LineaALTO_Salida');line.set_folder_path('Urbano/Senalizacion')
line.static_mesh_component.set_static_mesh(unreal.EditorAssetLibrary.load_asset('/Engine/BasicShapes/Cube'))
line.static_mesh_component.set_material(0,unreal.EditorAssetLibrary.load_asset('/Game/RutaVirtual/Materials/M_SenalBlanca'))
line.static_mesh_component.set_collision_profile_name('NoCollision');line.set_actor_scale3d(unreal.Vector(3,.16,.01))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
RV_RESULT={'stop_rule':rule.get_actor_label(),'position_cm':str(rule.get_actor_location()),'forward':str(rule.get_actor_forward_vector()),'required_stopped_s':rule.stop_seconds}
