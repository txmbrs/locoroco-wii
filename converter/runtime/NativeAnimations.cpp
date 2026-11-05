#include "NativeAnimations.hpp"
#include <gccore.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <malloc.h>
#include <vector>
namespace NativeAnimations {
namespace {
char error[256]="Animation bank not loaded";
struct Reader {
 uint8_t* bytes=nullptr;size_t size=0,at=0;bool valid=false;
 bool open(const std::string& path,const char* magic){
  FILE* f=std::fopen(path.c_str(),"rb");if(!f){std::snprintf(error,sizeof(error),"Missing animation bank: %s",path.c_str());return false;}
  std::fseek(f,0,SEEK_END);long n=std::ftell(f);std::rewind(f);
  if(n<24 || n>16*1024*1024){std::fclose(f);return false;}
  bytes=static_cast<uint8_t*>(memalign(32,n));if(!bytes){std::fclose(f);return false;}
  size=n;valid=std::fread(bytes,1,size,f)==size;std::fclose(f);at=8;
  if(!valid || std::memcmp(bytes,magic,8) || u()!=1 || u()!=size-20){valid=false;return false;}
  uint32_t expected=u(),actual=2166136261u;for(size_t i=20;i<size;++i)actual=(actual^bytes[i])*16777619u;
  return valid=expected==actual;
 }
 // Clip names have variable byte lengths: subsequent floats need not be aligned.
 // Volatile byte reads prevent GCC from folding this decoder + memcpy into lfsx
 // on the packed file buffer. Gekko faults on that unaligned float load.
 __attribute__((noinline)) uint32_t u(){
  if(at>size || size-at<4){valid=false;return 0;}
  const volatile uint8_t* p=bytes+at;at+=4;
  return(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
 }
 int integer(){return static_cast<int32_t>(u());}
 float f(){uint32_t b=u();float value;std::memcpy(&value,&b,4);if(!std::isfinite(value)||std::abs(value)>1e8f)valid=false;return value;}
 unsigned char byte(){if(at>=size){valid=false;return 0;}return bytes[at++];}
 int count(int maximum){auto n=u();if(n>uint32_t(maximum)){valid=false;return 0;}return int(n);}
 std::string text(){int n=count(128);if(!valid || size-at<size_t(n)){valid=false;return {};}
  std::string value(reinterpret_cast<char*>(bytes+at),n);at+=n;return value;}
 bool end(){return valid && at==size;}
};
struct WorldTrack{int source,mode;float fps;std::vector<WorldPose> frames;};
struct Pose{float a,b,c,d,x,y;int visible,texture;};
struct Mesh{int texture,tint,hair;std::vector<Renderer::TexturedVertex> vertices;};
struct Clip{std::string name;int frames;float fps;std::vector<Pose> poses;};
std::vector<WorldTrack> world;
struct Bank {std::vector<Mesh> meshes;std::vector<Clip> clips;std::vector<Renderer::Sprite> textures;Reader file;int bigHair=0;};
Bank kulche,mui;
Reader worldFile;
bool loaded=false;
Clip* find(Bank& bank,const char* name){for(auto& c:bank.clips)if(c.name==name)return &c;return nullptr;}
float mix(float a,float b,float q){return a+(b-a)*q;}
bool loadWorld(const std::string& path){
 if(!worldFile.open(path,"LRCWANI1"))return false;
 auto& r=worldFile;int count=r.count(256);
 world.resize(count);
 for(auto& t:world){t.source=r.integer();t.mode=r.integer();t.fps=r.f();int n=r.count(5000);
  if(!r.valid || t.source<0 || t.mode<0 || t.mode>2 || t.fps<=0 || t.fps>120 || n<1 || uint64_t(n)*24>r.size-r.at)return false;
  t.frames.resize(n);for(auto& p:t.frames){p={r.f(),r.f(),r.f(),r.f(),r.f(),r.f()};if(p.visible!=0 && p.visible!=1)return false;}}
 return r.end();
}
bool loadCharacter(const std::string& path,Bank& bank,bool character){
 if(!bank.file.open(path,"LRCCANI1"))return false;
 auto& r=bank.file;auto& meshes=bank.meshes;auto& clips=bank.clips;auto& textures=bank.textures;auto& bigHair=bank.bigHair;
 int nm=r.count(64),nt=r.count(128),nc=r.count(128);bigHair=r.integer();
 if(!r.valid || nm<1 || nt<1 || nc<1 || bigHair<0 || bigHair>=nt)return false;
 meshes.resize(nm);
 for(auto& m:meshes){int n=r.count(10000);m.texture=r.integer();m.tint=r.integer();m.hair=r.integer();
  if(!r.valid || n<3 || n%3 || m.texture<0 || m.texture>=nt || (m.tint!=0 && m.tint!=1) || (m.hair!=0 && m.hair!=1) || uint64_t(n)*20>r.size-r.at)return false;
  m.vertices.resize(n);for(auto& v:m.vertices)v={r.f(),r.f(),r.f(),r.f(),r.byte(),r.byte(),r.byte(),r.byte()};}
 clips.resize(nc);
 for(auto& c:clips){c.name=r.text();c.frames=r.count(5000);c.fps=r.f();
  if(!r.valid || c.name.empty() || c.frames<1 || c.fps<=0 || c.fps>120 || uint64_t(c.frames)*nm*32>r.size-r.at)return false;
  c.poses.resize(c.frames*nm);
  for(auto& p:c.poses){p={r.f(),r.f(),r.f(),r.f(),r.f(),r.f(),r.integer(),r.integer()};if(p.visible<0 || p.visible>1 || p.texture<0 || p.texture>=nt)return false;}}
 textures.resize(nt);
 for(auto& t:textures){int w=r.count(1024),h=r.count(1024);auto bytes=r.u();r.at=(r.at+31)&~size_t(31);
  if(!r.valid || w<4 || h<4 || w%4 || h%4 || bytes!=uint32_t(w*h*4) || r.at>r.size || bytes>r.size-r.at)return false;
  t={w,h,r.bytes+r.at};DCFlushRange(r.bytes+r.at,bytes);r.at+=bytes;}
 return r.end() && (character?(find(bank,"eat") && find(bank,"call") && find(bank,"stay") && find(bank,"waaa") && find(bank,"kyoro")):(find(bank,"muimui_jump_idle") && find(bank,"muimui_jump_exit")));
}
}
const char* Error(){return error;}
bool Load(const std::string& root){
 if(loaded)return true;
 if(!loadWorld(root+"animations/world.lrcanim") || !loadCharacter(root+"animations/kulche.lrcanim",kulche,true) || !loadCharacter(root+"animations/muimui.lrcanim",mui,false)){
  if(std::strncmp(error,"Missing",7))std::snprintf(error,sizeof(error),"Invalid or incompatible PSP animation bank");
  std::free(worldFile.bytes);std::free(kulche.file.bytes);std::free(mui.file.bytes);worldFile.bytes=nullptr;kulche=Bank{};mui=Bank{};world.clear();return false;}
 error[0]=0;loaded=true;return true;
}
bool SampleWorld(int source,float seconds,WorldPose& out,bool triggered){
 for(const auto& t:world)if(t.source==source){
  if(t.mode==2 && !triggered)return false;
  float last=float(t.frames.size()-1),frame=std::max(0.f,seconds)*t.fps;
  frame=last>0?(t.mode==0 && !triggered?std::fmod(frame,last):std::min(frame,last)):0;
  int i=int(frame),j=std::min(i+1,int(t.frames.size()-1));float q=frame-i;auto& a=t.frames[i];auto& b=t.frames[j];
  out={mix(a.x,b.x,q),mix(a.y,b.y,q),mix(a.angle,b.angle,q),mix(a.sx,b.sx,q),mix(a.sy,b.sy,q),a.visible};return true;}
 return false;
}
float WorldEndAngle(int source){for(const auto& t:world)if(t.source==source)return t.frames.back().angle;return 0;}
float Duration(const char* name){auto* c=find(kulche,name);return c?(c->frames-1)/c->fps:0;}
static LocoData::Vec2 drawBank(Bank& bank,const char* name,float seconds,bool loop,float x,float y,float rotation,float unit,LocoData::Color4 body,bool large,float hairTilt){
 auto& meshes=bank.meshes;auto& textures=bank.textures;int bigHair=bank.bigHair;
 auto* clip=find(bank,name);if(!clip)return {x,y};float end=float(clip->frames-1),frame=std::max(0.f,seconds)*clip->fps;
 frame=end>0?(loop?std::fmod(frame,end):std::min(frame,end)):0;int i=int(frame),j=std::min(i+1,clip->frames-1);float q=frame-i;
 const float c=std::cos(rotation),s=std::sin(rotation),hc=std::cos(-hairTilt),hs=std::sin(-hairTilt);
 LocoData::Vec2 mouth{x,y};static std::vector<Renderer::TexturedVertex> vertices;
 for(size_t k=0;k<meshes.size();++k){auto& m=meshes[k];const auto& a=clip->poses[i*meshes.size()+k];const auto& b=clip->poses[j*meshes.size()+k];if(!a.visible)continue;
  Pose p{mix(a.a,b.a,q),mix(a.b,b.b,q),mix(a.c,b.c,q),mix(a.d,b.d,q),mix(a.x,b.x,q),mix(a.y,b.y,q),a.visible,a.texture};
  vertices.resize(m.vertices.size());for(size_t n=0;n<m.vertices.size();++n){auto v=m.vertices[n];float px=p.a*v.x+p.c*v.y,py=p.b*v.x+p.d*v.y;
   if(m.hair){float xx=hc*px-hs*py;py=hs*px+hc*py;px=xx;}
   float dx=(px+p.x)*unit,dy=-(py+p.y)*unit;v.x=x+c*dx-s*dy;v.y=y+s*dx+c*dy;
   if(m.tint){v.r=uint16_t(v.r)*body.r/255;v.g=uint16_t(v.g)*body.g/255;v.b=uint16_t(v.b)*body.b/255;}
   vertices[n]=v;}
  int tex=m.hair && large?bigHair:p.texture;Renderer::DrawWorldTexturedTriangles(textures[tex],vertices.data(),vertices.size(),false);
  // The mouth is the last native render component in the exported hierarchy.
  if(k+1==meshes.size()){float dx=p.x*unit,dy=-p.y*unit;mouth={x+c*dx-s*dy,y+s*dx+c*dy};}
 }
 return mouth;
}
LocoData::Vec2 DrawCharacter(const char* name,float seconds,bool loop,float x,float y,float rotation,float unit,LocoData::Color4 body,bool large,float hairTilt){
 return drawBank(kulche,name,seconds,loop,x,y,rotation,unit,body,large,hairTilt);
}
float MuiDuration(const char* name){auto* c=find(mui,name);return c?(c->frames-1)/c->fps:0;}
void DrawMui(const char* name,float seconds,bool loop,float x,float y,float unit){
 drawBank(mui,name,seconds,loop,x,y,0,unit,{255,255,255,255},false,0);
}

}

namespace NativeAnimations {float WorldDuration(int source){for(const auto& t:world)if(t.source==source)return float(t.frames.size()-1)/t.fps;return 0;}}
