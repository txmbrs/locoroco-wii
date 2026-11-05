#include "NativeEvents.hpp"
#include "NativeAnimations.hpp"
#include "Audio.hpp"
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <sys/stat.h>
namespace NativeEvents {
Sound sounds[10];
namespace {
std::vector<Def> defs;
struct Route{int signal,node,port;};
std::vector<Route> routes,outputs;
std::string savePath;
const char* error="Map events not loaded";
bool circle(const Def& d,const Actor& a){
 bool inside=false;float best=1e30f;
 for(int i=0,j=3;i<4;j=i++){
  float ax=d.p[8+j*2],ay=d.p[9+j*2],bx=d.p[8+i*2],by=d.p[9+i*2];
  if((ay>a.y)!=(by>a.y) && a.x<(bx-ax)*(a.y-ay)/(by-ay)+ax)inside=!inside;
  float dx=bx-ax,dy=by-ay,t=std::clamp(((a.x-ax)*dx+(a.y-ay)*dy)/std::max(.00001f,dx*dx+dy*dy),0.f,1.f);
  best=std::min(best,std::hypot(a.x-ax-t*dx,a.y-ay-t*dy));
 }return inside || best<=a.r;
}
}
const char* Error(){return error;}
const std::vector<Def>& Definitions(){return defs;}
bool Load(const std::string& root){
 defs.clear();routes.clear();outputs.clear();error="Invalid map event bank";
 FILE* file=std::fopen((root+"events/st_flower01.lrcevt").c_str(),"rb");if(!file){error="Missing events/st_flower01.lrcevt";return false;}
 std::fseek(file,0,SEEK_END);long length=std::ftell(file);std::rewind(file);
 if(length<32 || length>1024*1024){std::fclose(file);return false;}
 std::vector<uint8_t> data(length);bool valid=std::fread(data.data(),1,length,file)==size_t(length);std::fclose(file);
 size_t at=8;auto u=[&](){if(at+4>data.size()){valid=false;return uint32_t(0);}uint32_t x=(uint32_t(data[at])<<24)|(uint32_t(data[at+1])<<16)|(uint32_t(data[at+2])<<8)|data[at+3];at+=4;return x;};
 if(!valid || std::memcmp(data.data(),"LRCEVT01",8) || u()!=1 || u()!=data.size()-20)return false;
 uint32_t expected=u(),actual=2166136261u;for(size_t i=20;i<data.size();++i)actual=(actual^data[i])*16777619u;
 if(expected!=actual)return false;
 uint32_t n=u(),nr=u(),no=u();if(n>512 || nr>2048 || no>4096 || uint64_t(n)*72+uint64_t(nr+no)*12!=data.size()-32)return false;
 defs.resize(n);for(auto& d:defs){d.source=int32_t(u());d.kind=int32_t(u());if(d.source<0 || d.kind<0 || d.kind>16)valid=false;
  for(auto& f:d.p){uint32_t bits=u();std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::abs(f)>1e7f)valid=false;}}
 routes.resize(nr);for(auto& r:routes){r={int32_t(u()),int32_t(u()),int32_t(u())};if(r.signal<0||r.signal>4096||r.node<0||r.node>=int(n)||r.port<0||r.port>13)valid=false;}
 outputs.resize(no);for(auto& r:outputs){r.node=int32_t(u());r.port=int32_t(u());r.signal=int32_t(u());if(r.node<0||r.node>=int(n)||r.port<0||r.port>7||r.signal<0||r.signal>4096)valid=false;}
 if(!valid || at!=data.size()){defs.clear();routes.clear();outputs.clear();return false;}
 savePath=root+"../save/flower01-rewards.bin";error="";return true;
}
World::World():states(defs.size()){
 for(size_t i=0;i<defs.size();++i){auto& s=states[i];const auto& d=defs[i];s.enabled=d.p[6]==0;
  if(d.kind==3){s.active=true;for(const auto& r:routes)if(r.node==int(i) && r.port==0 && std::any_of(outputs.begin(),outputs.end(),[&](const Route& out){return out.signal==r.signal;}))s.active=false;s.enabled=!s.active;}
  if(d.kind==4)s.alpha=s.target=s.enabled?1:0;
  if(d.kind==14)s.alpha=s.target=0;
 }
 FILE* f=std::fopen(savePath.c_str(),"rb");if(f){unsigned char b[12];bool ok=std::fread(b,1,12,f)==12 && std::fgetc(f)==EOF;std::fclose(f);
  if(ok && !std::memcmp(b,"LRCREW01",8) && b[9]==uint8_t(b[8]^0xa5) && b[10]==0 && b[11]==0)rewardBits=b[8]&7;}
}
void World::Award(int id){if(id<1||id>3 || (rewardBits&(1u<<(id-1))))return;rewardBits|=1u<<(id-1);
 auto slash=savePath.find_last_of('/');auto dir=savePath.substr(0,slash);mkdir(dir.c_str(),0777);
 auto temp=savePath+".tmp";FILE* f=std::fopen(temp.c_str(),"wb");if(!f)return;
 unsigned char b[12]={'L','R','C','R','E','W','0','1',uint8_t(rewardBits),uint8_t(rewardBits^0xa5),0,0};
 bool ok=std::fwrite(b,1,12,f)==12;if(std::fclose(f))ok=false;if(ok)std::rename(temp.c_str(),savePath.c_str());
}
void World::Emit(int node,int port){for(const auto& r:outputs)if(r.node==node && r.port==port && queue.size()<1024)queue.push_back(r.signal);}
void World::Deliver(int i,int port){auto& s=states[i];const auto& d=defs[i];
 if(d.kind==3){
  if(port==5)s.collision=false;else if(port==6)s.collision=true;
  else if(port==2)s.active=false;
  else if(port==0 || port==1 || port==3 || port==4){
   if(port==1){s.time=0;s.direction=1;}else if(port==3){s.time=NativeAnimations::WorldDuration(d.source);s.direction=-1;}else if(port==4)s.direction=-s.direction;
   if(port==0 && (s.done || s.active))return;
   s.active=true;s.done=false;if(port==0 || port==1)s.time=-d.p[2];
  }return;
 }
 if(d.kind==4){if(port==7)s.target=0;if(port==8)s.target=1;return;}
 if(d.kind==5){if(port==0){s.time=d.p[2];s.active=true;}else if(port==2)s.active=false;return;}
 if(d.kind==6){if(port==10)Emit(i,7);return;}
 if(d.kind==7 || d.kind==8){if(port==11)s.enabled=true;if(port==12)s.enabled=false;
  if(port==10 && s.enabled){Emit(i,7);if(d.kind==8)s.enabled=false;}return;}
 if(d.kind==12 || d.kind==13){
  int sound=int(d.p[4])-1;
  if(port==0 && sound>=0 && sound<10){s.time=d.p[2];s.active=true;if(s.time<=0){Audio::PlayWorldEvent(sound);s.active=false;}}
  if(port==2){s.active=false;if(sound>=0 && sound<10)Audio::StopWorldEvent(sound);}return;
 }
 if(d.kind==15){
  if(port==0 && !s.done && !s.active && d.p[4]>0){s.active=true;s.time=d.p[2];Emit(i,0);}
  if((port==9 || port==13) && s.active){s.active=false;s.done=true;Emit(i,1);}
  return;
 }
 if(d.kind==14){if(port==0)s.target=1;if(port==2)s.target=0;return;}
 if(d.kind==11){if(port==0)s.active=true;if(port==2)s.active=false;return;}
 if(d.kind==9 || d.kind==10){if(port==0 && !s.done){s.done=true;Award(int(d.p[4]));Emit(i,4);}return;}
 if(d.kind==0){if(port==13 && !s.done){s.done=true;s.time=0;s.active=true;Emit(i,1);}return;}
 if(port==9){s.occupied=false;s.population=0;}
}
void World::Dispatch(){size_t at=0;while(at<queue.size() && at<1024){int signal=queue[at++];for(const auto& r:routes)if(r.signal==signal)Deliver(r.node,r.port);}queue.clear();}
void World::Update(float dt,const std::vector<Actor>& actors){
 dt=std::clamp(dt,0.f,.1f);int weight=0,total=0;for(auto& a:actors)total+=a.size;
 for(size_t i=0;i<defs.size();++i){const auto& d=defs[i];auto& s=states[i];
  if(d.kind==1 || d.kind==2){int count=0;for(const auto& a:actors)if(d.kind==1?(weight<32 && (a.weights&(1u<<weight))):circle(d,a))count+=a.size;
   bool occupied=count>=int(d.p[3]) && (!d.p[7] || count>=total);
   if(occupied && !s.occupied)Emit(i,1);
   if(d.kind==1 && count>0 && count<int(d.p[3]) && (s.population==0 || s.occupied))Emit(i,5);
   if(count==0 && s.population>0)Emit(i,6);
   s.occupied=occupied;s.population=count;if(d.kind==1)++weight;
  }
  if(d.kind==15 && s.active && s.time>0)s.time=std::max(0.f,s.time-dt);
  if((d.kind==12 || d.kind==13) && s.active){s.time-=dt;if(s.time<=0){s.active=false;Audio::PlayWorldEvent(int(d.p[4])-1);}}
  if(d.kind==14){float duration=s.target>s.alpha?d.p[11]:d.p[12];float step=dt/std::max(.001f,duration);s.alpha+=std::clamp(s.target-s.alpha,-step,step);}
  if(d.kind==5 && s.active){s.time-=dt;if(s.time<=0){s.active=false;Emit(i,1);}}
  if(d.kind==0 && s.done){s.time+=dt;if(s.active && s.time>=NativeAnimations::MuiDuration("muimui_jump_exit")){s.active=false;Emit(i,1);}}
  if(d.kind==3 && s.enabled && s.active){s.time+=dt*s.direction;float end=NativeAnimations::WorldDuration(d.source);
   if((s.direction>0 && s.time>=end) || (s.direction<0 && s.time<=0)){s.time=std::clamp(s.time,0.f,end);s.active=false;s.done=true;Emit(i,1);}}
  if(d.kind==4){float step=dt/std::max(.001f,d.p[5]);s.alpha+=std::clamp(s.target-s.alpha,-step,step);}
 }
 Dispatch();
}
int World::Encounter(float x,float y,float radius)const{
 if(pending>=0)return -1;
 for(size_t i=0;i<defs.size();++i)if(defs[i].kind==0 && !states[i].done && !states[i].occupied && circle(defs[i],{x,y,radius+24,1,0}))return int(i);
 return -1;
}
void World::Begin(int i){if(pending>=0 || i<0 || i>=int(defs.size()) || defs[i].kind!=0 || states[i].done)return;pending=i;states[i].occupied=true;Emit(i,0);Emit(i,2);Dispatch();}
void World::Claim(){if(pending<0)return;int i=pending;pending=-1;states[i].done=true;states[i].time=0;states[i].active=true;Emit(i,3);Dispatch();}
int World::TakeTutorial(){
 if(pendingTutorial>=0)return 0;
 for(size_t i=0;i<defs.size();++i)if(defs[i].kind==15 && states[i].active && states[i].time<=0 && !states[i].done){pendingTutorial=int(i);return int(defs[i].p[4]);}
 return 0;
}
void World::FinishTutorial(){if(pendingTutorial<0)return;int i=pendingTutorial;pendingTutorial=-1;states[i].active=false;states[i].done=true;Emit(i,1);Dispatch();}
int World::PendingMui()const{return pending<0?0:int(defs[pending].p[4]);}
int World::FoundCount()const{int count=0;for(size_t i=0;i<defs.size();++i)if(defs[i].kind==0 && states[i].done)++count;return count;}
int World::Rewards()const{return int(bool(rewardBits&1))+int(bool(rewardBits&2))+int(bool(rewardBits&4));}
bool World::Motion(int source,float& time,bool& collision)const{for(size_t i=0;i<defs.size();++i)if(defs[i].kind==3 && defs[i].source==source && states[i].enabled){time=std::max(0.f,states[i].time);collision=states[i].collision;return true;}return false;}
bool World::MuiCamera(float& x,float& y,float& width,float& duration)const{
 if(pending<0)return false;const auto& mui=defs[pending];float nearest=1e30f;const Def* found=nullptr;
 for(const auto& d:defs)if(d.kind==16){float distance=std::hypot(d.p[0]-mui.p[0],d.p[1]-mui.p[1]);if(distance<nearest){nearest=distance;found=&d;}}
 if(!found || nearest>600)return false;x=found->p[0];y=found->p[1];width=found->p[3];duration=found->p[2];return width>0;
}
void World::CameraOffset(float& x,float& y)const{
 x=y=0;for(size_t i=0;i<defs.size();++i)if(defs[i].kind==14){x+=defs[i].p[8]*states[i].alpha;y+=defs[i].p[9]*states[i].alpha;}
}
float World::Alpha(int source)const{for(size_t i=0;i<defs.size();++i)if(defs[i].kind==4 && defs[i].source==source)return states[i].alpha;return 1;}
}
