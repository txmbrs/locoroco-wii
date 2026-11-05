#include "Loco.hpp"
#include "Level.hpp"
#include "Renderer.hpp"
#include "PspSprites.hpp"
#include "Audio.hpp"
#include "SceneSprites.hpp"
#include "NativeAnimations.hpp"
#include <cmath>
#include <algorithm>

static constexpr float PHYSICS_SCALE = 0.02f;      // 1 pixel = 0.02 meters (50 pixels = 1 meter)
static constexpr float INV_PHYSICS_SCALE = 50.0f;  // 1 meter = 50 pixels
static constexpr float MINI_RIM_FREQUENCY = 3.8f * .70710678f;
static constexpr float MINI_CROSS_FREQUENCY = 2.4f * .70710678f;

Loco::Loco(b2World* world, float startX, float startY, int size, Level* level, float birthScale)
    : m_world(world), m_level(level), m_size(std::clamp(size, 1, 20)),
      m_spawnScale(std::clamp(birthScale, 0.55f, 1.0f)) {
    CreatePhysics(startX, startY);
}

Loco::~Loco() {
    DestroyPhysics();
}

void Loco::DestroyPhysics() {
    if (m_destroyed || !m_world) return;

    for (auto* j : m_rimJoints) m_world->DestroyJoint(j);
    for (auto* j : m_spokeJoints) m_world->DestroyJoint(j);
    for (auto* j : m_crossJoints) m_world->DestroyJoint(j);
    m_rimJoints.clear();
    m_spokeJoints.clear();
    m_crossJoints.clear();

    for (auto* b : m_perimBodies) m_world->DestroyBody(b);
    m_perimBodies.clear();

    if (m_centerBody) {
        m_world->DestroyBody(m_centerBody);
        m_centerBody = nullptr;
        m_groupFixture = nullptr;
    }

    m_destroyed = true;
}

