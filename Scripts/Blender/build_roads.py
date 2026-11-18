"""Build the Sierra road, northern avenue and Mexican road signs. Run in Blender 3.6.
Coordinates below use Unreal X forward, Y right and metres. Intermediate FBX files
are regenerated in Saved, the Unreal packages are the runtime deliverables.
"""
import json
import math
import random
from pathlib import Path

import bpy
import bmesh

out = Path(__file__).resolve().parents[2] / "Saved/RoadSource"
out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete()
bpy.context.scene.unit_settings.system = "METRIC"
tau = math.tau
sin, cos = math.sin, math.cos
randomizer = random.Random(920)
materials = {}


def material(name, color):
    m = bpy.data.materials.new("M_Via_" + name)
    m.use_nodes = True
    m.diffuse_color = (*color, 1)
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = m.diffuse_color
    bsdf.inputs["Roughness"].default_value = .82
    materials[name] = m


for key, color in {
    "Asfalto": (.065,.071,.073), "Blanco": (.9,.9,.83), "Amarillo": (.95,.57,.025),
    "Rojo": (.67,.008,.014), "Negro": (.012,.014,.016), "Acero": (.39,.43,.44),
    "Verde": (.014,.18,.091), "Azul": (.013,.12,.38), "Arcilla": (.26,.20,.118),
    "Pasto": (.185,.225,.113), "Matorral": (.12,.17,.085), "Roca": (.25,.235,.19),
    "Cumbre": (.15,.19,.158), "Hormigon": (.47,.45,.40), "Vidrio": (.105,.18,.20),
    "Chapa": (.30,.36,.36), "Ladrillo": (.38,.235,.15), "Arena": (.58,.50,.36)
}.items():
    material(key, color)


class Mesh:
    def __init__(self):
        self.verts, self.faces, self.slots = [], [], []

    def face(self, points, mat):
        first = len(self.verts)
        self.verts.extend((x,-y,z) for x,y,z in points)
        self.faces.append(list(reversed(range(first, len(self.verts)))))
        self.slots.append(mat)

    def box(self, x,y,z, w,d,h, mat):
        base = [(x-w/2,y-d/2,z),(x+w/2,y-d/2,z),(x+w/2,y+d/2,z),(x-w/2,y+d/2,z)]
        top = [(a,b,c+h) for a,b,c in base]
        self.face(top,mat)
        for i in range(4):
            j=(i+1)%4
            self.face([base[i],base[j],top[j],top[i]],mat)

    def save(self, name, lettering=()):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(self.verts, [], self.faces)
        mesh.update()
        keys = list(dict.fromkeys(self.slots))
        for key in keys:
            mesh.materials.append(materials[key])
        uv = mesh.uv_layers.new(name="Pavimento")
        for poly, key in zip(mesh.polygons, self.slots):
            poly.material_index = keys.index(key)
            normal = poly.normal
            for li in poly.loop_indices:
                x,y,z = mesh.vertices[mesh.loops[li].vertex_index].co
                uv.data[li].uv = (x/4,-y/4) if abs(normal.z)>.55 else ((x if abs(normal.y)>.5 else y)/4,z/4)
        if name == "SM_Sierra_Relieve":
            topology=bmesh.new()
            topology.from_mesh(mesh)
            bmesh.ops.remove_doubles(topology,verts=topology.verts,dist=.001)
            topology.to_mesh(mesh)
            topology.free()
            for face in mesh.polygons:face.use_smooth=True
        obj=bpy.data.objects.new(name,mesh)
        bpy.context.collection.objects.link(obj)
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        for label in lettering:
            label.select_set(True)
        bpy.ops.export_scene.fbx(filepath=str(out/(name+".fbx")), use_selection=True, object_types={"MESH"},
            axis_forward="X",axis_up="Z",apply_unit_scale=True,bake_space_transform=True,
            mesh_smooth_type="FACE",add_leaf_bones=False,bake_anim=False)
        for part in [obj,*lettering]:
            bpy.data.objects.remove(part,do_unlink=True)


def glyph(text, cx, z, width, height, mat, depth=-.022):
    font=bpy.data.curves.new("Inscripcion","FONT")
    font.body=text
    font.font=bold
    obj=bpy.data.objects.new("Leyenda",font)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    coords=[v.co.copy() for v in obj.data.vertices]
    x0,x1=min(v.x for v in coords),max(v.x for v in coords)
    y0,y1=min(v.y for v in coords),max(v.y for v in coords)
    # Local glyph +Z points toward approaching traffic (-X).
    for v,p in zip(obj.data.vertices,coords):
        v.co=(depth,-cx-width*((p.x-x0)/(x1-x0)-.5),z+height*((p.y-y0)/(y1-y0)-.5))
    obj.data.materials.append(materials[mat])
    return obj


