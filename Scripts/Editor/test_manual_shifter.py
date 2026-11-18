"""Exercise the wheel sample path and real Chaos gears in L_CiudadPractica PIE.

Use an idle simulation with -RVNoWheelInput or with the physical wheel disconnected.
The samples are synthetic and deliberately bypass the DirectInput calibration.
"""
import datetime
import json
from pathlib import Path

import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
controller = unreal.GameplayStatics.get_player_controller(world, 0) if world else None
if not isinstance(controller, unreal.RVPracticePlayerController):
    raise RuntimeError("Start PIE in L_CiudadPractica before running this test.")
car = unreal.GameplayStatics.get_player_pawn(world, 0)
assist = car.get_component_by_class(unreal.RVDrivingAssistComponent)
if not assist or assist.urban_mode:
    raise RuntimeError("This test uses the short course with no parking handbrake.")
controller.start_session()
movement = car.vehicle_movement_component

# (label, duration, gear, accelerator, brake, clutch)
phases = [
    ("startup_in_gear", 0.3, 1, 0.8, 0, 0),
    ("safe_start_with_first_held", 0.35, 1, 0, 1, 1),
    ("couple_first_from_stop",0.35,1,0,1,0),
    ("arm", 0.3, 0, 0, 1, 0),
    ("neutral_rev", 0.5, 0, 0.5, 0, 0),
    ("without_clutch", 0.3, 1, 0.4, 0, 0),
    ("late_clutch_accepts_held_first", 0.3, 1, 0, 1, 1),
    ("late_clutch_release", 0.3, 1, 0, 1, 0),
]
for gear in range(1, 7):
    phases.extend([
        (f"neutral_{gear}", 0.15, 0, 0, 1, 1),
        (f"select_{gear}", 0.25, gear, 0, 1, 1),
        (f"couple_{gear}", 0.3, gear, 0, 1, 0),
    ])
phases.extend([
    ("press_before_gate", 0.25, 0, 0, 1, 1),
    ("release_entering_third", 0.35, 3, 0, 1, 0),
    ("neutral_launch", 0.2, 0, 0, 1, 1),
    ("select_launch", 0.3, 1, 0, 1, 1),
    ("clutch_down_no_drive", 0.4, 1, 0.65, 0, 1),
    ("drive_first", 1.4, 1, 0.65, 0, 0),
    ("reverse_while_moving", 0.25, 6, 0, 0, 1),
    ("stop_with_reverse_rejected", 1.5, 6, 0, 1, 1),
    ("rejected_reverse_cannot_drive", 0.5, 6, 0.65, 0, 0),
    ("neutral_reverse", 0.25, 0, 0, 1, 1),
    ("select_reverse", 0.3, -1, 0, 1, 1),
    ("drive_reverse", 1.0, -1, 0.65, 0, 0),
    ("forward_while_reversing", 0.25, 1, 0, 0, 1),
    ("connection_lost", 0.4, None, 0, 0, 0),
    ("reconnect_while_held", 0.5, -1, 0.6, 0, 0),
    ("safe_neutral", 0.3, 0, 0, 1, 0),
    ("paused_then_held", 0.35, 1, 0.6, 0, 0),
    ("reset_then_held", 0.35, 1, 0.6, 0, 0),
    ("finish", 0.3, 0, 0, 1, 0),
])
state = {"index": 0, "start": None}
samples = {}


def write_report(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    checks = {}
    if not error:
        def neutral(label):
            return samples[label]["selected"] == samples[label]["physical"] == 0

        def idle(label):
            sample = samples[label]
            return neutral(label) and sample["throttle"] == 0 and abs(sample["speed"]) < 10

        checks = {
            "unsafe_start_blocked": idle("startup_in_gear"),
            "stationary_start_with_clutch": samples["safe_start_with_first_held"]["selected"] == 1
            and samples["couple_first_from_stop"]["physical"] == 1,
            "clutch_seen_during_gate_transition": samples["release_entering_third"]["physical"] == 3,
            "neutral_rev_without_motion": neutral("neutral_rev") and samples["neutral_rev"]["throttle"] > 0.4
            and abs(samples["neutral_rev"]["speed"]) < 10,
            "clutch_required": idle("without_clutch"),
            "late_clutch_without_reselecting": samples["late_clutch_accepts_held_first"]["selected"] == 1
            and samples["late_clutch_release"]["physical"] == 1,
            "five_forward_gears": all(
                samples[f"select_{gear}"]["selected"] == gear and samples[f"select_{gear}"]["physical"] == 0
                and samples[f"couple_{gear}"]["physical"] == gear and samples[f"couple_{gear}"]["label"] == str(gear)
                for gear in range(1, 6)
            ),
            "sixth_gate_selects_reverse": samples["select_6"]["selected"] == -1
            and samples["couple_6"]["physical"] == -1 and samples["couple_6"]["label"] == "R",
            "full_clutch_decouples": samples["clutch_down_no_drive"]["physical"] == 0
            and abs(samples["clutch_down_no_drive"]["speed"]) < 10,
            "first_drives_forward": samples["drive_first"]["speed"] > 25,
            "reverse_rejected_in_motion": neutral("reverse_while_moving") and idle("rejected_reverse_cannot_drive"),
            "reverse_drives_backward": samples["drive_reverse"]["speed"] < -20 and samples["drive_reverse"]["label"] == "R",
            "forward_rejected_while_reversing": neutral("forward_while_reversing"),
            "device_loss_clears_input": neutral("connection_lost") and samples["connection_lost"]["throttle"] == 0
            and samples["connection_lost"]["brake"] == 1,
            "reconnect_requires_neutral": idle("reconnect_while_held"),
            "pause_requires_neutral": idle("paused_then_held"),
            "reset_requires_neutral": idle("reset_then_held"),
        }
    path = Path(unreal.Paths.project_saved_dir()) / "AgentReports" / "manual_shifter_test.json"
    path.parent.mkdir(exist_ok=True, parents=True)
    path.write_text(json.dumps({
        "generated": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "passed": bool(checks) and all(checks.values()), "checks": checks, "samples": samples,
        "error": error, "method": "PIE, synthetic wheel samples and real Chaos transmission",
    }, indent=2), encoding="utf-8")


def tick(delta):
    try:
        label, duration, gear, throttle, brake, clutch = phases[state["index"]]
        now = unreal.GameplayStatics.get_time_seconds(world)
        if state["start"] is None:
            state["start"] = now
            if label == "paused_then_held":
                assist.toggle_practice_pause()
                controller.start_session()
            elif label == "reset_then_held":
                assist.reset_practice()
        if gear is not None:
            assist.submit_wheel_sample(0, throttle, brake, clutch, gear)
        if now - state["start"] < duration:
            return
        samples[label] = {
            "selected": assist.selected_gear, "physical": movement.get_current_gear(),
            "label": assist.get_gear_label(), "speed": movement.get_forward_speed(),
            "throttle": movement.get_throttle_input(), "brake": movement.get_brake_input(),
        }
        state["start"] = None
        state["index"] += 1
        if state["index"] == len(phases):
            write_report()
    except Exception as exc:
        write_report(str(exc))


handle = unreal.register_slate_post_tick_callback(tick)
RV_RESULT = {"started": True, "report": "Saved/AgentReports/manual_shifter_test.json"}