void Loco::CreatePhysics(float startX, float startY) {
    // Only fixtures belonging to the same soft body ignore each other.
    // The previous shared -1 group disabled contacts between every Loco.
    bool used[64] = {};
    for (const b2Body* body = m_world->GetBodyList(); body; body = body->GetNext()) {
        for (const b2Fixture* fixture = body->GetFixtureList(); fixture; fixture = fixture->GetNext()) {
            int group = -fixture->GetFilterData().groupIndex;
            if (group > 0 && group < 64) used[group] = true;
        }
    }
    int group = 1;
    while (group < 63 && used[group]) ++group;
    m_collisionGroup = static_cast<int16>(-group);
    m_numPoints = 12 + (m_size / 2);
    m_radius = std::sqrt(1000.0f * static_cast<float>(m_size));

    float boxRadius = m_radius * m_spawnScale * PHYSICS_SCALE;
    float posX = startX * PHYSICS_SCALE;
    float posY = startY * PHYSICS_SCALE;

    // 1. Center Body (Core: solid core prevents inversion while allowing 65% jelly squish)
    b2BodyDef centerDef;
    centerDef.type = b2_dynamicBody;
    centerDef.position.Set(posX, posY);
    centerDef.bullet = true;
    centerDef.linearDamping = 0.10f;
    centerDef.angularDamping = 0.30f;
    m_centerBody = m_world->CreateBody(&centerDef);
    if(m_size==1){
        // One coarse contact per mini pair on the final landing replaces up to
        // 144 skin-particle pairs. Terrain still deforms the full soft skin.
        b2CircleShape proxy;proxy.m_radius=CollisionRadius(1)*m_spawnScale*PHYSICS_SCALE*.9f;
        b2FixtureDef group;group.shape=&proxy;group.friction=.08f;group.restitution=.05f;
        group.filter.groupIndex=m_collisionGroup;group.filter.categoryBits=0x0004;
        group.filter.maskBits=0;group.userData.pointer=2;
        m_groupFixture=m_centerBody->CreateFixture(&group);
    }

    b2CircleShape centerShape;
    centerShape.m_radius = boxRadius * 0.35f;
    b2FixtureDef centerFix;
    centerFix.shape = &centerShape;
    centerFix.density = 0.60f;
    centerFix.friction = 0.30f;
    centerFix.restitution = 0.15f;
    centerFix.isSensor = false; // Solid core so perimeter can squish up to 65% like real jelly!
    centerFix.filter.groupIndex = m_collisionGroup;
    centerFix.filter.categoryBits = 0x0002;
    centerFix.userData.pointer = m_size==1?2:0;
    m_centerBody->CreateFixture(&centerFix);

    // 2. Perimeter Bodies (ring of soft mass points that deform on impact)
    float perimRadius = (2.0f * M_PI * boxRadius / m_numPoints) * 0.45f;
    m_perimBodies.resize(m_numPoints);
    m_skinRestDirections.resize(m_numPoints);
    m_skinRotation = 0.0f;

    for (int i = 0; i < m_numPoints; ++i) {
        float angle = (i * 2.0f * M_PI) / m_numPoints;
        m_skinRestDirections[i].Set(std::cos(angle), std::sin(angle));
        float px = posX + std::cos(angle) * boxRadius;
        float py = posY + std::sin(angle) * boxRadius;

        b2BodyDef perimDef;
        perimDef.type = b2_dynamicBody;
        // Skin particles must transfer tangential contact forces to the ring,
        // rather than spinning individually like free rollers under a sled.
        perimDef.fixedRotation = true;
        perimDef.position.Set(px, py);
        // Box2D retains CCD against static and kinematic terrain without bullet.
        // Keep bullet on the core, avoiding skin-vs-skin TOI storms in groups.
        perimDef.bullet = false;
        perimDef.linearDamping = 0.08f;
        perimDef.angularDamping = 0.20f;
        b2Body* pBody = m_world->CreateBody(&perimDef);

        b2CircleShape perimShape;
        perimShape.m_radius = perimRadius;

        b2FixtureDef perimFix;
        perimFix.shape = &perimShape;
        perimFix.density = 0.30f;
        perimFix.friction = 1.0f;
        perimFix.userData.pointer = m_size==1?1:0;
        perimFix.restitution = 0.20f; // Bouncy jelly restitution
        perimFix.filter.groupIndex = m_collisionGroup;
        perimFix.filter.categoryBits = 0x0002;
        pBody->CreateFixture(&perimFix);

        m_perimBodies[i] = pBody;
    }

    // 3. Spoke Joints (Soft Jelly Core Springs: gentle damping for authentic wobbly gelatin ripple)
    float frequency = 3.2f;
    float dampingRatio = 0.50f;

    for (int i = 0; i < m_numPoints; ++i) {
        b2DistanceJointDef jointDef;
        jointDef.Initialize(m_centerBody, m_perimBodies[i], m_centerBody->GetPosition(), m_perimBodies[i]->GetPosition());
        jointDef.minLength = jointDef.length * 0.45f;
        jointDef.maxLength = jointDef.length * 1.12f;
        b2LinearStiffness(jointDef.stiffness, jointDef.damping, frequency, dampingRatio, m_centerBody, m_perimBodies[i]);
        jointDef.collideConnected = false;
        m_spokeJoints.push_back((b2DistanceJoint*)m_world->CreateJoint(&jointDef));
    }

    // 4. Rim Joints (Soft Skin Springs connecting neighboring perimeter vertices)
    float rimFrequency = m_size==1?MINI_RIM_FREQUENCY:3.8f;
    float rimDamping = 0.50f;

    for (int i = 0; i < m_numPoints; ++i) {
        int next = (i + 1) % m_numPoints;
        b2DistanceJointDef rimDef;
        rimDef.Initialize(m_perimBodies[i], m_perimBodies[next], m_perimBodies[i]->GetPosition(), m_perimBodies[next]->GetPosition());
        rimDef.minLength = rimDef.length * (m_size==1?.4f:.65f);
        rimDef.maxLength = rimDef.length * 1.15f;
        b2LinearStiffness(rimDef.stiffness, rimDef.damping, rimFrequency, rimDamping, m_perimBodies[i], m_perimBodies[next]);
        rimDef.collideConnected = false;
        m_rimJoints.push_back((b2DistanceJoint*)m_world->CreateJoint(&rimDef));
    }

    // 5. Cross Joints (Elastic support to preserve volume while allowing organic wobble)
    float crossFrequency = m_size==1?MINI_CROSS_FREQUENCY:2.4f;
    float crossDamping = 0.55f;
    int half = m_numPoints / 2;
    for (int i = 0; i < half; ++i) {
        int opp = i + half;
        b2DistanceJointDef crossDef;
        crossDef.Initialize(m_perimBodies[i], m_perimBodies[opp], m_perimBodies[i]->GetPosition(), m_perimBodies[opp]->GetPosition());
        crossDef.minLength = crossDef.length * (m_size==1?.4f:.65f);
        crossDef.maxLength = crossDef.length * 1.10f;
        b2LinearStiffness(crossDef.stiffness, crossDef.damping, crossFrequency, crossDamping, m_perimBodies[i], m_perimBodies[opp]);
        crossDef.collideConnected = false;
        m_crossJoints.push_back((b2DistanceJoint*)m_world->CreateJoint(&crossDef));
    }

    m_destroyed = false;
}

