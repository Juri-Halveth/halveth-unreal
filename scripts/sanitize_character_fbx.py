# SPDX-License-Identifier: MIT
"""Own wrapper around Blender's FBX parser/writer; bind numeric roundtrip."""
from pathlib import Path
import sys,hashlib,json,array
R=Path(__file__).resolve().parent
import bpy
from io_scene_fbx import parse_fbx,encode_bin,data_types
root=R.parents[0]/'ArtSource/Characters'
methods={getattr(data_types,key):'add_'+key.lower() for key in ['BOOL','CHAR','INT8','INT16','INT32','INT64','FLOAT32','FLOAT64','BYTES','STRING','INT32_ARRAY','INT64_ARRAY','FLOAT32_ARRAY','FLOAT64_ARRAY','BOOL_ARRAY','BYTE_ARRAY']}
def numeric_digest(element):
 h=hashlib.sha256()
 def visit(e):
  h.update(e.id)
  for typ,value in zip(e.props_type,e.props):
   if typ in [data_types.STRING,data_types.BYTES]:continue
   h.update(bytes([typ]));h.update(value.tobytes() if isinstance(value,array.array) else repr(value).encode())
  for e2 in e.elems:visit(e2)
 for child in element.elems:
  if child.id in [b'Objects',b'Connections',b'GlobalSettings']:visit(child)
 return h.hexdigest()
records=[]
windows_user_marker=b':'+bytes([92])+b'Users'+bytes([92])
slash_user_marker=b':'+bytes([47])+b'Users'+bytes([47])
for name in ['Scarlet','Lucinet','Rachel']:
 p=root/(name+'.fbx');parsed,version=parse_fbx.parse(str(p));before=numeric_digest(parsed);changes=[]
 def clone(e):
  target=encode_bin.FBXElem(e.id)
  for typ,value in zip(e.props_type,e.props):
   if typ==data_types.STRING and (windows_user_marker in value or slash_user_marker in value):
    text=value.decode('utf8');replacement=Path(text.replace('\\','/')).name
    if replacement.endswith('.blend'):replacement=name+'.blend'
    changes.append({'element':e.id.decode(),'replacement':replacement});value=replacement.encode()
   getattr(target,methods[typ])(value)
  target.elems=[clone(child) for child in e.elems];return target
 rebuilt=clone(parsed);temp=root/(name+'.sanitized.fbx');encode_bin.write(str(temp),rebuilt,version)
 after,v2=parse_fbx.parse(str(temp));assert version==v2 and before==numeric_digest(after)
 assert windows_user_marker not in temp.read_bytes() and slash_user_marker not in temp.read_bytes()
 temp.replace(p);records.append({'name':name,'fbxVersion':version,'numericGeometryAnimationDigest':before,'numericRoundtripEqual':True,'embeddedPathsReplaced':changes,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
manifest=json.loads((root/'characters.json').read_text())
for c in manifest['characters']:c['fbxSha256']=next(r['sha256'] for r in records if r['name']==c['name'])
(root/'characters.json').write_text(json.dumps(manifest,indent=2)+'\n')
(root/'fbx-path-roundtrip.json').write_text(json.dumps({'schema':'halveth.fbx-path-redaction.v1','records':records,'scope':'Embedded local authoring paths converted to relative filenames; numeric geometry, indices, skeleton and animation properties retain the same parsed digest.'},indent=2)+'\n')
print('Three FBX files retain numeric geometry/animation while embedded local paths are relative.')
