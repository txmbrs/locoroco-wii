"""Render named SGXD sound events from original imported banks.
Envelope/controller opcodes are recorded; this is an approximate sequencer.
"""
from pathlib import Path
import struct,json,subprocess,wave,hashlib
import numpy as np
RATE=32000

def bank(path):
    path=Path(path);b=path.read_bytes();chunks={};p=16
    while p<struct.unpack_from('<I',b,8)[0]:
        chunks[b[p:p+4]]=p;p+=8+struct.unpack_from('<I',b,p+4)[0]
    p=chunks[b'SEQD'];sequences={}
    for group in range(struct.unpack_from('<I',b,p+12)[0]):
        g=struct.unpack_from('<I',b,p+16+4*group)[0]
        if not g:continue
        for i in range(struct.unpack_from('<I',b,g+4)[0]):
            q=struct.unpack_from('<I',b,g+8+4*i)[0]
            if not q:continue
            _,off,fmt,_,_,_,size=struct.unpack_from('<IIHHHHI',b,q)
            title=b[off:b.find(b'\0',off)].decode('ascii','replace') if off else ''
            sequences[title]=(group,i,b[q+20:q+20+size])
    return path,b,chunks,sequences
def render(bank_path,event,work,decoder):
    WORK=Path(work);WORK.mkdir(parents=True,exist_ok=True);bank_name=Path(bank_path).stem
    path,b,chunks,seqs=bank(bank_path);group,seq,data=seqs[event]
    pos=0;time=0;notes=[];active={};ignored=[]
    def val(default=0):
        nonlocal pos
        if pos>=len(data) or data[pos]>=0xa0:return default
        tag=data[pos];pos+=1;typ=tag>>4;n=tag&15
        if typ==2:
            low,high,step=val(),val(),val();return low
        raw=data[pos:pos+n];pos+=n
        if not n:return default
        number=int(raw.hex());return -number if typ==1 else number
    arity={0xc0:6,0xc1:3,0xc2:4,0xc3:11,0xb0:4,0xb1:2,0xb2:2,0xd1:2,0xd2:2}
    while pos<len(data):
        command=data[pos];pos+=1
        if command==0xff:break
        if command==0xd0:
            ident,program,key,volume=val(),val(),val(),val(4096)
            notes.append(dict(start=time,program=program,key=key,volume=volume,id=ident));active[ident]=notes[-1]
        elif command==0xf1:time+=val()/1000
        elif command in arity:
            args=[val() for _ in range(arity[command])]
            if command in (0xd1,0xd2) and args[0] in active:active[args[0]]['stop']=time
            else:ignored.append(hex(command))
        else:raise ValueError(f'{event}: unsupported opcode {command:02x} at {pos-1}')
    assert notes,f'No sample starts in {event}'
    mix=np.zeros((RATE*10,2),np.float32);end=0;trace=[];cache={}
    for note in notes:
        p=chunks[b'RGND'];count,offset=struct.unpack_from('<II',b,p+16+8*note['program'])
        matching=[offset+i*56 for i in range(count) if b[offset+i*56+24]<=note['key']<=b[offset+i*56+25]]
        assert matching,(event,note)
        for q in matching:
            sid=struct.unpack_from('<I',b,q+52)[0]+1
            if sid not in cache:
                out=WORK/f'{bank_name}-{sid}.wav'
                subprocess.run([str(decoder),'-i','-s',str(sid),'-o',str(out),str(path)],check=True,stdout=subprocess.DEVNULL)
                with wave.open(str(out)) as w:cache[sid]=(np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,w.getnchannels()).astype(np.float32)/32768,w.getframerate())
            sample,rate=cache[sid];fine=struct.unpack_from('b',b,q+29)[0]
            pitch=2**((note['key']-b[q+28]+fine/100)/12)
            length=round(len(sample)/rate/pitch*RATE)
            if 'stop' in note:length=min(length,max(0,round((note['stop']-note['start'])*RATE)))
            first=round(note['start']*RATE);length=min(length,len(mix)-first)
            if length<=0:continue
            positions=np.arange(length)*rate*pitch/RATE
            audio=np.column_stack([np.interp(positions,np.arange(len(sample)),sample[:,c]) for c in range(sample.shape[1])])
            if audio.shape[1]==1:audio=np.repeat(audio,2,axis=1)
            gain=struct.unpack_from('<H',b,q+32)[0]/4096*note['volume']/4096
            pan=np.clip(struct.unpack_from('<h',b,q+34)[0]/1024,-1,1)
            audio*=np.array([min(1,1-pan),min(1,1+pan)])*gain
            ramp=min(128,length//2)
            if ramp:audio[:ramp]*=np.linspace(0,1,ramp)[:,None];audio[-ramp:]*=np.linspace(1,0,ramp)[:,None]
            mix[first:first+length]+=audio;end=max(end,first+length)
            trace.append(dict(sample=sid,start=note['start'],key=note['key'],root=b[q+28],pitch=pitch))
    mix=mix[:(end+7)//8*8];peak=np.max(np.abs(mix));mix*=min(0.85,0.95/max(peak,.01))
    raw=np.rint(np.clip(mix,-1,1)*32767).astype('<i2').tobytes();raw+=b'\0'*((-len(raw))%32)
    with wave.open(str(WORK/f'{event}.wav'),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(raw)
    return raw,dict(bank=bank_name,event=event,group=group,sequence=seq,samples=trace,ignored_controls=sorted(set(ignored)),bank_sha256=hashlib.sha256(b).hexdigest())