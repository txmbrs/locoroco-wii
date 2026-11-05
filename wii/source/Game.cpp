#include "Game.hpp"
#include <cstdio>
#include "Renderer.hpp"
#include "Audio.hpp"
#include "Dialogue.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

// Gravity in the Wii physics meter scale.
// Reconstruction tuning; these are not extracted PSP engine constants.
static constexpr float GRAVITY_MAGNITUDE = 9.81f;

Game::Game() {}
Game::~Game() {}

void Game::Init() {
    Renderer::ClearBackground(255, 255, 255); // Clean white sky
    ResetLevel();
    m_tutorialPending=true;
    m_tutorialPage = 0;
    m_tutorialTime=0;
}

void Game::ResetLevel() {
    // 1. Safely clear old physics entities first to avoid dangling pointers into deleted b2World
    m_locos.clear();
    m_level.reset();
    m_world.reset();

    // 2. Create Box2D world
    m_world = std::make_unique<b2World>(b2Vec2(0.0f, GRAVITY_MAGNITUDE));
    m_world->SetContactListener(&m_contactListener);

    // 3. Load Level terrain and items
    m_level = std::make_unique<Level>(m_world.get());

    // Drop onto the left flower, then bounce right and back to the ground spawn.
    auto spawn = m_level->GetOpeningFlowerPoint(false);
    spawn.y-=400;
    m_locos.push_back(std::make_unique<Loco>(m_world.get(), spawn.x, spawn.y, 1, m_level.get()));
    m_introStage=0;m_introTime=m_introContactTime=0;
    m_windNextRelease=0;
    m_tutorialPending=true;m_tutorialPage=0;m_tutorialTime=0;m_nativeHelp=0;

    m_camX = spawn.x;
    m_camY = spawn.y;
    m_camZoom = 0.43f;
    m_gravAngle = 0.0f;
    m_jumpCharge = 0.0f;
    m_mergeTimer = 0.0f;
    m_callCooldown=m_mergeEffect=m_mergeFlash=0.0f;m_callVariation=0;
    m_muiPage=0;m_muiTime=0;
    m_splitCooldown = 0.0f;
    m_goalTimer = 0.0f;
    m_gameTime = 0.0f;
    m_physicsAccumulator = 0.0f;
    m_musicTier = 1;
    Audio::PlayBGM(m_musicTier);
    m_state = STATE_PLAYING;
}