void Loco::UpdateContactMaterial() {
    if(m_size!=1 || m_destroyed)return;
    b2Vec2 down=m_world->GetGravity();if(down.Normalize()<.001f)return;
    b2Vec2 right(down.y,-down.x);bool left=false,rightSide=false;
    for(auto* body:m_perimBodies)for(auto* edge=body->GetContactList();edge;edge=edge->next){
        if(!edge->contact->IsTouching())continue;
        b2WorldManifold manifold;edge->contact->GetWorldManifold(&manifold);
        float side=b2Dot(manifold.normal,right);
        if(edge->contact->GetFixtureA()->GetBody()==body)side=-side;
        left=left || side>.3f;rightSide=rightSide || side<-.3f;
    }
    // Yield when terrain or neighbouring blobs pinch both sides. Open-ground traction remains
    // unchanged, so minis roll instead of sliding after leaving the tunnel.
    if(left && rightSide)m_wallSlipTime=.35f;
    else m_wallSlipTime=std::max(0.0f,m_wallSlipTime-1.0f/60.0f);
    float friction=m_wallSlipTime>0?.08f:1.0f;
    for(auto* body:m_perimBodies)body->GetFixtureList()->SetFriction(friction);
}

void Loco::Update(float dt) {
    m_nativeClock+=dt;
    if (m_destroyed || !m_centerBody) return;
    if(m_windExitGrace>0){
        m_windExitGrace=std::max(0.0f,m_windExitGrace-dt);
        if(m_windExitGrace==0){
            // Restore only the outer skin against other Locos. The internal
            // cores do not need mutual contacts on the final landing.
            auto* core=m_centerBody->GetFixtureList();auto filter=core->GetFilterData();filter.maskBits=0x0001;core->SetFilterData(filter);
            for(auto* body:m_perimBodies){auto* f=body->GetFixtureList();auto flags=f->GetFilterData();flags.maskBits=0x0001;f->SetFilterData(flags);}
            if(m_groupFixture){auto flags=m_groupFixture->GetFilterData();flags.maskBits=0x0006;m_groupFixture->SetFilterData(flags);}
        }
    }

    if (m_mouthOpenTimer > 0.0f) {
        m_mouthOpenTimer -= dt;
    }
    m_foodLookTimer=std::max(0.0f,m_foodLookTimer-dt);
    m_eatingTimer=std::max(0.0f,m_eatingTimer-dt);

    // Antenna pendulum physics with momentum
    b2Vec2 vel = m_centerBody->GetLinearVelocity();
    float targetAngle = -vel.x * 0.08f;
    float spring = (targetAngle - m_antennaAngle) * 35.0f;
    m_antennaVel += spring * dt;
    m_antennaVel *= (1.0f - 4.5f * dt);
    m_antennaAngle += m_antennaVel * dt;

    // Best-fit orientation of the entire deformable ring. Translation alone
    // does not imply rotation: a sliding or airborne body must not wind up
    // the face at an invented no-slip v/r speed.
    float dot=0.0f,cross=0.0f;
    for(int i=0;i<m_numPoints;++i) {
        b2Vec2 offset=m_perimBodies[i]->GetPosition()-m_centerBody->GetPosition();
        dot+=b2Dot(m_skinRestDirections[i],offset);
        cross+=b2Cross(m_skinRestDirections[i],offset);
    }
    if(dot*dot+cross*cross>0.000001f) {
        float angle=std::atan2(cross,dot);
        float delta=std::remainder(angle-m_skinRotation,2.0f*M_PI);
        m_visualRotation+=delta;
        m_skinRotation=angle;
    }
    m_mergeMorphTime = std::max(0.0f, m_mergeMorphTime - dt);

    // Newly divided miniatures expand continuously within the old body volume.
    // Moving spring targets, rather than teleporting skin nodes, lets the floor
    // resolve the expansion normally.
    if (m_spawnScale < 1.0f) {
        m_spawnScale = std::min(1.0f, m_spawnScale + dt * (0.40f / 0.24f));
        float radius = m_radius * PHYSICS_SCALE * m_spawnScale;
        auto resize = [](b2Body* body, float newRadius) {
            static_cast<b2CircleShape*>(body->GetFixtureList()->GetShape())->m_radius = newRadius;
            body->ResetMassData();
            body->SetAwake(true);
        };
        resize(m_centerBody, radius * 0.35f);
        if(m_groupFixture)static_cast<b2CircleShape*>(m_groupFixture->GetShape())->m_radius=CollisionRadius(1)*m_spawnScale*PHYSICS_SCALE*.9f;
        for (auto* body : m_perimBodies)
            resize(body, radius * 2.0f * M_PI / m_numPoints * 0.45f);
        auto adjust = [](b2DistanceJoint* joint, float length, float minimum, float maximum, float frequency, float damping) {
            joint->SetMaxLength(length * maximum);
            joint->SetMinLength(length * minimum);
            joint->SetLength(length);
            float stiffness, drag;
            b2LinearStiffness(stiffness, drag, frequency, damping, joint->GetBodyA(), joint->GetBodyB());
            joint->SetStiffness(stiffness); joint->SetDamping(drag);
        };
        for (auto* joint : m_spokeJoints) adjust(joint, radius, 0.45f, 1.12f, 3.2f, 0.5f);
        for (auto* joint : m_rimJoints) adjust(joint, 2.0f * radius * std::sin(M_PI / m_numPoints), m_size==1?.4f:.65f, 1.15f, m_size==1?MINI_RIM_FREQUENCY:3.8f, 0.5f);
        float crossLength = 2.0f * radius * std::sin(M_PI * (m_numPoints / 2) / m_numPoints);
        for (auto* joint : m_crossJoints) adjust(joint, crossLength, m_size==1?.4f:.65f, 1.1f, m_size==1?MINI_CROSS_FREQUENCY:2.4f, 0.55f);
    }
    // A shared velocity correction keeps spring motion intact while bounding
    // travel speed on long slopes. Values are in meters, not render pixels.
    b2Vec2 speed = GetVelocity();
    b2Vec2 limited(std::clamp(speed.x, -8.0f, 8.0f), std::clamp(speed.y, -14.0f, 14.0f));
    b2Vec2 correction = limited - speed;
    if (!m_inWind && correction.LengthSquared() > 0.0f) {
        m_centerBody->SetLinearVelocity(speed + correction);
        for (auto* body : m_perimBodies) body->SetLinearVelocity(body->GetLinearVelocity() + correction);
    }

    // Grow once, then morph from the old silhouette into the safe new body.
    // Scaling the old outline first and rebuilding it again caused a visible
    // double swell/snap and could draw the swollen outline through the floor.
    if (m_pendingGrowth > 0) {
        m_growthTimer -= dt;
        int targetSize = std::clamp(m_size + m_pendingGrowth, 1, 20);
        m_growthVisualScale = 1.0f;
        if (m_growthTimer <= 0.0f) {
            auto outline=GetOutline();
            auto center=GetPosition();
            float faceScale=std::pow(static_cast<float>(m_size),0.15f);
            float skinRadius=m_perimBodies[0]->GetFixtureList()->GetShape()->m_radius*INV_PHYSICS_SCALE;
            if (SetSize(targetSize)) {
                AnimateFromOutline(std::move(outline),center,faceScale,skinRadius);
                m_pendingGrowth = 0;
            } else {
                // A narrow tunnel may not fit the grown body yet. Keep the fruit
                // and retry after moving into open space rather than spawning in rock.
                m_growthTimer = 0.15f;
            }
            return;
        }
    }
}

