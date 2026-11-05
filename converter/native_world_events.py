"""Compile first-level root signal wiring without mixing embedded SubRoot scopes."""
import json,struct,math
from pathlib import Path
from native_animations import Graph,matrix,mul,save
KINDS=['muimui','hakarisw_a','area_trigger','moving','kakushi','delay','dummy','switch','switch1kai','event_item','kcoin_item','fukidashi','soundeffect','oneshot_se','camera_offset','tutorialmuimui','fixed_camera']
INPUTS=['in_start','in_restart','in_stop','in_backward','in_reverse','in_collisionoff','in_collisionon','in_toumei','in_hyouji','in_clear','in_signal','in_on','in_off','in_finish']
SOUNDS=['seikai01','Fatks','errer01','Switch_Scale','Door_L_LP','Door_L','muimui_voice01','button05','kakushi_in01','kakushi_out01']
OUTPUTS=['out_start','out_finish','out_get','out_got','out_taken','out_tarinai','out_empty','out_signal']
def build(map_dir,output):
 g=Graph(Path(map_dir)/'graph.json');obs={o['id']:o for o in json.loads((Path(map_dir)/'objects.json').read_text())['objects']};defs=[];indices={};props={}
 def walk(oid,parent=(1,0,0,1,0,0)):
  o=obs[oid];p={k:g.value(v) for k,v in g.properties(oid).items()};m=mul(parent,matrix(p))
  if o['type'] in KINDS:
   indices[oid]=len(defs);props[oid]=p;b=p.get('shori_box',p.get('bounding_box',dict(minX=-5,minY=0,maxX=5,maxY=10)))
   corners=[]
   for x,y in [(b['minX'],b['minY']),(b['maxX'],b['minY']),(b['maxX'],b['maxY']),(b['minX'],b['maxY'])]:corners += [(m[0]*x+m[2]*y+m[4])*6,-(m[1]*x+m[3]*y+m[5])*6]
   values=[m[4]*6,-m[5]*6,float(p.get('delay',0))/30,float(p.get('kazu',1)),float(p.get('muimui_no',p.get('item_id',0))),float(p.get('toumeitime',30))/30,float(bool(p.get('startoff',False))),float(bool(p.get('all',False))),*corners]
   if o['type']=='tutorialmuimui':
    mapping={'tutorialmuimui_katamuku':1,'tutorialmuimui_jump':2,'tutorialmuimui_jump2':2,'tutorialmuimui_jump3':3,'tutorialmuimui_bunretsu':4,'tutorialmuimui_gattai':5,'tutorialmuimui_goal':6}
    values[4]=mapping.get(o['name'].split('|')[-1],0)
   if o['type'] in ('soundeffect','oneshot_se'):
    sound=p.get('sound',{}).get('value','');values[4]=SOUNDS.index(sound)+1 if sound in SOUNDS else 0
   if o['type']=='camera_offset':
    offset=p.get('offset',{});values[8:13]=[offset.get('x',0)*6,-offset.get('y',0)*6,offset.get('z',0),p.get('duration_start',0)/30,p.get('duration_stop',0)/30]
   defs.append((oid,KINDS.index(o['type']),values))
  # Reference objects own another signal namespace. Their embedded scenes are not root routes.
  if o['type']=='reference':
   if o['name'].endswith('reference_muimui'):
    subprop=g.properties(oid).get('object')
    if subprop and g.node(subprop)['type']=='SubRoot':
     pending=[(g.fields(subprop)['object']['$ref'],m)];visited=set()
     while pending:
      subid,base=pending.pop()
      if subid in visited or subid not in obs:continue
      visited.add(subid);inner=obs[subid];ip={k:g.value(v) for k,v in g.properties(subid).items()};world=mul(base,matrix(ip))
      if inner['type']=='fixed_camera':
       v=[0.0]*16;v[0]=world[4]*6;v[1]=-world[5]*6;v[2]=float(ip.get('duration_start',30))/30;v[3]=float(ip.get('orthographicWidth',0))*6
       if v[3]>0:defs.append((subid,KINDS.index('fixed_camera'),v))
      pending.extend((c,world) for c in inner['children'])
   return
  for child in o['children']:walk(child,m)
 root=g.fields(g.nodes[0]['fields']['subRoot']);walk(root['object']['$ref'])
 signals=g.array(g.fields(root['signalList'])['signals']);routes=[];outs=[];unhandled=[]
 for signal,ref in enumerate(signals):
  f=g.fields(ref);oid=f['object']['$ref'];name=f['signalName1']
  if oid in indices and name in INPUTS:routes.append((signal,indices[oid],INPUTS.index(name)))
 # Follow reference junctions into their own signal namespace only when their
 # destination is a native sound. This preserves secret entrance/exit sounds.
 for signal,sigref in enumerate(signals):
  sig=g.fields(sigref);oid=sig['object']['$ref'];obj=obs.get(oid)
  if not obj or obj['type']!='reference' or not sig['signalName1'].startswith('ein_'):continue
  tail=sig['signalName2'].split(':')[-1]
  if not tail.startswith('ein_'):continue
  prop=g.properties(oid).get('object')
  if not prop or g.node(prop)['type']!='SubRoot':continue
  sub=g.fields(prop);local=g.array(g.fields(sub['signalList'])['signals']) if sub.get('signalList') else []
  todo=[sub['object']['$ref']];seen=set()
  while todo:
   child=todo.pop()
   if child in seen or child not in obs:continue
   seen.add(child);node=obs[child];todo.extend(node['children'])
   if node['type']!='junction':continue
   for out in g.value(g.properties(child).get('out_signals',[])):
    if out.get('propertyName')!='eout_'+tail[4:]:continue
    for local_id in g.value(out['signalIds']):
     if not isinstance(local_id,int) or not 0<=local_id<len(local):continue
     destination=g.fields(local[local_id]);sid=destination['object']['$ref'];soundobj=obs.get(sid)
     if not soundobj or soundobj['type'] not in ('soundeffect','oneshot_se'):continue
     pp={k:g.value(v) for k,v in g.properties(sid).items()};sound=pp.get('sound',{}).get('value','')
     if sound not in SOUNDS or destination['signalName1']!='in_start':continue
     values=[0.0]*16;values[2]=float(pp.get('delay',0))/30;values[4]=SOUNDS.index(sound)+1
     ni=len(defs);defs.append((sid,KINDS.index(soundobj['type']),values));routes.append((signal,ni,0))
 for oid,i in indices.items():
  for name in OUTPUTS:
   for signal in props[oid].get(name,[]):
    if not isinstance(signal,int) or signal<0 or signal>=len(signals):raise ValueError('Invalid root signal reference')
    outs.append((i,OUTPUTS.index(name),signal))
    if not any(r[0]==signal for r in routes):unhandled.append({'source':oid,'output':name,'destination':g.fields(signals[signal])['signalName2']})
 payload=bytearray(struct.pack('>III',len(defs),len(routes),len(outs)))
 for oid,kind,values in defs:payload.extend(struct.pack('>ii16f',oid,kind,*values))
 for r in routes+outs:payload.extend(struct.pack('>iii',*r))
 save(output,b'LRCEVT01',payload)
 report={'nodes':len(defs),'input_routes':len(routes),'output_routes':len(outs),'muimui':sum(d[1]==0 for d in defs),'weight_required':[d[2][3] for d in defs if d[1]==1],'unhandled_presentation_routes':unhandled,'timebase':'Nominal 30 Hz; executable handler timings not recovered'}
 Path(output).with_suffix('.json').write_text(json.dumps(report,indent=2));return report
