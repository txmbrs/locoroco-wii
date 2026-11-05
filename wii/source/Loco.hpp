#pragma once

#include <box2d/box2d.h>
#include <vector>
#include <memory>
#include "LevelData.hpp"

class Level;

class Loco {
public:
    Loco(b2World* world, float startX, float startY, int size = 1, Level* level = nullptr, float birthScale = 1.0f);
    ~Loco();

    void Update(float dt);
    void UpdateContactMaterial();
    void Draw(float singing=0);

    void ApplyJump(float strength);
    void ApplyImpulse(float ix, float iy);

    // Get center position in world pixel coordinates
    LocoData::Vec2 GetPosition() const;
    b2Vec2 GetVelocity() const;
    bool IsGrounded() const;

    int GetSize() const { return m_size; }
    int GetTotalSize() const { return m_size + m_pendingGrowth; }
    static float CollisionRadius(int size);
    float GetPhysicalRadius() const { return CollisionRadius(m_size) * m_spawnScale; }
    std::vector<LocoData::Vec2> GetOutline() const;
    void AnimateFromOutline(std::vector<LocoData::Vec2> outline, const LocoData::Vec2& center, float faceScale, float skinRadius = 0.0f);
    bool SetSize(int size);
    bool IsTouching(const Loco& other) const;
    bool IsTouchingBody(const b2Body* body) const;
    void QueueGrowth(int amount, float delay = 1.0f);

    void PlayCallAnimation();
    void OpenMouth(float duration = 0.5f);
    void AnticipateFood(const LocoData::Vec2& target);
    void EatFruit(const LocoData::Vec2& target);
    void EnterWind(float distance,float delay=0);
    void UpdateWind(float dt);
    bool IsInWind() const { return m_inWind; }
    bool HasFinishedWind() const { return m_windFinished; }
    void SetSpringValues(float freqMult, float dampMult);

    bool IsDestroyed() const { return m_destroyed; }
    void DestroyPhysics();

    // Pull toward target for merging
    void PullToward(const LocoData::Vec2& target, float force);

private:
    friend struct GamePhysicsTests;
    void CreatePhysics(float startX, float startY);

    b2World* m_world = nullptr;
    Level* m_level = nullptr;
    int16 m_collisionGroup = -1;
    int m_size = 1;
    int m_numPoints = 12;
    float m_wallSlipTime = 0;
    float m_radius = 31.6f; // in world pixels

    b2Body* m_centerBody = nullptr;
    b2Fixture* m_groupFixture = nullptr;
    std::vector<b2Body*> m_perimBodies;
    std::vector<b2Vec2> m_skinRestDirections;
    float m_skinRotation = 0.0f;
    std::vector<b2DistanceJoint*> m_spokeJoints;
    std::vector<b2DistanceJoint*> m_rimJoints;
    std::vector<b2DistanceJoint*> m_crossJoints;

    bool m_destroyed = false;
    bool m_inWind = false;
    bool m_windFinished = false;
    float m_windDistance = 0.0f;
    float m_windDelay = 0.0f;
    float m_windExitGrace = 0.0f;
    float m_mouthOpenTimer = 0.0f;
    float m_foodLookTimer = 0.0f;
    float m_eatingTimer = 0.0f;
    LocoData::Vec2 m_foodTarget{};
    float m_nativeClock=0,m_nativeEatStart=-100,m_nativeCallStart=-100,m_nativeOpenStart=-100;
    float m_blinkTimer = 0.0f;
    bool m_isBlinking = false;
    float m_antennaAngle = 0.0f;
    float m_antennaVel = 0.0f;
    float m_visualRotation = 0.0f;
    int m_pendingGrowth = 0;
    float m_growthTimer = 0.0f;
    float m_growthDelay = 1.0f;
    float m_growthVisualScale = 1.0f;
    float m_spawnScale = 1.0f;
    float m_mergeMorphTime = 0.0f;
    float m_mergeStartFaceScale = 1.0f;
    float m_mergeStartSkinRadius = 0.0f;
    LocoData::Vec2 m_mergeStartCenter{};
    std::vector<LocoData::Vec2> m_mergeOutline;

    // Original yellow values from PSP petton_setting.cnf; no artificial stroke.
    LocoData::Color4 m_bodyColor = { 247, 186, 29, 255 };
    LocoData::Color4 m_outlineColor = { 247, 186, 29, 255 };
};
