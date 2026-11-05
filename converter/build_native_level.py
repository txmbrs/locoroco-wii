"""Convert an extracted original PSP scene into an external Wii map pack."""
import json, math, hashlib
from pathlib import Path

def build(map_dir,import_dir,output):
 SRC=Path(map_dir); IMPORT=Path(import_dir); OUT=Path(output)
 nodes=json.loads((SRC/'graph.json').read_text())['nodes']
 objects={x['id']:x for x in json.loads((SRC/'objects.json').read_text())['objects']}
 meshes={x['id']:x for x in json.loads((SRC/'render-meshes.json').read_text())['meshes']}
 polys={x['id']:x for x in json.loads((SRC/'collisions.json').read_text())['polygons']}
 texture_info={x['id']:x for x in json.loads((SRC/'textures.json').read_text())['textures']}
 level_index=json.loads((IMPORT/'blv-textures.json').read_text())
 candidates=[x for x in level_index['levels'] if 'st_flower01.dlk' in x['names']]
 if len(candidates)!=1:raise ValueError('Expected exactly one st_flower01 level')
 level_source=candidates[0]['source']
 texture_source={x['offset']:x for x in json.loads((IMPORT/'blv-textures.json').read_text())['textures'] if x['source']==level_source}
 SCALE=6.0
 spawn=goal=mui_actor=None
 
 def ref(x):return x.get('$ref') if isinstance(x,dict) else None
 def fields(x):return nodes[ref(x)]['fields'] if ref(x) is not None else x
 def props(o):return {p['name']:p['value'] for p in o['properties']}
 def vec(v,default):return fields(v) if v is not None else default
 I=(1,0,0,1,0,0,0)
 def multiply(a,b):
  a0,a1,a2,a3,ax,ay,az=a;b0,b1,b2,b3,bx,by,bz=b
  return (a0*b0+a2*b1,a1*b0+a3*b1,a0*b2+a2*b3,a1*b2+a3*b3,a0*bx+a2*by+ax,a1*bx+a3*by+ay,az+bz)
 def transform(p,parent):
  v=vec(p.get('pos'),{'x':0,'y':0,'z':0});s=vec(p.get('scale'),{'x':1,'y':1,'z':1});r=p.get('rotZ',0);c=math.cos(r);q=math.sin(r)
  return multiply(parent,(c*s['x'],q*s['x'],-q*s['y'],c*s['y'],v['x'],v['y'],v.get('z',0)))
 def point(m,x=0,y=0):return ((m[0]*x+m[2]*y+m[4])*SCALE,-(m[1]*x+m[3]*y+m[5])*SCALE)
 vertices=[];pieces=[];edges=[];bodies=[];fruits=[];flies=[];wind=[];openings=[];muimuis=[];textures=[];texture_ids={};locations={}
 def gx_rgba(data,w,h):
  result=bytearray()
  for by in range(0,h,4):
   for bx in range(0,w,4):
    block=[data[((by+y)*w+bx+x)*4:((by+y)*w+bx+x)*4+4] for y in range(4) for x in range(4)]
    result.extend(v for p in block for v in (p[3],p[0]));result.extend(v for p in block for v in (p[1],p[2]))
  return result
 def texture(tid):
  if tid is None:w=h=4;raw=b'\xff'*64
  else:
   info=texture_info[tid];record=texture_source[info['fileOffset']]
   w,h=record['width'],record['height'];raw=(IMPORT/record['rgba']).read_bytes()
  if w%4 or h%4:
   oldw,oldh=w,h;w=(w+3)//4*4;h=(h+3)//4*4;raw=b''.join(raw[(min(y,oldh-1)*oldw+min(x,oldw-1))*4:(min(y,oldh-1)*oldw+min(x,oldw-1))*4+4] for y in range(h) for x in range(w))
  key=(w,h,hashlib.sha256(raw).hexdigest())
  if key not in texture_ids:texture_ids[key]=len(textures);textures.append((w,h,gx_rgba(raw,w,h)))
  return texture_ids[key]
 def add_mesh(mid,m,body=-1,fruit=-1):
  for comp in meshes[mid]['components']:
   color=comp['color'];tx=texture(comp['texture']);ti=texture_info.get(comp['texture']);uv=comp['uvScale']
   ufac=uv[0]*(2**math.ceil(math.log2(ti['width']))/ti['width'] if ti else 1)/32768
   vfac=uv[1]*(2**math.ceil(math.log2(ti['height']))/ti['height'] if ti else 1)/32768
   if ti:ufac*=ti['width']/textures[tx][0];vfac*=ti['height']/textures[tx][1]
   mat=nodes[comp['material']]['fields'];file=fields(mat['file']) if mat.get('file') else None;repeat=int(bool(file) and file['textureMode']!=0x10203)
   first=len(vertices);zs=[]
   for strip in comp['strips']:
    for tri in strip['triangles']:
     vs=[strip['vertices'][i] for i in tri]
     if abs((vs[1]['x']-vs[0]['x'])*(vs[2]['y']-vs[0]['y'])-(vs[2]['x']-vs[0]['x'])*(vs[1]['y']-vs[0]['y']))<1e-8:continue
     for v in vs:
      x,y=point(m,v['x'],v['y']);zs.append(m[6]+v.get('z',0))
      rgba=[round(max(0,min(255,color[k]*v.get(k,255)))) for k in 'rgba']
      vertices.append((x,y,v['u']*ufac,v['v']*vfac,*rgba))
   if len(vertices)>first:
    vs=vertices[first:];pieces.append((first,len(vs),tx,body,fruit,repeat,min(zs),min(v[0] for v in vs),min(v[1] for v in vs),max(v[0] for v in vs),max(v[1] for v in vs)))
 def material_box(mat,m,x,y,w,h,body=-1,fruit=-1,flipx=False,flipy=False):
  if ref(mat) is None:return
  material=fields(fields(mat)['material']);file=fields(material['file']) if material['file'] else None
  tid=ref(file['buffer']) if file else None;tx=texture(tid);c=fields(material['color']);rgba=[round(c[k]*255) for k in 'rgba'];first=len(vertices)
  for px,py,u,v in [(x,y,0,1),(x+w,y,1,1),(x+w,y+h,1,0),(x,y,0,1),(x+w,y+h,1,0),(x,y+h,0,0)]:
   a,b=point(m,px,py);vertices.append((a,b,1-u if flipx else u,1-v if flipy else v,*rgba))
  vs=vertices[first:];pieces.append((first,6,tx,body,fruit,int(bool(file) and file["textureMode"]!=0x10203),m[6],min(v[0] for v in vs),min(v[1] for v in vs),max(v[0] for v in vs),max(v[1] for v in vs)))
 def add_edges(poly,m,body=-1):
  for comp in poly['components']:
   for a,b in comp['edges']:
    p,q=poly['points'][a],poly['points'][b];p=point(m,p['x'],p['y']);q=point(m,q['x'],q['y'])
    if math.dist(p,q)>.3:
     # PSP -10000 denotes default, not zero friction. Positive material
     # values and slippery zero are carried through to the Wii approximation.
     value=comp['friction'];friction=.25 if value<=-9999 else max(0.,min(2.,value))
     edges.append((*p,*q,body,friction))
 def walk(oid,parent=I,body=-1,fruit=-1,mi=None,seen=()):
  if oid in seen:raise ValueError('Scene hierarchy cycle')
  o=objects[oid];p=props(o);typ=o['type'];name=o['name'];m=transform(p,parent);locations[oid]=point(m)
  if typ=='reference' and 'goalsmallfinish' not in name:return
  if typ in ['tutorialmuimui','muimui','shukuhou','stageend','fukidashi']:
   if typ=='muimui':
    nonlocal mui_actor
    muimuis.append(point(m))
    if name.split('|')[-1]=='muimui_2':mui_actor=point(m)
   return
  if typ=='objectBundle' and name in ['shukuhou.dlk','stageclear2.dlk']:return
  if p.get('visibility') is False:return
  if typ in ['coin_one','coin_ten']:flies.append((*point(m),int(p.get('numCoins',1))));return
  if typ=='start':nonlocal spawn;spawn=point(m)
  if typ=='goalhole':nonlocal goal;goal=point(m)
  if typ=='kaze':
   for r in nodes[ref(p['path'])]['items']:
    v=fields(r);wind.append(point(m,v['x'],v['y']))
  if typ in ['hakarisw_a','kakushi']:
   body=len(bodies);bodies.append((6 if typ=='hakarisw_a' else 7,*point(m),oid))
  if body>=0 and bodies[body][0]==6:
   for key in ['collision','polygon']:
    if ref(p.get(key)) in polys and typ!='hakarisw_a':add_edges(polys[ref(p[key])],m,body)
  if typ=='movingbg' and nodes[oid]['fields'].get('animation'):
   body=len(bodies);bodies.append((5,*point(m),oid))
  if typ in ['moving','rollhana'] and ref(p.get('collision')) is not None:
   kind=2 if typ=='rollhana' else 1 if 'move_kaiten_01' in name else 3 if name.endswith('move_mui_gate') else 0
   body=len(bodies);pivot=point(m);bodies.append((kind,*pivot,oid));add_edges(polys[ref(p['collision'])],m,body)
  if typ=='hana':
   sy=vec(p.get('scale'),{'y':1})['y'];head=point(m,0,98.3)
   body=len(bodies);bodies.append((4,*head,oid));openings.append((oid,*head))
   material_box(p.get('kuki_mat'),m,-1,0,2,100,flipy=True)
   material_box(p.get('happa_mat'),m,-33.6,.38-3/sy,60,30/sy,flipy=True)
   material_box(p.get('hana_mat'),m,-30.2,98.3-7.5/sy,60,15/sy,body,flipx=True,flipy=True)
   a=point(m,-30,98.3);b=point(m,30,98.3);edges.append((*a,*b,body,.65))
  if typ.startswith('mi_'):
   index=len(fruits);fruits.append([*point(m),body,*point(m)])
   if 'treeObject' in p:
    sub=fields(p['treeObject']);walk(ref(sub['object']),m,body,-1,(p.get('miObject'),index),seen+(oid,))
   elif 'miObject' in p:
    sub=fields(p['miObject']);walk(ref(sub['object']),m,body,index,None,seen+(oid,))
   return
  if typ=='child_spot' and mi and mi[0]:
   sub=fields(mi[0]);fruits[mi[1]][3:]=point(m)
   walk(ref(sub['object']),m,body,mi[1],None,seen+(oid,))
  if ref(p.get('mesh')) in meshes:add_mesh(ref(p['mesh']),m,body,fruit)
  for prop in o['properties']:
   v=prop['value'];idx=ref(v)
   if idx is not None and nodes[idx]['type']=='SubRoot':
    sub=fields(v);walk(ref(sub['object']),m,body,fruit,mi,seen+(oid,))
  for child in o['children']:walk(child,m,body,fruit,mi,seen+(oid,))
 root=nodes[0]['fields'];grid=fields(root['levelGrid'])
 for r in nodes[ref(grid['meshGrid'])]['items']:
  mesh=ref(fields(r).get('mesh'))
  if mesh is not None:add_mesh(mesh,I)
 add_edges(polys[ref(root['collision'])],I)
 walk(ref(fields(root['subRoot'])['object']))
 pieces.sort(key=lambda p:p[6])
 import struct
 openings.sort()
 if spawn is None or goal is None or mui_actor is None or len(openings)<2 or len(wind)<2:
  raise ValueError('Incomplete st_flower01 scene')
 payload=bytearray()
 def put(fmt,*values):payload.extend(struct.pack('>'+fmt,*values))
 for data in [vertices,pieces,edges,bodies,fruits,flies,wind,textures]:put('I',len(data))
 for pair in [spawn,goal,openings[0][1:],openings[1][1:],mui_actor]:put('2f',*pair)
 for data,fmt in [(vertices,'4f4B'),(pieces,'6i5f'),(edges,'4fif'),(bodies,'iffi'),(fruits,'2fi2f'),(flies,'2fi'),(wind,'2f')]:
  for record in data:put(fmt,*record)
 for w,h,raw in textures:
  put('3I',w,h,len(raw))
  payload.extend(bytes((-20-len(payload))%32));payload.extend(raw)
 checksum=2166136261
 for byte in payload:checksum=((checksum^byte)*16777619)&0xffffffff
 OUT.parent.mkdir(parents=True,exist_ok=True)
 OUT.write_bytes(b'LRCMAP01'+struct.pack('>III',3,len(payload),checksum)+payload)
 report={'map_file':OUT.name,'map_format':3,'payload_bytes':len(payload),'payload_fnv1a':f'{checksum:08x}','source':'Original st_flower01 from verified PSP ISO','coordinateScale':SCALE,'yAxisFlipped':True,'vertices':len(vertices),'pieces':len(pieces),'textures':len(textures),'edges':len(edges),'bodies':len(bodies),'fruits':len(fruits),'pickoryObjects':len(flies),'pickoryValue':sum(x[2] for x in flies),'windPoints':len(wind),'spawn':spawn,'goal':goal,'limitations':['Root map signals are provided by the companion event bank; embedded presentation scenes are not a PSP VM.','Wii soft-body movement and camera are retained.','Native animation banks are loaded separately; unknown PSP signal handlers remain disabled.']}
 OUT.with_suffix('.json').write_text(json.dumps(report,indent=2));return report
 
 

if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser()
 parser.add_argument('--map-dir',type=Path,required=True)
 parser.add_argument('--import-dir',type=Path,required=True)
 parser.add_argument('--output',type=Path,required=True)
 args=parser.parse_args()
 print(json.dumps(build(args.map_dir,args.import_dir,args.output),indent=2))