void Game::Update(float dt) {
    const auto& input = Input::GetState();

    if (input.restartPressed) {
        ResetLevel();
        return;
    }

    if (m_state == STATE_VICTORY) return;
    if(m_state==STATE_MUI_REWARD){
        float previousTime=m_muiSceneTime;m_muiTime+=dt;m_muiSceneTime+=dt;
        // The PSP MuiMui reference starts these two sounds after 24 and 36 frames.
        if(previousTime<.8f && m_muiSceneTime>=.8f)Audio::PlayWorldEvent(6);
        if(previousTime<1.2f && m_muiSceneTime>=1.2f)Audio::PlayWorldEvent(7);
        float x,y,width,duration;
        if(m_level->GetMuiCamera(x,y,width,duration)){
            float t=std::clamp(m_muiSceneTime/std::max(.001f,duration),0.f,1.f);
            Renderer::Camera camera;camera.x=m_muiStartX+(x-m_muiStartX)*t;camera.y=m_muiStartY+(y-m_muiStartY)*t;
            float zoom=Renderer::GetScreenWidth()/width;camera.scale=m_muiStartZoom+(zoom-m_muiStartZoom)*t;camera.angle=m_muiStartAngle*(1-t);Renderer::SetCamera(camera);
        }
        if(input.dialogueNextPressed && m_muiTime>.3f){
            m_muiTime=0;
            if(++m_muiPage>=2){auto camera=Renderer::GetCamera();m_camX=camera.x;m_camY=camera.y;m_camZoom=camera.scale;m_level->ClaimMuiReward();m_state=STATE_PLAYING;}
        }
        return;
    }
    if (m_state == STATE_TUTORIAL) {
        m_tutorialTime+=dt;
        if(input.dialogueNextPressed){if(m_nativeHelp){m_level->FinishTutorial();m_nativeHelp=0;m_state=STATE_PLAYING;}else if(++m_tutorialPage>=4)m_state=STATE_PLAYING;}
        return;
    }

    if (m_state == STATE_PAUSED) {
        if (input.pausePressed) m_state = STATE_PLAYING;
        return;
    }

    if (input.pausePressed) {
        m_state = STATE_PAUSED;
        return;
    }

    m_gameTime += dt;
    m_splitCooldown = std::max(0.0f, m_splitCooldown - dt);
    // The opening is optional choreography, never an input lock. Any deliberate
    // player action takes over before tilt, jump charging or scripted impulses.
    if(m_introStage<3 && (std::abs(input.tilt)>.20f || input.jumpHold ||
        input.jumpReleased || input.splitPressed || input.mergeHold)){
        m_introStage=3;m_introContactTime=0;
    }

    // 1. World Tilt Input
    float targetTilt = input.tilt * m_maxAngle;
    m_gravAngle += (targetTilt - m_gravAngle) * (1.0f-std::exp(-std::max(0.0f,dt)/0.090f));
    m_gravAngle = std::clamp(m_gravAngle, -m_maxAngle, m_maxAngle);

    // Update Box2D gravity vector based on tilt angle
    b2Vec2 grav(std::sin(m_gravAngle) * GRAVITY_MAGNITUDE, std::cos(m_gravAngle) * GRAVITY_MAGNITUDE);
    m_world->SetGravity(grav);

    // 2. Charged shoulder-button jump, with directional release.
    if (input.jumpHold) {
        if (m_jumpCharge < 100.0f) {
            m_jumpCharge += 280.0f * dt;
        }
    }

    if (input.jumpReleased || (!input.jumpHold && m_jumpCharge > 0.0f)) {
        if (m_jumpCharge > 2.0f) {
            float jumpFraction = std::min(100.0f, m_jumpCharge) / 100.0f;
            float jumpVel = 4.0f + jumpFraction * 9.5f;

            // Directional impulse based on jump release direction (-1 left, 0 straight, +1 right)
            float dirH = input.jumpDir;
            float hBoost = dirH * (1.6f + jumpFraction * 1.2f);

            // Vector components along the tilted world axes
            float jx = -std::sin(m_gravAngle) * jumpVel + std::cos(m_gravAngle) * hBoost;
            float jy = -std::cos(m_gravAngle) * jumpVel - std::sin(m_gravAngle) * hBoost;

            bool jumped = false;
            for (auto& loco : m_locos) {
                if (loco->IsGrounded()) {
                    loco->ApplyImpulse(jx, jy);
                    loco->OpenMouth(0.6f);
                    jumped = true;
                }
            }
            if (jumped) Audio::PlayJumpSound();
        }
        m_jumpCharge = 0.0f;
    }

    // 3. Split / Break Apart
    if (input.splitPressed) {
        SplitLocos();
    }

    // 4. Merge
    bool windInProgress=false;
    for(const auto& loco:m_locos)windInProgress=windInProgress || loco->IsInWind();
    m_mergeEffect=std::max(0.0f,m_mergeEffect-dt);
    m_mergeFlash=std::max(0.0f,m_mergeFlash-dt);
    if(input.mergeHold && m_locos.size()>1 && m_splitCooldown<=0.0f) {
        if(m_mergeEffect<=0.0f)m_mergeFlash=1.0f;
        m_mergeEffect=.25f;
        m_callCooldown-=dt;
        if(m_callCooldown<=0.0f) {
            Audio::PlayCallSound(m_callVariation++);m_callCooldown=1.2f;
            for(auto& loco:m_locos)loco->PlayCallAnimation();
        }
    } else m_callCooldown=0.0f;
    if (input.mergeHold && m_splitCooldown <= 0.0f && !windInProgress) {
        TryMergeLocos(dt);
    } else {
        m_mergeTimer = 0.0f;
    }

    // Fixed steps follow elapsed time, including PAL's 50 Hz video output.
    struct WindGroup {LocoData::Vec2 position;float radius,progress;};
    std::vector<WindGroup> windGroups;
    for(const auto& loco:m_locos)if(!loco->IsInWind() && !loco->HasFinishedWind()){
        float progress=m_level->WindEntryDistance(loco->GetPosition(),loco->GetPhysicalRadius());
        if(progress>=0)windGroups.push_back({loco->GetPosition(),std::max(950.0f,loco->GetPhysicalRadius()*2.0f+100.0f),progress});
    }
    if(!windGroups.empty()) {
        SplitLocos(true);
        for(auto& loco:m_locos)if(!loco->IsInWind() && !loco->HasFinishedWind()) {
            auto p=loco->GetPosition();float progress=-1;
            for(const auto& group:windGroups)if(std::hypot(p.x-group.position.x,p.y-group.position.y)<=group.radius){progress=group.progress;break;}
            if(progress>=0){
                m_windNextRelease=std::max(m_windNextRelease,m_gameTime);
                loco->EnterWind(progress,m_windNextRelease-m_gameTime);
                m_windNextRelease+=.14f;
            }
        }
    }
    m_physicsAccumulator += std::clamp(dt, 0.0f, 1.0f / 15.0f);
    while (m_physicsAccumulator + 0.0000001f >= 1.0f / 60.0f) {
        m_level->UpdateFlowerPhysics(1.0f/60.0f);
        for(auto& loco:m_locos)loco->UpdateWind(1.0f/60.0f);
        for(auto& loco:m_locos)loco->UpdateContactMaterial();
        m_world->Step(1.0f / 60.0f, 6, 2);
        m_physicsAccumulator = std::max(0.0f, m_physicsAccumulator - 1.0f / 60.0f);
    }

    // 6. Update Level & Locos
    m_level->Update(dt);

    int groupSize = 0;
    for (const auto& loco : m_locos) groupSize += loco->GetTotalSize();
    std::vector<Loco*> eventActors;eventActors.reserve(m_locos.size());
    for(auto& loco:m_locos){loco->Update(dt);eventActors.push_back(loco.get());}
    m_level->UpdateInteractions(eventActors,dt);
    if(m_introStage>=3 && !m_tutorialPending){int help=m_level->TakeTutorial();if(help){m_nativeHelp=help;m_tutorialTime=0;m_state=STATE_TUTORIAL;}}
    bool allAtGoal = !m_locos.empty();
    for (auto& loco : m_locos) {

        auto pos = loco->GetPosition();
        float radius = std::sqrt(1000.0f * static_cast<float>(loco->GetSize()));


        allAtGoal = allAtGoal && m_level->IsAtGoal(pos);

        // Check fruit collisions (eating fruit grows Loco size)
        LocoData::Vec2 fruitTarget;
        bool foodNearby=m_level->GetNearbyFruit(pos,radius+130.0f,fruitTarget);
        if(foodNearby)loco->AnticipateFood(fruitTarget);
        int fruitsEaten = m_level->CheckFruitCollisions(pos, radius);
        if (fruitsEaten > 0) {
            int growth = std::min(fruitsEaten, 20 - groupSize);
            loco->QueueGrowth(growth, 1.0f);
            if(foodNearby)loco->EatFruit(fruitTarget);
            groupSize += growth;
            Audio::PlayBiteSound();
        }

        // Check fly collisions
        m_level->CheckFlyCollisions(pos, radius);
        if(m_state==STATE_PLAYING && m_level->CheckMuiEncounter(pos,loco->GetPhysicalRadius())){
            auto camera=Renderer::GetCamera();m_muiStartX=camera.x;m_muiStartY=camera.y;m_muiStartZoom=camera.scale;m_muiStartAngle=camera.angle;m_muiSceneTime=0;m_muiPage=0;m_muiTime=0;m_state=STATE_MUI_REWARD;loco->OpenMouth(.8f);
        }
    }

    UpdateOpening(dt);
    int newTier = groupSize >= 5 ? 3 : (groupSize >= 2 ? 2 : 1);
    if (newTier != m_musicTier) {
        m_musicTier = newTier;
        Audio::SetBGMTier(m_musicTier);
    }
    m_goalTimer = allAtGoal ? m_goalTimer + dt : 0.0f;
    if (m_goalTimer >= 1.0f) {
        m_state = STATE_VICTORY;
        Audio::PlayJingle();
    }

    // 7. Dynamic Camera tracking, Velocity Look-Ahead, Speed Zoom & Zone Vista Zoom
    if (!m_locos.empty()) {
        float avgX = 0, avgY = 0;
        float avgVelX = 0, avgVelY = 0;
        float totalWeight = 0.0f;
        for (const auto& loco : m_locos) {
            auto p = loco->GetPosition();
            auto v = loco->GetVelocity();
            float weight = static_cast<float>(loco->GetTotalSize());
            totalWeight += weight;
            avgX += p.x * weight;
            avgY += p.y * weight;
            avgVelX += v.x * weight;
            avgVelY += v.y * weight;
        }
        avgX /= totalWeight;
        avgY /= totalWeight;
        avgVelX /= totalWeight;
        avgVelY /= totalWeight;

        // Frame the whole visible group in the rotated camera plane, including
        // body radii. A fixed vertical offset cropped the lower split children.
        float cosine=std::cos(m_gravAngle),sine=std::sin(m_gravAngle);
        float minX=1e9f,minY=1e9f,maxX=-1e9f,maxY=-1e9f;
        for (const auto& loco : m_locos) {
            auto p = loco->GetPosition();
            float dx=p.x-avgX,dy=p.y-avgY,r=loco->GetPhysicalRadius();
            float x=dx*cosine-dy*sine,y=dx*sine+dy*cosine;
            minX=std::min(minX,x-r);maxX=std::max(maxX,x+r);
            minY=std::min(minY,y-r);maxY=std::max(maxY,y+r);
        }
        float centerX=(minX+maxX)*.5f,centerY=(minY+maxY)*.5f;
        // Velocity prediction compensates the follow filter's delay rather
        // than placing the player permanently below the centre of the frame.
        float targetCamX=avgX+centerX*cosine+centerY*sine+std::clamp(avgVelX*5.0f,-80.0f,80.0f);
        float targetCamY=avgY-centerX*sine+centerY*cosine+std::clamp(avgVelY*5.0f,-80.0f,80.0f);
        float follow = 1.0f - std::exp(-dt / 0.10f);
        m_camX += (targetCamX - m_camX) * follow;
        m_camY += (targetCamY - m_camY) * follow;
        float targetZoom = 0.43f * Renderer::GetScreenHeight() / 480.0f;
        if(m_locos.size()>1) {
            float fit=std::min((Renderer::GetScreenWidth()-96.0f)/(maxX-minX),
                               (Renderer::GetScreenHeight()-96.0f)/(maxY-minY));
            targetZoom=std::max(.08f,std::min(targetZoom,fit));
        }
        float zoomTime=targetZoom<m_camZoom?.12f:.50f;
        m_camZoom += (targetZoom - m_camZoom) * (1.0f - std::exp(-dt / zoomTime));
    }

    if(m_introStage>=3 && m_tutorialPending) for(const auto& loco:m_locos) if(loco->IsGrounded() &&
        loco->GetPosition().y>m_level->GetOpeningFlowerPoint(true).y+120) {
        m_tutorialPending=false;m_state=STATE_TUTORIAL;break;
    }
    Renderer::Camera cam;
    float cameraOffsetX=0,cameraOffsetY=0;m_level->GetNativeCameraOffset(cameraOffsetX,cameraOffsetY);
    cam.x = m_camX+cameraOffsetX;
    cam.y = m_camY+cameraOffsetY;
    cam.scale = m_camZoom;
    cam.angle = m_gravAngle;
    if(m_mergeEffect>0.0f){float shake=std::min(1.0f,m_mergeEffect/.15f);
        cam.x+=std::sin(m_gameTime*83.0f)*1.6f*shake/m_camZoom;
        cam.y+=std::sin(m_gameTime*67.0f)*1.0f*shake/m_camZoom;}
    Renderer::SetCamera(cam);
}

