# SPDX-License-Identifier: MIT
"""Bake new channel-packed art masks from unchanged adult FBX rest geometry.

Run with Blender 5.2 --background --factory-startup --python this-file.
RGBA: lips / cheeks / upper eyelids / oily zones. Data are linear UNORM8;
PNG rows are top-down, UV V is bottom-up. This creates masks, not edited skins.
"""
from pathlib import Path
import bpy,hashlib,json,struct,sys,zlib
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from skin_layer_model import landmarks,zones
DEST=ROOT/'ArtSource/Characters'
RES=2048
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()


def png(path,pixels):
    h,w,_=pixels.shape
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    raw=b''.join(b'\0'+row.tobytes() for row in pixels)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))
                     +chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b''))


catalogue=json.loads((DEST/'characters.json').read_text())
receipt=dict(schema='halveth.anatomical-skin-layers.v1',operator='rest-local-bone-ellipsoids-to-skin-UV-barycentric-v1',
    channels=['lips','cheeks','upper_eyelids','oily_zones'],resolution=[RES,RES],dataSpace='LINEAR_UNORM8',
    coordinateContract='FBX local centimetres; UV V upwards; PNG row 0 top',
    sourceGeometry='Existing eight adult FBX files remain byte-for-byte unchanged',
    license='CC0-1.0 for generated masks derived from CC0 geometry; MIT generator',characters=[])
for character in catalogue['characters']:
    name=character['name']; source=DEST/(name+'.fbx'); before=sha(source)
    if before!=character['fbxSha256']:raise ValueError('FBX binding changed: '+name)
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(source),use_anim=False)
    arm=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
    body=next(o for o in bpy.context.scene.objects if o.type=='MESH')
    marks=landmarks({b.name:list(b.head_local) for b in arm.data.bones})
    vertices=np.array([list(v.co) for v in body.data.vertices],dtype=np.float64)
    values=np.array([zones(p,marks) for p in vertices],dtype=np.float64)
    canvas=np.zeros((RES,RES,4),dtype=np.float64)
    coverage=np.zeros((RES,RES),dtype=np.bool_)
    body.data.calc_loop_triangles();uv=body.data.uv_layers.active.data
    skin_slots={i for i,m in enumerate(body.data.materials) if m.name.split('.')[0]=='Skin'}
    triangles=0
    for tri in body.data.loop_triangles:
        if tri.material_index not in skin_slots:continue
        v=values[list(tri.vertices)]
        if np.max(v)<1/1024:continue
        tex=np.array([list(uv[i].uv) for i in tri.loops]);tex[:,1]=1.-tex[:,1]
        tex=tex*RES-.5
        lo=np.maximum(0,np.floor(tex.min(axis=0)).astype(int));hi=np.minimum(RES-1,np.ceil(tex.max(axis=0)).astype(int))
        if np.any(hi<lo):continue
        xs,ys=np.meshgrid(np.arange(lo[0],hi[0]+1),np.arange(lo[1],hi[1]+1))
        a,b,c=tex; denominator=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(denominator)<1e-10:continue
        wa=((b[1]-c[1])*(xs-c[0])+(c[0]-b[0])*(ys-c[1]))/denominator
        wb=((c[1]-a[1])*(xs-c[0])+(a[0]-c[0])*(ys-c[1]))/denominator
        wc=1.-wa-wb;inside=(wa>=-1e-7)&(wb>=-1e-7)&(wc>=-1e-7)
        out=wa[...,None]*v[0]+wb[...,None]*v[1]+wc[...,None]*v[2]
        tile=canvas[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
        np.maximum(tile,np.where(inside[...,None],out,0),out=tile)
        coverage[lo[1]:hi[1]+1,lo[0]:hi[0]+1]|=inside
        triangles+=1
    # Two-texel outward dilation protects filtered UV boundaries, not arbitrary
    # new body regions. Use neighbour max rather than blurring across islands.
    for _ in range(2):
        padded=np.pad(canvas,((1,1),(1,1),(0,0)))
        canvas=np.maximum.reduce([padded[y:y+RES,x:x+RES] for y,x in ((1,1),(0,1),(2,1),(1,0),(1,2))])
    pixels=np.rint(np.clip(canvas,0,1)*255).astype(np.uint8)
    path=DEST/(name+'_skin_layers.png');png(path,pixels)
    maxima=pixels.max(axis=(0,1)).tolist();counts=(pixels>8).sum(axis=(0,1)).tolist()
    if min(maxima)<80 or min(counts)<20:raise ValueError('An anatomical channel is empty: '+name)
    if before!=sha(source):raise ValueError('Source FBX mutated')
    entry=dict(name=name,sourceFbxSha256=before,mask=path.name,maskSha256=sha(path),landmarksCm=marks,
        nonzeroPixelCountsAbove8=counts,maxima=maxima,rasterisedSkinTriangles=triangles,
        bounds='Only front face; no world-space lookup',anatomicalScope='Authored cosmetic zones, not biological measurements')
    receipt['characters'].append(entry)
    print('SKIN_LAYER_MASK',name,counts,maxima,flush=True)
(DEST/'skin_layers.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf8')
# Preserve all original bytes; only the current inventory gains new masks.
inventory=json.loads((DEST/'MANIFEST.json').read_text())
public_suffixes={'.png','.md','.txt','.json','.fbx','.obj','.mhskel','.mhw','.mhclo','.mhmat','.target'}
inventory['files']=[dict(path=p.relative_to(DEST).as_posix(),bytes=p.stat().st_size,sha256=sha(p))
    for p in sorted(DEST.rglob('*')) if p.is_file() and p.name!='MANIFEST.json' and p.suffix.lower() in public_suffixes
    and not any(q.startswith('.') or q.lower().endswith('.fbm') for q in p.relative_to(DEST).parts)]
(DEST/'MANIFEST.json').write_text(json.dumps(inventory,indent=2)+'\n',encoding='utf8')
print('SKIN_LAYER_GENERATION_COMPLETE models=8 input_fbx_unchanged=true')
