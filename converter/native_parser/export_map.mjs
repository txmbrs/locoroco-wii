import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {readBlvBuffer,GameVersion,LocoObject,Mesh,Polygon,Buffer as TextureBuffer} from './blv.ts';

const input=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]);
fs.mkdirSync(out,{recursive:true});fs.mkdirSync(path.join(out,'binary'),{recursive:true});
const raw=fs.readFileSync(input);
let graph=readBlvBuffer(GameVersion.Gold,raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength));
// Optional original nested object bank, e.g. MuiMui's own scene/animation root.
if(process.argv[4]){
    const seen=new Set(),matches=[];
    function visit(value){
        if(!value || typeof value!=='object' || seen.has(value) || ArrayBuffer.isView(value))return;
        seen.add(value);
        if(value.constructor?.name==='SubRoot' && value.object?.name===process.argv[4])matches.push(value);
        for(const v of Object.values(value))visit(v);
    }
    visit(graph.root);
    if(matches.length!==1)throw new Error(`Expected one native SubRoot ${process.argv[4]}, found ${matches.length}`);
    graph={root:matches[0]};
}
const ids=new Map(),nodes=[],originals=[];
function encode(value){
    if(value===null || typeof value==='string' || typeof value==='boolean')return value;
    if(typeof value==='number')return Number.isFinite(value)?value:{$number:String(value)};
    if(typeof value==='undefined')return {$undefined:true};
    if(typeof value==='bigint')return {$bigint:String(value)};
    if(ids.has(value))return {$ref:ids.get(value)};
    const id=nodes.length;ids.set(value,id);originals.push(value);
    const node={id,type:value.constructor?.name??'Object'};nodes.push(node);
    if(ArrayBuffer.isView(value)){
        const data=Buffer.from(value.buffer,value.byteOffset,value.byteLength);
        const sha256=createHash('sha256').update(data).digest('hex');
        const file='binary/'+sha256+'.bin';
        if(!fs.existsSync(path.join(out,file)))fs.writeFileSync(path.join(out,file),data);
        Object.assign(node,{binary:file,bytes:data.length,sha256});
    }else if(Array.isArray(value))node.items=value.map(encode);
    else if(value instanceof Map)node.entries=[...value].map(([k,v])=>[encode(k),encode(v)]);
    else if(value instanceof Set)node.items=[...value].map(encode);
    else node.fields=Object.fromEntries(Object.entries(value).map(([k,v])=>[k,encode(v)]));
    return {$ref:id};
}
const rootRef=encode(graph.root);
const json=(name,value)=>fs.writeFileSync(path.join(out,name),JSON.stringify(value,null,2));
json('graph.json',{format:1,root:rootRef,nodes});

