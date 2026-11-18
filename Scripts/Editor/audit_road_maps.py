"""Sweep the full rural loop or the northern extension at four-metre intervals in the saved map."""
import json
from pathlib import Path
import unreal
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world=editor.get_editor_world()
highway=world.get_name()=='L_Carretera'
if not highway and world.get_name()!='L_CiudadUrbana':raise RuntimeError('Open a road map outside PIE.')
routes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficRoute)
scenario=unreal.GameplayStatics.get_actor_of_class(world,unreal.RVUrbanScenario)
issues=[]
samples=0
for route in routes:
    spline=route.spline
    for d in range(0,int(spline.get_spline_length()),400):
        p=spline.get_location_at_distance_along_spline(d,unreal.SplineCoordinateSpace.WORLD)
        if not highway and p.y<23200:continue
        q=spline.get_location_at_distance_along_spline(d+400,unreal.SplineCoordinateSpace.WORLD)
        direction=(q-p).rotator()
        hit=unreal.SystemLibrary.line_trace_single(world,p+unreal.Vector(0,0,100),p-unreal.Vector(0,0,210),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[],unreal.DrawDebugTrace.NONE)
        obstacle=unreal.SystemLibrary.box_trace_single(world,p+unreal.Vector(0,0,22),q+unreal.Vector(0,0,22),
            unreal.Vector(223,100,58),direction,unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,[],unreal.DrawDebugTrace.NONE)
        if not hit or abs(p.z-hit.to_tuple()[4].z-70)>18 or obstacle or not scenario.is_inside_bounds(p):
            issues.append([route.get_actor_label(),d,hit.to_tuple()[9].get_actor_label() if hit else None,
                           obstacle.to_tuple()[9].get_actor_label() if obstacle else None,p.to_tuple()])
        samples+=1
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
signs={}
for a in actors:
    if not isinstance(a,unreal.StaticMeshActor) or not a.static_mesh_component.static_mesh:continue
    name=a.static_mesh_component.static_mesh.get_name()
    if name.startswith('SM_Via_'):
        signs[name]=signs.get(name,0)+1
    if not highway and name.lower().startswith(('sm_speedlimitsign','sm_stopsign')):
        issues.append(['obsolete_plate',name])
expected={'SM_Via_80':4,'SM_Via_CurvaDerecha':2,'SM_Via_CurvaIzquierda':2} if highway else {'SM_Via_Alto':7,'SM_Via_50Placa':53}
for name,count in expected.items():
    if signs.get(name)!=count:issues.append(['road_sign_count',name,count,signs.get(name)])
result={'signs':signs,'map':world.get_name(),'pass':not issues,'samples':samples,'issues':issues,'posted':scenario.posted_speed_kmh}
Path(unreal.Paths.project_saved_dir(),'AgentReports',world.get_name()+'_vialidad.json').write_text(json.dumps(result,indent=2))
RV_RESULT={**result,'issues':issues[:5]}
