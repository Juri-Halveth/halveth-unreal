# SPDX-License-Identifier: MIT
"""Blender batch: CC0 human topology/rig, own clothing, hair and idle animation."""
from pathlib import Path
import bpy,json,math,sys,shutil,hashlib
from mathutils import Vector,Quaternion,kdtree
R=Path(__file__).resolve().parents[1]
SRC=R/'ArtSource/Characters/Sources'
DEST=R/'ArtSource/Characters';DEST.mkdir(parents=True,exist_ok=True)
P=[];UV=[];F=[];group=''
for line in (SRC/'base.obj').read_text().splitlines():
 a=line.split()
 if not a:continue
 if a[0]=='v':P.append(Vector(tuple(map(float,a[1:4]))))
 elif a[0]=='vt':UV.append(tuple(map(float,a[1:3])))
 elif a[0]=='g':group=' '.join(a[1:])
 elif a[0]=='f' and group=='body':F.append([tuple(int(q)-1 for q in s.split('/')[:2]) for s in a[1:]])
used=sorted({v for face in F for v,t in face});MAP={v:i for i,v in enumerate(used)}
BASE_P=[v.copy() for v in P]
FACE_TARGETS={
 'Scarlet':{'head/head-age-incr.target':.25,'chin/chin-prominent-incr.target':.22,'nose/nose-point-up.target':.1,'cheek/l-cheek-bones-incr.target':.35,'cheek/r-cheek-bones-incr.target':.35},
 'Lucinet':{'macrodetails/caucasian-male-young.target':.7,'head/head-age-incr.target':.65,'chin/chin-width-incr.target':.75,'chin/chin-prominent-incr.target':.4,'nose/nose-curve-convex.target':.35,'nose/nose-scale-depth-incr.target':.35,'cheek/l-cheek-volume-decr.target':.4,'cheek/r-cheek-volume-decr.target':.4},
 'Rachel':{'head/head-age-incr.target':.4,'chin/chin-width-incr.target':.2,'nose/nose-point-up.target':.3,'cheek/l-cheek-bones-incr.target':.15,'cheek/r-cheek-bones-incr.target':.15}}
bottom=min(P[v].y for v in used);top=max(P[v].y for v in used);SCALE=180/(top-bottom)
def convert(p):return Vector((p.x*SCALE,-p.z*SCALE,(p.y-bottom)*SCALE))
rigdef=json.loads((SRC/'default.mhskel').read_text());weights=json.loads((SRC/'default_weights.mhw').read_text())['weights']
source_weights={i:[] for i in used}
for bone,values in weights.items():
 for v,w in values:
  if v in source_weights:source_weights[v].append((bone,w))
def material(name,color,metal=0,rough=.6,texture=None):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF')
 p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if texture:
  t=m.node_tree.nodes.new('ShaderNodeTexImage');t.image=bpy.data.images.load(str(texture));m.node_tree.links.new(t.outputs['Color'],p.inputs['Base Color'])
 return m
def mesh(name,points,faces,mat,uv=None):
 d=bpy.data.meshes.new(name);d.from_pydata(points,[],faces);d.update();o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);d.materials.append(mat)
 for p in d.polygons:p.use_smooth=True
 if uv:
  layer=d.uv_layers.new(name='UVMap')
  for p,face in zip(d.polygons,uv):
   for loop,v in zip(p.loop_indices,face):layer.data[loop].uv=v
 return o
def rig(o,arm,groups):
 for bone,vals in groups.items():
  vg=o.vertex_groups.new(name=bone)
  for v,w in vals:vg.add([v],w,'REPLACE')
 mod=o.modifiers.new('Skin','ARMATURE');mod.object=arm;o.parent=arm
def rigid(o,arm,bone):rig(o,arm,{bone:[(v.index,1) for v in o.data.vertices]})
def sphere(name,center,size,mat,arm,bone,segments=32):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=20,location=center)
 o=bpy.context.object;o.name=name;o.scale=size;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
 o.data.materials.append(mat)
 for p in o.data.polygons:p.use_smooth=True
 rigid(o,arm,bone);return o
def tube(name,points,radius,mat,arm,bone):
 d=bpy.data.curves.new(name,'CURVE');d.dimensions='3D';d.resolution_u=8;d.bevel_depth=radius;d.bevel_resolution=3
 spl=d.splines.new('POLY');spl.points.add(len(points)-1)
 for a,p in zip(spl.points,points):a.co=(*p,1)
 o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);d.materials.append(mat)
 bpy.context.view_layer.objects.active=o;o.select_set(True);bpy.ops.object.convert(target='MESH');o=bpy.context.object;o.select_set(False);rigid(o,arm,bone);return o
