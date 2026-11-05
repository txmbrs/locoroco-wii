"""Bake native PSP property/file tracks into small runtime animation banks.
The nominal 30 Hz timebase follows the native format viewer; it is explicit
metadata rather than a claim that PSP executable timing has been recovered.
"""
from pathlib import Path
import json,math,struct,hashlib

class Graph:
 def __init__(self,path):self.nodes=json.loads(Path(path).read_text(encoding='utf8'))['nodes']
 def node(self,r):return self.nodes[r['$ref']]
 def fields(self,r):return self.node(r)['fields']
 def array(self,r):return self.node(r)['items']
 def value(self,r):
  if not isinstance(r,dict) or '$ref' not in r:return r
  n=self.node(r)
  if n['type']=='Array':return [self.value(v) for v in n['items']]
  return n.get('fields',n)
 def properties(self,oid):return {self.fields(p)['name']:self.fields(p)['value'] for p in self.array(self.nodes[oid]['fields']['properties'])}
 def tracks(self,part):
  result={}
  for target,propList in self.node(part['objectToAnimatedProperties']).get('entries',[]):
   result[target['$ref']]={}
   for p in self.array(self.fields(propList)['properties']):
    f=self.fields(p)
    if f.get('data'):result[target['$ref']][f['propertyName']]=self.node(f['data'])
  return result
 def sample(self,track,t):
  f=track['fields'];data=self.value(f['data'])
  if not data:raise ValueError('Empty native animation track')
  frame=max(0,min(t-f['startTime'],f['duration']))/max(.000001,f['elementDuration']);i=min(int(frame),len(data)-1);j=min(i+1,len(data)-1);a,b=data[i],data[j];q=frame-int(frame)
  if track['type'] in ['AnimationTrackBool','AnimationTrackFileSwitch']:return a
  if isinstance(a,dict) and all(k in a for k in ('x','y','z')):return {k:a[k]+(b[k]-a[k])*q for k in ('x','y','z')}
  delta=math.remainder(b-a,2*math.pi) if track['type']=='AnimationTrackAngle' else b-a
  return a+delta*q

def mul(a,b):
 a0,a1,a2,a3,ax,ay=a;b0,b1,b2,b3,bx,by=b
 return (a0*b0+a2*b1,a1*b0+a3*b1,a0*b2+a2*b3,a1*b2+a3*b3,a0*bx+a2*by+ax,a1*bx+a3*by+ay)
def matrix(props):
 p=props.get('pos',{'x':0,'y':0});s=props.get('scale',{'x':1,'y':1});r=props.get('rotZ',0);c=math.cos(r);q=math.sin(r)
 return (c*s['x'],q*s['x'],-q*s['y'],c*s['y'],p['x'],p['y'])
def checksum(data):
 h=2166136261
 for b in data:h=((h^b)*16777619)&0xffffffff
 return h
def save(path,magic,payload):
 path=Path(path);path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(magic+struct.pack('>III',1,len(payload),checksum(payload))+payload)

def world(map_dir,output):
 g=Graph(Path(map_dir)/'graph.json');objects=json.loads((Path(map_dir)/'objects.json').read_text(encoding='utf8'))['objects'];groups=[]
 for obj in objects:
  if obj['type'] not in ['movingbg','moving']:continue
  ref=g.nodes[obj['id']]['fields'].get('animation')
  if not ref:continue
  named=g.fields(ref);part=g.fields(named['part']);tracks=g.tracks(part).get(obj['id'],{})
  if not tracks:continue
  base={k:g.value(v) for k,v in g.properties(obj['id']).items() if k in ['pos','rotZ','scale','visibility','velocity_rate']}
  mode=1 if obj['name'].endswith('move_mui_gate') else 0 if obj['type']=='movingbg' or 'move_kaiten_01' in obj['name'] else 2
  end=max(t['fields']['endTime'] for t in tracks.values());start=min(t['fields']['startTime'] for t in tracks.values());count=int(math.ceil(end-start))+1
  if count>5000:raise ValueError('Native world animation too long')
  poses=[];last=None;unwrapped=0
  for frame in range(count):
   p=dict(base)
   for name,track in tracks.items():p[name]=g.sample(track,start+frame)
   pos=p.get('pos',{'x':0,'y':0});r=p.get('rotZ',0)-base.get('rotZ',0)
   if last is None:unwrapped=r
   else:unwrapped+=math.remainder(r-last,2*math.pi)
   last=r;s=p.get('scale',{'x':1,'y':1});bs=base.get('scale',{'x':1,'y':1})
   sx=s['x']/bs['x'];sy=s['y']/bs['y']
   if obj['type']=='moving' and mode!=2 and (abs(sx-1)>1e-4 or abs(sy-1)>1e-4):raise ValueError('Animated collider scale needs a dedicated handler: '+obj['name'])
   bp=base.get('pos',{'x':0,'y':0})
   poses.append(((pos['x']-bp['x'])*6,-(pos['y']-bp['y'])*6,-unwrapped,sx,sy,float(p.get('visibility',True))))
  groups.append((obj['id'],mode,30*float(base.get('velocity_rate',1)),poses,obj['name']))
 payload=bytearray(struct.pack('>I',len(groups)))
 for source,mode,fps,poses,name in groups:
  payload.extend(struct.pack('>iifI',source,mode,fps,len(poses)))
  for p in poses:payload.extend(struct.pack('>6f',*p))
 save(output,b'LRCWANI1',payload)
 return [{'source':src,'mode':mode,'fps':fps,'frames':len(p),'name':name} for src,mode,fps,p,name in groups]

