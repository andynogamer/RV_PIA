"""Start PIE without modifying PlayerStart or the active level."""
import unreal
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if levels.is_in_play_in_editor(): raise RuntimeError('PIE is already active')
levels.editor_request_begin_play()
RV_RESULT={'requested':True}
