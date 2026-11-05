#pragma once
#include "Renderer.hpp"
namespace NativeLevel {
bool Load(const char* appDirectory=nullptr);
const char* Error();
struct Vertex { float x,y,u,v; unsigned char r,g,b,a; };
struct Piece { int first,count,texture,body,fruit,repeat; float z,minX,minY,maxX,maxY; };
struct Edge { float ax,ay,bx,by; int body; float friction; };
struct Body { int kind; float x,y; int source; };
struct Fruit { float x,y; int body; float centerX,centerY; };
struct Fly { float x,y; int value; };
extern const Vertex* VERTICES; extern const Piece* PIECES; extern const Edge* EDGES;
extern const Body* BODIES; extern const Fruit* FRUITS; extern const Fly* FLIES;
extern const Renderer::Sprite* TEXTURES; extern const LocoData::Vec2* WIND;
extern LocoData::Vec2 SPAWN,GOAL,OPENING[2],MUI;
extern int PIECE_COUNT,EDGE_COUNT,BODY_COUNT,FRUIT_COUNT,FLY_COUNT,WIND_COUNT;
}
