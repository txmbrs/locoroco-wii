#include <gccore.h>
#include <fat.h>
#include <ogc/lwp_watchdog.h>
#include "Renderer.hpp"
#include "Input.hpp"
#include "Audio.hpp"
#include "RuntimeAssets.hpp"
#include "Game.hpp"
#include "NativeLevelData.hpp"
#include <string>
#include <algorithm>
#include <cstdio>

namespace {
const char* bootLogPath=nullptr;
bool firstFrame=true;
void BootStage(const char* stage) {
    if(!bootLogPath)return;
    FILE* file=std::fopen(bootLogPath,"a");
    if(file){std::fprintf(file,"%s\n",stage);std::fclose(file);}
}
}

int main(int argc, char** argv) {
    fatInitDefault();
    for(const char* path:{"sd:/apps/LocoRocoWii/boot.log","usb:/apps/LocoRocoWii/boot.log"}) {
        FILE* file=std::fopen(path,"w");if(file){std::fclose(file);bootLogPath=path;break;}
    }
    BootStage("LocoRoco Wii 1.16.1 PSP MAP EVENTS PREVIEW START");
    // Data is loaded only after video and input can show an error.
    BootStage("EXTERNAL ASSETS; NO EMBEDDED FALLBACK");

    if (!Renderer::Init()) {
        BootStage("VIDEO FAILED");
        return 1;
    }
    BootStage("VIDEO OK");
    Renderer::BeginFrame();
    Renderer::DrawScreenRect(0,0,Renderer::GetScreenWidth(),Renderer::GetScreenHeight(),{255,255,255,255});
    Renderer::DrawScreenText(32,40,"LocoRoco Wii 1.16.1 PSP MAP EVENTS PREVIEW",{143,109,48,255},2);
    Renderer::DrawScreenText(32,80,"Loading...",{143,109,48,255},2);
    Renderer::EndFrame();
    BootStage("LOADING FRAME OK");

    Input::Init();
    if(!RuntimeAssets::Load()) {
        BootStage(RuntimeAssets::Error());
        for(;;){
            Input::Update(1.0f/60);
            if(Input::GetState().exitPressed)return 1;
            Renderer::BeginFrame();
            Renderer::DrawScreenText(24,40,"Missing or invalid data pack",{100,30,20,255},2);
            Renderer::DrawScreenText(24,84,"Run the PC converter first.",{60,60,60,255},1);
            Renderer::DrawScreenText(24,108,"assets/preview-assets.json",{60,60,60,255},1);
            Renderer::DrawScreenText(24,132,RuntimeAssets::Error(),{60,60,60,255},1);
            Renderer::DrawScreenText(24,180,"HOME: exit",{60,60,60,255},1);
            Renderer::EndFrame();
        }
    }
    BootStage("DATA PACK OK");
    std::string appDirectory;
    if(argc>0 && argv && argv[0]){
        std::string executable=argv[0];auto slash=executable.find_last_of("/\\");
        if(slash!=std::string::npos)appDirectory=executable.substr(0,slash);
    }
    if(!NativeLevel::Load(appDirectory.c_str())){
        BootStage(NativeLevel::Error());
        for(;;){
            Input::Update(1.f/60);
            if(Input::GetState().exitPressed)return 1;
            Renderer::BeginFrame();
            Renderer::DrawScreenText(24,40,"Missing or invalid PSP map",{100,30,20,255},2);
            Renderer::DrawScreenText(24,84,"Run the PC converter first.",{60,60,60,255},1);
            Renderer::DrawScreenText(24,108,"assets/maps/st_flower01.lrcmap",{60,60,60,255},1);
            Renderer::DrawScreenText(24,132,NativeLevel::Error(),{60,60,60,255},1);
            Renderer::DrawScreenText(24,180,"HOME: exit",{60,60,60,255},1);
            Renderer::EndFrame();
        }
    }
    BootStage("EXTERNAL PSP MAP OK");
    Audio::Init();
    BootStage("INPUT AND AUDIO OK; INITIALIZING MAP");

    Game game;
    game.Init();
    BootStage("MAP AND MUSIC OK");

    u64 previousTick = gettime();

    while (true) {
        u64 tick = gettime();
        float dt = std::clamp(static_cast<float>(ticks_to_microsecs(tick - previousTick)) / 1000000.0f,
                              0.0f, 1.0f / 15.0f);
        previousTick = tick;
        Input::Update(dt);

        if (Input::GetState().exitPressed) {
            Audio::StopBGM();
            exit(0);
        }

        if(firstFrame)BootStage("FIRST UPDATE BEGIN");
        game.Update(dt);
        if(firstFrame)BootStage("FIRST UPDATE OK; FIRST RENDER BEGIN");
        game.Render();
        if(firstFrame){BootStage("FIRST GAME FRAME OK");firstFrame=false;}
    }

    return 0;
}