manifest=[]
for identity,name,color,haircolor,female in [(1,'Scarlet',(.19,.017,.039),(.055,.011,.014),True),(2,'Lucinet',(.026,.082,.105),(.044,.025,.016),False),(3,'Rachel',(.095,.038,.021),(.22,.078,.025),True)]:
 P=[v.copy() for v in BASE_P]
 for target,weight in FACE_TARGETS[name].items():
  for line in (SRC/'targets'/target).read_text().splitlines():
   if not line.strip() or line.startswith('#'):continue
   index,dx,dy,dz=line.split();index=int(index)
   if P[index].y>6.2:P[index]+=Vector((float(dx),float(dy),float(dz)))*weight
 # Transfer hair/beard through the same head deformation as the face.
 headtree=kdtree.KDTree(len([i for i in used if BASE_P[i].y>6.2]))
 for i in used:
  if BASE_P[i].y>6.2:headtree.insert(BASE_P[i],i)
 headtree.balance()
 def headpoint(p):
  near=headtree.find_n(p,8);delta=Vector();total=0
  for base,index,distance in near:
   w=1/(distance+.08)**2;delta+=(P[index]-BASE_P[index])*w;total+=w
  return convert(p+delta/total)
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 for a in list(bpy.data.actions):bpy.data.actions.remove(a)
 scene=bpy.context.scene;scene.render.fps=30;scene.frame_start=1;scene.frame_end=361;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=.01
 srcskin=DEST/('skin_female.png' if female else 'skin_male.png')
 skinfile=DEST/('skin_female.png' if female else 'skin_male.png');
 if srcskin.resolve()!=skinfile.resolve():shutil.copyfile(srcskin,skinfile)
 skin=material('Skin',(.7,.47,.35),rough=.52,texture=skinfile)
 cloth=material('Cloth',color,rough=.86);leather=material('Leather',(.035,.025,.02),rough=.58)
 gold=material('Metal',(.49,.29,.091),metal=.83,rough=.3);hair=material('Hair',haircolor,rough=.49)
 white=material('Sclera',(.8,.77,.72),rough=.25);iris=material('Iris',(.047,.14,.105) if female else (.057,.095,.16),rough=.25);pupil=material('Pupil',(.005,.006,.008),rough=.18)
 d=bpy.data.armatures.new('HumanRig');arm=bpy.data.objects.new('HumanRig',d);bpy.context.collection.objects.link(arm);bpy.context.view_layer.objects.active=arm;arm.select_set(True);bpy.ops.object.mode_set(mode='EDIT')
 for bone,b in rigdef['bones'].items():
  eb=d.edit_bones.new(bone)
  def joint(key):return sum((P[i] for i in rigdef['joints'][key]),Vector())/len(rigdef['joints'][key])
  eb.head=convert(joint(b['head']));eb.tail=convert(joint(b['tail']))
  if (eb.tail-eb.head).length<.01:eb.tail.z+=.05
 for bone,b in rigdef['bones'].items():
  if b['parent']:d.edit_bones[bone].parent=d.edit_bones[b['parent']]
 bpy.ops.object.mode_set(mode='OBJECT');arm.select_set(False)
 points=[convert(P[i]) for i in used]
 # Authored small variations retain the shared topology and skin weights.
 for idx,v in enumerate(used):
  if not female and 3<P[v].y<6.0:points[idx].x*=1.06
 body=mesh('Human',points,[[MAP[v] for v,t in face] for face in F],skin,[[UV[t] for v,t in face] for face in F])
 groups={}
 for src,new in MAP.items():
  total=sum(w for bone,w in source_weights[src])
  if total<=0:groups.setdefault('root',[]).append((new,1));continue
  for bone,w in source_weights[src]:groups.setdefault(bone,[]).append((new,w/total))
 rig(body,arm,groups)
 tree=kdtree.KDTree(len(points))
 for i,pt in enumerate(points):tree.insert(pt,i)
 tree.balance()
 def garment_weights(vertices):
  result={}
  for index,pt in enumerate(vertices):
   neighbours=tree.find_n(Vector(pt),4);sums={};total=0
   for point,near,distance in neighbours:
    coefficient=1/(distance+.25);total+=coefficient
    for bone,weight in source_weights[used[near]]:sums[bone]=sums.get(bone,0)+coefficient*weight
   weight_sum=sum(sums.values())
   assert weight_sum>0,'garment attachment requires actual source skinning'
   for bone,weight in sums.items():result.setdefault(bone,[]).append((index,weight/weight_sum))
  return result
 body.shape_key_add(name='Basis');blink=body.shape_key_add(name='Blink');talk=body.shape_key_add(name='Talk')
 for src,new in MAP.items():
  p=BASE_P[src];x,y,z=p
  eye=min(abs(x-.30775),abs(x+.30775))
  if eye<.205 and 7.18<y<7.39 and z>1.17:
   blink.data[new].co.z+=(7.28415-y)*SCALE*max(0,1-(eye/.205)**2)
  if abs(x)<.46 and 6.57<y<6.99 and z>1.20:
   talk.data[new].co.z-=.09*SCALE*max(0,1-(abs(x)/.46)**2)*max(0,1-abs(y-6.85)/.28)
 # Real curved cloth shell with a folded hem, belt, inset panels and raised trim.
 rings=48;cols=112;verts=[];faces=[];clothgroups={}
 for row in range(rings+1):
  t=row/rings;z=14+t*139
  if z<91:rx=32-(z-14)*.13;ry=25-(z-14)*.09;cy=-2
  elif z<112:rx=22-(z-91)*.12;ry=17;cy=-4
  elif z<143:rx=20+(z-112)*.13;ry=17;cy=-4
  else:rx=24-(z-143)*1.55;ry=17-(z-143)*.6;cy=-4
  for col in range(cols):
   a=2*math.pi*col/cols;fold=1+.026*math.cos(a*16)+.018*math.sin(a*7+t*5)
   idx=len(verts);verts.append((rx*math.cos(a)*fold,ry*math.sin(a)*fold+cy,z))
   bn='spine04' if z<94 else 'spine03' if z<112 else 'spine02' if z<131 else 'spine01'
   clothgroups.setdefault(bn,[]).append((idx,1))
 for row in range(rings):
  for col in range(cols):
   a=row*cols+col;b=row*cols+(col+1)%cols;c=b+cols;e=a+cols;faces.append((a,b,c,e))
 robeuv=[[(col/cols,row/rings),((col+1)/cols,row/rings),((col+1)/cols,(row+1)/rings),(col/cols,(row+1)/rings)] for row in range(rings) for col in range(cols)]
 robe=mesh('Coat',verts,faces,cloth,robeuv);rig(robe,arm,garment_weights(verts))
 # Raised stitched lapels use their own geometry rather than a painted wedge.
 for sign in [-1,1]:
  panel=[(sign*3,-16,153),(sign*13,-23,145),(sign*17,-23,132),(sign*5,-23,117),(sign*7,-23,139)]
  o=mesh('Lapel',panel,[(0,1,4),(1,2,4),(2,3,4)],leather);rig(o,arm,garment_weights(panel))
  tube('LapelStitch',[(sign*3,-16.5,153),(sign*13,-23.5,145),(sign*17,-23.5,132),(sign*5,-23.5,117)],.2,gold,arm,'spine01')
 for h in [18,21,95,99]:
  rx=32-(h-14)*.13 if h<91 else 22-(h-91)*.12;ry=25-(h-14)*.09 if h<91 else 17;cy=-2 if h<91 else -4
  tube('CoatTrim',[(rx*math.cos(2*math.pi*i/112)*1.02,ry*math.sin(2*math.pi*i/112)*1.02+cy,h) for i in range(113)],.48,gold if h<30 else leather,arm,'spine04')
 sphere('Clasp',(0,-22,98),(3.4,1.25,3.4),gold,arm,'spine04')
 # Sleeves follow the rig's rest arms and retain arm skinning.
 for side in ['L','R']:
  for section in ['upperarm01','upperarm02','lowerarm01','lowerarm02']:
   bone=d.bones[section+'.'+side];a=bone.head_local;b=bone.tail_local;direction=(b-a).normalized();u=direction.cross(Vector((0,1,0))).normalized();v=direction.cross(u)
   vs=[];fs=[]
   for row in range(13):
    t=row/12;rad=(7.3 if section.startswith('upperarm') else 6.1)*(1-t*.05)
    for c in range(40):
     angle=2*math.pi*c/40;pt=(a-direction*.8).lerp(b+direction*.8,t)+(u*math.cos(angle)+v*math.sin(angle))*rad*(1+.025*math.sin(angle*7+t*9));vs.append(pt)
   for row in range(12):
    for c in range(40):i=row*40+c;j=row*40+(c+1)%40;fs.append((i,j,j+40,i+40))
   sleeveuv=[[(c/40,row/12),((c+1)/40,row/12),((c+1)/40,(row+1)/12),(c/40,(row+1)/12)] for row in range(12) for c in range(40)]
   o=mesh('Sleeve',vs,fs,cloth,sleeveuv);rig(o,arm,garment_weights(vs))
 # Rounded eyeballs and separate visible iris/pupil volumes.
 for side in ['L','R']:
  center=d.bones['eye.'+side].head_local.copy();r=.122*SCALE
  sphere('Eye',center,(r,r,r),white,arm,'head')
  sphere('Iris',center+Vector((0,-r*.93,0)),(r*.47,r*.12,r*.47),iris,arm,'eye.'+side)
  sphere('Pupil',center+Vector((0,-r*1.04,0)),(r*.24,r*.075,r*.24),pupil,arm,'eye.'+side)
 # Dense volumetric hair cap with individually raised swept strands.
 cap=[];cf=[];rows=18;cols=80
 for row in range(rows+1):
  theta=.035+row/rows*1.72
  for col in range(cols):
   a=2*math.pi*col/cols;front=math.sin(a)>.35
   edge=1.24+.15*math.cos(a*2+identity) if front else 1.86+.07*math.sin(a*3+identity)
   th=.035+row/rows*edge
   cap.append(headpoint(Vector((.94*math.sin(th)*math.cos(a),7.55+1.01*math.cos(th),.55+1.10*math.sin(th)*math.sin(a)))))
 for row in range(rows):
  for c in range(cols):i=row*cols+c;j=row*cols+(c+1)%cols;cf.append((i,j,j+cols,i+cols))
 o=mesh('HairCap',cap,cf,hair);rigid(o,arm,'head')
 for n in range(42):
  a=2*math.pi*n/42;pts=[]
  for j in range(15):
   t=j/14;edge=1.24+.15*math.cos(a*2+identity) if math.sin(a)>.35 else 1.86+.07*math.sin(a*3+identity)
   th=.06+t*(edge-.025);aa=a+.12*math.sin(t*math.pi)
   pts.append(headpoint(Vector((.955*math.sin(th)*math.cos(aa),7.55+1.025*math.cos(th),.55+1.115*math.sin(th)*math.sin(aa)))))
  tube('HairStrand',pts,.21,hair,arm,'head')
 if female:
  for n in range(3):
   pts=[headpoint(Vector((.15*math.sin(j*.75+n*2*math.pi/3),7.35-j*.075,-.47-.03*math.sin(j*.6)))) for j in range(24)]
   tube('Braid',pts,.85,hair,arm,'head')
 else:
  for side in [-1,1]:
   pts=[headpoint(Vector((side*(.04+.32*t/12),6.95-.055*math.sin(t/12*math.pi),1.36-.10*t/12))) for t in range(13)]
   tube('Moustache',pts,.25,hair,arm,'head')
  for n in range(13):
   x=-.36+n*.06;pts=[headpoint(Vector((x,6.59+.30*t/8,1.17+.10*math.sin(t/8*math.pi)))) for t in range(9)]
   tube('BeardStrand',pts,.14,hair,arm,'head')
 # Individually weighted boot volumes attached to the feet.
 for side in ['L','R']:
  c=d.bones['foot.'+side].head_local
  sphere('Boot',c+Vector((0,-12,-1)),(6.7,18,8),leather,arm,'foot.'+side)
 # Skin/cloth/hair deformation channels retain independent spatial motion.
 # Cloth is a damped morph approximation, not a claim of cloth simulation.
 breath=body.shape_key_add(name='Breath');smile=body.shape_key_add(name='Smile')
 for src,new in MAP.items():
  x,y,z=BASE_P[src]
  if 4.6<y<6.15 and abs(x)<1.0:
   amount=max(0,1-abs(y-5.5)/.9);breath.data[new].co.y-=.45*amount*max(0,z+.2)
  if 6.7<y<7.0 and .16<abs(x)<.48 and z>1.15:
   smile.data[new].co.z+=.28*max(0,1-abs(y-6.84)/.17)
 for obj in list(scene.objects):
  if obj.type!='MESH' or obj==body:continue
  if not obj.data.shape_keys:obj.shape_key_add(name='Basis')
  if obj.name.startswith(('Coat','Sleeve','Lapel','Clasp')):
   left=obj.shape_key_add(name='ClothLeft');right=obj.shape_key_add(name='ClothRight');breathe=obj.shape_key_add(name='Breath')
   for v in obj.data.vertices:
    co=v.co;hem=max(0,min(1,(105-co.z)/85))**1.5;flutter=math.sin(co.x*.15+co.y*.09)
    delta=Vector((3.5*hem,.8*hem*flutter,.55*hem*flutter))
    left.data[v.index].co+=delta;right.data[v.index].co-=delta
    if 110<co.z<150:breathe.data[v.index].co.y-=.6*max(0,1-abs(co.z-130)/20)
  if obj.name.startswith(('Hair','Braid','Beard','Moustache')):
   left=obj.shape_key_add(name='HairLeft');right=obj.shape_key_add(name='HairRight')
   for v in obj.data.vertices:
    amount=.18 if obj.name.startswith('HairCap') else .35 if obj.name.startswith('HairStrand') else max(.25,min(2.5,(170-v.co.z)*.08))
    left.data[v.index].co.x+=amount;right.data[v.index].co.x-=amount
 # Bake the base idle; runtime layers independent gaze, hands, knees, foot
 # placement, weight transfer, gait and response onto this stable source pose.
 for frame in range(1,362,10):
  t=(frame-1)/30
  for bone in ['spine03','spine02','neck03','head']:
   p=arm.pose.bones[bone];p.rotation_mode='QUATERNION'
   axis=Vector((1,0,0)) if bone.startswith('spine') else Vector((0,1,0))
   angle=.012*math.sin(t*2*math.pi/4) if bone.startswith('spine') else .018*math.sin(t*2*math.pi/12)
   p.rotation_quaternion=Quaternion(axis,angle);p.keyframe_insert('rotation_quaternion',frame=frame,group=bone)
  for side,sgn in [('L',1),('R',-1)]:
   bone='upperarm01.'+side;p=arm.pose.bones[bone];p.rotation_mode='QUATERNION'
   q=d.bones[bone].matrix_local.to_quaternion();p.rotation_quaternion=q.inverted()@Quaternion(Vector((0,1,0)),sgn*math.radians(32)+.012*math.sin(t*1.8))@q;p.keyframe_insert('rotation_quaternion',frame=frame,group=bone)
  p=arm.pose.bones['spine02'];p.scale=(1,1,1+.007*math.sin(t*2*math.pi/4));p.keyframe_insert('scale',frame=frame,group='Breath')
 arm.animation_data.action.name='HumanIdle'
 # One coherent skinned surface exports one bind-pose contract. The body's
 # shape keys remain active; attached garment/hair vertices receive neutral keys.
 bpy.ops.object.select_all(action='DESELECT')
 for obj in list(scene.objects):
  if obj.type=='MESH':obj.select_set(True)
 bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
 scene.frame_set(1)
 # Export only own + CC0 geometry; no Epic content is copied to this FBX.
 bpy.ops.object.select_all(action='SELECT');bpy.context.view_layer.objects.active=arm
 file=DEST/(name+'.fbx')
 bpy.ops.export_scene.fbx(filepath=str(file),use_selection=True,object_types={'ARMATURE','MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,use_mesh_modifiers=False,mesh_smooth_type='FACE',path_mode='RELATIVE',embed_textures=False)
 bpy.ops.wm.save_as_mainfile(filepath=str(DEST/(name+'.blend')))
 manifest.append({'identity':identity,'name':name,'vertices':sum(len(o.data.vertices) for o in scene.objects if o.type=='MESH'),'bones':len(d.bones),'seconds':12,'morphs':['Blink','Talk','Breath','Smile','ClothLeft','ClothRight','HairLeft','HairRight'],'faceTargets':FACE_TARGETS[name],'fbxSha256':hashlib.sha256(file.read_bytes()).hexdigest()})
 print('HUMAN_BUILT',manifest[-1],flush=True)
(DEST/'characters.json').write_text(json.dumps({'schema':'halveth.human-source.v1','geometryLicense':'CC0 MakeHuman core assets plus own authored clothing and hair','characters':manifest},indent=2)+'\n')