bold=bpy.data.fonts.load("C:/Windows/Fonts/arialbd.ttf")
def sign(name, outline, layers, text, bottom=2.2, with_post=True):
    mesh=Mesh()
    center=bottom-min(z for y,z in outline) if with_post else 0
    if with_post:mesh.box(.045,0,0,.07,.065,center+.12,"Acero")
    def panel(points,depth,mat):
        # CCW in (right,up) faces +X. Reverse for the driver's face.
        signed_area=sum(y*points[(i+1)%len(points)][1]-z*points[(i+1)%len(points)][0] for i,(y,z) in enumerate(points))
        face=reversed(points) if signed_area>0 else points
        mesh.face([(depth,y,center+z) for y,z in face],mat)
    panel(outline,0,"Acero")
    mesh.face([(.025,y,center+z) for y,z in outline],"Acero")
    for i,(y,z) in enumerate(outline):
        yy,zz=outline[(i+1)%len(outline)]
        mesh.face([(0,y,center+z),(.025,y,center+z),(.025,yy,center+zz),(0,yy,center+zz)],"Acero")
    for i,(points,mat) in enumerate(layers):
        panel(points,-.002*(i+1),mat)
    labels=[glyph(t,0,center+dz,w,h,mat) for t,dz,w,h,mat in text]
    mesh.save("SM_Via_"+name,labels)


def scale(points,k):
    return [(x*k,z*k) for x,z in points]


def rounded(w,h,r=.035):
    points=[]
    for i,(x,z) in enumerate(((w/2-r,h/2-r),(-w/2+r,h/2-r),(-w/2+r,-h/2+r),(w/2-r,-h/2+r))):
        for k in range(5):
            angle=(i+k/4)*math.pi/2
            points.append((x+r*cos(angle),z+r*sin(angle)))
    return points


octagon=[(.43*cos((i+.5)*tau/8)/cos(tau/16),.43*sin((i+.5)*tau/8)/cos(tau/16)) for i in range(8)]
sign("Alto",octagon,[(octagon,"Rojo"),(scale(octagon,.967),"Blanco"),(scale(octagon,.916),"Rojo")],
     [("ALTO",0,.67,.231,"Blanco")])
for speed,size,h in [(50,.71,2.2),(80,.86,2)]:
    plaque=rounded(size,size)
    circle=[(.435*size*cos(i*tau/64),.435*size*sin(i*tau/64)) for i in range(64)]
    layers=[(plaque,"Blanco"),(scale(plaque,.96),"Negro"),(scale(plaque,.935),"Blanco"),
            (circle,"Rojo"),(scale(circle,.84),"Blanco")]
    legend=[(str(speed),.055*size,.49*size,.31*size,"Negro"),("km/h",-.18*size,.30*size,.104*size,"Negro")]
    sign(str(speed),plaque,layers,legend,h)
    if speed==50:sign("50Placa",plaque,layers,legend,with_post=False)
square=rounded(.75,.75)
sign("Estacionamiento",square,[(square,"Azul"),(scale(square,.94),"Blanco"),(scale(square,.91),"Azul")],[("E",0,.35,.47,"Blanco")])
guide=rounded(2.55,.76,.055)
sign("Distrito",guide,[(guide,"Verde"),(scale(guide,.975),"Blanco"),(scale(guide,.948),"Verde")],
     [("DISTRITO NORTE",.11,2.12,.19,"Blanco"),("Av. de los Pinos",-.16,1.54,.15,"Blanco")])
diamond=[(0,-.61),(.61,0),(0,.61),(-.61,0)]
for name,side in [("CurvaDerecha",1),("CurvaIzquierda",-1)]:
    arrow=[(-.16,-.33),(-.058,-.33),(-.052,-.12),(-.028,-.047),(.035,.017),(.102,.041),(.14,.041),(.14,-.048),(.32,.088),(.14,.249),(.14,.155),(.083,.155),(-.019,.125),(-.100,.054),(-.147,-.046),(-.16,-.14)]
    arrow=[(side*y,z) for y,z in arrow]
    if side==-1: arrow.reverse()
    sign(name,diamond,[(diamond,"Amarillo"),(scale(diamond,.948),"Negro"),(scale(diamond,.91),"Amarillo"),(arrow,"Negro")],[],2)

