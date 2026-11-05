#include "NativeLevelData.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <malloc.h>
#include <string>
#include <vector>
namespace NativeLevel {
const Vertex* VERTICES=nullptr;const Piece* PIECES=nullptr;const Edge* EDGES=nullptr;
const Body* BODIES=nullptr;const Fruit* FRUITS=nullptr;const Fly* FLIES=nullptr;
const Renderer::Sprite* TEXTURES=nullptr;const LocoData::Vec2* WIND=nullptr;
LocoData::Vec2 SPAWN{},GOAL{},OPENING[2]{},MUI{};
int PIECE_COUNT=0,EDGE_COUNT=0,BODY_COUNT=0,FRUIT_COUNT=0,FLY_COUNT=0,WIND_COUNT=0;
namespace {
uint8_t* blob=nullptr;size_t size=0,pos=0;bool valid=false;
const char* error="Map not loaded";
std::vector<Vertex> vertices;std::vector<Piece> pieces;std::vector<Edge> edges;
std::vector<Body> bodies;std::vector<Fruit> fruits;std::vector<Fly> flies;
std::vector<LocoData::Vec2> wind;std::vector<Renderer::Sprite> textures;
uint32_t u32(){
    if(pos>size || size-pos<4){valid=false;return 0;}
    auto* p=blob+pos;pos+=4;return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
}
int i32(){return static_cast<int32_t>(u32());}
float number(){uint32_t bits=u32();float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f) || std::abs(f)>1e8f)valid=false;return f;}
uint8_t byte(){if(pos>=size){valid=false;return 0;}return blob[pos++];}
LocoData::Vec2 point(){return {number(),number()};}
int count(int max){auto n=u32();if(n>uint32_t(max)){valid=false;return 0;}return int(n);}
bool fail(const char* message){error=message;std::free(blob);blob=nullptr;size=pos=0;valid=false;
    vertices.clear();pieces.clear();edges.clear();bodies.clear();fruits.clear();flies.clear();wind.clear();textures.clear();return false;}
bool loadFile(const std::string& path){
    FILE* f=std::fopen(path.c_str(),"rb");if(!f)return false;
    if(std::fseek(f,0,SEEK_END)){std::fclose(f);error="Cannot read map size";return false;}
    long length=std::ftell(f);std::rewind(f);
    if(length<92 || length>16*1024*1024){std::fclose(f);error="Invalid map size";return false;}
    blob=static_cast<uint8_t*>(memalign(32,size_t(length)));
    if(!blob){std::fclose(f);error="Not enough memory for map";return false;}
    size=size_t(length);bool read=std::fread(blob,1,size,f)==size;std::fclose(f);
    if(!read)return fail("Incomplete map file");
    pos=8;valid=true;
    if(std::memcmp(blob,"LRCMAP01",8)!=0 || u32()!=3)return fail("Unsupported map format");
    if(u32()!=size-20)return fail("Incorrect map length");
    uint32_t checksum=u32(),actual=2166136261u;
    for(size_t i=20;i<size;++i)actual=(actual^blob[i])*16777619u;
    if(checksum!=actual)return fail("Map checksum mismatch");
    int nv=count(1500000),np=count(20000),ne=count(30000),nb=count(512),nf=count(128),nl=count(5000),nw=count(5000),nt=count(512);
    if(!valid || nv<3 || !np || !ne || !nf || nw<2 || !nt)return fail("Invalid map counts");
    // Reject impossible counts before allocating decoded records.
    uint64_t needed=40ull+uint64_t(nv)*20+uint64_t(np)*44+uint64_t(ne)*24+uint64_t(nb)*16+uint64_t(nf)*20+uint64_t(nl)*12+uint64_t(nw)*8+uint64_t(nt)*12;
    if(needed>size-pos)return fail("Map tables exceed file");
    auto spawn=point(),goal=point(),left=point(),right=point(),mui=point();
    vertices.resize(nv);for(auto& v:vertices)v={number(),number(),number(),number(),byte(),byte(),byte(),byte()};
    pieces.resize(np);for(auto& p:pieces){p={i32(),i32(),i32(),i32(),i32(),i32(),number(),number(),number(),number(),number()};
        if(p.first<0 || p.count<=0 || p.count%3 || p.count>65535 || p.first>nv-p.count || p.texture<0 || p.texture>=nt || p.body< -1 || p.body>=nb || p.fruit< -1 || p.fruit>=nf || p.repeat<0 || p.repeat>1 || p.minX>p.maxX || p.minY>p.maxY)valid=false;}
    edges.resize(ne);for(auto& e:edges){e={number(),number(),number(),number(),i32(),number()};if(e.body< -1 || e.body>=nb || e.friction<0 || e.friction>2 || std::hypot(e.ax-e.bx,e.ay-e.by)<.001f)valid=false;}
    bodies.resize(nb);for(auto& b:bodies){b={i32(),number(),number(),i32()};if(b.kind<0 || b.kind>7)valid=false;}
    fruits.resize(nf);for(auto& f:fruits){f={number(),number(),i32(),number(),number()};if(f.body< -1 || f.body>=nb)valid=false;}
    flies.resize(nl);for(auto& f:flies){f={number(),number(),i32()};if(f.value<1 || f.value>100)valid=false;}
    wind.resize(nw);for(auto& w:wind)w=point();
    if(!valid)return fail("Invalid map geometry");
    textures.resize(nt);for(auto& t:textures){int w=count(1024),h=count(1024);auto bytes=u32();
        if(!w || !h || w%4 || h%4 || bytes!=uint32_t(w*h*4))return fail("Invalid map texture");
        pos=(pos+31)&~size_t(31);if(pos>size || bytes>size-pos)return fail("Map texture exceeds file");
        t={w,h,blob+pos};pos+=bytes;}
    if(!valid || pos!=size)return fail("Unexpected end of map");
    SPAWN=spawn;GOAL=goal;OPENING[0]=left;OPENING[1]=right;MUI=mui;
    VERTICES=vertices.data();PIECES=pieces.data();EDGES=edges.data();BODIES=bodies.data();FRUITS=fruits.data();FLIES=flies.data();WIND=wind.data();TEXTURES=textures.data();
    PIECE_COUNT=np;EDGE_COUNT=ne;BODY_COUNT=nb;FRUIT_COUNT=nf;FLY_COUNT=nl;WIND_COUNT=nw;error="";return true;
}
}
bool Load(const char* appDirectory){
    if(VERTICES)return true;
    std::vector<std::string> paths;
    if(appDirectory && *appDirectory)paths.push_back(std::string(appDirectory)+"/assets/maps/st_flower01.lrcmap");
    paths.push_back("sd:/apps/LocoRocoWii/assets/maps/st_flower01.lrcmap");
    paths.push_back("usb:/apps/LocoRocoWii/assets/maps/st_flower01.lrcmap");
    paths.push_back("assets/maps/st_flower01.lrcmap");
    for(const auto& p:paths){FILE* f=std::fopen(p.c_str(),"rb");if(!f)continue;std::fclose(f);return loadFile(p);}
    error="Missing assets/maps/st_flower01.lrcmap";return false;
}
const char* Error(){return error;}
}
