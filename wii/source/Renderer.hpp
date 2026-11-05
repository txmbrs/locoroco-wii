#pragma once

#include <cmath>
#include <vector>
#include <string>
#include "LevelData.hpp"

namespace Renderer {

struct Camera {
    float x = 2553.0f;
    float y = 411.0f;
    float scale = 0.55f;
    float angle = 0.0f; // radians
};

struct Sprite;
struct TexturedVertex { float x,y,u,v; unsigned char r,g,b,a; };
void DrawWorldTexturedTriangles(const Sprite& sprite,const TexturedVertex* vertices,int count,bool repeat);

struct Sprite {
    int width, height;
    const uint8_t* pixels; // GX_RGBA8, tiled 4x4, aligned to 32 bytes
};

bool Init();
void BeginFrame();
void EndFrame();

void SetCamera(const Camera& cam);
const Camera& GetCamera();

int GetScreenWidth();
int GetScreenHeight();

void ClearBackground(uint8_t r, uint8_t g, uint8_t b);

// 2D World-space drawing primitives (transformed by Camera)
void DrawWorldTriangle(const LocoData::Vec2& a, const LocoData::Vec2& b, const LocoData::Vec2& c, LocoData::Color4 color);
void DrawWorldMesh(const LocoData::PolyMesh& mesh, float offsetX = 0.0f, float offsetY = 0.0f, float scale = 1.0f, float angle = 0.0f);
void DrawWorldMeshRotatedAround(const LocoData::PolyMesh& mesh, float pivotX, float pivotY, float angle);
void DrawWorldCircle(float cx, float cy, float radius, LocoData::Color4 color, int segments = 24);
void DrawWorldBlob(const std::vector<LocoData::Vec2>& points, LocoData::Color4 fillColor, LocoData::Color4 outlineColor);
void DrawWorldLine(const LocoData::Vec2& a, const LocoData::Vec2& b, float width, LocoData::Color4 color);
void DrawWorldSprite(const Sprite& sprite, float cx, float cy, float width, float height,
                     LocoData::Color4 tint = {255, 255, 255, 255}, float angle = 0.0f);

// 2D Screen-space drawing primitives (UI / HUD)
void DrawScreenRect(float x, float y, float w, float h, LocoData::Color4 color);
void DrawScreenSprite(const Sprite& sprite,float cx,float cy,float width,float height,
                     LocoData::Color4 tint={255,255,255,255},float angle=0);
void DrawScreenEllipse(float x,float y,float rx,float ry,LocoData::Color4 color);
void DrawScreenCircle(float cx, float cy, float radius, LocoData::Color4 color, int segments = 20);
void DrawScreenText(float x, float y, const char* text, LocoData::Color4 color, float scale = 2.0f);

} // namespace Renderer
