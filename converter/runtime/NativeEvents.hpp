#pragma once
#include <string>
#include <vector>
#include <cstdint>
namespace NativeEvents {
bool Load(const std::string& assetsRoot);
const char* Error();
struct Def {int source,kind;float p[16];};
struct State {bool enabled=true,active=false,done=false,occupied=false,collision=true;float time=0,alpha=1,target=1,direction=1;int population=0;};
struct Actor {float x,y,r;int size;unsigned weights;};
const std::vector<Def>& Definitions();
struct Sound {int rate=0,size=0;const uint8_t* data=nullptr;};
extern Sound sounds[10];
class World {
public:
 World();
 void Update(float dt,const std::vector<Actor>& actors);
 int Encounter(float x,float y,float radius) const;
 void Begin(int index);
 void Claim();
 int TakeTutorial();
 void FinishTutorial();
 int PendingMui()const;
 int FoundCount()const;
 int Rewards()const;
 const State& Get(int index)const{return states[index];}
 bool Motion(int source,float& time,bool& collision)const;
 float Alpha(int source)const;
 void CameraOffset(float& x,float& y)const;
 bool MuiCamera(float& x,float& y,float& width,float& duration)const;
private:
 std::vector<State> states;
 std::vector<int> queue;
 int pending=-1,pendingTutorial=-1;unsigned rewardBits=0;
 void Emit(int node,int port);
 void Dispatch();
 void Deliver(int node,int port);
 void Award(int id);
};
}
