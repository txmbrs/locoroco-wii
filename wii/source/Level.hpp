#pragma once

#include <box2d/box2d.h>
#include <vector>
#include "LevelData.hpp"
#include "NativeAnimations.hpp"
#include "NativeEvents.hpp"
class Loco;

class Level {
public:
    struct FruitItem {
        LocoData::Vec2 basePos;
        bool eaten = false;
        float animTimer = 0.0f;
        int surface = -1;
        LocoData::Vec2 localRoot{};
        float angle = 0.0f;
        bool blossomed = true;
        float bloom = 1.0f;
        bool tall = false;
    };

    struct FlyItem {
        LocoData::Vec2 basePos;
        bool collected = false;
        float animTimer = 0.0f;
    };

    Level(b2World* world);
    ~Level();

    void Update(float dt);
    void UpdateFlowerPhysics(float dt);
    void DrawBackground();
    void DrawTerrain();
    void DrawItems();
    void DrawMasks();
    void DrawForeground();

    // Check collisions with Locos
    int CheckFruitCollisions(const LocoData::Vec2& locoPos, float locoRadius);
    int CheckFlyCollisions(const LocoData::Vec2& locoPos, float locoRadius);

    struct SpawnCircle { LocoData::Vec2 position; float radius; };
    bool IsCircleFree(const LocoData::Vec2& position, float radius) const;
    bool IsPathClear(const LocoData::Vec2& from, const LocoData::Vec2& to) const;
    bool FindFreePosition(const LocoData::Vec2& origin, float radius, float maxDistance,
                          LocoData::Vec2& result,
                          const std::vector<SpawnCircle>& occupied = {}) const;
    void RevealSecrets(const LocoData::Vec2& position, float radius, float dt);
    bool IsAtGoal(const LocoData::Vec2& position) const;

    LocoData::Vec2 GetSpawnPoint() const;
    LocoData::Vec2 GetOpeningFlowerPoint(bool right) const;
    LocoData::Vec2 GetFruitRoot(const FruitItem& fruit) const;
    LocoData::Vec2 GetFruitCenter(const FruitItem& fruit) const;
    bool GetNearbyFruit(const LocoData::Vec2& position,float radius,LocoData::Vec2& target) const;
    bool IsAtWindEntry(const LocoData::Vec2& position) const;
    float WindEntryDistance(const LocoData::Vec2& position,float radius=0) const;
    bool CheckMuiEncounter(const LocoData::Vec2& position,float radius);
    void ClaimMuiReward();
    void UpdateInteractions(const std::vector<Loco*>& actors,float dt);
    bool GetMuiCamera(float& x,float& y,float& width,float& duration)const{return m_events.MuiCamera(x,y,width,duration);}
    int TakeTutorial(){return m_events.TakeTutorial();}
    void FinishTutorial(){m_events.FinishTutorial();}
    void GetNativeCameraOffset(float& x,float& y)const{m_events.CameraOffset(x,y);}
    int GetFoundMui()const{return m_events.FoundCount();}
    int GetRewardCount()const{return m_events.Rewards();}
    int GetPendingMui()const{return m_events.PendingMui();}
    LocoData::Vec2 WindPoint(float distance) const;
    float WindLength() const;

    int GetTotalFruits() const { return static_cast<int>(m_fruits.size()); }
    int GetEatenFruits() const { return m_eatenFruitsCount; }

    int GetTotalFlies() const { return static_cast<int>(m_flies.size()); }
    int GetCollectedFlies() const { return m_collectedFliesCount; }

private:
    NativeEvents::World m_events;
    std::vector<std::vector<int>> m_edgeBins;
    std::vector<float> m_windDistances;
    float m_edgeMinY=0,m_edgeMaxY=0,m_edgeBinScale=0;
    std::vector<b2Fixture*> m_nativeEdgeFixtures;
    void DrawNativePieces(int layer);
    void UpdateNativeAnimations(float dt);
    float m_nativeTime=0,m_nativeGateTime=0;
    std::vector<NativeAnimations::WorldPose> m_nativePoses;
    LocoData::Vec2 NativePoint(LocoData::Vec2 p,int body) const;
    friend struct GamePhysicsTests;
    void CreateTerrainPhysics();
    void CreateFlowerPhysics();
    void DrawFruit(const FruitItem& fruit);
    void DrawGoal();
    float GoalGround(float x) const;
    void CreateReferencePhysics();
    const LocoData::PolyMesh& TerrainMesh(int index) const;

    b2World* m_world = nullptr;
    std::vector<b2Body*> m_terrainBodies;
    struct FlowerSurface { int head,overlay; b2Body* body; LocoData::Vec2 center; float bend=0,velocity=0,drop=0,dropVelocity=0,restAngle=0; };
    std::vector<FlowerSurface> m_flowerSurfaces;
    struct ReferenceSurface { b2Body* body; const LocoData::PolyMesh* mesh; LocoData::Vec2 origin; float minX,minY,maxX,maxY; };
    std::vector<ReferenceSurface> m_referenceSurfaces;
    b2Body* m_bridgeBody = nullptr;
    b2RevoluteJoint* m_bridgeJoint = nullptr;
    bool m_muiEncountered = false;
    bool m_housePart9 = false;
    LocoData::Vec2 m_muiFoot{12630,7320};
    struct BackgroundPlacement { float x,y,scale; };
    std::vector<BackgroundPlacement> m_backgroundPlacement;

    std::vector<FruitItem> m_fruits;
    std::vector<FlyItem> m_flies;

    int m_eatenFruitsCount = 0;
    int m_collectedFliesCount = 0;

    float m_globalTime = 0.0f;
    float m_goalGroundY = 0;
    float m_goalPlantGround[6]{};
    // Indexed exactly like TERRAIN_MESHES; source SVG properties decide behavior.
    std::vector<b2Body*> m_surfaceBodies;
    std::vector<float> m_secretAlpha;
    struct Bounds { float minX, minY, maxX, maxY; };
    std::vector<Bounds> m_surfaceBounds;
};
