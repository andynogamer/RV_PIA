"""PIE regression for the three live player mirrors and steering while parked."""
import configparser
import datetime
import re
import json
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0) if world else None
if not pc or "RVNoWheelInput" not in unreal.SystemLibrary.get_command_line():
    raise RuntimeError("Usa Play con -RVNoWheelInput y la práctica desocupada.")
pc.start_session()
pawn = unreal.GameplayStatics.get_player_pawn(world,0)
assist = pawn.get_component_by_class(unreal.RVDrivingAssistComponent)
cabin = pawn.get_component_by_class(unreal.RVCabinComponent)
movement = pawn.vehicle_movement_component
movement.set_handbrake_input(True)
components = {c.get_name():c for c in pawn.get_components_by_class(unreal.SceneComponent)}
wheel = components["VolanteVisual"]
initial_up = wheel.get_up_vector()
right = pawn.get_actor_right_vector()
labels = ["Izquierdo","Central","Derecho"]
widths = [19.5,25,19.5]
checks, samples, markers = {}, {}, []
phase, elapsed, index = "wheel_right", 0., 0
base_rotation = None
applied_adjustment = None
settings = configparser.ConfigParser(interpolation=None, strict=False)
settings.read(Path(unreal.Paths.project_saved_dir())/"Config/WindowsEditor/GameUserSettings.ini",encoding="utf-8-sig")
offsets = []
for label in labels:
    pair = settings.get("RutaVirtual.Retrovisores",label,fallback="X=0 Y=0")
    coords = [float(x) for x in re.findall(r"[XY]=([-+.0-9]+)",pair)]
    if len(coords)!=2: raise ValueError("Ángulos ilegibles en "+label)
    # Move toward the centre so a saved angle at its limit is still fully reversible.
    offsets.append((-5 if coords[0]>0 else 5,-3 if coords[1]>0 else 3))
target = None
original_preview = components["FrontCamera"].get_editor_property("relative_rotation")
report = Path(unreal.Paths.project_saved_dir())/"AgentReports"/"driver_cabin_test.json"
cones = [a for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.StaticMeshActor)
         if a.get_actor_label() in ("Practica_Cono_0_I", "Practica_Cono_0_D")]
if len(cones) != 2:
    raise RuntimeError("Ejecuta la prueba en L_CiudadPractica.")
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
lamp = unreal.load_asset("/Game/RutaVirtual/Materials/M_TrafficLamp")


def remove_markers():
    while markers:
        actor, transform, mesh, materials, collision, mobility = markers.pop()
        part = actor.static_mesh_component
        part.set_static_mesh(mesh)
        part.set_editor_property("override_materials",materials)
        actor.set_actor_transform(transform,False,True)
        part.set_collision_enabled(collision)
        part.set_mobility(mobility)


