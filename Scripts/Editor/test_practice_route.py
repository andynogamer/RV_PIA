"""Drive the marked exercise through its real checkpoints in PIE using action injection."""
import json
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
car = unreal.GameplayStatics.get_player_pawn(world, 0) if world else None
if not car:
    raise RuntimeError('Start the city level in PIE first.')
assist = car.get_component_by_class(unreal.RVDrivingAssistComponent)
subsystems = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
              if s.get_path_name().startswith('/Engine/Transient.')]
if not assist or len(subsystems) != 1:
    raise RuntimeError('Expected one player and the training component.')
movement = car.vehicle_movement_component
subsystem = subsystems[0]
actions = {name: unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_' + name)
           for name in ['Throttle', 'Brake', 'Steering', 'Handbrake']}
assist.reset_practice()
state = {'start': unreal.GameplayStatics.get_time_seconds(world), 'stages': [], 'peak_kmh': 0.0}
result = Path(unreal.Paths.project_saved_dir()) / 'AgentReports' / 'practice_route_test.json'


def finish(passed, error=None):
    movement.set_throttle_input(0.0)
    movement.set_brake_input(1.0)
    unreal.unregister_slate_post_tick_callback(handle)
    result.write_text(json.dumps({'passed': passed, 'error': error,
        'method': 'PIE / real physics and Enhanced Input action injection',
        'stages': state['stages'], 'peak_speed_kmh': state['peak_kmh'],
        'final_speed_kmh': assist.get_speed_kmh(), 'final_instruction': assist.get_instruction(),
        'elapsed_simulated_s': unreal.GameplayStatics.get_time_seconds(world) - state['start'],
        'limits': 'Desktop editor only; no physical peripherals tested.'}, indent=2), encoding='utf-8')


def tick(delta):
    try:
        elapsed = unreal.GameplayStatics.get_time_seconds(world) - state['start']
        stage = assist.get_editor_property('practice_stage')
        if not state['stages'] or state['stages'][-1] != stage:
            state['stages'].append(stage)
        speed_m_s = abs(movement.get_forward_speed()) * 0.01
        state['peak_kmh'] = max(state['peak_kmh'], speed_m_s * 3.6)
        if stage == 2:
            finish(state['stages'] == [0, 1, 2] and assist.get_speed_kmh() < 0.5)
            return
        if elapsed > 45:
            finish(False, 'Exercise did not finish within 45 simulated seconds.')
            return
        values = {'Throttle': 0.7 if elapsed > 1 and speed_m_s < 3.0 else 0.0}
        if stage == 1 and assist.get_distance_to_goal_meters() < max(2.5, speed_m_s * speed_m_s / 5.0 + 1.2):
            values = {'Brake': 1.0}
        for name, action in actions.items():
            subsystem.inject_input_vector_for_action(action, unreal.Vector(values.get(name, 0), 0, 0), [], [])
    except Exception as error:
        finish(False, str(error))


handle = unreal.register_slate_post_tick_callback(tick)
RV_RESULT = {'started': True, 'report': str(result)}
