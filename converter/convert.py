"""Local PSP importer. Drop an ISO/CSO or PSP_GAME folder beside this file.

Never downloads game files. Extracted data stays in the user's output folder.
This importer does NOT claim raw PSP archives are a ready Wii data pack.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import mmap
import os
import shutil
import subprocess
import sys
import struct
import time
from pathlib import Path
from psp_formats import Disc, FormatError, garc, gprs, sfo, safe_name

HOME=Path(sys.executable).resolve().parent if getattr(sys,'frozen',False) else Path(__file__).resolve().parent
RESOURCES=Path(getattr(sys,'_MEIPASS',Path(__file__).resolve().parent))

def emit(message):print(message,flush=True)

def fingerprint(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as source:
        for block in iter(lambda:source.read(4*1024**2),b''):h.update(block)
    return h.hexdigest()

def select_source(argument):
    if argument:return Path(argument).resolve()
    candidates=[]
    for p in HOME.iterdir():
        if p.is_file() and p.suffix.lower() in ('.iso','.cso'):candidates.append(p)
        if p.is_dir() and p.name not in ('output','__pycache__','tools'):
            if p.name.upper()=='PSP_GAME' and (p/'PARAM.SFO').is_file():candidates.append(p.parent)
            elif (p/'PSP_GAME/PARAM.SFO').is_file():candidates.append(p)
    candidates=sorted(set(candidates))
    if len(candidates)!=1:
        raise FormatError('Place one game copy (.iso, .cso, or PSP_GAME folder) next to the converter. '
                          f'Found: {len(candidates)}. You can also drag a file onto CONVERT.bat.')
    return candidates[0]

def extract_disc(source,work):
    if source.is_dir():
        root=source if source.name.upper()!='PSP_GAME' else source.parent
        info=sfo((root/'PSP_GAME/PARAM.SFO').read_bytes())
        data=root/'PSP_GAME/USRDIR/data'
        if not data.is_dir():raise FormatError('Missing PSP_GAME/USRDIR/data')
        return info,data
    disc=Disc(source)
    try:
        entries=list(disc.entries());names={name.upper():(name,offset,size) for name,offset,size in entries}
        if 'PSP_GAME/PARAM.SFO' not in names:raise FormatError('This is not a PSP game image')
        _,offset,size=names['PSP_GAME/PARAM.SFO'];info=sfo(disc.read(offset,size))
        validate_title(info)
        selected=[x for x in entries if x[0].upper().startswith('PSP_GAME/USRDIR/DATA/')
                  and '/MOVIE/' not in x[0].upper() and '/SHARING/' not in x[0].upper()]
        for name,offset,size in selected:
            relative=name.split('/',3)[-1];target=work/'disc-data'/safe_name(relative)
            target.parent.mkdir(parents=True,exist_ok=True)
            # Never reuse a partially written previous extraction.
            temporary=target.with_name(target.name+'.partial')
            with temporary.open('wb') as out:
                for start in range(0,size,1024**2):out.write(disc.read(offset+start,min(1024**2,size-start)))
            temporary.replace(target)
        return info,work/'disc-data'
    finally:disc.close()

def validate_title(info):
    title=info.get('TITLE','');disc=info.get('DISC_ID','')
    if 'locoroco' not in title.lower().replace(' ','') or '2' in title:
        raise FormatError(f'Expected LocoRoco 1 for PSP; found {title!r} ({disc})')
    if not disc:raise FormatError('Missing game ID in PARAM.SFO')

def unpack_members(path,destination,manifest):
    with path.open('rb') as file, mmap.mmap(file.fileno(),0,access=mmap.ACCESS_READ) as data:
        offset=0;number=0
        while offset+8<=len(data):
            start=data.find(b'GPRS',offset)
            if start<0:break
            if start%2048:
                offset=start+4;continue
            emit(f'  {path.name}: archive {number+1}, offset 0x{start:x}')
            payload,end=gprs(data,start)
            folder=destination/f'{number:04d}'
            extract_payload(payload,folder,manifest,f'{path.name}@{start:x}')
            number+=1;offset=end
        if not number:
            if bytes(data[:4])==b'GARC':extract_payload(data,destination/'0000',manifest,path.name)
            else:raise FormatError(f'No recognized GPRS/GARC archive in {path.name}')

def extract_payload(data,folder,manifest,origin,depth=0):
    if depth>8:raise FormatError('Too many nested archives')
    if bytes(data[:4])!=b'GARC':
        # DATA.BIN also holds compressed standalone banks and stage bundles.
        folder.mkdir(parents=True,exist_ok=True);target=folder/'payload.bin'
        target.write_bytes(data)
        manifest.append({'path':target.as_posix(),'origin':origin,'name':target.name,'size':len(data),
                         'magic':bytes(data[:4]).hex(),'sha256':hashlib.sha256(data).hexdigest()})
        return
    entries=garc(data)
    for index,(name,raw) in enumerate(entries):
        target=folder/safe_name(name)
        target.parent.mkdir(parents=True,exist_ok=True)
        if target.exists() and target.read_bytes()!=raw:raise FormatError(f'Duplicate archive filename: {name}')
        target.write_bytes(raw)
        manifest.append({'path':target.as_posix(),'origin':origin,'name':name,'size':len(raw),
                         'magic':raw[:4].hex(),'sha256':hashlib.sha256(raw).hexdigest()})
        if raw[:4]==b'GPRS':
            nested,_=gprs(raw);extract_payload(nested,target.with_name(target.name+'.files'),manifest,origin+'/'+name,depth+1)
        elif raw[:4]==b'GARC':extract_payload(raw,target.with_name(target.name+'.files'),manifest,origin+'/'+name,depth+1)

def run(source,output):
    output=Path(output).resolve()
    protected=source if source.is_file() or source.name.upper()=='PSP_GAME' else source/'PSP_GAME'
    if output==protected or protected in output.parents:raise FormatError('Output overlaps the source')
    output.mkdir(parents=True,exist_ok=True)
    (output/'wii-preview-ready.json').unlink(missing_ok=True)
    work=output/'work';work.mkdir(exist_ok=True)
    hash_source=source if source.is_file() else (source if source.name.upper()=='PSP_GAME' else source/'PSP_GAME')/'USRDIR/data/DATA.BIN'
    source_hash=fingerprint(hash_source)
    marker=work/'source.json'
    if marker.exists() and json.loads(marker.read_text())['sha256']!=source_hash:
        raise FormatError('The output folder contains a different game copy. Choose a new folder with --output.')
    marker.write_text(json.dumps({'sha256':source_hash}),encoding='utf-8')
    emit(f'Game copy: {source.name}')
    info,data=extract_disc(source,work);validate_title(info)
    emit(f"Detected: {info['TITLE']} / {info['DISC_ID']}")
    manifest=[]
    # Only original archives, never the old developer .dec or texture dumps.
    archives=[p for p in data.iterdir() if p.is_file() and p.name.lower() in ('data.bin','first_eu.arc','first_us.arc','first_jp.arc')]
    if not any(p.name.lower()=='data.bin' for p in archives):raise FormatError('Missing DATA.BIN')
    for path in archives:unpack_members(path,output/'native'/path.name,manifest)
    for item in manifest:item['path']=str(Path(item['path']).relative_to(output)).replace('\\','/')
    report={'format':1,'disc':info,'source_name':source.name,'source_sha256':source_hash,
            'files':manifest,'data_archive':str(next(p for p in archives if p.name.lower()=='data.bin').resolve()),'wii_ready':False,
            'remaining':['External native map conversion pending',
                         'Complete native UI and audio conversion',
                         'PSP signal/event handlers in the Wii engine']}
    (output/'import-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    emit(f'Extracted {len(manifest)} PSP files to {output / "native"}.')
    from decode_textures import decode
    decode(output)
    node=HOME/'tools/node.exe'
    if not node.exists():
        found=shutil.which('node');node=Path(found) if found else None
    if node:
        emit('Reading textures embedded in BLV/CLV levels...')
        subprocess.run([str(node),'--disable-warning=ExperimentalWarning','--experimental-transform-types',
            str(RESOURCES/'native_parser/extract.mjs'),str(output)],check=True)
        from finish_preview import finish
        finish(output,RESOURCES,HOME,node)

    else:
        raise FormatError('Missing tools/node.exe. Extract the complete converter folder.')
    emit('PREVIEW CONVERSION COMPLETE. Copy output/apps/LocoRocoWii to your Wii storage.')
    return output

def main():
    parser=argparse.ArgumentParser(description='Local LocoRoco PSP importer; does not download game data.')
    parser.add_argument('source',nargs='?');parser.add_argument('--output',type=Path,default=HOME/'output')
    args=parser.parse_args()
    try:run(select_source(args.source),args.output)
    except (FormatError,OSError,ValueError,KeyError,IndexError,AssertionError,StopIteration,struct.error,subprocess.CalledProcessError) as error:
        emit(f'ERROR: {error}');return 1
    return 0

if __name__=='__main__':sys.exit(main())
