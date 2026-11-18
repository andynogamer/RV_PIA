"""Inspect the real shifter in PIE. Optional bridge arguments: expected_gear and watch."""
import datetime
import json
from pathlib import Path
import time

import unreal

args = globals().get("RV_ARGS", {})
expected = args.get("expected_gear")
watch = bool(args.get("watch", False))
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0) if world else None
assist = pawn.get_component_by_class(unreal.RVDrivingAssistComponent) if pawn else None
if not assist or (assist.practice_paused and not watch):
    raise RuntimeError("Abre Play, comienza la práctica y mantén el juego en primer plano.")
movement = pawn.vehicle_movement_component
reports = Path(unreal.Paths.project_saved_dir()) / "AgentReports"
reports.mkdir(parents=True, exist_ok=True)


def save(name, data):
    target = reports / name
    temp = target.with_suffix(".tmp")
    temp.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    temp.replace(target)


def sample():
    lever = assist.get_physical_shifter_label() or None
    raw_gate = "6" if lever and lever.startswith("6 ") else lever
    return {
        "time_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "map": world.get_name(),
        "hardware_reading": lever is not None,
        "waiting_for_report": assist.is_waiting_for_wheel_report(),
        "wheel_mode": assist.wheel_mode,
        "gear": assist.get_gear_label(),
        "physical_shifter": raw_gate,
        "requested_transmission": "R" if raw_gate == "6" else raw_gate,
        "chaos_gear": movement.get_current_gear(),
        "steering": movement.get_steering_input(),
        "throttle": movement.get_throttle_input(),
        "brake": movement.get_brake_input(),
        "parking_brake": movement.get_handbrake_input(),
        "speed_kmh": assist.get_speed_kmh(),
        "expected_gear": expected,
        "passed": bool(lever and assist.wheel_mode and (expected is None or raw_gate == str(expected))),
    }


if not watch:
    RV_RESULT = sample()
    save("g920_real_pie.json", RV_RESULT)
else:
    # Only one recorder owns the live report in an editor session.
    previous = getattr(unreal, "_rv_physical_shifter_watcher", None)
    if previous is not None:
        unreal.unregister_slate_post_tick_callback(previous)
    state = {"active": True, "map": world.get_name(), "positions": [], "events": []}
    last = None
    next_poll = 0.0

    def record(_delta):
        global last, next_poll
        if time.monotonic() < next_poll:
            return
        next_poll = time.monotonic() + 0.05
        if editor.get_game_world() != world or not unreal.SystemLibrary.is_valid(assist):
            state["active"] = False
            save("palanca_en_unreal.json", state)
            unreal.unregister_slate_post_tick_callback(handle)
            unreal._rv_physical_shifter_watcher = None
            return
        if assist.practice_paused:
            last = None
            return
        reading = sample()
        key = (reading["physical_shifter"], reading["gear"], reading["chaos_gear"])
        if key == last:
            return
        last = key
        gear = reading["physical_shifter"]
        if gear and gear not in state["positions"]:
            state["positions"].append(gear)
        state["events"].append(reading)
        save("palanca_en_unreal.json", state)

    save("palanca_en_unreal.json", state)
    handle = unreal.register_slate_post_tick_callback(record)
    unreal._rv_physical_shifter_watcher = handle
    RV_RESULT = {"watching": True, "report": "Saved/AgentReports/palanca_en_unreal.json"}
