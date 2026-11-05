#pragma once
#include "ContactMaterial.hpp"

#include <box2d/box2d.h>
#include <vector>
#include <memory>
#include "Level.hpp"
#include "Loco.hpp"
#include "Input.hpp"

enum GameState {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_VICTORY,
    STATE_TUTORIAL,
    STATE_MUI_REWARD
};

class Game {
public:
    Game();
    ~Game();

    void Init();
    void Update(float dt);
    void Render();

private:
    friend struct GamePhysicsTests;
    void ResetLevel();
    void UpdateOpening(float dt);
    void SplitLocos(bool intoWind=false);
    void TryMergeLocos(float dt);

    GameState m_state = STATE_PLAYING;

    MiniContactListener m_contactListener;
    std::unique_ptr<b2World> m_world;
    std::unique_ptr<Level> m_level;
    std::vector<std::unique_ptr<Loco>> m_locos;

    float m_gravAngle = 0.0f;
    float m_maxAngle = 0.785398163f; // 45 degrees, selected by the user
    float m_jumpCharge = 0.0f;
    float m_mergeTimer = 0.0f;
    float m_callCooldown = 0.0f;
    float m_mergeEffect = 0.0f;
    float m_mergeFlash = 0.0f;
    int m_callVariation = 0;
    float m_splitCooldown = 0.0f;
    float m_goalTimer = 0.0f;
    int m_musicTier = 1;

    float m_camX = 2553.0f;
    float m_camY = 411.0f;
    float m_camZoom = 0.65f;

    float m_gameTime = 0.0f;
    float m_physicsAccumulator = 0.0f;
    int m_introStage=0;
    float m_introTime=0,m_introContactTime=0;
    float m_windNextRelease=0;
    int m_nativeHelp=0;
    float m_muiSceneTime=0,m_muiStartX=0,m_muiStartY=0,m_muiStartZoom=1,m_muiStartAngle=0;
    int m_tutorialPage = 0;
    float m_tutorialTime=0;
    bool m_tutorialPending=false;
    int m_muiPage=0;
    float m_muiTime=0;
};