void Loco::Draw(float singing) {
    if (m_destroyed || !m_centerBody) return;

    b2Vec2 centerMeters = m_centerBody->GetPosition();
    float centerX = centerMeters.x * INV_PHYSICS_SCALE;
    float centerY = centerMeters.y * INV_PHYSICS_SCALE;
    if(m_mergeMorphTime<=0){const auto& cam=Renderer::GetCamera();
        float view=std::hypot(float(Renderer::GetScreenWidth()),float(Renderer::GetScreenHeight()))*.5f/std::max(.08f,cam.scale);
        float safeRadius=GetPhysicalRadius()*3.f+80.f;
        if(std::hypot(centerX-cam.x,centerY-cam.y)>view+safeRadius)return;
    }

    // Collect perimeter vertices in world coordinates
    std::vector<LocoData::Vec2> points(m_numPoints);

    for (int i = 0; i < m_numPoints; ++i) {
        b2Vec2 p = m_perimBodies[i]->GetPosition();
        float px = p.x * INV_PHYSICS_SCALE;
        float py = p.y * INV_PHYSICS_SCALE;
        points[i] = { centerX + (px - centerX) * m_growthVisualScale,
                      centerY + (py - centerY) * m_growthVisualScale };
    }

    float morph = 1.0f;
    if (m_mergeMorphTime > 0.0f && m_mergeOutline.size() == points.size()) {
        morph = 1.0f - m_mergeMorphTime / 0.26f;
        morph = morph * morph * (3.0f - 2.0f * morph);
        for (size_t i = 0; i < points.size(); ++i) {
            points[i].x = m_mergeOutline[i].x + (points[i].x - m_mergeOutline[i].x) * morph;
            points[i].y = m_mergeOutline[i].y + (points[i].y - m_mergeOutline[i].y) * morph;
        }
        centerX = m_mergeStartCenter.x + (centerX - m_mergeStartCenter.x) * morph;
        centerY = m_mergeStartCenter.y + (centerY - m_mergeStartCenter.y) * morph;
    }
    // Render the outside of the skin fixtures, not their centers. Sample the
    // rounded offset at each corner so Chaikin smoothing cannot shrink away
    // the contact surface and leave the character visibly floating.
    std::vector<LocoData::Vec2> skin;
    for (size_t i=0;i<points.size();++i) {
        auto prev=points[(i+points.size()-1)%points.size()],p=points[i],next=points[(i+1)%points.size()];
        float a=std::atan2(-(p.x-prev.x),p.y-prev.y);
        float b=std::atan2(-(next.x-p.x),next.y-p.y);
        float turn=std::remainder(b-a,2.0f*M_PI);
        float radius=m_perimBodies[i]->GetFixtureList()->GetShape()->m_radius*INV_PHYSICS_SCALE*m_growthVisualScale;
        if(morph<1.0f && m_mergeStartSkinRadius>0.0f)
            radius=m_mergeStartSkinRadius+(radius-m_mergeStartSkinRadius)*morph;
        float x=std::cos(a)*radius,y=std::sin(a)*radius;
        float cosine=std::cos(turn/6.0f),sine=std::sin(turn/6.0f);
        for(int j=0;j<=6;++j) {
            skin.push_back({p.x+x,p.y+y});
            float nextX=x*cosine-y*sine;
            y=x*sine+y*cosine;x=nextX;
        }
    }
    points=std::move(skin);
    Renderer::DrawWorldBlob(points, m_bodyColor, m_outlineColor);

    // PSP features grow more slowly than body area. Scaling every facial part
    // by sqrt(population) produced enormous eyes and a different silhouette.
    float faceScale=std::pow(float(m_size),.15f)*m_spawnScale*m_growthVisualScale;
    if(morph<1)faceScale=m_mergeStartFaceScale+(faceScale-m_mergeStartFaceScale)*morph;
    float cosine=std::cos(m_visualRotation),sine=std::sin(m_visualRotation),topExtent=0;
    for(auto p:points)topExtent=std::max(topExtent,(p.x-centerX)*sine-(p.y-centerY)*cosine);
    const char* clip="stay";float time=m_nativeClock;bool loop=true;
    if(m_nativeEatStart>=0 && m_nativeClock-m_nativeEatStart<=NativeAnimations::Duration("eat")){
        clip="eat";time=m_nativeClock-m_nativeEatStart;loop=false;
    }else if(m_nativeCallStart>=0 && m_nativeClock-m_nativeCallStart<=NativeAnimations::Duration("call")){
        clip="call";time=m_nativeClock-m_nativeCallStart;loop=false;
    }else if(m_mouthOpenTimer>0){clip="waaa";time=m_nativeClock-m_nativeOpenStart;loop=false;}
    else if(m_foodLookTimer>0){clip="kyoro";time=m_nativeClock;}
    else if(singing>.05f){clip="uta";time=m_nativeClock;}
    float anchorY=-topExtent+6.f*faceScale;
    auto mouth=NativeAnimations::DrawCharacter(clip,time,loop,centerX-sine*anchorY,centerY+cosine*anchorY,
        m_visualRotation,4.f*faceScale,m_bodyColor,m_size>1,m_antennaAngle*.35f);
    float duration=NativeAnimations::Duration("eat"),progress=(m_nativeClock-m_nativeEatStart)/std::max(duration,.001f);
    if(m_nativeEatStart>=0 && progress>=0 && progress<.3f){float t=progress/.3f;
        Renderer::DrawWorldSprite(SceneSprites::FRUIT,m_foodTarget.x+(mouth.x-m_foodTarget.x)*t,
            m_foodTarget.y+(mouth.y-m_foodTarget.y)*t,40*(1-t),40*(1-t));}
}

