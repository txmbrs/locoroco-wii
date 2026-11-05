#pragma once
#include "Renderer.hpp"
#include <string>
namespace NativeAnimations {
struct WorldPose {float x=0,y=0,angle=0,sx=1,sy=1,visible=1;};
bool Load(const std::string& assetsRoot);
const char* Error();
bool SampleWorld(int source,float seconds,WorldPose& out,bool triggered=false);
float WorldDuration(int source);
float WorldEndAngle(int source);
float Duration(const char* clip);
float MuiDuration(const char* clip);
void DrawMui(const char* clip,float seconds,bool loop,float x,float y,float unit);
LocoData::Vec2 DrawCharacter(const char* clip,float seconds,bool loop,float x,float y,
                           float rotation,float unit,LocoData::Color4 body,bool large,float hairTilt);
}