def finish(error=None):
    unreal.unregister_slate_post_tick_callback(handle)
    remove_markers()
    if applied_adjustment is not None:
        cabin.adjust_mirror(-applied_adjustment[0],-applied_adjustment[1])
    cabin.finish_adjustment()
    pc.show_pause_menu()
    report.write_text(json.dumps({"utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "passed":not error and bool(checks) and all(checks.values()),"checks":checks,"samples":samples,"error":error},
        indent=2),encoding="utf-8")


def setup_targets():
    name = labels[index]
    capture,mirror = components["Reflejo"+name],components["Espejo"+name]
    virtual_eye = capture.get_world_location()
    mid = mirror.get_world_location()+capture.get_forward_vector()*.1
    scale = 4.
    for sign,color in [(-1,unreal.LinearColor(5,.003,.003,1)),(1,unreal.LinearColor(.003,.003,5,1))]:
        ray = mid+capture.get_right_vector()*(widths[index]*.25*sign)-virtual_eye
        transform = unreal.Transform(virtual_eye+ray*scale,unreal.Rotator(),unreal.Vector(.19,.19,.19))
        actor = cones[0 if sign<0 else 1]
        part = actor.static_mesh_component
        markers.append((actor,actor.get_actor_transform(),part.static_mesh,list(part.get_editor_property("override_materials")),
                        part.get_collision_enabled(),part.mobility))
        part.set_mobility(unreal.ComponentMobility.MOVABLE)
        part.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        part.set_static_mesh(cube)
        material = part.create_dynamic_material_instance(0,lamp)
        material.set_vector_parameter_value("LampColor",color)
        actor.set_actor_transform(transform,False,True)


def tick(delta):
    global phase,elapsed,index,base_rotation,applied_adjustment,target
    try:
        elapsed += delta
        steer = .1 if phase=="wheel_right" else -.1 if phase=="wheel_left" else 0.
        assist.submit_wheel_sample(steer,0,1,0,0)
        if elapsed < .55:
            return
        elapsed = 0.
        if phase in ("wheel_right","wheel_left"):
            displacement=(wheel.get_up_vector()-initial_up).dot(right)
            samples[phase]=displacement
            checks[phase]=displacement>.4 if phase=="wheel_right" else displacement<-.4
            if phase=="wheel_right": phase="wheel_left"
            else:
                cabin.cycle_mirror()
                phase="markers"
        elif phase=="markers":
            base_rotation=components["Espejo"+labels[index]].get_forward_vector()
            setup_targets()
            phase="read"
        elif phase=="read":
            c=components["Reflejo"+labels[index]]
            target=c.texture_target
            glass=components["Espejo"+labels[index]]
            slot=glass.get_material_index("M_RV_Cristal")
            checks[labels[index]+"_material"]=slot>=0 and glass.get_material(slot).get_texture_parameter_value("Reflejo")==target
            red,blue=[unreal.RenderingLibrary.read_render_target_raw_uv(world,target,u,.5,False) for u in (.25,.75)]
            samples[labels[index]]=[[red.r,red.g,red.b],[blue.r,blue.g,blue.b]]
            checks[labels[index]+"_rear_image"]=red.r>max(red.g,red.b)*1.7 and blue.b>max(blue.r,blue.g)*1.7
            applied_adjustment=offsets[index]
            cabin.adjust_mirror(*applied_adjustment)
            phase="adjust"
        elif phase=="adjust":
            changed=components["Espejo"+labels[index]].get_forward_vector()
            checks[labels[index]+"_adjustment"]=(changed-base_rotation).length()>.04
            cabin.adjust_mirror(-applied_adjustment[0],-applied_adjustment[1])
            applied_adjustment=None
            remove_markers()
            cabin.cycle_mirror()
            index+=1
            if index<3:
                phase="markers"
            else:
                restored=components["FrontCamera"].get_editor_property("relative_rotation")
                checks["camera_restored"]=max(abs(getattr(original_preview,k)-getattr(restored,k)) for k in ("pitch","yaw","roll"))<.2
                checks["parked"]=assist.get_speed_kmh()<.5 and movement.get_handbrake_input()
                for screen in ("CuadroInstrumentos", "PantallaConsola"):
                    display=components[screen].get_material(0).get_texture_parameter_value("Reflejo")
                    checks[screen+"_display"]=isinstance(display,unreal.CanvasRenderTarget2D) and display.size_x>=640
                checks["shell_from_seat"]=components["CarroceriaCabina"].is_visible() and not components["Chasis_Main"].is_visible()
                components["FrontCamera"].deactivate()
                phase="exterior"
        elif phase=="exterior":
            checks["exterior_body_restored"]=components["Chasis_Main"].is_visible() and not components["CarroceriaCabina"].is_visible()
            components["FrontCamera"].activate()
            phase="interior"
        elif phase=="interior":
            checks["interior_body_restored"]=components["CarroceriaCabina"].is_visible() and not components["Chasis_Main"].is_visible()
            finish()
    except Exception as exc:
        finish(str(exc))

handle=unreal.register_slate_post_tick_callback(tick)
RV_RESULT={"started":True,"report":str(report)}
