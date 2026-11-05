"""Decode local importer output to PNG; extracted pictures are private data."""
from pathlib import Path
import argparse
import hashlib
import json
import struct
from psp_formats import timp,png_bytes,FormatError

def decode(root):
    root=Path(root);images=[];unsupported=[]
    for path in sorted((root/'native').rglob('*.tip')):
        try:
            raw=path.read_bytes();banks=max(1,struct.unpack_from('<H',raw,22)[0])
            if banks>256:raise FormatError('Too many TIMP palette banks')
            for bank in range(banks):
                w,h,rgba=timp(raw,bank)
                output=root/'textures'/path.relative_to(root/'native').with_suffix('.png' if bank==0 else f'.p{bank}.png')
                output.parent.mkdir(parents=True,exist_ok=True);output.write_bytes(png_bytes(w,h,rgba))
                images.append({'source':path.relative_to(root).as_posix(),'png':output.relative_to(root).as_posix(),
                               'width':w,'height':h,'palette':bank,'rgba_sha256':hashlib.sha256(rgba).hexdigest()})
        except (FormatError,ValueError,IndexError) as error:
            unsupported.append({'source':path.relative_to(root).as_posix(),'error':str(error)})
    report={'images':images,'unsupported':unsupported}
    (root/'textures.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(f'PNG: {len(images)}, unsupported TIMP variants: {len(unsupported)}',flush=True)
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('root',type=Path);decode(parser.parse_args().root)
