# SPDX-License-Identifier: MIT
"""Independent hm08 data reader: affine anatomy addresses, weights, UV, masks.

Input is licensed mesh data. No MakeHuman application code is embedded. The
linear map is sum(w_i * anatomical_vertex_i) + axis_scaled_offset. Negative
geometric coefficients are valid affine extrapolation; skeletal weights remain
nonnegative and normalized. Unsupported records fail instead of changing rigs.
"""
from pathlib import Path
import re, math
import bpy
from mathutils import Vector

def proxy_contract(path):
 header={};references=[];mask=set();mode='HEADER'
 for raw in Path(path).read_text(encoding='utf-8-sig').splitlines():
  line=raw.strip()
  if not line or line.startswith('#'):continue
  values=line.split()
  if values[0]=='verts':
   if values!=['verts','0']:raise ValueError('Only zero-based hm08 proxy indices supported')
   mode='VERTS';continue
  if values[0]=='delete_verts':mode='MASK';continue
  if values[0] in ['material','vertexboneweights_file','z_depth','max_pole']:
   header[values[0]]=values[1:];continue
  if mode=='HEADER':header[values[0]]=values[1:];continue
  if mode=='MASK':
   tokens=re.findall(r'\d+\s*-\s*\d+|\d+',line)
   if ''.join(tokens).replace(' ','')!=line.replace(' ',''):raise ValueError('Invalid mask range')
   for t in tokens:
    if '-'in t:a,b=map(int,t.split('-'));mask.update(range(a,b+1))
    else:mask.add(int(t))
   continue
  if len(values)==1:
   index=int(values[0]);references.append(((index,index,index),(1.,0.,0.),(0.,0.,0.)))
  elif len(values)==9:
   ids=tuple(map(int,values[:3]));coef=tuple(map(float,values[3:6]));off=tuple(map(float,values[6:]))
   if not all(math.isfinite(v) for v in coef+off)or abs(sum(coef)-1)>0.005:raise ValueError('Invalid affine coordinate')
   references.append((ids,coef,off))
  else:raise ValueError('Unsupported proxy vertex record: '+line[:80])
 if header.get('basemesh')!=['hm08']:raise ValueError('Requires the declared hm08 anatomical topology')
 return header,references,Path(path).parent/header['obj_file'][0],mask

def anatomical_asset(definition,kind,material,arm,anatomy,convert,source_weights):
 header,ref,objpath,mask=proxy_contract(definition)
 uv=[];faces=[];obj_count=0
 for line in objpath.read_text(encoding='utf-8-sig').splitlines():
  a=line.split()
  if not a:continue
  if a[0]=='v':obj_count+=1
  elif a[0]=='vt':uv.append(tuple(map(float,a[1:3])))
  elif a[0]=='f':faces.append([tuple(int(i)-1 for i in token.split('/')[:2])for token in a[1:]])
 if len(ref)!=obj_count:raise ValueError('Proxy/OBJ vertex topology mismatch')
 axis_scale=[]
 for axis,key in enumerate(['x_scale','y_scale','z_scale']):
  if key not in header:axis_scale.append(1.);continue
  a,b,den=header[key];den=float(den)
  if den<=0:raise ValueError('Invalid axis denominator')
  axis_scale.append(abs(anatomy[int(a)][axis]-anatomy[int(b)][axis])/den)
 points=[];weights={}
 for index,(ids,coef,offset)in enumerate(ref):
  if any(i<0 or i>=len(anatomy) for i in ids):raise ValueError('Anatomical address outside hm08 source')
  pt=sum((anatomy[i]*c for i,c in zip(ids,coef)),Vector())+Vector(tuple(v*s for v,s in zip(offset,axis_scale)))
  points.append(convert(pt));sums={}
  for i,c in zip(ids,coef):
   for bone,w in source_weights[i]:sums[bone]=sums.get(bone,0)+c*w
  # Interpolated influence contracts are nonnegative, normalized and bounded.
  ordered=sorted(((bn,max(0.,w))for bn,w in sums.items()if w>1e-6),key=lambda x:(-x[1],x[0]))[:8]
  total=sum(w for bn,w in ordered)
  if total<=0:raise ValueError('Proxy lacks an anatomical skin-weight binding')
  for bn,w in ordered:weights.setdefault(bn,[]).append((index,w/total))
 mesh=bpy.data.meshes.new(kind);mesh.from_pydata(points,[],[[v for v,t in f]for f in faces]);mesh.update()
 o=bpy.data.objects.new(kind,mesh);bpy.context.collection.objects.link(o);mesh.materials.append(material)
 layer=mesh.uv_layers.new(name='UVMap')
 for polygon,face in zip(mesh.polygons,faces):
  polygon.use_smooth=True
  for loop,(v,t)in zip(polygon.loop_indices,face):layer.data[loop].uv=uv[t]
 for bone,values in weights.items():
  group=o.vertex_groups.new(name=bone)
  for index,weight in values:group.add([index],weight,'REPLACE')
 mod=o.modifiers.new('AnatomicalSkin','ARMATURE');mod.object=arm;o.parent=arm
 o['anatomical_operator']='hm08-affine-proxy-v1';o['source_definition']=definition.name
 return o