void Game::UpdateOpening(float dt) {
    if(m_introStage>=3 || m_locos.empty())return;
    m_introTime+=dt;
    auto& loco=*m_locos.front();auto p=loco.GetPosition();
    auto left=m_level->GetOpeningFlowerPoint(false),right=m_level->GetOpeningFlowerPoint(true);
    auto ground=m_level->GetSpawnPoint();
    auto landing=m_introStage==0?left:(m_introStage==1?right:ground);
    bool near=std::abs(p.x-landing.x)<250 && std::abs(p.y-landing.y)<220;
    if(near && loco.IsGrounded())m_introContactTime+=dt;
    else if(m_introContactTime>0)m_introContactTime+=dt;
    if(m_introContactTime>.06f){
        m_introContactTime=0;
        if(m_introStage==2){m_introStage=3;return;}
        auto target=m_introStage==0?right:ground;
        if(m_introStage==0)target.y-=loco.GetPhysicalRadius()+2;
        float drop=std::max(0.0f,(target.y-p.y)*.02f);
        float flight=(2.0f+std::sqrt(4.0f+2*GRAVITY_MAGNITUDE*drop))/GRAVITY_MAGNITUDE;
        float vx=std::clamp((target.x-p.x)*.02f/flight,-8.0f,8.0f);
        auto v=loco.GetVelocity();loco.ApplyImpulse(vx-v.x,-2.0f-v.y);
        loco.OpenMouth(.5f);++m_introStage;
    }
    // Release control if a flower was missed; never strand the player in an intro.
    if(p.y>right.y+140 && loco.IsGrounded() && m_introTime>3)m_introStage=3;
    if(m_introTime>12)m_introStage=3;
}

