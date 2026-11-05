"""Bounded readers for local LocoRoco PSP copies; no game data in this module.

GARC/TIMP layout research: owodzeg/GARCExtract and owodzeg/libTIMP.
GPRS is a byte-oriented LZ stream with MSB-first control bits.
This implementation does not require emulator memory dumps.
"""
from __future__ import annotations
import io
import math
import struct
import zlib
from pathlib import Path, PurePosixPath

class FormatError(ValueError):
    pass

def checked_slice(data, offset, size):
    if offset < 0 or size < 0 or offset + size > len(data):
        raise FormatError('Offset outside input file')
    return data[offset:offset+size]

def safe_name(name):
    name=name.replace('\\','/')
    path=PurePosixPath(name)
    reserved={'CON','PRN','AUX','NUL',*(f'COM{i}' for i in range(1,10)),*(f'LPT{i}' for i in range(1,10))}
    if not path.parts or path.is_absolute() or any(p in ('..','.') or ':' in p or '\x00' in p or
        p.rstrip(' .')!=p or p.split('.')[0].upper() in reserved for p in path.parts):
        raise FormatError(f'Unsafe archive path: {name!r}')
    return str(path)

def gprs(data, start=0, limit=128*1024*1024):
    if bytes(checked_slice(data,start,4)) != b'GPRS':
        raise FormatError('Not a GPRS stream')
    expected=struct.unpack_from('>I',data,start+4)[0]
    if expected<=0 or expected>limit:
        raise FormatError(f'Invalid GPRS expanded size: {expected}')
    pos=start+8; control=0; mask=0; out=bytearray()
    def byte():
        nonlocal pos
        if pos>=len(data):raise FormatError('Truncated GPRS stream')
        v=data[pos];pos+=1;return v
    def bit():
        nonlocal control,mask
        if not mask:control=byte();mask=128
        value=bool(control & mask);mask>>=1;return int(value)
    while True:
        if not bit():
            out.append(byte())
        else:
            if bit():
                offset=byte()-256
                for _ in range(4):offset=offset*2+bit()
                offset-=255
            else:
                raw=byte()
                if raw==0:break
                offset=raw-256
            length=1
            while bit():
                length=length*2+bit()
                if length>expected:raise FormatError('Invalid GPRS match length')
            length+=1
            if offset>=0 or -offset>len(out) or len(out)+length>expected:
                raise FormatError('Invalid GPRS back reference')
            distance=-offset
            while length:
                n=min(length,distance);source=len(out)-distance
                out.extend(out[source:source+n]);length-=n
        if len(out)>expected:raise FormatError('GPRS exceeds declared size')
    if len(out)!=expected:
        raise FormatError(f'GPRS size mismatch: {len(out)} / {expected}')
    return out,pos

def garc(data):
    if bytes(data[:4])!=b'GARC' or bytes(data[32:36])!=b'FILE':
        raise FormatError('Unsupported GARC header')
    count=struct.unpack_from('<I',data,40)[0]
    if count>100000 or 64+count*24>len(data):raise FormatError('Invalid GARC table')
    entries=[]
    for i in range(count):
        size,offset,name_at=struct.unpack_from('<III',data,64+i*24+4)
        if not 0<=name_at<len(data):raise FormatError('Invalid GARC filename offset')
        end=data.find(b'\x00',name_at,min(len(data),name_at+1024))
        if end<0:raise FormatError('Unterminated GARC filename')
        name=safe_name(bytes(data[name_at:end]).decode('utf-8','strict'))
        entries.append((name,bytes(checked_slice(data,offset,size))))
    return entries

