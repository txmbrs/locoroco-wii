import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {readBlvBuffer,GameVersion,Buffer as TextureBuffer} from './blv.ts';
import {decodeTextureToRGBA} from './texture_format.ts';

const root=path.resolve(process.argv[2]);
const report=JSON.parse(fs.readFileSync(path.join(root,'import-report.json'),'utf8'));
const output=path.join(root,'blv-textures');fs.mkdirSync(output,{recursive:true});
const index={textures:[],levels:[],errors:[]};
for(const file of report.files){
    if(!file.path.endsWith('.blv') && !file.path.endsWith('payload.bin'))continue;
    const raw=fs.readFileSync(path.join(root,file.path));
    if(raw.length<16 || raw.readUInt32LE(0)!==0xffffffff || raw.readUInt32LE(4)!==0)continue;
    try{
        const graph=readBlvBuffer(GameVersion.Gold,raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength));
        const visited=new Set();const buffers=[];const names=new Set();const objects=[];
        function walk(obj){
            if(!obj || typeof obj!=='object' || visited.has(obj))return;
            visited.add(obj);
            if(obj instanceof TextureBuffer){buffers.push(obj);return;}
            if(ArrayBuffer.isView(obj))return;
            if(typeof obj.name==='string')names.add(obj.name);
            if(obj.constructor?.name==='ObjectInstance' || obj.constructor?.name==='Group')objects.push(obj);
            for(const value of Object.values(obj))walk(value);
        }
        walk(graph);
        index.levels.push({source:file.path,names:[...names],textures:buffers.length});
        for(const texture of buffers){
            if(!texture.mips.length)continue;
            const rgba=decodeTextureToRGBA(texture.textureFormat,texture.mips[0].data.data,texture.palette?.rawValues??null,texture.width,texture.height);
            const hash=createHash('sha256').update(rgba).digest('hex');
            const name=hash+'.rgba';const dest=path.join(output,name);
            if(!fs.existsSync(dest))fs.writeFileSync(dest,rgba);
            index.textures.push({source:file.path,offset:texture.fileOffset,sourceName:texture.sourceFilename,
                width:texture.width,height:texture.height,rgba:'blv-textures/'+name,sha256:hash});
        }
        console.log(`${file.path}: ${buffers.length} textures`);
    }catch(error){index.errors.push({source:file.path,error:String(error)});}
}
fs.writeFileSync(path.join(root,'blv-textures.json'),JSON.stringify(index,null,2));
console.log(`Native BLV textures: ${index.textures.length}; unsupported levels: ${index.errors.length}`);
