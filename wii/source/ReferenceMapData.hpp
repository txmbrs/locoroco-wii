#pragma once
#include "AssetArray.hpp"
#include "LevelData.hpp"
namespace ReferenceMap {
struct BoundaryLoop {int count;const LocoData::Vec2* points;};

extern AssetArray<LocoData::Tri2> BRIDGE_TRIS;
extern AssetArray<LocoData::Vec2> BRIDGE_OUTLINE;
extern AssetArray<LocoData::Vec2> BRIDGE_LOOP_0;
extern AssetArray<LocoData::Vec2> BRIDGE_LOOP_1;
extern AssetArray<LocoData::Vec2> BRIDGE_LOOP_2;
extern AssetArray<BoundaryLoop> BRIDGE_LOOPS;
extern LocoData::PolyMesh BRIDGE;
extern AssetArray<LocoData::Tri2> FINISH_TRIS;
extern AssetArray<LocoData::Vec2> FINISH_OUTLINE;
extern AssetArray<LocoData::Vec2> FINISH_LOOP_0;
extern AssetArray<LocoData::Vec2> FINISH_LOOP_1;
extern AssetArray<BoundaryLoop> FINISH_LOOPS;
extern LocoData::PolyMesh FINISH;
extern AssetArray<LocoData::Tri2> LAUNCH_TERRAIN_TRIS;
extern AssetArray<LocoData::Vec2> LAUNCH_TERRAIN_OUTLINE;
extern AssetArray<LocoData::Vec2> LAUNCH_TERRAIN_LOOP_0;
extern AssetArray<LocoData::Vec2> LAUNCH_TERRAIN_LOOP_1;
extern AssetArray<BoundaryLoop> LAUNCH_TERRAIN_LOOPS;
extern LocoData::PolyMesh LAUNCH_TERRAIN;
extern AssetArray<LocoData::Tri2> ISLAND_TRIS;
extern AssetArray<LocoData::Vec2> ISLAND_OUTLINE;
extern AssetArray<LocoData::Vec2> ISLAND_LOOP_0;
extern AssetArray<BoundaryLoop> ISLAND_LOOPS;
extern LocoData::PolyMesh ISLAND;
extern AssetArray<LocoData::Tri2> ISLAND_SHADE_TRIS;
extern AssetArray<LocoData::Vec2> ISLAND_SHADE_OUTLINE;
extern AssetArray<LocoData::Vec2> ISLAND_SHADE_LOOP_0;
extern AssetArray<BoundaryLoop> ISLAND_SHADE_LOOPS;
extern LocoData::PolyMesh ISLAND_SHADE;
extern LocoData::Vec2 BRIDGE_PIVOT;
extern LocoData::Vec2 FINISH_TARGET;
extern AssetArray<LocoData::Vec2> WIND;
extern AssetArray<LocoData::Vec2> WIND_PICKIES;
extern AssetArray<float> WIND_DISTANCE;
extern int WIND_COUNT;
extern float WIND_LENGTH;
}