const objects=[],meshes=[],polygons=[],textures=[],types={},elements=[];
for(let id=0;id<originals.length;++id){
    const value=originals[id];
    if(value instanceof LocoObject){
        types[value.objectType]=(types[value.objectType]??0)+1;
        objects.push({id,name:value.name,type:value.objectType,bitfield:value.bitfield,
            handlerHash:value.objectTypeHandlerHash,
            children:value.children.map(c=>ids.get(c.object)),
            properties:value.properties.map(p=>({name:p.name,typeId:p.typeId,count:p.valueCount,value:encode(p.value)}))});
        const parameters={};
        for(const p of value.properties){
            if(p.value===null || ['number','boolean','string'].includes(typeof p.value))parameters[p.name]=p.value;
            else if(['Vec2','Vec3','RotRect','Box'].includes(p.value?.constructor?.name))parameters[p.name]=p.value;
        }
        elements.push({id,name:value.name,type:value.objectType,
            coordinateSpace:'Native property values; parent transforms and animations are not baked',parameters});
    }
    if(value instanceof Mesh){
        meshes.push({id,coordinateSpace:'native mesh local coordinates; transforms retained in graph.json',
            components:value.meshComponents.map(component=>({
                material:ids.get(component.material),name:component.material.name,color:component.material.color,
                texture:component.material.file?ids.get(component.material.file.buffer):null,
                uvScale:component.material.file?[component.material.file.uScaling,component.material.file.vScaling]:[1,1],
                strips:component.vertices.map(buffer=>{
                    const triangles=[];
                    for(let i=2;i<buffer.data.length;++i)triangles.push(i%2?[i-2,i,i-1]:[i-2,i-1,i]);
                    return {type:buffer.type,vertices:buffer.data,triangles};
                })}))});
    }
    if(value instanceof Polygon){
        polygons.push({id,points:value.points,components:value.components.map(c=>({
            edges:c.indices.map(e=>[e.value1,e.value2]),friction:c.friction,surfaceType:c.surfaceType,
            unk4:c.unk4,unk5:c.unk5,unk6:c.unk6,onCollisionSignals:c.onCollisionSignals}))});
    }
    if(value instanceof TextureBuffer){
        textures.push({id,name:value.sourceFilename,width:value.width,height:value.height,
            format:value.formatIdx,palette:encode(value.palette),mips:encode(value.mips),fileOffset:value.fileOffset});
    }
}
const collision=graph.root.collision??null;
json('objects.json',{coordinateSpace:'original native values, not Wii/SVG coordinates',objects});
json('elements.json',{elements});
json('render-meshes.json',{meshes});json('collisions.json',{root:collision?ids.get(collision):null,polygons});
json('textures.json',{textures});

let edgeCount=0;
if(collision && collision.points.length){
    const pts=collision.points;
    const minX=Math.min(...pts.map(p=>p.x)),maxX=Math.max(...pts.map(p=>p.x));
    const minY=Math.min(...pts.map(p=>p.y)),maxY=Math.max(...pts.map(p=>p.y));
    const margin=Math.max(maxX-minX,maxY-minY)*.02;
    let svg=`<svg xmlns="http://www.w3.org/2000/svg" viewBox="${minX-margin} ${-maxY-margin} ${maxX-minX+2*margin} ${maxY-minY+2*margin}">`;
    svg+='<title>st_flower01: original static collision edges from PSP ROM</title><rect x="'+(minX-margin)+'" y="'+(-maxY-margin)+'" width="'+(maxX-minX+2*margin)+'" height="'+(maxY-minY+2*margin)+'" fill="white"/>';
    let obj='# Original static collision edges, native PSP units, Y up\n';
    for(const p of pts)obj+=`v ${p.x} ${p.y} 0\n`;
    for(const component of collision.components){
        let d='';
        for(const pair of component.indices){
            const a=pts[pair.value1],b=pts[pair.value2];
            if(!a || !b)throw new Error('Collision edge index outside vertex table');
            d+=`M${a.x},${-a.y}L${b.x},${-b.y}`;
            obj+=`l ${pair.value1+1} ${pair.value2+1}\n`;edgeCount++;
        }
        svg+=`<path d="${d}" fill="none" stroke="${component.surfaceType===1?'#8d40a6':'#ed7900'}" stroke-width="1.5" vector-effect="non-scaling-stroke"/>`;
    }
    svg+='</svg>';fs.writeFileSync(path.join(out,'static-collision-map.svg'),svg);
    fs.writeFileSync(path.join(out,'static-collision.obj'),obj);
}
const summary={level:graph.root.subRoot?.object?.name??graph.root.object?.name??null,
    objectCount:objects.length,meshCount:meshes.length,polygonCount:polygons.length,textureCount:textures.length,
    rootCollisionPoints:collision?.points.length??0,rootCollisionEdges:edgeCount,nodeCount:nodes.length,objectTypes:types,
    notes:['Raw CLV and complete reference graph retain original data, including animation and signals.',
        'Render meshes are in local coordinates; do not treat them as an assembled world without graph transforms.',
        'The SVG/OBJ show root static collision edges; moving platforms and other local colliders are in collisions.json.']};
json('summary.json',summary);
console.log(JSON.stringify(summary,null,2));
