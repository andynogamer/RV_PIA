"""PIE smoke test through Enhanced Input actions, not production gameplay Python.

Start PIE first. Results are written asynchronously to Saved/AgentReports/driving_test.json.
Never run while a person is driving or while physical force output is enabled.
"""
import datetime
import json
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
controller = unreal.GameplayStatics.get_player_controller(world, 0) if world else None
if isinstance(controller, unreal.RVPracticePlayerController): controller.start_session()
car = unreal.GameplayStatics.get_player_pawn(world, 0) if world else None
if not car or not isinstance(car, unreal.WheeledVehiclePawn):
    raise RuntimeError('Start PIE with the training vehicle before running this test.')
subsystems = [s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem)
              if s.get_path_name().startswith('/Engine/Transient.')]
if len(subsystems) != 1:
    raise RuntimeError('Expected exactly one local Enhanced Input subsystem.')
subsystem = subsystems[0]
movement = car.vehicle_movement_component
assist = car.get_component_by_class(unreal.RVDrivingAssistComponent)
if not assist:
    raise RuntimeError('Training assist component is not installed.')
if assist.get_editor_property('urban_mode'):
    raise RuntimeError('Run this regression in L_CiudadPractica; use urban tests for L_CiudadUrbana.')
actions = {name: unreal.load_asset('/Game/VehicleTemplate/Input/Actions/IA_' + name)
           for name in ('Throttle', 'Brake', 'Steering', 'Handbrake')}
phases = [
    ('rest', 0.5, {}),
    ('accelerate', 2.0, {'Throttle': 0.4}),
    ('brake', 1.0, {'Brake': 1.0}),
    ('stop', 1.5, {'Handbrake': 1.0}),
    ('turn', 2.5, {'Throttle': 0.5, 'Steering': 0.5}),
    ('stop_after_turn', 1.5, {'Handbrake': 1.0}),
    ('reset_held', 1.0, {'Throttle': 0.5}),
    ('release_after_reset', 0.5, {}),
    ('reset_held_paused', 1.0, {'Throttle': 0.5}),
    ('release_after_paused_reset', 0.5, {}),
    ('forward_limit', 15.0, {'Throttle': 1.0}),
    ('stop_after_limit', 2.0, {'Handbrake': 1.0}),
    ('reverse', 6.0, {'Throttle': 1.0}),
    ('final_stop', 1.5, {'Handbrake': 1.0}),
]
state = {'phase': 0, 'start': unreal.GameplayStatics.get_time_seconds(world), 'samples': [],
         'enter': True, 'peaks': {}, 'gear_change_blocked': False, 'reverse_selected': False}
report_path = Path(unreal.Paths.project_saved_dir()) / 'AgentReports' / 'driving_test.json'


def sample(label):
    position = car.get_actor_location()
    return {'phase': label, 'speed_cm_s': movement.get_forward_speed(),
            'gear': movement.get_current_gear(), 'yaw_deg': car.get_actor_rotation().yaw,
            'position_cm': [position.x, position.y, position.z],
            'throttle': movement.get_throttle_input(), 'brake': movement.get_brake_input()}


def finish(error=None):
    movement.set_throttle_input(0.0)
    movement.set_brake_input(1.0)
    movement.set_handbrake_input(True)
    unreal.unregister_slate_post_tick_callback(handle)
    samples = state['samples']
    by_name = {s['phase']: s for s in samples}
    checks = {}
    if not error:
        checks = {
            'accelerates': by_name['accelerate']['speed_cm_s'] > 50,
            'brakes': abs(by_name['brake']['speed_cm_s']) < abs(by_name['accelerate']['speed_cm_s']),
            'stops': abs(by_name['stop']['speed_cm_s']) < 20,
            'steers': abs(by_name['turn']['yaw_deg'] - by_name['stop']['yaw_deg']) > 1,
            'reverses_from_rest': by_name['reverse']['speed_cm_s'] < -20 and by_name['reverse']['gear'] < 0,
            'ground_support': all(s['position_cm'][2] > -100 for s in samples),
            'brake_does_not_select_reverse': by_name['brake']['gear'] >= 0,
            'gear_change_blocked_in_motion': state['gear_change_blocked'],
            'explicit_reverse_accepted_at_rest': state['reverse_selected'],
            'forward_speed_limit': assist.forward_limit_kmh - 2 <= state['peaks'].get('forward_limit', 0) <= assist.forward_limit_kmh + 2,
            'reverse_speed_limit': assist.reverse_limit_kmh - 0.5 <= state['peaks'].get('reverse', 0) <= assist.reverse_limit_kmh + 2,
            'held_throttle_does_not_restart_vehicle': abs(by_name['reset_held']['speed_cm_s']) < 10,
            'paused_reset_requires_throttle_release': abs(by_name['reset_held_paused']['speed_cm_s']) < 10,
        }
        assist.toggle_practice_pause()
        checks['pause_clears_throttle'] = unreal.GameplayStatics.is_game_paused(world) and movement.get_throttle_input() == 0
        assist.toggle_practice_pause()
        assist.reset_practice()
        checks['reset_selects_forward'] = not assist.get_editor_property('reverse_mode') and movement.get_throttle_input() == 0
    report = {'generated': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'method': 'PIE / Enhanced Input action injection', 'samples': samples,
              'checks': checks, 'passed': bool(checks) and all(checks.values()), 'error': error,
              'peak_speed_kmh': state['peaks'],
              'limitations': 'Editor test only. No G920, visor, physical haptic or packaged executable validation.'}
    report_path.parent.mkdir(exist_ok=True, parents=True)
    report_path.write_text(json.dumps(report, indent=2), encoding='utf-8')


def tick(delta):
    try:
        index = state['phase']
        name, duration, values = phases[index]
        if state['enter']:
            if name == 'reverse':
                state['reverse_selected'] = assist.try_toggle_reverse()
            if name == 'reset_held':
                movement.set_throttle_input(0.5)
                assist.reset_practice()
            if name == 'reset_held_paused':
                assist.toggle_practice_pause()
                assist.reset_practice()
            state['enter'] = False
        for action_name, action in actions.items():
            subsystem.inject_input_vector_for_action(action, unreal.Vector(values.get(action_name, 0.0), 0, 0), [], [])
        if car.get_actor_location().z < -1000:
            raise RuntimeError('Vehicle fell below the road.')
        now = unreal.GameplayStatics.get_time_seconds(world)
        state['peaks'][name] = max(state['peaks'].get(name, 0), abs(movement.get_forward_speed()) * 0.036)
        if now - state['start'] >= duration:
            state['samples'].append(sample(name))
            if name == 'accelerate':
                state['gear_change_blocked'] = not assist.try_toggle_reverse()
            state['phase'] += 1
            state['start'] = now
            state['enter'] = True
            if state['phase'] >= len(phases):
                finish()
    except Exception as exc:
        finish(str(exc))


handle = unreal.register_slate_post_tick_callback(tick)
RV_RESULT = {'started': True, 'report': str(report_path), 'simulated_duration_s': sum(p[1] for p in phases)}