class Disc:
    """Random access over uncompressed ISO or CSO v1; CSO read is block bounded."""
    def __init__(self,path):
        self.file=Path(path).open('rb');self.size=self.file.seek(0,2);self.file.seek(0)
        head=self.file.read(24);self.cso=None
        if head[:4]==b'CISO':
            _,header,total,block,version,align,_=struct.unpack('<4sIQIBB2s',head)
            if version!=1 or block!=2048 or header<24:raise FormatError('Only CSO v1 / 2048-byte blocks supported')
            count=(total+block-1)//block
            if total>4*1024**3:raise FormatError('Disc too large')
            self.file.seek(header);raw=self.file.read((count+1)*4)
            if len(raw)!=(count+1)*4:raise FormatError('Truncated CSO index')
            self.cso=(total,block,align,struct.unpack(f'<{count+1}I',raw));self.size=total
        if self.read(32768,7)!=b'\x01CD001\x01':raise FormatError('No ISO9660 primary volume')
    def close(self):self.file.close()
    def read(self,offset,size):
        if offset<0 or size<0 or offset+size>self.size:raise FormatError('ISO read outside image')
        if self.cso is None:
            self.file.seek(offset);out=self.file.read(size)
        else:
            _,block,align,index=self.cso;parts=[]
            for i in range(offset//block,(offset+size+block-1)//block):
                start=(index[i]&0x7fffffff)<<align;end=(index[i+1]&0x7fffffff)<<align
                self.file.seek(start)
                raw=self.file.read(block if index[i]&0x80000000 else end-start)
                if index[i]&0x80000000:parts.append(raw)
                else:
                    d=zlib.decompressobj(-15);decoded=d.decompress(raw,block+1)
                    if len(decoded)!=block or not d.eof:raise FormatError('Invalid CSO sector')
                    parts.append(decoded)
            out=b''.join(parts)[offset%block:offset%block+size]
        if len(out)!=size:raise FormatError('Truncated disc image')
        return out
    def entries(self):
        root=self.read(32768+156,34)
        pending=[('',struct.unpack_from('<I',root,2)[0],struct.unpack_from('<I',root,10)[0])];seen=set()
        while pending:
            prefix,lba,size=pending.pop()
            if (lba,size) in seen:continue
            seen.add((lba,size))
            if len(seen)>10000 or size>16*1024**2:raise FormatError('Invalid ISO directory tree')
            data=self.read(lba*2048,size);p=0
            while p<len(data):
                n=data[p]
                if n==0:p=(p//2048+1)*2048;continue
                record=checked_slice(data,p,n);p+=n
                if n<34 or 33+record[32]>n:raise FormatError('Invalid ISO directory record')
                raw=record[33:33+record[32]]
                if raw in (b'\0',b'\1'):continue
                name=raw.decode('ascii').split(';',1)[0]
                path=safe_name(prefix+name);at,length=struct.unpack_from('<I',record,2)[0],struct.unpack_from('<I',record,10)[0]
                if record[25]&128:raise FormatError('Multi-extent ISO files unsupported')
                if record[25]&2:pending.append((path+'/',at,length))
                else:yield path,at*2048,length

def sfo(data):
    if bytes(data[:4])!=b'\0PSF':raise FormatError('Invalid PARAM.SFO')
    keys,values,count=struct.unpack_from('<III',data,8)
    if count>256:raise FormatError('Invalid SFO table')
    result={}
    for i in range(count):
        key,fmt,length,maximum,offset=struct.unpack_from('<HHIII',data,20+i*16)
        end=data.find(b'\0',keys+key)
        if end<0:raise FormatError('Invalid SFO key')
        name=bytes(data[keys+key:end]).decode('ascii')
        raw=bytes(checked_slice(data,values+offset,length))
        result[name]=raw.rstrip(b'\0').decode('utf-8','replace') if fmt==0x204 else raw.hex()
    return result

def timp(data,palette_index=0):
    """Decode 4/8-bit indexed or RGBA32 swizzled TIMP to linear RGBA bytes."""
    if bytes(data[:4])!=b'TIMP':raise FormatError('Not TIMP')
    width,height=struct.unpack_from('<HH',data,18)
    palette,pixels=struct.unpack_from('<II',data,32)
    if width<1 or height<1 or width>4096 or height>4096:raise FormatError('Invalid TIMP dimensions')
    format_code=struct.unpack_from('<H',data,24)[0]
    colors={0x0403:16,0x0503:256,0x030b:0}.get(format_code)
    if colors is None:raise FormatError(f'Unsupported TIMP format: {format_code:#x}')
    if colors:
        banks=struct.unpack_from('<H',data,22)[0]
        if palette_index<0 or palette_index>=banks:raise FormatError('Invalid TIMP palette index')
        table=bytes(checked_slice(data,palette+palette_index*colors*4,colors*4));bpp=8 if colors==256 else 4
    else:table=None;bpp=32
    block_width=128//bpp;blocks=(width+block_width-1)//block_width
    padded_height=(height+7)//8*8
    raw=checked_slice(data,pixels,blocks*padded_height*16)
    result=bytearray(width*height*4)
    for y in range(height):
        for x in range(width):
            at=((y//8)*blocks+x//block_width)*128+(y%8)*16+(x%block_width)*bpp//8
            if bpp==32:color=raw[at:at+4]
            else:
                index=raw[at] if bpp==8 else (raw[at]>>(4*(x%2)))&15
                color=table[index*4:index*4+4]
            dest=(y*width+x)*4;result[dest:dest+4]=color
    return width,height,bytes(result)

def png_bytes(width,height,rgba):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    rows=b''.join(b'\0'+rgba[y*width*4:(y+1)*width*4] for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b'')