std::vector<LocoData::Vec2> Loco::GetOutline() const {
    std::vector<LocoData::Vec2> result;
    for (auto* body : m_perimBodies) {
        const auto& p = body->GetPosition(); result.push_back({p.x * INV_PHYSICS_SCALE, p.y * INV_PHYSICS_SCALE});
    }
    return result;
}

void Loco::AnimateFromOutline(std::vector<LocoData::Vec2> points, const LocoData::Vec2& center, float faceScale, float skinRadius) {
    m_mergeStartSkinRadius=skinRadius;
    if (points.size() < 3) return;
    std::sort(points.begin(), points.end(), [](const auto& a, const auto& b) { return a.x == b.x ? a.y < b.y : a.x < b.x; });
    auto cross = [](const auto& a, const auto& b, const auto& c) { return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x); };
    std::vector<LocoData::Vec2> hull;
    for (const auto& p : points) {
        while (hull.size() >= 2 && cross(hull[hull.size()-2], hull.back(), p) <= 0) hull.pop_back();
        hull.push_back(p);
    }
    size_t lower = hull.size();
    for (int i = static_cast<int>(points.size()) - 2; i >= 0; --i) {
        while (hull.size() > lower && cross(hull[hull.size()-2], hull.back(), points[i]) <= 0) hull.pop_back();
        hull.push_back(points[i]);
    }
    hull.pop_back();
    if (hull.size() < 3) return;
    auto right = std::max_element(hull.begin(), hull.end(), [](const auto& a, const auto& b) { return a.x < b.x; });
    std::rotate(hull.begin(), right, hull.end());
    float perimeter = 0;
    std::vector<float> lengths;
    for (size_t i = 0; i < hull.size(); ++i) {
        auto a = hull[i], b = hull[(i + 1) % hull.size()];
        lengths.push_back(std::hypot(a.x - b.x, a.y - b.y)); perimeter += lengths.back();
    }
    m_mergeOutline.clear();
    for (int i = 0; i < m_numPoints; ++i) {
        float distance = perimeter * i / m_numPoints;
        size_t edge = 0;
        while (edge + 1 < lengths.size() && distance > lengths[edge]) { distance -= lengths[edge]; ++edge; }
        float t = lengths[edge] > 0.001f ? distance / lengths[edge] : 0;
        auto a = hull[edge], b = hull[(edge + 1) % hull.size()];
        m_mergeOutline.push_back({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t});
    }
    m_mergeStartCenter = center; m_mergeStartFaceScale = faceScale; m_mergeMorphTime = 0.26f;
}

