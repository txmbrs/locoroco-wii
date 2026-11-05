"""Re-read a named level from the original ISO using a verified import index."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
from psp_formats import Disc,gprs,sfo,FormatError
from convert import fingerprint,validate_title

def extract(source,index,output,name,node):
    index=Path(index);output=Path(output);source=Path(source)
    report=json.loads((index/'import-report.json').read_text(encoding='utf-8'))
    levels=json.loads((index/'blv-textures.json').read_text(encoding='utf-8'))['levels']
    candidates=[level for level in levels if name+'.dlk' in level['names']]
    if len(candidates)!=1:raise FormatError(f'Expected one original level {name}, found {len(candidates)}')
    member=next(item for item in report['files'] if item['path']==candidates[0]['source'])
    print('Sprawdzanie sumy oryginalnego ISO...',flush=True)
    checksum=fingerprint(source)
    if checksum!=report['source_sha256']:raise FormatError('ISO differs from the indexed original copy')
    archive,position=member['origin'].split('@');position=int(position,16)
    disc=Disc(source)
    try:
        files={p.upper():(p,offset,size) for p,offset,size in disc.entries()}
        _,offset,size=files['PSP_GAME/PARAM.SFO'];info=sfo(disc.read(offset,size));validate_title(info)
        path,base,size=files['PSP_GAME/USRDIR/DATA/'+archive.upper()]
        # The compressed member is smaller than 16 MiB in the indexed retail copy.
        raw=disc.read(base+position,min(16*1024*1024,size-position))
        payload,consumed=gprs(raw)
    finally:disc.close()
    payload_hash=hashlib.sha256(payload).hexdigest()
    if payload_hash!=member['sha256']:raise FormatError('Extracted level checksum differs from index')
    output.mkdir(parents=True,exist_ok=True)
    clv=output/(name+'.clv');clv.write_bytes(payload)
    (output/(name+'.gprs')).write_bytes(raw[:consumed])
    provenance={'name':name,'disc_id':info['DISC_ID'],'source_iso':source.name,'source_iso_sha256':checksum,
        'archive':path,'archive_offset':position,'iso_offset':base+position,'compressed_bytes':consumed,
        'expanded_bytes':len(payload),'clv_sha256':payload_hash,
        'origin':'Direct ISO read and GPRS decompression; no SVG or reference-image reconstruction'}
    (output/'provenance.json').write_text(json.dumps(provenance,indent=2),encoding='utf-8')
    parser=Path(__file__).resolve().parent/'native_parser/export_map.mjs'
    subprocess.run([str(node),'--disable-warning=ExperimentalWarning','--experimental-transform-types',str(parser),str(clv),str(output)],check=True)
    print(output.resolve(),flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('source',type=Path)
    parser.add_argument('--index',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--name',default='st_flower01')
    parser.add_argument('--node',type=Path,default=Path(shutil.which('node') or Path(__file__).resolve().parent/'tools/node.exe'))
    args=parser.parse_args();extract(args.source,args.index,args.output,args.name,args.node)