def frame(t):
    a=t*tau
    x=650*sin(a)+35*sin(2*a)
    y=295*(1-cos(a))+25*sin(a)**2*sin(2*a)
    z=7*sin(2*a)+9*(1-cos(a))
    dx=650*cos(a)+70*cos(2*a)
    dy=295*sin(a)+50*sin(a)*cos(a)*sin(2*a)+50*sin(a)**2*cos(2*a)
    length=math.hypot(dx,dy)
    return x,y,z,-dy/length,dx/length


def road(t,d=0,h=0):
    x,y,z,nx,ny=frame(t)
    return (x+d*nx,y+d*ny,z+h)


def strip(mesh,point,t0,t1,left,right,h,mat):
    mesh.face([point(t0,left,h),point(t1,left,h),point(t1,right,h),point(t0,right,h)],mat)


def bay(t):
    delta=abs((t+.5)%1-.5)
    return 4*max(0,min(1,(.017-delta)/.010))


def land(t,d):
    x,y,z=road(t,d)
    if d>10:
        ground=-.25+(d-10)*(.047+.035*sin(7*t*tau)) + min(2,(d-10)*.035)*sin(t*9*tau)
    else:
        r=max(0,-d-8)
        # Rolling foothills and two uneven ridgelines. Heights are continuous at the road edge.
        angle=t*tau
        near=(5+3*cos(7*angle))*(1-math.exp(-r/42))
        ridge=(65+31*sin(3*angle+.8)+18*cos(8*angle))*math.exp(-((r-440-85*sin(4*angle))/205)**2)
        peaks=(125+48*cos(5*angle)+20*sin(11*angle))*math.exp(-((r-990-105*sin(3*angle))/290)**2)
        ground=-.25+near+(ridge+peaks)*(1-math.exp(-r/65))
    return x,y,z+ground


pavement,paint,terrain,fittings=Mesh(),Mesh(),Mesh(),Mesh()
count=800
for i in range(count):
    t,u=i/count,(i+1)/count
    pavement.face([road(t,-5.1),road(u,-5.1),road(u,5.1+bay(u)),road(t,5.1+bay(t))],"Asfalto")
    for d in (-3.64,3.64,-.11,.11):
        strip(paint,road,t,u,d-.065,d+.065,.018,"Blanco" if abs(d)>1 else "Amarillo")
    for d in (-5.8,5.1+bay(t)):
        e=d+.7
        f,g=(d,e) if d<0 else (5.1+bay(u),5.8+bay(u))
        pavement.face([road(t,d,-.015),road(u,f,-.015),road(u,g,-.015),road(t,e,-.015)],"Arcilla")
    # The road and shoulders own their entire footprint. The first grass vertices
    # share the shoulder edges, including the variable-width turnout at the start.
    for side,distances in [(-1,[5.8,10,22,40,65,95,140,195,265,345,425,520,620,730,860,975,1100,1250,1500]),
                           (1,[5.8,12,36,70])]:
        def verge(v,d):
            if d==5.8:
                return road(v,side*d+(bay(v) if side>0 else 0),-.015)
            return land(v,side*d)
        for near,far in zip(distances,distances[1:]):
            l,r=(near,far) if side>0 else (far,near)
            terrain.face([verge(t,l),verge(u,l),verge(u,r),verge(t,r)],"Pasto")
    # Fill the inner valley with radial rings, avoiding overlapping offset curves.
    def interior(v,f):
        x,y,z=land(v,70)
        return x*(1-f),y*(1-f)+295*f,z*(1-f)+(29+13*sin(3*v*tau)*(1-f))*f
    for f,g in [(0,.38),(.38,.75),(.75,1)]:
        pts=[interior(t,f),interior(u,f),interior(u,g)]
        if g<1:pts.append(interior(t,g))
        terrain.face(pts,"Pasto")
    for side in [-1,1]:
        d=side*5.95
        if (side<0 and (.165<t<.293 or .635<t<.765)):
            for za,zb,offset in [(.48,.57,0),(.57,.66,.07),(.66,.75,0)]:
                v=[road(t,d,za),road(u,d,za),road(u,d+offset,zb),road(t,d+offset,zb)]
                fittings.face(v,"Acero"); fittings.face(v[::-1],"Acero")
            if i%2==0:
                x,y,z=road(t,d)
                fittings.box(x,y,z-.17,.11,.13,.94,"Acero")
        elif i%9==0 and bay(t)<.1:
            x,y,z=road(t,side*6.2)
            fittings.box(x,y,z-.23,.14,.14,1.04,"Blanco")
            fittings.box(x,y,z+.52,.15,.15,.16,"Negro")
            fittings.box(x,y,z+.57,.153,.153,.05,"Amarillo")
