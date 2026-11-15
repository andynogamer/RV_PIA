"""Sample ground and sweep traffic-sized volumes along every configured lane."""
import json
from pathlib import Path
import unreal

editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world=editor.get_game_world() or editor.get_editor_world()
routes=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficRoute)
if len(routes)!=4: raise RuntimeError('Expected the four urban lanes.')
ignore=list(unreal.GameplayStatics.get_all_actors_of_class(world,unreal.RVTrafficVehicle))
player=unreal.GameplayStatics.get_player_pawn(world,0)
if player: ignore.append(player)
missing=[];obstructions=[];samples=0
for route in routes:
    spline=route.spline
    for distance in range(0,int(spline.get_spline_length()),1000):
        samples+=1
        point=spline.get_location_at_distance_along_spline(distance,unreal.SplineCoordinateSpace.WORLD)
        end=spline.get_location_at_distance_along_spline(min(distance+1000,spline.get_spline_length()),unreal.SplineCoordinateSpace.WORLD)
        direction=spline.get_rotation_at_distance_along_spline(distance,unreal.SplineCoordinateSpace.WORLD)
        ground=unreal.SystemLibrary.line_trace_single(world,unreal.Vector(point.x,point.y,200),unreal.Vector(point.x,point.y,-200),
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,ignore,unreal.DrawDebugTrace.NONE)
        if not ground:missing.append({'lane':route.get_actor_label(),'distance_cm':distance})
        hit=unreal.SystemLibrary.box_trace_single(world,point,end,unreal.Vector(238,115,65),direction,
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,False,ignore,unreal.DrawDebugTrace.NONE)
        if hit:
            values=hit.to_tuple();actor=values[9]
            obstructions.append({'lane':route.get_actor_label(),'distance_cm':distance,
                'actor':actor.get_actor_label() if actor else None})
RV_RESULT={'passed':not missing and not obstructions,'ground_samples':samples,
    'missing_ground':missing,'static_obstructions':obstructions,
    'limits':'Lane sampling and swept collision volume; not a full-city or suspension clearance certification.'}
Path(unreal.Paths.project_saved_dir(),'AgentReports','urban_road_audit.json').write_text(json.dumps(RV_RESULT,indent=2),encoding='utf-8')
