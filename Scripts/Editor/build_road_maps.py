"""Place the road kit in the urban map or the separate rural highway. Run after import_roads.py."""
import json
import math
from pathlib import Path
import unreal

plan=json.loads(Path(unreal.Paths.project_saved_dir(),'RoadSource','trazado.json').read_text())
editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
lib=unreal.EditorAssetLibrary
folder='/Game/RutaVirtual/Environment/Vialidad/'
city_mesh='/Game/Ciudad/Downtown_YongeDundas/StaticMeshes/'
maps='/Game/RutaVirtual/Maps/'
is_highway=globals().get('RV_ARGS',{}).get('highway',False)
is_parking=globals().get('RV_ARGS',{}).get('parking',False)
map_name='L_Estacionamiento' if is_parking else 'L_Carretera' if is_highway else 'L_CiudadUrbana'
if editor.get_game_world():raise RuntimeError('Stop PIE before editing the map.')
world=editor.get_editor_world()
if world.get_name()!=map_name:
    if unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages() or unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages():
        raise RuntimeError('Save current edits before changing map')
    path=maps+map_name
    ok=levels.load_level(path) if lib.does_asset_exist(path) else (is_highway or is_parking) and levels.new_level(path)
    if not ok:raise RuntimeError('Could not open '+path)
    world=editor.get_editor_world()
existing={a.get_actor_label():a for a in actors.get_all_level_actors()}


def place(cls,label,point=(0,0,0),yaw=0,category='Vialidad'):
    a=existing.get(label) or actors.spawn_actor_from_class(cls,unreal.Vector(*point),unreal.Rotator(yaw=yaw))
    if not isinstance(a,cls):raise RuntimeError('Unexpected actor '+label)
    a.set_actor_label(label)
    a.set_folder_path(category)
    a.set_actor_location(unreal.Vector(*point),False,True)
    a.set_actor_rotation(unreal.Rotator(yaw=yaw),False)
    existing[label]=a
    return a


def prop(asset,label,point,yaw=0,block=False):
    # The legacy FBX X-front conversion maps authored +Y to mesh +X.
    a=place(unreal.StaticMeshActor,label,point,yaw+90,'Vialidad/Mobiliario')
    c=a.static_mesh_component
    c.set_static_mesh(lib.load_asset(folder+asset))
    c.set_collision_profile_name('BlockAll' if block else 'NoCollision')
    return a


def vegetation(trees,origin):
    actor=place(unreal.RVRoadsidePines,'Vialidad_Arbolado',origin,category='Vialidad/Paisaje')
    # Both original meshes have baked world coordinates. Rotate about the shared trunk base.
    transforms=[]
    for x,y,z,yaw,size in trees:
        c,s=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
        tx,ty=-2695*size,14353*size
        p=unreal.Vector(x*100-(c*tx-s*ty),y*100-(s*tx+c*ty),z*100-41*size)
        transforms.append(unreal.Transform(location=p,rotation=unreal.Rotator(yaw=yaw),scale=unreal.Vector(size,size,size)))
    for comp,mesh in [(actor.get_editor_property('trunks'),'SM_Pine'),(actor.get_editor_property('crowns'),'SM_PineLeaves')]:
        comp.set_static_mesh(lib.load_asset(city_mesh+mesh))
        comp.clear_instances()
        comp.add_instances(transforms,False,False)


def lane(route,points,closed=False):
    spline=route.spline
    spline.clear_spline_points(False)
    for i,p in enumerate(points):
        spline.add_spline_point(unreal.Vector(*p),unreal.SplineCoordinateSpace.WORLD,False)
        spline.set_spline_point_type(i,unreal.SplinePointType.LINEAR,False)
    spline.set_closed_loop(closed,False)
    spline.update_spline()


def cm(p):
    return tuple(v*100 for v in p[:3])


def panorama(label):
    sky=place(unreal.StaticMeshActor,label,(0,29500,0))
    sky.static_mesh_component.set_static_mesh(lib.load_asset('/Engine/BasicShapes/Sphere'))
    sky.static_mesh_component.set_material(0,lib.load_asset(folder+'M_CieloSierra'))
    sky.static_mesh_component.set_collision_profile_name('NoCollision')
    sky.static_mesh_component.set_cast_shadow(False)
    sky.set_actor_scale3d(unreal.Vector(9000,9000,9000))


