#pragma once
#include "AssetArray.hpp"
#include <vector>
#include <cstdint>

namespace LocoData {

struct Vec2 { float x, y; };
struct Color4 { uint8_t r, g, b, a; };
struct Tri2 { Vec2 a, b, c; };

struct PolyMesh {
    Color4 color;
    int numTris;
    const Tri2* triangles;
    int numOutline;
    const Vec2* outline;
};

struct BgInstance {
    int templateIdx;
    float x, y;
    float angle;
};

struct TerrainProperties {
    bool invisible;
    bool secret;
    float rotationSpeed;
    Vec2 pivot;
};





extern AssetArray<Tri2> TERRAIN_TRIS_0;
extern AssetArray<Vec2> TERRAIN_OUTLINE_0;
extern AssetArray<Tri2> TERRAIN_TRIS_1;
extern AssetArray<Vec2> TERRAIN_OUTLINE_1;
extern AssetArray<Tri2> TERRAIN_TRIS_2;
extern AssetArray<Vec2> TERRAIN_OUTLINE_2;
extern AssetArray<Tri2> TERRAIN_TRIS_3;
extern AssetArray<Vec2> TERRAIN_OUTLINE_3;
extern AssetArray<Tri2> TERRAIN_TRIS_4;
extern AssetArray<Vec2> TERRAIN_OUTLINE_4;
extern AssetArray<Tri2> TERRAIN_TRIS_5;
extern AssetArray<Vec2> TERRAIN_OUTLINE_5;
extern AssetArray<Tri2> TERRAIN_TRIS_6;
extern AssetArray<Vec2> TERRAIN_OUTLINE_6;
extern AssetArray<Tri2> TERRAIN_TRIS_7;
extern AssetArray<Vec2> TERRAIN_OUTLINE_7;
extern AssetArray<Tri2> TERRAIN_TRIS_8;
extern AssetArray<Vec2> TERRAIN_OUTLINE_8;
extern AssetArray<Tri2> TERRAIN_TRIS_9;
extern AssetArray<Vec2> TERRAIN_OUTLINE_9;
extern AssetArray<Tri2> TERRAIN_TRIS_10;
extern AssetArray<Vec2> TERRAIN_OUTLINE_10;
extern AssetArray<Tri2> TERRAIN_TRIS_11;
extern AssetArray<Vec2> TERRAIN_OUTLINE_11;
extern AssetArray<Tri2> TERRAIN_TRIS_12;
extern AssetArray<Vec2> TERRAIN_OUTLINE_12;
extern AssetArray<Tri2> TERRAIN_TRIS_13;
extern AssetArray<Vec2> TERRAIN_OUTLINE_13;
extern AssetArray<Tri2> TERRAIN_TRIS_14;
extern AssetArray<Vec2> TERRAIN_OUTLINE_14;
extern AssetArray<Tri2> TERRAIN_TRIS_15;
extern AssetArray<Vec2> TERRAIN_OUTLINE_15;
extern AssetArray<Tri2> TERRAIN_TRIS_16;
extern AssetArray<Vec2> TERRAIN_OUTLINE_16;
extern AssetArray<Tri2> TERRAIN_TRIS_17;
extern AssetArray<Vec2> TERRAIN_OUTLINE_17;
extern AssetArray<Tri2> TERRAIN_TRIS_18;
extern AssetArray<Vec2> TERRAIN_OUTLINE_18;
extern AssetArray<Tri2> TERRAIN_TRIS_19;
extern AssetArray<Vec2> TERRAIN_OUTLINE_19;
extern AssetArray<Tri2> TERRAIN_TRIS_20;
extern AssetArray<Vec2> TERRAIN_OUTLINE_20;
extern AssetArray<Tri2> TERRAIN_TRIS_21;
extern AssetArray<Vec2> TERRAIN_OUTLINE_21;
extern AssetArray<Tri2> TERRAIN_TRIS_22;
extern AssetArray<Vec2> TERRAIN_OUTLINE_22;
extern AssetArray<Tri2> TERRAIN_TRIS_23;
extern AssetArray<Vec2> TERRAIN_OUTLINE_23;
extern AssetArray<Tri2> TERRAIN_TRIS_24;
extern AssetArray<Vec2> TERRAIN_OUTLINE_24;
extern AssetArray<Tri2> TERRAIN_TRIS_25;
extern AssetArray<Vec2> TERRAIN_OUTLINE_25;
extern AssetArray<Tri2> TERRAIN_TRIS_26;
extern AssetArray<Vec2> TERRAIN_OUTLINE_26;
extern AssetArray<Tri2> TERRAIN_TRIS_27;
extern AssetArray<Vec2> TERRAIN_OUTLINE_27;
extern AssetArray<Tri2> TERRAIN_TRIS_28;
extern AssetArray<Vec2> TERRAIN_OUTLINE_28;
extern AssetArray<Tri2> TERRAIN_TRIS_29;
extern AssetArray<Vec2> TERRAIN_OUTLINE_29;
extern AssetArray<Tri2> TERRAIN_TRIS_30;
extern AssetArray<Vec2> TERRAIN_OUTLINE_30;
extern AssetArray<Tri2> TERRAIN_TRIS_31;
extern AssetArray<Vec2> TERRAIN_OUTLINE_31;
extern AssetArray<Tri2> TERRAIN_TRIS_32;
extern AssetArray<Vec2> TERRAIN_OUTLINE_32;
extern AssetArray<Tri2> TERRAIN_TRIS_33;
extern AssetArray<Vec2> TERRAIN_OUTLINE_33;
extern AssetArray<Tri2> TERRAIN_TRIS_34;
extern AssetArray<Vec2> TERRAIN_OUTLINE_34;
extern AssetArray<Tri2> TERRAIN_TRIS_35;
extern AssetArray<Vec2> TERRAIN_OUTLINE_35;
extern AssetArray<Tri2> TERRAIN_TRIS_36;
extern AssetArray<Vec2> TERRAIN_OUTLINE_36;
extern AssetArray<Tri2> TERRAIN_TRIS_37;
extern AssetArray<Vec2> TERRAIN_OUTLINE_37;
extern AssetArray<PolyMesh> TERRAIN_MESHES;
extern int NUM_TERRAIN_MESHES;
extern AssetArray<TerrainProperties> TERRAIN_PROPERTIES;
extern AssetArray<Tri2> MASK_TRIS_0;
extern AssetArray<Vec2> MASK_OUTLINE_0;
extern AssetArray<Tri2> MASK_TRIS_1;
extern AssetArray<Vec2> MASK_OUTLINE_1;
extern AssetArray<Tri2> MASK_TRIS_2;
extern AssetArray<Vec2> MASK_OUTLINE_2;
extern AssetArray<PolyMesh> MASK_MESHES;
extern int NUM_MASK_MESHES;
extern AssetArray<PolyMesh> BG_DIRECT_MESHES;
extern int NUM_BG_DIRECT_MESHES;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_0;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_0;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_1;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_1;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_2;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_2;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_3;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_3;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_4;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_4;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_5;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_5;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_6;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_6;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_7;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_7;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_8;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_8;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_9;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_9;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_10;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_10;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_11;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_11;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_12;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_12;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_13;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_13;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_14;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_14;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_15;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_15;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_16;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_16;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_17;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_17;
extern AssetArray<Tri2> BG_TEMPLATE_TRIS_18;
extern AssetArray<Vec2> BG_TEMPLATE_OUTLINE_18;
extern AssetArray<PolyMesh> BG_TEMPLATES;
extern int NUM_BG_TEMPLATES;
extern AssetArray<BgInstance> BG_INSTANCES;
extern int NUM_BG_INSTANCES;
extern Vec2 SPAWN_POINT;
extern AssetArray<float> GOAL_BOUNDS;
extern AssetArray<Vec2> FRUIT_POSITIONS;
extern int NUM_FRUITS;
extern AssetArray<Vec2> FLY_POSITIONS;
extern int NUM_FLIES;
}