pavement.save("SM_Sierra_Calzada");paint.save("SM_Sierra_Rayas");terrain.save("SM_Sierra_Relieve");fittings.save("SM_Sierra_Defensas")

def pose(t,d,h=0):
    x,y,z=road(t,d,h)
    _,_,_,nx,ny=frame(t)
    return [x,y,z,math.degrees(math.atan2(-nx,ny))]

trees=[]
for j in range(730):
    t=randomizer.random()
    d=randomizer.choice([-1,1])*randomizer.uniform(13,65)
    x,y,z=land(t,d)
    trees.append([x,y,z,randomizer.uniform(0,360),randomizer.uniform(.78,1.65)])
route_count=240
highway={"lanes":[[pose(i/route_count,1.81,.70)[:3] for i in range(route_count)],
                  [pose(-i/route_count,-1.81,.70)[:3] for i in range(route_count)]],
         "trees":trees,"start":pose(.002,7.25,1.1),"goals":[pose(t,1.8)[:3] for t in [.018,.25,.5,.75]]+[pose(.998,7.25)[:3]],
         "signs":[],"length_m":sum(math.dist(road(i/count),road((i+1)/count)) for i in range(count))}
for t,d,name,increasing in [(.012,8.8,"80",True),(.51,6.35,"80",True),(.48,-6.35,"80",False),(.987,-6.35,"80",False),
                              (.145,6.35,"CurvaDerecha",True),(.65,6.35,"CurvaDerecha",True),
                              (.35,-6.35,"CurvaIzquierda",False),(.838,-6.35,"CurvaIzquierda",False),(.994,9.45,"Estacionamiento",True)]:
    p=pose(t,d)
    if not increasing:p[3]+=180
    highway["signs"].append([name,*p])

# Northern avenue starts at the city's northern edge, with a turning head and two industrial courts.
def avenue(t,d,h):
    y=t*520
    x=19*(1-cos(t*tau))
    dx=19*tau*sin(t*tau)/520
    k=(1+dx**2)**-.5
    return x-d*k,y+d*dx*k,h

street,marking,neighborhood=Mesh(),Mesh(),Mesh()
for i in range(130):
    t,u=i/130,(i+1)/130
    strip(street,avenue,t,u,-5.6,5.6,0,"Asfalto")
    strip(marking,avenue,t,u,-.07,.07,.015,"Amarillo")
    for side in [-1,1]:
        lo,hi=sorted((side*5.6,side*8))
        if u<.99:strip(street,avenue,t,u,lo,hi,.105,"Hormigon")
        near,far=sorted((side*8,side*94))
        strip(street,avenue,t,u,near,far,.028,"Pasto")
        if i%8==3:
            x,y,_=avenue(t,side*7.2,0)
            neighborhood.box(x,y,0,.15,.15,6.65,"Acero")
            neighborhood.box(x+side*.54,y,6.54,1.2,.13,.12,"Acero")
            neighborhood.box(x+side*1.12,y,6.46,.50,.24,.15,"Chapa")
# A single continuous apron sits below the imported city and both ends of the
# curved avenue. Long perpendicular offsets used to fold and expose the sky.
def outskirts(x,y):
    edge=max(0,abs(x)-560,abs(y-65)-730)
    rise=(1-math.exp(-edge/500))**2
    return x,y,-.12+rise*(42+23*sin(x/210+y/330)+18*cos(y/255-x/470))

for x in range(-2400,2400,60):
    for y in range(-2400,2400,60):
        street.face([outskirts(x,y),outskirts(x+60,y),outskirts(x+60,y+60),outskirts(x,y+60)],"Pasto")
for i in range(60):
    a,b=i*tau/60,(i+1)*tau/60
    for r1,r2,h,mat in [(0,15.4,.012,"Asfalto"),(15.4,17,.105,"Hormigon"),(17,30,-.14,"Pasto")]:
        p=(r1*cos(a),528+r1*sin(a),h)
        ring=[p,(r2*cos(a),528+r2*sin(a),h),(r2*cos(b),528+r2*sin(b),h)]
        if r1: ring.append((r1*cos(b),528+r1*sin(b),h))
        street.face(ring,mat)