if is_parking:
    world.get_world_settings().set_editor_property('default_game_mode',lib.load_blueprint_class('/Game/RutaVirtual/Blueprints/BP_ModoPractica'))
    yard=plan['parking']
    prop('SM_Patio_Escuela','Patio_Escuela',(0,0,0),block=True)
    prop('SM_Patio_Marcas','Patio_Lineas',(0,0,0))
    panorama('Patio_Panorama')
    sun=place(unreal.DirectionalLight,'Patio_Sol',(0,0,3000))
    sun.set_actor_rotation(unreal.Rotator(pitch=-48,yaw=-65),False)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity',3.2)
    fill=place(unreal.SkyLight,'Patio_Cielo',(0,0,3000))
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property('real_time_capture',True)
    place(unreal.SkyAtmosphere,'Patio_Atmosfera')
    place(unreal.PlayerStart,'Patio_Salida',(0,-1300,110),72)
    scenario=place(unreal.RVParkingScenario,'Patio_Maniobras')
    scenario.set_editor_property('bays',[unreal.Transform(location=unreal.Vector(x*100,y*100,0),rotation=unreal.Rotator(yaw=yaw))
        for x,y,yaw in yard['bays']])
    scenario.set_editor_property('bay_half_size',unreal.Vector2D(*yard['half_size_cm']))
    body=lib.load_asset('/Game/Vehicles/SportsCar/SM_SportsCar')
    bounds=body.get_bounding_box()
    scenario.set_editor_property('body_min',unreal.Vector2D(bounds.min.x,bounds.min.y))
    scenario.set_editor_property('body_max',unreal.Vector2D(bounds.max.x,bounds.max.y))
    scenario.bounds_min=unreal.Vector(-1600,-2200,-80)
    scenario.bounds_max=unreal.Vector(1600,2200,1200)
    for i,(x,y,yaw) in enumerate(yard['bays']):
        side=1 if x>0 else -1
        prop('SM_Via_Estacionamiento','Patio_E_'+str(i),(side*1370,y*100+116,14),yaw)
        # Fixed stall labels are visible from the aisle even when the target changes.
        sign=place(unreal.TextRenderActor,'Patio_Placa_'+str(i),(side*1360,y*100+116,185),yaw+180)
        lettering=sign.text_render
        lettering.set_text('A1' if i==0 else 'B2')
        lettering.set_world_size(46)
        lettering.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    collision_path=folder+'SM_Patio_Coche'
    fixed_body=lib.load_asset(collision_path) if lib.does_asset_exist(collision_path) else lib.duplicate_asset(body.get_path_name().split('.')[0],collision_path)
    meshes=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    if meshes.get_simple_collision_count(fixed_body)==0:
        if not meshes.set_convex_decomposition_collisions(fixed_body,4,16,90000):
            raise RuntimeError('No se pudo generar la colisión del coche estacionado')
        lib.save_loaded_asset(fixed_body)
    paints=[]
    palette=[('Grafito',(.11,.12,.14)),('Azul',(.025,.12,.27)),('Plata',(.49,.52,.54))]
    for name,rgb in palette:
        path=folder+'MI_Patio_'+name
        paint=lib.load_asset(path) if lib.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'MI_Patio_'+name,folder.rstrip('/'),unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
        unreal.MaterialEditingLibrary.set_material_instance_parent(paint,body.get_material(1))
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(paint,'Paint Tint',unreal.LinearColor(*rgb,1))
        lib.save_loaded_asset(paint)
        paints.append(paint)
    car_folder='/Game/Vehicles/SportsCar/'
    tire=lib.load_asset(car_folder+'SM_SportsCar_Wheel')
    tire_bounds=tire.get_bounds()
    hub_z=tire_bounds.box_extent.z-tire_bounds.origin.z
    lateral_center=bounds.max.y-tire_bounds.box_extent.y-2
    for i,(x,y,yaw) in enumerate([(950,-1020,0),(950,0,180),(950,1020,0),(-950,-680,180),(-950,680,0)]):
        side=1 if yaw==0 else -1
        parts=[('Carroceria','SM_SportsCar',0,0,hub_z-24,0),('Vidrio','SM_SportsCar_Glass',0,0,hub_z-24,0)]
        for j in range(4):
            lateral=1 if j%2 else -1
            parts.append(('Rueda'+str(j),'SM_SportsCar_Wheel',142 if j<2 else -136,
                          lateral*(lateral_center-tire_bounds.origin.y),hub_z,0 if lateral==1 else 180))
        for label,asset,dx,dy,z,turn in parts:
            parked=place(unreal.StaticMeshActor,f'Patio_Coche{i}_{label}',(x+side*dx,y+side*dy,z),yaw+turn,'Patio/Vehículos')
            comp=parked.static_mesh_component
            comp.set_static_mesh(fixed_body if label=='Carroceria' else lib.load_asset(car_folder+asset))
            if label=='Carroceria':comp.set_material(1,paints[i%len(paints)])
            comp.set_collision_profile_name('BlockAll' if label=='Carroceria' else 'NoCollision')
    trees=[(x,y,-.26,(i*39)%360,1.15+(i%4)*.15) for i,(x,y) in enumerate(
        [(x,y) for x in [-23,22] for y in range(-32,37,7)]+[(x,34) for x in range(-15,16,6)]+[(x,-32) for x in range(-13,17,6)])]
    vegetation(trees,(0,0,0))
    editor.set_level_viewport_camera_info(unreal.Vector(-2750,-3100,2350),unreal.Rotator(pitch=-31,yaw=49))