void Loco::ApplyJump(float strength) {
    if (m_destroyed || !m_centerBody) return;

    b2Vec2 impulse(0.0f, -strength * 0.015f * m_centerBody->GetMass());
    m_centerBody->ApplyLinearImpulseToCenter(impulse, true);

    for (auto* b : m_perimBodies) {
        b->ApplyLinearImpulseToCenter(b2Vec2(0.0f, -strength * 0.01f * b->GetMass()), true);
    }

    OpenMouth(0.5f);
}

void Loco::ApplyImpulse(float vx, float vy) {
    if (m_destroyed || !m_centerBody) return;
    b2Vec2 velChange(vx, vy);
    m_centerBody->ApplyLinearImpulseToCenter(m_centerBody->GetMass() * velChange, true);
    for (auto* b : m_perimBodies) {
        b->ApplyLinearImpulseToCenter(b->GetMass() * velChange, true);
    }
}

LocoData::Vec2 Loco::GetPosition() const {
    if (!m_centerBody) return { 0.0f, 0.0f };
    b2Vec2 p = m_centerBody->GetPosition();
    return { p.x * INV_PHYSICS_SCALE, p.y * INV_PHYSICS_SCALE };
}

b2Vec2 Loco::GetVelocity() const {
    if (!m_centerBody) return b2Vec2_zero;
    return m_centerBody->GetLinearVelocity();
}

