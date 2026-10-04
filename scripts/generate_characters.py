# SPDX-License-Identifier: MIT
"""Blender batch: CC0 anatomical human, authored hm08 garments/hair/footwear, skinned idle."""
from pathlib import Path
import bpy,json,math,sys,shutil,hashlib
from mathutils import Vector,Quaternion
sys.path.insert(0,str(Path(__file__).resolve().parent))
from character_proxy import anatomical_asset,proxy_contract
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
 'Scarlet':{'chin/chin-prominent-incr.target':.1,'nose/nose-point-up.target':.1,'cheek/l-cheek-bones-incr.target':.15,'cheek/r-cheek-bones-incr.target':.15},
 'Lucinet':{'macrodetails/caucasian-male-young.target':.7,'chin/chin-width-incr.target':.35,'chin/chin-prominent-incr.target':.15,'nose/nose-curve-convex.target':.1,'nose/nose-scale-depth-incr.target':.1,'cheek/l-cheek-volume-decr.target':.1,'cheek/r-cheek-volume-decr.target':.1},
 'Rachel':{'chin/chin-width-incr.target':.2,'nose/nose-point-up.target':.2,'cheek/l-cheek-bones-incr.target':.1,'cheek/r-cheek-bones-incr.target':.1}}
bottom=min(P[v].y for v in used);top=max(P[v].y for v in used);SCALE=180/(top-bottom)
def convert(p):return Vector((p.x*SCALE,-p.z*SCALE,(p.y-bottom)*SCALE))
rigdef=json.loads((SRC/'default.mhskel').read_text());weights=json.loads((SRC/'default_weights.mhw').read_text())['weights']
source_weights={i:[] for i in range(len(P))}
for bone,values in weights.items():
 for v,w in values:
  source_weights[v].append((bone,w))
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
manifest=[]
for identity,name,color,haircolor,female in [(1,'Scarlet',(.19,.017,.039),(.055,.011,.014),True),(2,'Lucinet',(.026,.082,.105),(.044,.025,.016),False),(3,'Rachel',(.095,.038,.021),(.22,.078,.025),True)]:
 P=[v.copy() for v in BASE_P]
 for target,weight in FACE_TARGETS[name].items():
  for line in (SRC/'targets'/target).read_text().splitlines():
   if not line.strip() or line.startswith('#'):continue
   index,dx,dy,dz=line.split();index=int(index)
   if target.startswith('macrodetails/') or P[index].y>6.2:P[index]+=Vector((float(dx),float(dy),float(dz)))*weight
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
 body=mesh('Human',points,[[MAP[v] for v,t in face] for face in F],skin,[[UV[t] for v,t in face] for face in F])
 groups={}
 for src,new in MAP.items():
  total=sum(w for bone,w in source_weights[src])
  if total<=0:groups.setdefault('root',[]).append((new,1));continue
  for bone,w in source_weights[src]:groups.setdefault(bone,[]).append((new,w/total))
 rig(body,arm,groups)
 body.shape_key_add(name='Basis');blink=body.shape_key_add(name='Blink');talk=body.shape_key_add(name='Talk')
 for src,new in MAP.items():
  p=BASE_P[src];x,y,z=p
  eye=min(abs(x-.30775),abs(x+.30775))
  if eye<.205 and 7.18<y<7.39 and z>1.17:
   blink.data[new].co.z+=(7.28415-y)*SCALE*max(0,1-(eye/.205)**2)
  if abs(x)<.46 and 6.57<y<6.99 and z>1.20:
   talk.data[new].co.z-=.09*SCALE*max(0,1-(abs(x)/.46)**2)*max(0,1-abs(y-6.85)/.28)
 # Authored garment coordinates and skinning share exact anatomical addresses.
 parts=[]
 footwear=material('Footwear',(.06,.045,.03),rough=.68)
 for kind,mat in [('Wardrobe',cloth),('Footwear',footwear),('Hair',hair)]:
  folder=SRC/kind/name;definition=next(folder.glob('*.mhclo'))
  o=anatomical_asset(definition,kind,mat,arm,P,convert,source_weights)
  if kind=='Wardrobe':
   o.data.materials.append(leather)
   for face in o.data.polygons:
    height=sum(o.data.vertices[v].co.z for v in face.vertices)/len(face.vertices)
    if height<96:face.material_index=1
  parts.append(o)
 # The clothing's own occlusion mask removes covered body faces, avoiding
 # duplicated skin surfaces through cuffs, collars, knees and footwear.
 covered=set()
 for kind in ['Wardrobe','Footwear']:
  covered.update(proxy_contract(next((SRC/kind/name).glob('*.mhclo')))[3])
 import bmesh
 bm=bmesh.new();bm.from_mesh(body.data)
 bm.verts.ensure_lookup_table();discard=[f for f in bm.faces if all(used[v.index] in covered for v in f.verts)]
 bmesh.ops.delete(bm,geom=discard,context='FACES_ONLY');bm.to_mesh(body.data);bm.free();body.data.update()
 # Rounded eyeballs and separate visible iris/pupil volumes.
 for side in ['L','R']:
  center=d.bones['eye.'+side].head_local.copy();r=.122*SCALE
  sphere('Eye',center,(r,r,r),white,arm,'head')
  sphere('Iris',center+Vector((0,-r*.93,0)),(r*.47,r*.12,r*.47),iris,arm,'eye.'+side)
  sphere('Pupil',center+Vector((0,-r*1.04,0)),(r*.24,r*.075,r*.24),pupil,arm,'eye.'+side)
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
  if obj.name.startswith(('Wardrobe',)):
   left=obj.shape_key_add(name='ClothLeft');right=obj.shape_key_add(name='ClothRight');breathe=obj.shape_key_add(name='Breath')
   for v in obj.data.vertices:
    co=v.co;hem=.03+max(0,min(1,(100-co.z)/45))**1.5*.12;flutter=math.sin(co.x*.15+co.y*.09)
    delta=Vector((3.5*hem,.8*hem*flutter,.55*hem*flutter))
    left.data[v.index].co+=delta;right.data[v.index].co-=delta
    if 110<co.z<150:breathe.data[v.index].co.y-=.6*max(0,1-abs(co.z-130)/20)
  if obj.name.startswith(('Hair',)):
   left=obj.shape_key_add(name='HairLeft');right=obj.shape_key_add(name='HairRight')
   for v in obj.data.vertices:
    amount=max(.03,min(1.5,(174-v.co.z)*.04))
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
 manifest.append({'identity':identity,'name':name,'construction':'hm08-affine-proxy-v1','vertices':sum(len(o.data.vertices) for o in scene.objects if o.type=='MESH'),'bones':len(d.bones),'seconds':12,'morphs':['Blink','Talk','Breath','Smile','ClothLeft','ClothRight','HairLeft','HairRight'],'faceTargets':FACE_TARGETS[name],'fbxSha256':hashlib.sha256(file.read_bytes()).hexdigest()})
 print('HUMAN_BUILT',manifest[-1],flush=True)
(DEST/'characters.json').write_text(json.dumps({'schema':'halveth.human-source.v1','geometryLicense':'CC0 anatomical MakeHuman hm08 + explicitly CC0 authored garments, footwear and hair; own morph/idle additions','characters':manifest},indent=2)+'\n')