elif is_highway:
    h=plan['highway']
    world.get_world_settings().set_editor_property('default_game_mode',lib.load_blueprint_class('/Game/RutaVirtual/Blueprints/BP_ModoPractica'))
    for key in ['Calzada','Rayas','Relieve','Defensas']:
        prop('SM_Sierra_'+key,'Sierra_'+key,(0,0,0),block=key!='Rayas')
    sun=place(unreal.DirectionalLight,'Sierra_Sol',(0,0,15000))
    sun.set_actor_rotation(unreal.Rotator(pitch=-36,yaw=-120),False)
    sun.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity',4)
    place(unreal.SkyAtmosphere,'Sierra_Atmosfera')
    if 'Sierra_Cielo' in existing:actors.destroy_actor(existing['Sierra_Cielo'])
    panorama('Sierra_Panorama')
    fill=place(unreal.SkyLight,'Sierra_LuzCielo',(0,0,3000))
    fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    fill.light_component.set_editor_property('intensity',.85)
    fill.light_component.set_editor_property('real_time_capture',True)
    fog=place(unreal.ExponentialHeightFog,'Sierra_Bruma',(0,0,-600))
    fog.component.set_editor_property('fog_density',.009)
    fog.component.set_editor_property('start_distance',12000)
    start=h['start']
    place(unreal.PlayerStart,'Sierra_Apartadero',cm(start),start[3])
    scenario=place(unreal.RVUrbanScenario,'Sierra_Escenario')
    scenario.posted_speed_kmh=80
    scenario.bounds_min=unreal.Vector(-86000,-19000,-3500)
    scenario.bounds_max=unreal.Vector(86000,79000,50000)
    scenario.checkpoints=[unreal.Vector(*cm(p)) for p in h['goals']]
    for i,points in enumerate(h['lanes']):
        route=place(unreal.RVTrafficRoute,'Sierra_Carril_'+str(i),category='Trafico/Carriles')
        route.vehicle_count=5
        route.cruise_kmh=72 if i==0 else 70
        lane(route,[cm(p) for p in points],True)
    for i,(kind,x,y,z,yaw) in enumerate(h['signs']):
        prop('SM_Via_'+kind,'Sierra_Senal_'+str(i)+'_'+kind,cm([x,y,z]),yaw)
    vegetation(h['trees'],(0,0,0))
    editor.set_level_viewport_camera_info(unreal.Vector(-2400,420,480),unreal.Rotator(pitch=-6,yaw=2))
