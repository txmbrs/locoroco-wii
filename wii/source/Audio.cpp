#include "Audio.hpp"
#include "NativeEvents.hpp"
#include "AudioData.hpp"
#include "EventAudioData.hpp"
#include <asndlib.h>
#include <ogc/audio.h>
#include <ogc/system.h>
#include <ogc/cache.h>
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <ogc/lwp_watchdog.h>
#include "MusicData.hpp"

namespace Audio {

static bool s_initialized = false;
static const int SFX_VOICE = 6;
static const int FX_VOICE = 7;
static uint8_t* s_music[5] = {};
static bool s_musicLoaded = false;
static int s_tier = 1;
static bool s_musicPlaying=false;
static uint64_t s_musicStartTick=0;
static int MusicChannels(int) { return 2; }
static int MusicVolume(int voice) {
    if (voice <= 1) return 220;
    if (voice == 2) return s_tier >= 2 ? 220 : 0;
    return s_tier >= 3 ? 220 : 0;
}
static bool LoadTheme() {
    if (s_musicLoaded) return true;
    static const char* names[] = {"backing", "voice0", "voice1", "voice2", "voice3"};
    static const char* roots[] = {"sd:/apps/LocoRocoWii/assets/music/", "usb:/apps/LocoRocoWii/assets/music/",
                                   "assets/music/"};
    // Reserve the complete synchronized score in MEM2, leaving MEM1 for physics/GX.
    uintptr_t start = (reinterpret_cast<uintptr_t>(SYS_GetArena2Lo()) + 31) & ~uintptr_t(31);
    const uintptr_t limit = reinterpret_cast<uintptr_t>(SYS_GetArena2Hi());
    uintptr_t next = start;
    for (int voice = 0; voice < 5; ++voice) {
        int bytes = PspMusic::FRAMES * MusicChannels(voice) * 2;
        if (next + bytes + 32 > limit) return false;
        s_music[voice] = reinterpret_cast<uint8_t*>(next);
        FILE* file = nullptr;
        for (const char* root : roots) {
            char path[256]; std::snprintf(path, sizeof(path), "%s%s.pcm", root, names[voice]);
            file = std::fopen(path, "rb");
            if (file) break;
        }
        if (!file) return false;
        size_t read = std::fread(s_music[voice], 1, bytes, file);
        std::fclose(file);
        if (read != static_cast<size_t>(bytes)) return false;
        DCFlushRange(s_music[voice], bytes);
        next = (next + bytes + 31) & ~uintptr_t(31);
    }
    SYS_SetArena2Lo(reinterpret_cast<void*>(next));
    s_musicLoaded = true;
    return true;
}
static void QueueThemeLoop(s32 voice) {
    int frameBytes = MusicChannels(voice) * 2;
    ASND_AddVoice(voice, s_music[voice] + PspMusic::LOOP_START * frameBytes,
                  (PspMusic::FRAMES - PspMusic::LOOP_START) * frameBytes);
}

void Init() {
    AUDIO_Init(nullptr);
    ASND_Init();
    ASND_Pause(0); // ASND starts in paused state; unpause to enable output
    s_initialized = true;
}

void PlayBGM(int tier) {
    if (!s_initialized || !LoadTheme()) return;
    s_tier = tier;
    for (int voice = 0; voice < 5; ++voice) {
        int format = VOICE_STEREO_16BIT_LE;
        int volume = MusicVolume(voice);
        ASND_SetVoice(voice, format, PspMusic::RATE, 0, s_music[voice],
                      PspMusic::FRAMES * MusicChannels(voice) * 2,
                      volume, volume, QueueThemeLoop);
    }
    s_musicStartTick=gettime();s_musicPlaying=true;
}

float SingingLevel(){
    if(!s_musicPlaying || !s_musicLoaded)return 0;
    uint64_t frame=ticks_to_microsecs(gettime()-s_musicStartTick)*PspMusic::RATE/1000000;
    uint64_t loop=PspMusic::FRAMES-PspMusic::LOOP_START;
    if(frame>=uint64_t(PspMusic::FRAMES))frame=PspMusic::LOOP_START+(frame-PspMusic::FRAMES)%loop;
    float peak=0;
    for(int voice=1;voice<5;++voice)if(MusicVolume(voice)>0){
        for(int i=0;i<128;i+=4){uint64_t sample=frame+i;
            if(sample>=uint64_t(PspMusic::FRAMES))sample=PspMusic::LOOP_START+(sample-PspMusic::FRAMES)%loop;
            const uint8_t* p=s_music[voice]+sample*4;
            int16_t value=static_cast<int16_t>(uint16_t(p[0])|(uint16_t(p[1])<<8));
            int16_t right=static_cast<int16_t>(uint16_t(p[2])|(uint16_t(p[3])<<8));
            peak=std::max(peak,std::max(std::abs(float(value)),std::abs(float(right)))/32768.f);
        }
    }
    return std::clamp(peak*8.f,0.f,1.f);
}

void SetBGMTier(int tier) {
    if (!s_initialized || !s_musicLoaded) return;
    s_tier = tier;
    // All four original parts keep the same playhead. Growth only changes gain.
    for (int voice = 0; voice < 5; ++voice) {
        int volume = MusicVolume(voice);
        ASND_ChangeVolumeVoice(voice, volume, volume);
    }
}

void StopBGM() {
    s_musicPlaying=false;
    if (!s_initialized) return;
    for (int voice = 0; voice < 5; ++voice) ASND_StopVoice(voice);
}

void PlayJingle() {
    if (!s_initialized) return;
    ASND_SetVoice(SFX_VOICE, VOICE_STEREO_16BIT_LE, LocoAudioData::JINGLE_CLEAR_RATE, 0,
                  (void*)LocoAudioData::JINGLE_CLEAR_DATA, LocoAudioData::JINGLE_CLEAR_SIZE,
                  255, 255, nullptr);
}

void PlayBiteSound() {
    if (!s_initialized) return;
    // Crisp fruit bite
    ASND_SetVoice(FX_VOICE, VOICE_STEREO_16BIT_LE, LocoEvents::BITE_RATE, 0,
                  (void*)LocoEvents::BITE_DATA, LocoEvents::BITE_SIZE,
                  230, 230, nullptr);
}

void PlayJumpSound() {
    if (!s_initialized) return;
    // Single cheerful Kulche jump shout ('Hop!')
    ASND_SetVoice(SFX_VOICE, VOICE_STEREO_16BIT_LE, LocoEvents::JUMP_RATE, 0,
                  (void*)LocoEvents::JUMP_DATA, LocoEvents::JUMP_SIZE,
                  220, 220, nullptr);
}

void PlayBounceSound() {
    if (!s_initialized) return;
    ASND_SetVoice(SFX_VOICE, VOICE_STEREO_16BIT_LE, LocoAudioData::SFX_BOUNCE_VOCAL_RATE, 0,
                  (void*)LocoAudioData::SFX_BOUNCE_VOCAL_DATA, LocoAudioData::SFX_BOUNCE_VOCAL_SIZE,
                  220, 220, nullptr);
}

void PlaySplitSound() {
    if (!s_initialized) return;
    // Clean burst pop
    ASND_SetVoice(SFX_VOICE, VOICE_STEREO_16BIT_LE, LocoEvents::SPLIT_RATE, 0,
                  (void*)LocoEvents::SPLIT_DATA, LocoEvents::SPLIT_SIZE,
                  230, 230, nullptr);
}

void PlayMergeSound() {
    if (!s_initialized) return;
    // Original join effect.
    ASND_SetVoice(SFX_VOICE, VOICE_STEREO_16BIT_LE, LocoEvents::JOIN_RATE, 0,
                  (void*)LocoEvents::JOIN_DATA, LocoEvents::JOIN_SIZE,
                  240, 240, nullptr);
}

void PlayCallSound(int variation) {
    if(!s_initialized)return;
    // Named PSP group calls, separate from the eventual joining impact.
    const auto* data=variation%2?LocoEvents::CALL2_DATA:LocoEvents::CALL1_DATA;
    int size=variation%2?LocoEvents::CALL2_SIZE:LocoEvents::CALL1_SIZE;
    ASND_SetVoice(FX_VOICE,VOICE_STEREO_16BIT_LE,LocoEvents::CALL1_RATE,0,
                  (void*)data,size,225,225,nullptr);
}


// Dedicated channels keep map events independent from Kulche and music.
static int worldOwner[8]={-1,-1,-1,-1,-1,-1,-1,-1};
static int nextWorldVoice=0;
static void QueueDoorLoop(s32 voice){const auto& sound=NativeEvents::sounds[4];if(sound.data && sound.size>0)ASND_AddVoice(voice,const_cast<uint8_t*>(sound.data),sound.size);}
void PlayWorldEvent(int sound){
    if(!s_initialized || sound<0 || sound>=10)return;
    const auto& clip=NativeEvents::sounds[sound];if(!clip.data || clip.size<=0 || clip.rate<=0)return;
    int slot=sound==4?7:(nextWorldVoice++%7);int voice=8+slot;worldOwner[slot]=sound;
    ASND_SetVoice(voice,VOICE_STEREO_16BIT_LE,clip.rate,0,const_cast<uint8_t*>(clip.data),clip.size,220,220,sound==4?QueueDoorLoop:nullptr);
}
void StopWorldEvent(int sound){
    if(!s_initialized)return;
    for(int i=0;i<8;++i)if(worldOwner[i]==sound){ASND_StopVoice(8+i);worldOwner[i]=-1;}
}
} // namespace Audio