void Game::SplitLocos(bool intoWind) {
    bool didSplit = false;
    std::vector<std::unique_ptr<Loco>> newLocos;
    for (auto& loco : m_locos) {
        int size = loco->GetTotalSize();
        if (size <= 1) {
            newLocos.push_back(std::move(loco));
            continue;
        }
        const auto origin = loco->GetPosition();
        const auto velocity = loco->GetVelocity();
        const float radius = Loco::CollisionRadius(1)*(intoWind?.6f:1.0f);
        std::vector<Level::SpawnCircle> occupied;
        for (const auto& other : m_locos) {
            if (other && other.get() != loco.get())
                occupied.push_back({other->GetPosition(), other->GetPhysicalRadius()});
        }
        for (const auto& other : newLocos)
            occupied.push_back({other->GetPosition(), other->GetPhysicalRadius()});

        std::vector<LocoData::Vec2> positions;
        for (int i = 0; i < size; ++i) {
            LocoData::Vec2 position;
            if (!m_level->FindFreePosition(origin, radius,
                                           std::max(intoWind?900.0f:0.0f,Loco::CollisionRadius(size) * 2.0f + 80.0f), position, occupied)) break;
            positions.push_back(position);
            occupied.push_back({position, radius});
        }
        if (static_cast<int>(positions.size()) != size) {
            // Do not delete the parent or lose collected fruit if the passage
            // cannot hold every mini. The player can split in the next open area.
            newLocos.push_back(std::move(loco));
            continue;
        }
        loco->DestroyPhysics();
        for (const auto& position : positions) {
            auto mini = std::make_unique<Loco>(m_world.get(), position.x, position.y, 1, m_level.get(),intoWind?.6f:1.0f);
            mini->ApplyImpulse(velocity.x, velocity.y);
            mini->OpenMouth(0.5f);
            newLocos.push_back(std::move(mini));
        }
        didSplit = true;
    }
    m_locos = std::move(newLocos);
    if (didSplit) {
        m_mergeTimer = 0.0f;
        m_splitCooldown = 0.35f;
        Audio::PlaySplitSound();
    }
}