urban_trees=[]
for side in (-1,1):
    for j in range(6):
        t=.105+.148*j+(side+1)*.012
        x,y,_=avenue(t,side*30,0)
        w=randomizer.uniform(15,24); d=randomizer.uniform(29,44); h=randomizer.uniform(5.5,10)
        neighborhood.box(x,y,-.13,w,d,h,"Ladrillo" if j%3==0 else "Arena")
        neighborhood.box(x,y,h-.16,w+.4,d+.4,.3,"Chapa")
        neighborhood.box(x,y,h+.12,w*.19,3.8,.95,"Chapa")
        front=x+side*(w/2+.025)
        for row in (-.32,-.16,0,.16,.32):
            neighborhood.box(front,y+d*row,h*.56,.11,d*.117,1.24,"Acero")
            neighborhood.box(front+side*.065,y+d*row,h*.56+.055,.062,d*.112,1.12,"Vidrio")
            neighborhood.box(front+side*.11,y+d*row,h*.56+.055,.045,.05,1.12,"Chapa")
        neighborhood.box(front+.03*side,y+d*.22,.01,.12,3.5,3.65,"Chapa")
        for level in range(1,12):
            neighborhood.box(front+side*.10,y+d*.22,level*.295,.042,3.42,.036,"Acero")
        for offset in [-1.98,1.98]:
            neighborhood.box(front+side*.64,y+d*.22+offset,0,.19,.19,.86,"Amarillo")
        street.box((front+avenue(t,side*5.62,0)[0])/2,y+d*.22,-.09,abs(front-avenue(t,side*5.62,0)[0]),6,.205,"Hormigon")
    for j in range(32):
        t=randomizer.uniform(.06,.99)
        x,y,z=avenue(t,side*randomizer.choice([10,58,65]),-.13)
        urban_trees.append([x,y,z,randomizer.uniform(0,360),randomizer.uniform(.85,1.2)])
street.save("SM_Norte_Avenida");marking.save("SM_Norte_Rayas");neighborhood.save("SM_Norte_Naves")

# Practice yard in metres. The painted edges and the evaluator use the same clear dimensions.
yard,lines=Mesh(),Mesh()
def quad(mesh,x0,y0,x1,y1,z,mat):
    mesh.face([(x0,y0,z),(x1,y0,z),(x1,y1,z),(x0,y1,z)],mat)

quad(yard,-460,-460,460,460,-.20,"Pasto")
quad(yard,-15.8,-22,15.8,22,0,"Asfalto")
for side in (-1,1):
    yard.box(14.45*side,0,-.03,2.7,44,.17,"Hormigon")
    yard.box(0,21.45*side,-.03,26.2,1.1,.17,"Hormigon")
    for j in range(-4,6):
        y=(j-.5)*3.4
        quad(lines,min(side*6.3,side*12.7),y-.05,max(side*6.3,side*12.7),y+.05,.012,"Blanco")
    quad(lines,side*12.65-.05,-15.3,side*12.65+.05,15.3,.012,"Blanco")
    for j in range(-4,5):
        y=j*3.4
        # Concrete wheel stops, with reflective ends on the face that meets the tyres.
        yard.box(side*12.15,y,0,.15,1.65,.10,"Hormigon")
        for dy in [-.6,.6]:
            yard.box(side*12.15,y+dy,.097,.16,.23,.011,"Amarillo")
    for y in [-16.7,0,16.7]:
        yard.box(side*14.8,y,.14,.14,.14,5.3,"Acero")
        yard.box(side*14.15,y,5.32,1.45,.10,.12,"Acero")
        yard.box(side*13.52,y,5.25,.55,.32,.10,"Hormigon")
# A teaching pavilion and glazing along the pedestrian walk.
yard.box(0,26,-.2,26,5.5,3.5,"Ladrillo")
yard.box(0,25.3,3.3,27,7.6,.36,"Chapa")
for x in range(-12,13,4):
    yard.box(x,22.7,0,.14,.14,3.35,"Acero")
    yard.box(x,23.21,.74,3.1,.10,1.76,"Vidrio")
    yard.box(x,23.13,.74,.085,.09,1.76,"Chapa")
for x in [-9,-4,4,9]:
    yard.box(x,-21.3,.14,2,.47,.40,"Hormigon")
# Numbered target bays use contrasting corner marks. The open mouth remains driveable.
targets=[[9.5,-3.4,0],[-9.5,3.4,180]]
for x,y,angle in targets:
    q=1 if x>0 else -1
    for side in [-1,1]:
        cx,cy=x-q*3.14,y+side*1.65
        quad(lines,cx-.065,cy-.08,cx+.065,cy+.08,.019,"Amarillo")
        quad(lines,cx-.06,min(cy,cy-side*.51),cx+.06,max(cy,cy-side*.51),.019,"Amarillo")
yard.save("SM_Patio_Escuela")
lines.save("SM_Patio_Marcas")
(out/"trazado.json").write_text(json.dumps({"highway":highway,"urban_trees":urban_trees,
    "parking":{"bays":targets,"half_size_cm":[310,165]}},separators=(",",":")))
