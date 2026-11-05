"""Create all preview sprite/audio files from one local PSP import.
No developer PNG, PCM, old C++ asset headers or legacy pack is read.
"""
from pathlib import Path
import json,struct,hashlib,subprocess,wave
from PIL import Image,ImageOps,ImageDraw,ImageFont
import numpy as np
from native_audio import prepare as prepare_music
from native_events import render

def fnv(data):
 value=2166136261
 for byte in data:value=((value^byte)*16777619)&0xffffffff
 return value

def build(root,app,resources,tools):
 root=Path(root);app=Path(app);resources=Path(resources);tools=Path(tools)
 assets=app/'assets';(assets/'sprites').mkdir(parents=True,exist_ok=True);(assets/'audio').mkdir(exist_ok=True)
 work=root/'work/preview-audio';work.mkdir(parents=True,exist_ok=True)
 blvs=json.loads((root/'blv-textures.json').read_text(encoding='utf8'))['textures']
 timps=json.loads((root/'textures.json').read_text(encoding='utf8'))['images']
 recipe=json.loads((resources/'texture_recipe.json').read_text(encoding='utf8'))
 manifest={'format':1,'sprites':{},'audio':{},'source':'User PSP ROM import','legacy_fallback':False,'ui':'Wii help labels generated with bundled font; game imagery from ROM'}
 def basename(name):return name.replace('\\','/').split('/')[-1].lower()
 def native(name):
  choices=[t for t in blvs if basename(t['sourceName'])==name.lower()]
  if not choices:
   images=[t for t in timps if basename(t['source'])==name.lower() and t['palette']==0]
   if len(images)!=1:raise ValueError('Missing or ambiguous original PSP texture '+name)
   return Image.open(root/images[0]['png']).convert('RGBA'),images[0]
  # Prefer Kulche's own character bank when textures are shared with other characters.
  choices.sort(key=lambda t:('petton.blv' not in t['source'],t['source']))
  t=choices[0];im=Image.frombytes('RGBA',(t['width'],t['height']),(root/t['rgba']).read_bytes());return im,t
 def load(index):
  rec=recipe[str(index)]
  if rec['kind']=='BLV':return native(basename(rec['sourceName']))
  choices=[t for t in timps if basename(t['source'])==basename(rec['source']) and t['palette']==0]
  if not choices:raise ValueError('Missing TIMP '+rec['source'])
  t=choices[0];return Image.open(root/t['png']).convert('RGBA'),t
 def tex(index):return ImageOps.flip(load(index)[0])
 def fit(im,w,h):return im.resize((w,h),Image.Resampling.LANCZOS)
 def crop(im):
  box=im.getbbox()
  if not box:raise ValueError('Empty original texture')
  return im.crop(box)
 def save(name,im,source):
  w=(im.width+3)//4*4;h=(im.height+3)//4*4;pad=Image.new('RGBA',(w,h));pad.alpha_composite(im)
  raw=bytearray()
  for y in range(0,h,4):
   for x in range(0,w,4):
    ps=[pad.getpixel((x+px,y+py)) for py in range(4) for px in range(4)]
    raw.extend(v for r,g,b,a in ps for v in (a,r));raw.extend(v for r,g,b,a in ps for v in (g,b))
  path=assets/'sprites'/(name+'.gx');path.write_bytes(b'LRCSPR01'+struct.pack('>4I',w,h,len(raw),fnv(raw))+raw)
  manifest['sprites'][name]={'source':source,'width':w,'height':h,'sha256':hashlib.sha256(raw).hexdigest()}
 def label(lines,size=28,canvas=(512,128)):
  im=Image.new('RGBA',canvas);d=ImageDraw.Draw(im);font=ImageFont.load_default(size=size)
  for y,line in zip(np.linspace(25,canvas[1]-30,len(lines)),lines):d.text((canvas[0]//2,int(y)),line,font=font,anchor='mm',fill=(137,138,116,255))
  return im
 # Kulche and the world MuiMui use native mesh/animation banks, not composed face sprites.
 for obsolete in ['PspSprites.'+n for n in ('EYES_IDLE','EYES_LOOK','EYES_SQUINT','MOUTH_SMILE','MOUTH_OPEN','TUFT_BIG')]+['SceneSprites.MUI_NPC']:
  (assets/'sprites'/(obsolete+'.gx')).unlink(missing_ok=True)
 pickie=tex(20);body=Image.new('RGBA',pickie.size);wings=Image.new('RGBA',pickie.size);bp=[];wp=[]
 for r,g,b,a in pickie.getdata():
  wing=a>0 and min(r,g,b)>130 and max(r,g,b)<230 and max(r,g,b)-min(r,g,b)<45
  bp.append((r,g,b,0 if wing else a));wp.append((r,g,b,a if wing else 0))
 body.putdata(bp);wings.putdata(wp)
 save('SceneSprites.PICKIE_BODY',body,load(20)[1]);save('SceneSprites.PICKIE_WINGS',wings,load(20)[1])
 for name,index in [('FRUIT',92),('WIND_PARTICLES',58),('MUI_EGG',29)]:save('SceneSprites.'+name,tex(index),load(index)[1])
 save('SceneSprites.HUD_HEAD',load(17)[0].crop((0,0,24,24)),load(17)[1])
 # The native ico_number strip stores ten decimal digits, 12x16 each.
 digits=load(11)[0] if str(11) in recipe and basename(recipe[str(11)].get('sourceName',''))=='ico_number.tip' else native('ico_number.tip')[0]
 if digits.width<120 or digits.height<16:raise ValueError('Unsupported native number atlas')
 for i in range(10):save('SceneSprites.HUD_DIGIT'+str(i),fit(digits.crop((i*12,0,i*12+12,16)),16,24),['ico_number.tip'])
 hair=fit(crop(tex(79)),32,46);torso=fit(crop(tex(85)),80,80);atlas=tex(97)
 for smile in (False,True):
  im=Image.new('RGBA',(80,128));im.alpha_composite(torso,(0,48));im.alpha_composite(hair,(24,4))
  im.alpha_composite(fit(crop(ImageOps.flip(native('muimui_newmouseopen.tip' if not smile else 'muimui_newmouseclose.tip')[0])),18,10),(31,80))
  save('SceneSprites.'+('MUI_SMILE' if smile else 'MUI'),im,[load(79)[1],load(85)[1],'muimui_newmouse*.tip'])
 # The PSP stores separate localized variants. Use its English texture, flipped
 # vertically just like the other decoded PSP textures; choose this level's bank.
 level_index=json.loads((root/'blv-textures.json').read_text(encoding='utf8'))['levels']
 level_sources=[v['source'] for v in level_index if 'st_flower01.dlk' in v['names']]
 if len(level_sources)!=1:raise ValueError('Missing World 1 for localized dialogue')
 def level_text(name):
  choices=[v for v in blvs if v['source']==level_sources[0] and basename(v['sourceName'])==name.lower()]
  if not choices or len({(v['rgba'],v['width'],v['height']) for v in choices})!=1:raise ValueError('Missing or ambiguous original English dialogue '+name)
  t=choices[0];return ImageOps.flip(Image.frombytes('RGBA',(t['width'],t['height']),(root/t['rgba']).read_bytes())),t
 for i,name in enumerate(['serifu_mui-en.tip','serifu_mi-en.tip','serifu_mazasou-en.tip','serifu_start-en.tip']):
  im,origin=level_text(name);save('SceneSprites.TEXT'+str(i),im,origin)
 im,origin=level_text('serifu_muimui1-en.tip');save('SceneSprites.MUI_REWARD_TEXT',im,origin)
 for i,name in enumerate(['serifu_lkar-en.tip','serifu_ltor-en.tip','serifu_ltor2-en.tip','serifu_tyonmaru-en.tip','serifu_nagamaru-en.tip','serifu_goal-en.tip']):
  im,origin=level_text(name);save('SceneSprites.HINT'+str(i+1),im,origin)
 save('SceneSprites.HOUSE_PART_TITLE',label(['Loco House'],38,(512,64)),{'kind':'Generated text'})
 # Original grant artwork; no fabricated numbered House Part.
 save('SceneSprites.HOUSE_PART9',fit(native('muiget_locohouse.tip')[0],128,144),['muiget_locohouse.tip'])
 print('Native graphics generated:',len(manifest['sprites']),flush=True)
 decoder=tools/'vgmstream/vgmstream-cli.exe'
 if not decoder.exists():raise ValueError('Missing bundled vgmstream decoder')
 bank_files=list((root/'native').rglob('yel_locoroco.sgd'))
 if len(bank_files)!=1:raise ValueError('Expected one original Kulche audio bank')
 for key,event in [('BITE','yel_EatFruit'),('JUMP','jump01_1'),('SPLIT','yel_split01'),('JOIN','yel_join'),('CALL1','GrpCall_1'),('CALL2','GrpCall_2'),('BOUNCE','Tram01')]:
  print('Audio event:',event,flush=True);raw,trace=render(bank_files[0],event,work,decoder)
  name='LocoEvents.'+key;path=assets/'audio'/(name+'.pcm');path.write_bytes(b'LRCAUD01'+struct.pack('>4I',32000,2,len(raw),fnv(raw))+raw);manifest['audio'][name]=trace
 from native_world_events import SOUNDS
 prototypes=list((root/'native').rglob('petton_prototype.sgd'))
 if len(prototypes)!=1:raise ValueError('Missing original world sound bank')
 for event in SOUNDS:
  print('World audio:',event,flush=True);raw,trace=render(prototypes[0],event,work,decoder)
  name='WorldEvents.'+event;path=assets/'audio'/(name+'.pcm');path.write_bytes(b'LRCAUD01'+struct.pack('>4I',32000,2,len(raw),fnv(raw))+raw);manifest['audio'][name]=trace
 # Original clear jingle: SGXD header plus the paired disc stream.
 headers=list((root/'native').rglob('clear_jingle.sgh'));streams=list((root/'work/disc-data').rglob('clear_jingle.sgb'))
 if not streams:
  report=json.loads((root/'import-report.json').read_text(encoding='utf8'));data=Path(report.get('data_archive','')).parent;streams=list(data.rglob('clear_jingle.sgb'))
 if len(headers)!=1 or len(streams)!=1:raise ValueError('Missing original clear jingle stream')
 sgh=work/'clear_jingle.sgh';sgh.write_bytes(headers[0].read_bytes());sgh.with_suffix('.sgb').write_bytes(streams[0].read_bytes())
 sgd=work/'clear_jingle.sgd';sgd.write_bytes(sgh.read_bytes()+sgh.with_suffix('.sgb').read_bytes())
 wav=work/'clear.wav';subprocess.run([str(decoder),'-i','-o',str(wav),str(sgd)],check=True,stdout=subprocess.DEVNULL)
 with wave.open(str(wav)) as w:
  rate=w.getframerate();channels=w.getnchannels();audio=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,channels).astype(np.float32)
 target=np.arange(round(len(audio)*32000/rate))*rate/32000
 data=np.column_stack([np.interp(target,np.arange(len(audio)),audio[:,min(c,channels-1)]) for c in range(2)]).astype('<i2').tobytes();data+=bytes((-len(data))%32)
 (assets/'audio/LocoAudioData.JINGLE_CLEAR.pcm').write_bytes(b'LRCAUD01'+struct.pack('>4I',32000,2,len(data),fnv(data))+data)
 manifest['audio']['LocoAudioData.JINGLE_CLEAR']={'header':headers[0].relative_to(root).as_posix(),'stream':str(streams[0]),'sha256':hashlib.sha256(data).hexdigest()}
 print('Rendering original flower theme and four voice parts...',flush=True)
 manifest['music']=prepare_music(root,app,work,decoder)
 report=json.loads((root/'import-report.json').read_text(encoding='utf8'));manifest['disc']=report['disc'];manifest['source_sha256']=report['source_sha256']
 (assets/'preview-assets.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
 print('Complete preview assets generated:',app,flush=True)
 return manifest

if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--import-dir',required=True,type=Path);p.add_argument('--app',required=True,type=Path)
 p.add_argument('--tools',type=Path,default=Path(__file__).resolve().parent/'tools')
 args=p.parse_args();build(args.import_dir,args.app,Path(__file__).resolve().parent,args.tools)