bool Loco::IsGrounded() const {
    b2Vec2 up = -m_world->GetGravity();
    if (up.Normalize() < 0.001f) up.Set(0.0f, -1.0f);
    for (auto* body : m_perimBodies) {
        for (b2ContactEdge* edge = body->GetContactList(); edge; edge = edge->next) {
            if (!edge->contact->IsTouching()) continue;
            b2WorldManifold manifold;
            edge->contact->GetWorldManifold(&manifold);
            b2Vec2 normal = manifold.normal;
            if (edge->contact->GetFixtureA()->GetBody() == body) normal = -normal;
            if (b2Dot(normal, up) > 0.25f) return true;
        }
    }
    return false;
}

float Loco::CollisionRadius(int size) {
    float radius = std::sqrt(1000.0f * static_cast<float>(std::clamp(size, 1, 20)));
    int points = 12 + std::clamp(size, 1, 20) / 2;
    // Skin nodes have their own circle radius, outside the visual ring.
    return radius * (1.0f + 2.0f * M_PI * 0.45f / points);
}

bool Loco::IsTouching(const Loco& other) const {
    if (m_destroyed || other.m_destroyed) return false;
    for(auto* edge=m_centerBody->GetContactList();edge;edge=edge->next)
        if(edge->other->GetFixtureList()->GetFilterData().groupIndex==other.m_collisionGroup && edge->contact->IsTouching())return true;
    for (auto* body : m_perimBodies) {
        for (b2ContactEdge* edge = body->GetContactList(); edge; edge = edge->next) {
            const auto* fixture = edge->other->GetFixtureList();
            if (fixture && fixture->GetFilterData().groupIndex == other.m_collisionGroup && edge->contact->IsTouching())
                return true;
        }
    }
    return false;
}

bool Loco::SetSize(int size) {
    int clampedSize = std::clamp(size, 1, 20);
    if (m_size == clampedSize) return true;
    b2Vec2 oldVelocity = GetVelocity();
    auto position = GetPosition();
    const float radius = CollisionRadius(clampedSize);
    std::vector<Level::SpawnCircle> occupied;
    for (const b2Body* body = m_world->GetBodyList(); body; body = body->GetNext()) {
        const auto* fixture = body->GetFixtureList();
        if (body->GetType() != b2_dynamicBody || !fixture ||
            fixture->GetFilterData().groupIndex == m_collisionGroup) continue;
        const auto& p = body->GetPosition();
        occupied.push_back({{p.x * INV_PHYSICS_SCALE, p.y * INV_PHYSICS_SCALE},
                            fixture->GetShape()->m_radius * INV_PHYSICS_SCALE});
    }
    if (m_level && !m_level->FindFreePosition(position, radius, radius + 48.0f, position, occupied)) return false;
    if (!m_level) {
        b2Vec2 up = -m_world->GetGravity();
        if (up.Normalize() < 0.001f) up.Set(0.0f, -1.0f);
        float lift = std::max(0.0f, radius - CollisionRadius(m_size)) + 2.0f;
        position.x += up.x * lift;
        position.y += up.y * lift;
    }
    DestroyPhysics();
    m_size = clampedSize;
    m_spawnScale = 1.0f;
    CreatePhysics(position.x, position.y);
    m_centerBody->SetLinearVelocity(oldVelocity);
    for (auto* body : m_perimBodies) body->SetLinearVelocity(oldVelocity);
    OpenMouth(0.6f);
    return true;
}

void Loco::QueueGrowth(int amount, float delay) {
    if (amount <= 0 || m_size + m_pendingGrowth >= 20) return;
    m_pendingGrowth = std::min(20 - m_size, m_pendingGrowth + amount);
    m_growthDelay = delay;
    m_growthTimer = delay;
    m_growthVisualScale = 1.0f;
    OpenMouth(delay + 0.35f);
}

void Loco::OpenMouth(float duration) {
    m_mouthOpenTimer = duration;
    m_nativeOpenStart=m_nativeClock;
}

void Loco::AnticipateFood(const LocoData::Vec2& target) { m_foodTarget=target;m_foodLookTimer=.12f; }
void Loco::EatFruit(const LocoData::Vec2& target){
    m_foodTarget=target;m_nativeEatStart=m_nativeClock;m_eatingTimer=NativeAnimations::Duration("eat");
}
void Loco::PlayCallAnimation(){
    if(m_nativeClock-m_nativeCallStart>NativeAnimations::Duration("call"))m_nativeCallStart=m_nativeClock;
}