void Game::TryMergeLocos(float dt) {
    if (m_locos.size() <= 1) { m_mergeTimer = 0.0f; return; }
    // Call toward one local leader, avoiding mutually attracted pairs that
    // stop short of the rest of the group. Obstacles still block the call.
    size_t leader = 0;
    for (size_t i = 1; i < m_locos.size(); ++i)
        if (m_locos[i]->GetSize() > m_locos[leader]->GetSize()) leader = i;
    auto destination = m_locos[leader]->GetPosition();
    for (size_t i = 0; i < m_locos.size(); ++i) {
        if (i == leader) continue;
        auto p = m_locos[i]->GetPosition();
        if (std::hypot(p.x - destination.x, p.y - destination.y) <= 720.0f &&
            m_level->IsPathClear(p, destination)) m_locos[i]->PullToward(destination, 6.0f);
    }
    m_mergeTimer += dt;
    if (m_mergeTimer < 0.3f) return;

    // Join a connected local group with a continuous outline transition.
    const size_t count = m_locos.size();
    std::vector<int> group(count);
    std::vector<std::vector<size_t>> pairs;
    for (size_t i = 0; i < count; ++i) group[i] = static_cast<int>(i);
    auto root = [&](int id) { while (group[id] != id) id = group[id]; return id; };
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            auto pa = m_locos[i]->GetPosition(), pb = m_locos[j]->GetPosition();
            float gap = m_locos[i]->GetPhysicalRadius() + m_locos[j]->GetPhysicalRadius() + 5.0f;
            if ((m_locos[i]->IsTouching(*m_locos[j]) || std::hypot(pa.x - pb.x, pa.y - pb.y) <= gap) &&
                m_level->IsPathClear(pa, pb)) {
                group[root(static_cast<int>(j))] = root(static_cast<int>(i));
                pairs.push_back({i,j});
            }
        }
    }
    std::vector<std::vector<size_t>> candidates;
    for (size_t seed = 0; seed < count; ++seed) {
        if (root(static_cast<int>(seed)) != static_cast<int>(seed)) continue;
        std::vector<size_t> indices;
        for (size_t i = 0; i < count; ++i) if (root(static_cast<int>(i)) == static_cast<int>(seed)) indices.push_back(i);
        if (indices.size() >= 2) candidates.push_back(std::move(indices));
    }
    // A touching chain may wrap around terrain or be too long to fit as one
    // circle. Still join local pairs rather than rejecting the entire group.
    std::stable_sort(pairs.begin(), pairs.end(), [&](const auto& a, const auto& b) {
        return m_locos[a[0]]->GetTotalSize()+m_locos[a[1]]->GetTotalSize() >
               m_locos[b[0]]->GetTotalSize()+m_locos[b[1]]->GetTotalSize();
    });
    candidates.insert(candidates.end(),pairs.begin(),pairs.end());
    for (const auto& indices : candidates) {
        int size = 0; float biggestFace = 1;
        LocoData::Vec2 center{0,0}; b2Vec2 velocity(0,0);
        std::vector<LocoData::Vec2> outline;
        std::vector<Level::SpawnCircle> occupied;
        for (size_t i = 0; i < count; ++i) {
            auto& loco = m_locos[i];
            if (std::find(indices.begin(),indices.end(),i) == indices.end()) {
                occupied.push_back({loco->GetPosition(), loco->GetPhysicalRadius()}); continue;
            }
            int weight = loco->GetTotalSize(); size += weight;
            auto point = loco->GetPosition(); center.x += point.x * weight; center.y += point.y * weight;
            velocity += static_cast<float>(weight) * loco->GetVelocity();
            auto skin = loco->GetOutline(); outline.insert(outline.end(), skin.begin(), skin.end());
            biggestFace = std::max(biggestFace, std::pow(static_cast<float>(loco->GetSize()), 0.15f));
        }
        if (size > 20) continue;
        center.x /= size; center.y /= size; velocity *= 1.0f / size;
        float radius = Loco::CollisionRadius(size);
        LocoData::Vec2 position;
        if (!m_level->FindFreePosition(center, radius, radius + 24.0f, position, occupied)) continue;
        bool clear = true;
        for (size_t i : indices) clear = clear && m_level->IsPathClear(m_locos[i]->GetPosition(), position);
        if (!clear) continue;
        for (size_t i : indices) m_locos[i]->DestroyPhysics();
        auto merged = std::make_unique<Loco>(m_world.get(), position.x, position.y, size, m_level.get());
        merged->ApplyImpulse(velocity.x, velocity.y);
        merged->AnimateFromOutline(std::move(outline), center, biggestFace);
        merged->OpenMouth(0.45f);
        m_locos[indices.front()] = std::move(merged);
        for (size_t n = indices.size(); n-- > 1;) m_locos.erase(m_locos.begin() + indices[n]);
        m_mergeTimer = 0.18f;
        Audio::PlayMergeSound();
        return;
    }
}

