#include "RuntimeAssets.hpp"
#include "NativeEvents.hpp"
#include "SceneSprites.hpp"
#include "PspSprites.hpp"
#include "EventAudioData.hpp"
#include "AudioData.hpp"
#include "MusicData.hpp"
#include <gccore.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <malloc.h>
#include <string>
#include <vector>
namespace SceneSprites { Renderer::Sprite FRUIT{}; }
namespace SceneSprites { Renderer::Sprite HOUSE_PART9{}; }
namespace SceneSprites { Renderer::Sprite HOUSE_PART_TITLE{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT0{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT1{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT2{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT3{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT4{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT5{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT6{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT7{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT8{}; }
namespace SceneSprites { Renderer::Sprite HUD_DIGIT9{}; }
namespace SceneSprites { Renderer::Sprite HUD_HEAD{}; }
namespace SceneSprites { Renderer::Sprite MUI{}; }
namespace SceneSprites { Renderer::Sprite MUI_EGG{}; }
namespace SceneSprites { Renderer::Sprite MUI_REWARD_TEXT{}; }
namespace SceneSprites { Renderer::Sprite MUI_SMILE{}; }
namespace SceneSprites { Renderer::Sprite PICKIE_BODY{}; }
namespace SceneSprites { Renderer::Sprite PICKIE_WINGS{}; }
namespace SceneSprites { Renderer::Sprite TEXT0{}; }
namespace SceneSprites { Renderer::Sprite TEXT1{}; }
namespace SceneSprites { Renderer::Sprite TEXT2{}; }
namespace SceneSprites { Renderer::Sprite TEXT3{}; }
namespace SceneSprites { Renderer::Sprite HINT1{}; }
namespace SceneSprites { Renderer::Sprite HINT2{}; }
namespace SceneSprites { Renderer::Sprite HINT3{}; }
namespace SceneSprites { Renderer::Sprite HINT4{}; }
namespace SceneSprites { Renderer::Sprite HINT5{}; }
namespace SceneSprites { Renderer::Sprite HINT6{}; }
namespace SceneSprites { Renderer::Sprite WIND_PARTICLES{}; }
namespace LocoEvents { int BITE_RATE=0,BITE_SIZE=0;const uint8_t* BITE_DATA=nullptr; }
namespace LocoEvents { int JUMP_RATE=0,JUMP_SIZE=0;const uint8_t* JUMP_DATA=nullptr; }
namespace LocoEvents { int SPLIT_RATE=0,SPLIT_SIZE=0;const uint8_t* SPLIT_DATA=nullptr; }
namespace LocoEvents { int JOIN_RATE=0,JOIN_SIZE=0;const uint8_t* JOIN_DATA=nullptr; }
namespace LocoEvents { int CALL1_RATE=0,CALL1_SIZE=0;const uint8_t* CALL1_DATA=nullptr; }
namespace LocoEvents { int CALL2_RATE=0,CALL2_SIZE=0;const uint8_t* CALL2_DATA=nullptr; }
namespace LocoAudioData {int JINGLE_CLEAR_RATE=0,JINGLE_CLEAR_SIZE=0;const uint8_t* JINGLE_CLEAR_DATA=nullptr;
int SFX_BOUNCE_VOCAL_RATE=0,SFX_BOUNCE_VOCAL_SIZE=0;const uint8_t* SFX_BOUNCE_VOCAL_DATA=nullptr;}
namespace PspMusic {int RATE=0,FRAMES=0,LOOP_START=0;}
namespace RuntimeAssets {
namespace {
char error[256]="Assets not loaded";
std::string base;
bool loaded=false;
std::vector<void*> allocations;
uint32_t big(const uint8_t* p){return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];}
bool fail(const char* file,const char* reason){std::snprintf(error,sizeof(error),"%s: %s",reason,file);return false;}
bool read(const std::string& relative,const char* magic,uint8_t*& data,int& a,int& b,int& bytes){
    FILE* file=std::fopen((base+relative).c_str(),"rb");if(!file)return fail(relative.c_str(),"Missing file");
    uint8_t header[24];bool ok=std::fread(header,1,24,file)==24 && std::memcmp(header,magic,8)==0;
    if(!ok){std::fclose(file);return fail(relative.c_str(),"Wrong asset format");}
    a=int(big(header+8));b=int(big(header+12));bytes=int(big(header+16));uint32_t expected=big(header+20);
    if(bytes<=0 || bytes>8*1024*1024){std::fclose(file);return fail(relative.c_str(),"Invalid asset size");}
    if(std::memcmp(magic,"LRCSPR01",8)==0 && (a<4 || b<4 || a>1024 || b>1024 || a%4 || b%4 || bytes!=a*b*4)){
        std::fclose(file);return fail(relative.c_str(),"Invalid sprite dimensions");}
    if(std::memcmp(magic,"LRCAUD01",8)==0 && (a<8000 || a>48000 || b!=2 || bytes%4)){
        std::fclose(file);return fail(relative.c_str(),"Invalid PCM format");}
    data=static_cast<uint8_t*>(memalign(32,bytes));if(!data){std::fclose(file);return fail(relative.c_str(),"Out of memory");}
    ok=std::fread(data,1,bytes,file)==size_t(bytes) && std::fgetc(file)==EOF;std::fclose(file);
    uint32_t actual=2166136261u;for(int i=0;i<bytes;++i)actual=(actual^data[i])*16777619u;
    if(!ok || actual!=expected){std::free(data);data=nullptr;return fail(relative.c_str(),"Corrupt asset");}
    DCFlushRange(data,bytes);allocations.push_back(data);return true;
}
struct SpriteBinding {const char* name;Renderer::Sprite* target;};
static SpriteBinding sprites[]={
{"SceneSprites.FRUIT",&SceneSprites::FRUIT},
{"SceneSprites.HOUSE_PART9",&SceneSprites::HOUSE_PART9},
{"SceneSprites.HOUSE_PART_TITLE",&SceneSprites::HOUSE_PART_TITLE},
{"SceneSprites.HUD_DIGIT0",&SceneSprites::HUD_DIGIT0},
{"SceneSprites.HUD_DIGIT1",&SceneSprites::HUD_DIGIT1},
{"SceneSprites.HUD_DIGIT2",&SceneSprites::HUD_DIGIT2},
{"SceneSprites.HUD_DIGIT3",&SceneSprites::HUD_DIGIT3},
{"SceneSprites.HUD_DIGIT4",&SceneSprites::HUD_DIGIT4},
{"SceneSprites.HUD_DIGIT5",&SceneSprites::HUD_DIGIT5},
{"SceneSprites.HUD_DIGIT6",&SceneSprites::HUD_DIGIT6},
{"SceneSprites.HUD_DIGIT7",&SceneSprites::HUD_DIGIT7},
{"SceneSprites.HUD_DIGIT8",&SceneSprites::HUD_DIGIT8},
{"SceneSprites.HUD_DIGIT9",&SceneSprites::HUD_DIGIT9},
{"SceneSprites.HUD_HEAD",&SceneSprites::HUD_HEAD},
{"SceneSprites.MUI",&SceneSprites::MUI},
{"SceneSprites.MUI_EGG",&SceneSprites::MUI_EGG},
{"SceneSprites.MUI_REWARD_TEXT",&SceneSprites::MUI_REWARD_TEXT},
{"SceneSprites.MUI_SMILE",&SceneSprites::MUI_SMILE},
{"SceneSprites.PICKIE_BODY",&SceneSprites::PICKIE_BODY},
{"SceneSprites.PICKIE_WINGS",&SceneSprites::PICKIE_WINGS},
{"SceneSprites.TEXT0",&SceneSprites::TEXT0},
{"SceneSprites.TEXT1",&SceneSprites::TEXT1},
{"SceneSprites.TEXT2",&SceneSprites::TEXT2},
{"SceneSprites.TEXT3",&SceneSprites::TEXT3},
{"SceneSprites.HINT1",&SceneSprites::HINT1},
{"SceneSprites.HINT2",&SceneSprites::HINT2},
{"SceneSprites.HINT3",&SceneSprites::HINT3},
{"SceneSprites.HINT4",&SceneSprites::HINT4},
{"SceneSprites.HINT5",&SceneSprites::HINT5},
{"SceneSprites.HINT6",&SceneSprites::HINT6},
{"SceneSprites.WIND_PARTICLES",&SceneSprites::WIND_PARTICLES}
};
struct AudioBinding {const char* name;int* rate;int* size;const uint8_t** data;};
static AudioBinding sounds[]={
{"LocoEvents.BITE",&LocoEvents::BITE_RATE,&LocoEvents::BITE_SIZE,&LocoEvents::BITE_DATA},
{"LocoEvents.JUMP",&LocoEvents::JUMP_RATE,&LocoEvents::JUMP_SIZE,&LocoEvents::JUMP_DATA},
{"LocoEvents.SPLIT",&LocoEvents::SPLIT_RATE,&LocoEvents::SPLIT_SIZE,&LocoEvents::SPLIT_DATA},
{"LocoEvents.JOIN",&LocoEvents::JOIN_RATE,&LocoEvents::JOIN_SIZE,&LocoEvents::JOIN_DATA},
{"LocoEvents.CALL1",&LocoEvents::CALL1_RATE,&LocoEvents::CALL1_SIZE,&LocoEvents::CALL1_DATA},
{"LocoEvents.CALL2",&LocoEvents::CALL2_RATE,&LocoEvents::CALL2_SIZE,&LocoEvents::CALL2_DATA},
{"LocoEvents.BOUNCE",&LocoAudioData::SFX_BOUNCE_VOCAL_RATE,&LocoAudioData::SFX_BOUNCE_VOCAL_SIZE,&LocoAudioData::SFX_BOUNCE_VOCAL_DATA},
{"LocoAudioData.JINGLE_CLEAR",&LocoAudioData::JINGLE_CLEAR_RATE,&LocoAudioData::JINGLE_CLEAR_SIZE,&LocoAudioData::JINGLE_CLEAR_DATA}
};
}
const char* Error(){return error;}
bool Load(){
    if(loaded)return true;
    for(const char* root:{"sd:/apps/LocoRocoWii/assets/","usb:/apps/LocoRocoWii/assets/","assets/"}){
        FILE* f=std::fopen((std::string(root)+"music/info.bin").c_str(),"rb");
        if(f){std::fclose(f);base=root;break;}}
    if(base.empty())return fail("assets/music/info.bin","Run converter first");
    bool ok=true;
    for(auto& binding:sprites){uint8_t* data=nullptr;int w,h,size;
        if(!read("sprites/"+std::string(binding.name)+".gx","LRCSPR01",data,w,h,size)){ok=false;break;}
        *binding.target={w,h,data};}
    if(ok)for(auto& binding:sounds){uint8_t* data=nullptr;int rate,channels,size;
        if(!read("audio/"+std::string(binding.name)+".pcm","LRCAUD01",data,rate,channels,size)){ok=false;break;}
        *binding.rate=rate;*binding.size=size;*binding.data=data;}
    if(ok){const char* events[]={"seikai01","Fatks","errer01","Switch_Scale","Door_L_LP","Door_L","muimui_voice01","button05","kakushi_in01","kakushi_out01"};
        for(int i=0;i<10;++i){uint8_t* data=nullptr;int rate=0,channels=0,size=0;
            if(!read("audio/WorldEvents."+std::string(events[i])+".pcm","LRCAUD01",data,rate,channels,size)){ok=false;break;}
            NativeEvents::sounds[i]={rate,size,data};
        }}
    if(ok){FILE* f=std::fopen((base+"music/info.bin").c_str(),"rb");uint8_t header[20];
        ok=f && std::fread(header,1,20,f)==20 && std::memcmp(header,"LRCMUS01",8)==0 && std::fgetc(f)==EOF;
        if(f)std::fclose(f);
        if(ok){PspMusic::RATE=int(big(header+8));PspMusic::FRAMES=int(big(header+12));PspMusic::LOOP_START=int(big(header+16));
            ok=PspMusic::RATE==32000 && PspMusic::FRAMES>0 && PspMusic::FRAMES<=240*32000 && PspMusic::LOOP_START>=0 && PspMusic::LOOP_START<PspMusic::FRAMES;}
        if(!ok)fail("music/info.bin","Invalid music metadata");}
    if(ok)for(const char* name:{"backing","voice0","voice1","voice2","voice3"}){
        auto path=base+"music/"+name+".pcm";FILE* f=std::fopen(path.c_str(),"rb");
        if(!f){ok=fail(name,"Missing music part");break;}
        std::fseek(f,0,SEEK_END);long bytes=std::ftell(f);std::fclose(f);
        if(bytes!=PspMusic::FRAMES*4){ok=fail(name,"Wrong music part length");break;}}
    if(!ok){for(void* allocation:allocations)std::free(allocation);allocations.clear();return false;}
    error[0]=0;loaded=true;return true;
}
}