else:
    for key in ['Inicio','Limite','Estacionamiento','AltoLateral']:
        for prefix in ('Urbano_Senal_','Urbano_Tablero_','Urbano_Poste_'):
            old=existing.get(prefix+key)
            if old:actors.destroy_actor(old)
    if 'Cielo_Esfera' in existing:actors.destroy_actor(existing['Cielo_Esfera'])
    panorama('Urbano_Panorama')
    for label in ('Vialidad_Senal_Avenida','Vialidad_Senal_Regreso'):
        if label in existing:actors.destroy_actor(existing[label])
    for label,a in list(existing.items()):
        if not label.lower().startswith('sm_speedlimitsign'):continue
        mesh=a.static_mesh_component.static_mesh
        index=next(i for i,slot in enumerate(mesh.static_materials) if 'm_maximum' in str(slot.material_slot_name).lower())
        normals=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,index)[2]
        yaw=math.degrees(math.atan2(-sum(v.y for v in normals),-sum(v.x for v in normals)))
        center,extent=a.get_actor_bounds(False)
        prop('SM_Via_50Placa','Vialidad_Limite_'+label,center.to_tuple(),yaw)
        actors.destroy_actor(a)
    # Replace only placed Canadian STOP fixtures. Face direction is read from the original sign artwork.
    typ=unreal.RVStopRule
    for label,a in list(existing.items()):
        if not label.lower().startswith('sm_stopsign') or not isinstance(a,unreal.StaticMeshActor):continue
        mesh=a.static_mesh_component.static_mesh
        index=next(i for i,slot in enumerate(mesh.static_materials) if 'canada_stop_sign' in str(slot.material_slot_name).lower())
        normals=unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh,0,index)[2]
        yaw=math.degrees(math.atan2(-sum(v.y for v in normals),-sum(v.x for v in normals)))
        o,e=a.get_actor_bounds(False)
        # A sign is on the right shoulder. Set its stop line across the adjacent approach lane.
        prop('SM_Via_Alto','Vialidad_Alto_'+label,(o.x,o.y,o.z-e.z),yaw)
        right=(-math.sin(math.radians(yaw)),math.cos(math.radians(yaw)))
        rule=place(typ,'Regla_Alto_'+label,(o.x-right[0]*240,o.y-right[1]*240,0),yaw,'Urbano/Reglas')
        rule.half_width_cm=160
        actors.destroy_actor(a)
    for kind,label,p,yaw in [('Alto','Salida',(-10100,-17550,0),90),
        ('Estacionamiento','Destino',(-10010,6120,0),90),
        ('Distrito','AccesoNorte',(-9720,23850,0),90),('50','DistritoNorte',(-9580,27650,2),90)]:
        prop('SM_Via_'+kind,'Vialidad_Senal_'+label,p,yaw)
    # The final M_Asphalt vertices of SM_Roads span X=-9494..-8215 at Y=25615..25624.
    origin=(-8855,25596,.5)
    for key in ['Avenida','Rayas','Naves']:
        prop('SM_Norte_'+key,'DistritoNorte_'+key,origin,block=key!='Rayas')
    vegetation(plan['urban_trees'],origin)
    scenario=existing['Urbano_Escenario']
    # Leave a full car length plus the bay clear of the Pinos intersection's stop line.
    bay_y=5750
    checkpoints=list(scenario.checkpoints)
    checkpoints[-1]=unreal.Vector(-9650,bay_y,0)
    scenario.checkpoints=checkpoints
    for label,y in [('Urbano_BahiaDestino',bay_y),('Urbano_Bahia_Destino_Borde-1',bay_y),
                    ('Urbano_Bahia_Destino_Borde1',bay_y),('Urbano_Bahia_Destino_Extremo-1',bay_y-300),
                    ('Urbano_Bahia_Destino_Extremo1',bay_y+300)]:
        if label in existing:
            a=existing[label]
            loc=a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x,y,loc.z),False,True)
    scenario.posted_speed_kmh=50
    scenario.bounds_max=unreal.Vector(38500,82000,20000)
    # Extend both existing routes from the original avenue and turn traffic around beyond sight.
    for name,side in [('Norte',1),('Sur',-1)]:
        route=existing['Trafico_Carril_'+name]
        spline=route.spline
        original=[spline.get_location_at_spline_point(i,unreal.SplineCoordinateSpace.WORLD).to_tuple()
                  for i in range(spline.get_number_of_spline_points())]
        original=[p for p in original if p[1]<25500]
        extra=[]
        for i in range(32):
            t=i/32
            dx=19*math.tau*math.sin(t*math.tau)/520
            k=(1+dx**2)**-.5
            extra.append((origin[0]+1900*(1-math.cos(t*math.tau))-side*245*k,
                          origin[1]+52000*t+side*245*dx*k,70))
        lane(route,original+extra if side==1 else extra[::-1]+original)
    for name,speed in [('Norte',38),('Sur',37),('Este',32),('Oeste',32)]:
        existing['Trafico_Carril_'+name].cruise_kmh=speed
    editor.set_level_viewport_camera_info(unreal.Vector(-9890,-18800,165),unreal.Rotator(pitch=4,yaw=90))
if not levels.save_current_level():raise RuntimeError('Map save failed')
RV_RESULT={'map':world.get_name(),'trees':len(trees if is_parking else plan['highway']['trees'] if is_highway else plan['urban_trees']),
           'highway_length_m':round(plan['highway']['length_m'],1),'posted_kmh':scenario.posted_speed_kmh}