def character(map_dir,import_root,source,output):
 g=Graph(Path(map_dir)/'graph.json');objs=json.loads((Path(map_dir)/'objects.json').read_text(encoding='utf8'))['objects'];objs={o['id']:o for o in objs};meshData={m['id']:m for m in json.loads((Path(map_dir)/'render-meshes.json').read_text(encoding='utf8'))['meshes']}
 ti={t['id']:t for t in json.loads((Path(map_dir)/'textures.json').read_text(encoding='utf8'))['textures']};records={t['offset']:t for t in json.loads((Path(import_root)/'blv-textures.json').read_text(encoding='utf8'))['textures'] if t['source']==source}
 textures=[];texture_map={}
 for tid,info in ti.items():
  t=records[info['fileOffset']];raw=(Path(import_root)/t['rgba']).read_bytes();w,h=t['width'],t['height']
  if w%4 or h%4:raise ValueError('Character texture needs GX padding')
  tiled=bytearray()
  for y in range(0,h,4):
   for x in range(0,w,4):
    ps=[raw[((y+py)*w+x+px)*4:((y+py)*w+x+px)*4+4] for py in range(4) for px in range(4)]
    tiled.extend(v for p in ps for v in (p[3],p[0]));tiled.extend(v for p in ps for v in (p[1],p[2]))
  texture_map[tid]=len(textures);textures.append((w,h,tiled,info['name']))
 meshes=[];parents={};order=[];root_id=next(o['id'] for o in objs.values() if o['type']=='objectBundle')
 def walk(oid,parent=-1):
  parents[oid]=parent;order.append(oid)
  for child in objs[oid]['children']:walk(child,oid)
 walk(root_id)
 for oid in order:
  props=g.properties(oid);mid=props.get('mesh',{}).get('$ref')
  if mid not in meshData:continue
  for comp in meshData[mid]['components']:
   if not comp['strips']:continue
   mat=g.nodes[comp['material']]['fields'];file_ref=mat['file'];file=g.fields(file_ref);info=ti[comp['texture']];uv=comp['uvScale'];vs=[];col=comp['color']
   for strip in comp['strips']:
    for tri in strip['triangles']:
     for i in tri:
      v=strip['vertices'][i];vs.append((v['x'],v['y'],v['u']/32768*uv[0],v['v']/32768*uv[1],*[round(col[k]*255) for k in 'rgba']))
   meshes.append({'object':oid,'file':file_ref['$ref'],'texture':texture_map[comp['texture']],'tint':int(comp['name'] in ['hair','mabuta']),'hair':int(comp['name']=='hair'),'vertices':vs})
 clips=[]
 for node in g.nodes:
  if node['type']!='AnimationNamedPart':continue
  name=node['fields']['name'];part=g.fields(node['fields']['part']);tracks=g.tracks(part);count=int(math.ceil(part['endTime']-part['startTime']))+1;frames=[]
  file_tracks=[g.node(g.fields(r)['track']) for r in g.array(part['fileAnimationTracks'])]
  for frame in range(count):
   time=part['startTime']+frame;transforms={};visibility={}
   for oid in order:
    props={k:g.value(v) for k,v in g.properties(oid).items() if k in ['pos','rotZ','scale','visibility']}
    for key,track in tracks.get(oid,{}).items():props[key]=g.sample(track,time)
    p=parents[oid];transforms[oid]=mul(transforms[p],matrix(props)) if p>=0 else matrix(props);visibility[oid]=bool(props.get('visibility',True)) and visibility.get(p,True)
   swaps={}
   for tr in file_tracks:
    if tr['type']=='AnimationTrackFileSwitch':
     ref=g.sample(tr,time);buf=ref.get('buffer') if isinstance(ref,dict) else None
     if buf:swaps[tr['fields']['file']['$ref']]=texture_map[buf['$ref']]
   frames.append([(transforms[m['object']],int(visibility[m['object']]),swaps.get(m['file'],m['texture'])) for m in meshes])
  clips.append((name,frames))
 payload=bytearray(struct.pack('>4I',len(meshes),len(textures),len(clips),next((i for i,t in enumerate(textures) if t[3].replace('\\','/').endswith('pe_head01.tga')),0)))
 for m in meshes:
  payload.extend(struct.pack('>4I',len(m['vertices']),m['texture'],m['tint'],m['hair']))
  for v in m['vertices']:payload.extend(struct.pack('>4f4B',*v))
 for name,frames in clips:
  title=name.encode('ascii');payload.extend(struct.pack('>I',len(title))+title+struct.pack('>If',len(frames),30))
  for frame in frames:
   for transform,visible,tex in frame:payload.extend(struct.pack('>6f2I',*transform,visible,tex))
 for w,h,raw,name in textures:
  payload.extend(struct.pack('>3I',w,h,len(raw)));payload.extend(bytes((-20-len(payload))%32));payload.extend(raw)
 save(output,b'LRCCANI1',payload)
 return {'clips':[{'name':n,'frames':len(f)} for n,f in clips],'meshes':len(meshes),'textures':len(textures),'bytes':len(payload)+20}

def build(world_dir,char_dir,import_root,char_source,app,mui_dir=None,world_source=None):
 out=Path(app)/'assets/animations'
 report={'timebase':'Native curve samples at nominal 30 Hz; velocity_rate applied to world objects',
         'world':world(world_dir,out/'world.lrcanim'),'character':character(char_dir,import_root,char_source,out/'kulche.lrcanim')}
 if mui_dir is not None:report['muimui']=character(mui_dir,import_root,world_source,out/'muimui.lrcanim')
 (out/'manifest.json').write_text(json.dumps(report,indent=2),encoding='utf8');print('Native animation banks:',len(report['world']),'world tracks,',len(report['character']['clips']),'Kulche clips',flush=True)
 return report
