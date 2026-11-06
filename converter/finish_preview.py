"""Finish a native PSP import into the complete Wii preview directory."""
from pathlib import Path
import json,shutil,subprocess
from psp_formats import FormatError

def emit(message):print(message,flush=True)

def finish(output,resources,home,node):
    output=Path(output);RESOURCES=Path(resources);HOME=Path(home)
    report=json.loads((output/'import-report.json').read_text(encoding='utf8'))
    source_hash=report['source_sha256']
    report['wii_ready']=False
    (output/'wii-preview-ready.json').unlink(missing_ok=True)
    (output/'import-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
    level_index=json.loads((output/'blv-textures.json').read_text(encoding='utf-8'))
    levels=[x for x in level_index['levels'] if 'st_flower01.dlk' in x['names']]
    if len(levels)!=1:raise FormatError('Could not uniquely identify st_flower01 in this PSP copy.')
    map_dir=output/'map-export/st_flower01'
    emit('Converting the original PSP map to a Wii data file...')
    subprocess.run([str(node),'--disable-warning=ExperimentalWarning','--experimental-transform-types',
        str(RESOURCES/'native_parser/export_map.mjs'),str(output/levels[0]['source']),str(map_dir)],check=True)
    from build_native_level import build
    map_file=output/'apps/LocoRocoWii/assets/maps/st_flower01.lrcmap'
    map_report=build(map_dir,output,map_file)
    from native_world_events import build as build_events
    report['world_events']=build_events(map_dir,output/'apps/LocoRocoWii/assets/events/st_flower01.lrcevt')
    report['map_ready']=True
    report['map_file']=map_file.relative_to(output).as_posix()
    report['map_format']=map_report['map_format']
    emit('Generating graphics, sound effects, and music from PSP files...')
    from preview_assets import build as build_assets
    app=output/'apps/LocoRocoWii'
    build_assets(output,app,RESOURCES,HOME/'tools')
    chars=[x for x in level_index['levels'] if x['source'].replace('\\','/').endswith('/petton.blv')]
    if len(chars)!=1:raise FormatError('Could not uniquely identify the Kulche animation bank.')
    char_dir=output/'map-export/kulche'
    subprocess.run([str(node),'--disable-warning=ExperimentalWarning','--experimental-transform-types',
        str(RESOURCES/'native_parser/export_map.mjs'),str(output/chars[0]['source']),str(char_dir)],check=True)
    mui_dir=output/'map-export/muimui'
    subprocess.run([str(node),'--disable-warning=ExperimentalWarning','--experimental-transform-types',
        str(RESOURCES/'native_parser/export_map.mjs'),str(output/levels[0]['source']),str(mui_dir),'muimui_jump.dlk'],check=True)
    from native_animations import build as build_animations
    animation_report=build_animations(map_dir,char_dir,output,chars[0]['source'],app,mui_dir,levels[0]['source'])
    report['native_animation_clips']=len(animation_report['character']['clips'])
    report['native_world_animations']=len(animation_report['world'])
    report['native_muimui_clips']=len(animation_report['muimui']['clips'])

    template=RESOURCES/'wii'
    for name in ('boot.dol','meta.xml'):
        if not (template/name).is_file():raise FormatError('Missing Wii engine file in converter: '+name)
        shutil.copy2(template/name,app/name)
    (app/'assets/locoroco.pak').unlink(missing_ok=True)
    report['wii_ready']=True
    report['scope']='World 1 native map events preview; Wii physics and presentation adaptations'
    report['legacy_pack_required']=False
    report['gameplay_verified']=False
    report['remaining']=['Some PSP presentation scene scripts and camera depth commands; original executable physics and global Loco House editor']
    (output/'wii-preview-ready.json').write_text(json.dumps({'app':app.relative_to(output).as_posix(),
        'source_sha256':source_hash,'scope':report['scope']},indent=2),encoding='utf8')
    (output/'import-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    emit(f'Wii map saved: {map_file}')
    return app

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('output',type=Path)
    p.add_argument('--node',type=Path,default=Path(__file__).resolve().parent/'tools/node.exe')
    args=p.parse_args();folder=Path(__file__).resolve().parent
    print(finish(args.output,folder,folder,args.node))