void Loco::EnterWind(float distance,float delay) {
    m_inWind=true;m_windExitGrace=0;m_windDistance=std::max(0.0f,distance);m_windDelay=delay+std::max(0.0f,-distance)/1000.0f;
    m_centerBody->SetBullet(false);OpenMouth(.4f);
    auto filter=[](b2Body* body){auto* fixture=body->GetFixtureList();auto f=fixture->GetFilterData();f.maskBits&=~0x0002;fixture->SetFilterData(f);};
    filter(m_centerBody);for(auto* body:m_perimBodies)filter(body);
}
void Loco::UpdateWind(float dt) {
    if(!m_inWind || !m_level)return;
    auto before=m_level->WindPoint(m_windDistance),actual=GetPosition();
    float gap=std::hypot(before.x-actual.x,before.y-actual.y);
    // Queue time starts at the inlet, rather than expiring while a distant
    // mini is still approaching and releasing the whole group together.
    if(gap<180)m_windDelay=std::max(0.0f,m_windDelay-dt);
    bool captured=gap<180.0f && m_windDelay<=0;
    if(captured)m_windDistance+=1000.0f*dt;
    float progress=std::min(m_windDistance,m_level->WindLength());
    auto target=m_level->WindPoint(progress),next=m_level->WindPoint(std::min(progress+30,m_level->WindLength()));
    b2Vec2 tangent(next.x-target.x,next.y-target.y);if(tangent.Normalize()<.001f)tangent.Set(0,1);
    auto position=GetPosition();
    // Pull into the inlet first. Adding the full forward flow outside the
    // stream pushed approaching bodies away from the capture point.
    b2Vec2 desired=(captured?20.0f:0.0f)*tangent+8.0f*b2Vec2((target.x-position.x)*PHYSICS_SCALE,(target.y-position.y)*PHYSICS_SCALE);
    if(desired.Length()>32)desired*=32.0f/desired.Length();
    b2Vec2 correction=desired-GetVelocity();
    // Shared velocity correction preserves internal deformation. Wind carries
    // the existing bodies; it never teleports or disables terrain collision.
    m_centerBody->SetLinearVelocity(desired);
    float radius=m_radius*m_spawnScale*PHYSICS_SCALE;
    float cosine=std::cos(m_skinRotation),sine=std::sin(m_skinRotation);
    for(int i=0;i<m_numPoints;++i){
        auto* body=m_perimBodies[i];auto rest=m_skinRestDirections[i];
        b2Vec2 ideal=radius*b2Vec2(rest.x*cosine-rest.y*sine,rest.x*sine+rest.y*cosine);
        b2Vec2 offset=body->GetPosition()-m_centerBody->GetPosition();
        b2Vec2 relative=body->GetLinearVelocity()+correction-desired;
        // Wind carries the whole outline through turns. Damped restoration
        // releases trapped banana-shaped folds without moving body positions.
        b2Vec2 restore=70.0f*dt*(ideal-offset);
        if(restore.LengthSquared()>1.0f)restore*=1.0f/restore.Length();
        body->SetLinearVelocity(desired+std::max(0.0f,1-14*dt)*relative+restore);
    }
    if(m_windDistance>=m_level->WindLength()) {
        m_inWind=false;m_windFinished=true;
        // Static-terrain CCD is still active without bullet. Turning bullet
        // back on for overlapping cores can cause a large dynamic-vs-dynamic TOI spike.
        m_centerBody->SetBullet(false);
        m_windExitGrace=.65f+(-m_collisionGroup%7)*.05f;
    }
}

void Loco::PullToward(const LocoData::Vec2& target, float force) {
    if (m_destroyed || !m_centerBody) return;
    b2Vec2 diff(target.x * PHYSICS_SCALE - m_centerBody->GetPosition().x,
                target.y * PHYSICS_SCALE - m_centerBody->GetPosition().y);
    float distance = diff.Normalize();
    if (distance < 0.01f) return;
    auto pull = [&](b2Body* body) {
        float along = b2Dot(body->GetLinearVelocity(), diff);
        float acceleration = std::clamp(force * std::min(distance, 2.0f) - along * 2.0f, -12.0f, 12.0f);
        body->ApplyForceToCenter(body->GetMass() * acceleration * diff, true);
    };
    pull(m_centerBody);
    for (auto* body : m_perimBodies) pull(body);
}

bool Loco::IsTouchingBody(const b2Body* body)const{
    if(m_destroyed || m_inWind)return false;
    auto touches=[&](b2Body* part){if(!part)return false;for(auto* e=part->GetContactList();e;e=e->next)if(e->other==body && e->contact->IsTouching())return true;return false;};
    if(touches(m_centerBody))return true;
    for(auto* part:m_perimBodies)if(touches(part))return true;
    return false;
}
