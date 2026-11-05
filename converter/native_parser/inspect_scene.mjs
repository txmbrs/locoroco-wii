import fs from 'node:fs';
import {readBlvBuffer,GameVersion,LocoObject,Mesh} from './blv.ts';
const input=process.argv[2],output=process.argv[3];
const raw=fs.readFileSync(input);
const graph=readBlvBuffer(GameVersion.Gold,raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength));
const seen=new Set(),objects=[];
function walk(object){
    if(!object || typeof object!=='object' || seen.has(object) || ArrayBuffer.isView(object))return;
    seen.add(object);
    if(object instanceof LocoObject){
        const properties=object.properties.map(p=>({name:p.name,typeId:p.typeId,valueType:p.value?.constructor?.name,
            value: typeof p.value==='number' || typeof p.value==='boolean' ||
                ['Vec2','Vec3'].includes(p.value?.constructor?.name)?p.value:undefined,
            data:p.value instanceof Mesh?{meshComponents:p.value.meshComponents.map(c=>({
                material:{name:c.material.name,color:c.material.color,file:c.material.file?{
                    name:c.material.file.name,offset:c.material.file.buffer.fileOffset,
                    uScaling:c.material.file.uScaling,vScaling:c.material.file.vScaling}:null},
                vertices:c.vertices.map(v=>({type:v.type,data:v.data}))}))}:undefined}));
        objects.push({name:object.name,type:object.objectType,properties});
    }
    for(const value of Object.values(object))walk(value);
}
walk(graph);fs.writeFileSync(output,JSON.stringify(objects,null,2));
console.log(`Objects: ${objects.length}`);