void Game::Render() {
    Renderer::BeginFrame();

    // 1. Draw Level Background (sky, distant clouds, vegetation)
    m_level->DrawBackground();
    // Calling darkens the backdrop behind the terrain and characters.
    if(m_mergeFlash>0.0f) {
        float fade=std::min(1.0f,m_mergeFlash*4.0f);
        float h=Renderer::GetScreenHeight();
        Renderer::DrawScreenRect(0,0,Renderer::GetScreenWidth(),h,
                                {0,0,0,static_cast<uint8_t>(255*fade)});
    }

    // 2. Draw Level Terrain (solid orange ground)
    m_level->DrawTerrain();

    // 3. Draw Collectibles (berries/fruits and pickies/flies ON TOP of the terrain!)
    m_level->DrawItems();

    // 4. Draw Locos (Kulche rolls ON TOP of the ground, never buried under it!)
    float singing=m_state==STATE_PLAYING?Audio::SingingLevel():0.f;
    for (auto& loco : m_locos)loco->Draw(singing);

    // 5. Draw Foreground Masks (cave ceilings / secret tunnel roofs)
    m_level->DrawMasks();

    // 4. Authentic Minimalist LocoRoco HUD in Top-Left (Screenshot 4 style!)
    int currentLocos = 0;
    for (const auto& l : m_locos) currentLocos += l->GetTotalSize();

    // Mini Loco Head Icon in Top-Left
    float hudScale=Renderer::GetScreenHeight()/480.0f;
    float iconX = 30.0f*hudScale;
    float iconY = 30.0f*hudScale;
    Renderer::DrawScreenSprite(SceneSprites::HUD_HEAD,iconX,iconY,40*hudScale,40*hudScale);

    // Number next to Loco icon
    char countBuf[16];
    std::snprintf(countBuf, sizeof(countBuf), "%d", currentLocos);
    const Renderer::Sprite* digits[]={&SceneSprites::HUD_DIGIT0,&SceneSprites::HUD_DIGIT1,&SceneSprites::HUD_DIGIT2,&SceneSprites::HUD_DIGIT3,&SceneSprites::HUD_DIGIT4,
        &SceneSprites::HUD_DIGIT5,&SceneSprites::HUD_DIGIT6,&SceneSprites::HUD_DIGIT7,&SceneSprites::HUD_DIGIT8,&SceneSprites::HUD_DIGIT9};
    float numberX=58*hudScale;
    for(const char* p=countBuf;*p;++p)if(*p>='0' && *p<='9'){
        Renderer::DrawScreenSprite(*digits[*p-'0'],numberX+12*hudScale,iconY,24*hudScale,36*hudScale);numberX+=23*hudScale;}

    // Jump Charge Indicator (subtle small bar under the counter if charging)
    if (m_jumpCharge > 5.0f) {
        float barW = (m_jumpCharge / 100.0f) * 40.0f;
        Renderer::DrawScreenRect(iconX, iconY + 26.0f*hudScale, barW, 4.0f, { 255, 193, 7, 255 });
    }

    if (m_state == STATE_TUTORIAL) {
        if(m_nativeHelp)Dialogue::DrawHint(m_nativeHelp,m_tutorialTime,Renderer::GetScreenWidth(),Renderer::GetScreenHeight());else Dialogue::Draw(m_tutorialPage,m_tutorialTime,Renderer::GetScreenWidth(),Renderer::GetScreenHeight());
    }
    if(m_state==STATE_MUI_REWARD)Dialogue::DrawReward(m_muiPage,m_muiTime,Renderer::GetScreenWidth(),Renderer::GetScreenHeight());
    if(m_state==STATE_MUI_REWARD){char info[64];std::snprintf(info,sizeof(info),"MuiMui %d / 3",m_level->GetPendingMui());Renderer::DrawScreenText(24,48,info,{255,255,255,255},1);}

    if (m_state == STATE_PAUSED) {
        Renderer::DrawScreenRect(0, 0, Renderer::GetScreenWidth(), Renderer::GetScreenHeight(), { 0, 0, 0, 140 });
        Renderer::DrawScreenText(240, 200, "PAUSED", { 255, 255, 255, 255 }, 3.0f);
        Renderer::DrawScreenText(160, 250, "Press PLUS to Resume", { 220, 220, 220, 255 }, 1.5f);
        Renderer::DrawScreenText(175, 280, "Press HOME to Exit", { 200, 200, 200, 255 }, 1.5f);
    }

    if (m_state == STATE_VICTORY) {
        float width = static_cast<float>(Renderer::GetScreenWidth());
        float height = static_cast<float>(Renderer::GetScreenHeight());
        Renderer::DrawScreenRect(0, 0, width, height, {255, 255, 255, 220});
        Renderer::DrawScreenText(width * 0.5f - 132, height * 0.5f - 60, "LEVEL CLEAR", {228, 150, 16, 255}, 3.0f);
        char score[80];
        std::snprintf(score, sizeof(score), "LOCOS %d/%d   PICKIES %d/%d", currentLocos,
                      m_level->GetTotalFruits() + 1, m_level->GetCollectedFlies(), m_level->GetTotalFlies());
        Renderer::DrawScreenText(width * 0.5f - 168, height * 0.5f, score, {84, 61, 19, 255}, 1.5f);
        char secrets[48];std::snprintf(secrets,sizeof(secrets),"MUIMUI %d/3",m_level->GetFoundMui());Renderer::DrawScreenText(width*.5f-168,height*.5f+24,secrets,{84,61,19,255},1.5f);
        Renderer::DrawScreenText(width * 0.5f - 168, height * 0.5f + 48, "MINUS / Z: PLAY AGAIN", {84, 61, 19, 255}, 1.5f);
    }

    Renderer::EndFrame();
}
