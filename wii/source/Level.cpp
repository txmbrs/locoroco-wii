#include "Level.hpp"
#include "Loco.hpp"
#include "NativeLevelData.hpp"
#include "SceneSprites.hpp"
#include "PhysicsGeometry.hpp"
#include <gccore.h>
#include <algorithm>
#include <cmath>
static constexpr float PHYSICS_SCALE=.02f;
using namespace NativeLevel;
namespace {
class TerrainRay : public b2RayCastCallback {
public:
    bool blocked = false;
    float ReportFixture(b2Fixture* fixture, const b2Vec2&, const b2Vec2&, float) override {
        if (fixture->IsSensor() || fixture->GetFilterData().categoryBits != 1) return -1.0f;
        blocked = true;
        return 0.0f;
    }
};
}


Level::Level(b2World* world):m_world(world) {
    b2BodyDef base;auto* ground=world->CreateBody(&base);m_terrainBodies.push_back(ground);
    for(int i=0;i<BODY_COUNT;++i){const auto& b=BODIES[i];b2BodyDef d;
        d.position.Set(b.x*PHYSICS_SCALE,b.y*PHYSICS_SCALE);
        d.type=b.kind==2 || b.kind==3?b2_dynamicBody:(b.kind==0 || b.kind==1 || b.kind==4 || b.kind==5?b2_kinematicBody:b2_staticBody);
        d.angularDamping=b.kind==2?.18f:2.f;auto* body=world->CreateBody(&d);body->SetGravityScale(0);
        m_surfaceBodies.push_back(body);m_terrainBodies.push_back(body);

    }
    m_nativeEdgeFixtures.resize(EDGE_COUNT,nullptr);
    for(int i=0;i<EDGE_COUNT;++i){const auto& e=EDGES[i];b2Body* body=e.body<0?ground:m_surfaceBodies[e.body];
        auto origin=body->GetPosition();b2Vec2 a(e.ax*PHYSICS_SCALE,e.ay*PHYSICS_SCALE),b(e.bx*PHYSICS_SCALE,e.by*PHYSICS_SCALE);a-=origin;b-=origin;
        b2FixtureDef f;f.friction=e.friction;f.restitution=e.body>=0 && BODIES[e.body].kind==4?.32f:0;
        b2EdgeShape edge;edge.SetTwoSided(a,b);f.shape=&edge;
        if(e.body>=0 && (BODIES[e.body].kind==2 || BODIES[e.body].kind==3)){
            b2Vec2 vs[3]={{0,0},a,b};b2PolygonShape triangle;
            if(PhysicsGeometry::SetTriangle(triangle,vs)){f.shape=&triangle;f.density=.08f;body->CreateFixture(&f);}
        }else m_nativeEdgeFixtures[i]=body->CreateFixture(&f);
    }
    for(int i=0;i<BODY_COUNT;++i){const auto& b=BODIES[i];auto* body=m_surfaceBodies[i];
        if(b.kind==2 || b.kind==3){b2RevoluteJointDef j;j.Initialize(ground,body,body->GetPosition());
            if(b.kind==3){j.enableLimit=true;j.lowerAngle=std::min(0.f,NativeAnimations::WorldEndAngle(b.source))-.02f;j.upperAngle=std::max(0.f,NativeAnimations::WorldEndAngle(b.source))+.02f;j.enableMotor=true;j.maxMotorTorque=80;}
            auto* joint=static_cast<b2RevoluteJoint*>(world->CreateJoint(&j));if(b.kind==3){m_bridgeBody=body;m_bridgeJoint=joint;}}
        if(b.kind==4)m_flowerSurfaces.push_back({i,-1,body,{b.x,b.y}});
    }
    for(int i=0;i<FRUIT_COUNT;++i){FruitItem f;f.basePos={FRUITS[i].centerX,FRUITS[i].centerY};f.surface=FRUITS[i].body;m_fruits.push_back(f);}
    for(int i=0;i<FLY_COUNT;++i){FlyItem f;f.basePos={FLIES[i].x,FLIES[i].y};m_flies.push_back(f);}
    // Immutable lookup tables: geometry and wind points come from the same map.
    if(WIND_COUNT>0){m_windDistances.resize(WIND_COUNT);for(int i=1;i<WIND_COUNT;++i)
        m_windDistances[i]=m_windDistances[i-1]+std::hypot(WIND[i].x-WIND[i-1].x,WIND[i].y-WIND[i-1].y);}
    bool haveStatic=false;
    for(int i=0;i<EDGE_COUNT;++i)if(EDGES[i].body<0){const auto& e=EDGES[i];
        float lo=std::min(e.ay,e.by),hi=std::max(e.ay,e.by);
        if(!haveStatic){m_edgeMinY=lo;m_edgeMaxY=hi;haveStatic=true;}
        else{m_edgeMinY=std::min(m_edgeMinY,lo);m_edgeMaxY=std::max(m_edgeMaxY,hi);}}
    if(haveStatic && m_edgeMaxY>m_edgeMinY){
        constexpr int N=256;m_edgeBins.resize(N);m_edgeBinScale=N/(m_edgeMaxY-m_edgeMinY);
        for(int i=0;i<EDGE_COUNT;++i)if(EDGES[i].body<0){const auto& e=EDGES[i];
            int first=std::clamp(int((std::min(e.ay,e.by)-m_edgeMinY)*m_edgeBinScale),0,N-1);
            int last=std::clamp(int((std::max(e.ay,e.by)-m_edgeMinY)*m_edgeBinScale),0,N-1);
            for(int bin=first;bin<=last;++bin)m_edgeBins[bin].push_back(i);}}
    m_muiFoot=MUI;
    m_nativePoses.resize(BODY_COUNT);
    // All imported map textures are immutable and flushed once, before first GPU use.
    int maxTexture=-1;for(int i=0;i<PIECE_COUNT;++i)maxTexture=std::max(maxTexture,PIECES[i].texture);
    for(int i=0;i<=maxTexture;++i)DCFlushRange(const_cast<uint8_t*>(TEXTURES[i].pixels),TEXTURES[i].width*TEXTURES[i].height*4);
}
Level::~Level(){for(auto* body:m_terrainBodies)m_world->DestroyBody(body);}
LocoData::Vec2 Level::NativePoint(LocoData::Vec2 p,int body) const {
    if(body<0)return p;
    const auto& b=BODIES[body];const auto& scale=m_nativePoses[body];auto q=m_surfaceBodies[body]->GetWorldPoint({(p.x-b.x)*scale.sx*PHYSICS_SCALE,(p.y-b.y)*scale.sy*PHYSICS_SCALE});return {q.x/PHYSICS_SCALE,q.y/PHYSICS_SCALE};
}
LocoData::Vec2 Level::GetOpeningFlowerPoint(bool right) const {auto p=OPENING[right?1:0];p.x+=right?-60.f:60.f;return p;}
LocoData::Vec2 Level::GetSpawnPoint() const {
    auto p=OPENING[1];p.x-=180;float y=1e30f;
    for(int i=0;i<EDGE_COUNT;++i){auto e=EDGES[i];if(e.body>=0 || (e.ax>p.x)==(e.bx>p.x))continue;
        float hit=e.ay+(p.x-e.ax)*(e.by-e.ay)/(e.bx-e.ax);if(hit>p.y+30)y=std::min(y,hit);}
    if(y<1e29f)p.y=y-48;
    return p;
}
LocoData::Vec2 Level::GetFruitRoot(const FruitItem& f) const{return NativePoint({FRUITS[&f-m_fruits.data()].x,FRUITS[&f-m_fruits.data()].y},f.surface);}
LocoData::Vec2 Level::GetFruitCenter(const FruitItem& f) const{return NativePoint(f.basePos,f.surface);}
bool Level::GetNearbyFruit(const LocoData::Vec2& p,float radius,LocoData::Vec2& target)const {
    bool found=false;for(const auto& f:m_fruits)if(!f.eaten){auto q=GetFruitCenter(f);float d=std::hypot(q.x-p.x,q.y-p.y);if(d<radius){radius=d;target=q;found=true;}}return found;
}
bool Level::IsCircleFree(const LocoData::Vec2& p,float radius) const {
    bool inside=false;
    if(!m_edgeBins.empty() && p.y>=m_edgeMinY && p.y<=m_edgeMaxY){
        int bin=std::clamp(int((p.y-m_edgeMinY)*m_edgeBinScale),0,int(m_edgeBins.size())-1);
        for(int i:m_edgeBins[bin]){const auto& e=EDGES[i];
            if((e.ay>p.y)!=(e.by>p.y) && p.x<(e.bx-e.ax)*(p.y-e.ay)/(e.by-e.ay)+e.ax)inside=!inside;}
    }
    if(inside)return false;
    b2CircleShape circle;circle.m_radius=std::max(.01f,radius)*PHYSICS_SCALE;b2Transform t;t.Set({p.x*PHYSICS_SCALE,p.y*PHYSICS_SCALE},0);
    struct Query:b2QueryCallback {b2CircleShape* c;b2Transform* t;bool hit=false;Query(b2CircleShape* c,b2Transform* t):c(c),t(t){}
        bool ReportFixture(b2Fixture* f) override {if(f->IsSensor())return true;auto* b=f->GetBody();
            // Loco core/skin categories are 2/4. Rotating level bodies remain category 1.
            if(f->GetFilterData().categoryBits!=1)return true;
            for(int i=0;i<f->GetShape()->GetChildCount();++i)if(b2TestOverlap(c,0,f->GetShape(),i,*t,b->GetTransform())){hit=true;return false;}
            return true;}} query(&circle,&t);
    b2AABB box;b2Vec2 r(circle.m_radius+.01f,circle.m_radius+.01f);box.lowerBound=t.p-r;box.upperBound=t.p+r;m_world->QueryAABB(&query,box);return !query.hit;
}
bool Level::IsPathClear(const LocoData::Vec2& from, const LocoData::Vec2& to) const {
    if (std::hypot(to.x - from.x, to.y - from.y) < 0.01f) return true;
    TerrainRay ray;
    m_world->RayCast(&ray, b2Vec2(from.x * PHYSICS_SCALE, from.y * PHYSICS_SCALE),
                    b2Vec2(to.x * PHYSICS_SCALE, to.y * PHYSICS_SCALE));
    return !ray.blocked;
}

bool Level::FindFreePosition(const LocoData::Vec2& origin, float radius, float maxDistance,
                             LocoData::Vec2& result, const std::vector<SpawnCircle>& occupied) const {
    auto fits = [&](const LocoData::Vec2& point) {
        for (const auto& other : occupied) {
            float clearance = radius + other.radius + 1.0f;
            float dx = point.x - other.position.x, dy = point.y - other.position.y;
            if (dx * dx + dy * dy < clearance * clearance) return false;
        }
        return IsPathClear(origin, point) && IsCircleFree(point, radius + 1.5f);
    };
    if (fits(origin)) { result = origin; return true; }
    b2Vec2 up = -m_world->GetGravity();
    if (up.Normalize() < 0.001f) up.Set(0.0f, -1.0f);
    for (float distance = 8.0f; distance <= maxDistance; distance += 8.0f) {
        int samples = std::max(16, static_cast<int>(distance * 2.0f * M_PI / 12.0f));
        for (int i = 0; i < samples; ++i) {
            float angle = i * 2.0f * M_PI / samples;
            LocoData::Vec2 point = {origin.x + distance * (up.x * std::cos(angle) - up.y * std::sin(angle)),
                                   origin.y + distance * (up.x * std::sin(angle) + up.y * std::cos(angle))};
            if (fits(point)) { result = point; return true; }
        }
    }
    return false;
}


void Level::RevealSecrets(const LocoData::Vec2&,float,float){}
bool Level::IsAtGoal(const LocoData::Vec2& p)const{return std::hypot(p.x-GOAL.x,p.y-GOAL.y)<180;}
void Level::UpdateNativeAnimations(float dt){
    m_nativeTime+=dt;
    for(int i=0;i<BODY_COUNT;++i){const auto& source=BODIES[i];
        if(source.kind!=0 && source.kind!=1 && source.kind!=3 && source.kind!=5)continue;
        float time=m_nativeTime;bool collision=true;
        bool triggered=m_events.Motion(source.source,time,collision);
        NativeAnimations::WorldPose pose;
        if(!NativeAnimations::SampleWorld(source.source,time,pose,triggered))continue;
        auto old=m_nativePoses[i];m_nativePoses[i]=pose;auto* body=m_surfaceBodies[i];
        body->SetEnabled(collision);
        if(source.kind==3){if(m_bridgeJoint){m_bridgeJoint->SetMaxMotorTorque(480);m_bridgeJoint->SetMotorSpeed(std::clamp((pose.angle-body->GetAngle())*4.f-body->GetAngularVelocity()*1.5f,-1.2f,1.2f));}continue;}
        if(old.sx!=pose.sx || old.sy!=pose.sy){
            for(int e=0;e<EDGE_COUNT;++e)if(EDGES[e].body==i && m_nativeEdgeFixtures[e]){
                const auto& v=EDGES[e];auto* shape=static_cast<b2EdgeShape*>(m_nativeEdgeFixtures[e]->GetShape());
                shape->SetTwoSided({(v.ax-source.x)*pose.sx*PHYSICS_SCALE,(v.ay-source.y)*pose.sy*PHYSICS_SCALE},
                                  {(v.bx-source.x)*pose.sx*PHYSICS_SCALE,(v.by-source.y)*pose.sy*PHYSICS_SCALE});
            }
            body->SetTransform(body->GetPosition(),body->GetAngle());
        }
        auto position=body->GetPosition();
        body->SetLinearVelocity({((source.x+pose.x)*PHYSICS_SCALE-position.x)/dt,((source.y+pose.y)*PHYSICS_SCALE-position.y)/dt});
        body->SetAngularVelocity(std::remainder(pose.angle-body->GetAngle(),2.f*float(M_PI))/dt);
    }
}
void Level::UpdateInteractions(const std::vector<Loco*>& locos,float dt){
    std::vector<NativeEvents::Actor> actors;actors.reserve(locos.size());
    for(auto* loco:locos){auto p=loco->GetPosition();unsigned weights=0;int index=0;
        for(int b=0;b<BODY_COUNT;++b)if(BODIES[b].kind==6){if(index<32 && loco->IsTouchingBody(m_surfaceBodies[b]))weights|=1u<<index;++index;}
        actors.push_back({p.x,p.y,loco->GetPhysicalRadius(),loco->GetSize(),weights});
    }
    m_events.Update(dt,actors);
}
void Level::UpdateFlowerPhysics(float dt){
    UpdateNativeAnimations(dt);
    for(auto& f:m_flowerSurfaces){float load=0,moment=0;for(auto* e=f.body->GetContactList();e;e=e->next)if(e->contact->IsTouching() && e->other->GetType()==b2_dynamicBody){float mass=e->other->GetMass();load+=mass;moment+=mass*(e->other->GetPosition().x/PHYSICS_SCALE-f.center.x);}
        float target=std::clamp(moment*.003f,-.075f,.075f);f.velocity+=((target-f.bend)*50-f.velocity*8)*dt;f.bend=std::clamp(f.bend+f.velocity*dt,-.09f,.09f);
        f.dropVelocity+=((std::min(14.f,load*24)-f.drop)*65-f.dropVelocity*7)*dt;f.drop=std::clamp(f.drop+f.dropVelocity*dt,-5.f,18.f);
        auto p=f.body->GetPosition();f.body->SetLinearVelocity({(f.center.x*PHYSICS_SCALE-p.x)/dt,((f.center.y+f.drop)*PHYSICS_SCALE-p.y)/dt});f.body->SetAngularVelocity((f.bend-f.body->GetAngle())/dt);}
}
void Level::Update(float dt){m_globalTime+=dt;}
void Level::DrawNativePieces(int layer){
    const auto& camera=Renderer::GetCamera();float radius=std::hypot(float(Renderer::GetScreenWidth()),float(Renderer::GetScreenHeight()))*.5f/std::max(.08f,camera.scale);
    static std::vector<Renderer::TexturedVertex> buffer;
    for(int i=0;i<PIECE_COUNT;++i){const auto& p=PIECES[i];if((p.body>=0 && !m_nativePoses[p.body].visible) || (p.z>90?2:(p.z< -90?0:1))!=layer || (p.fruit>=0 && m_fruits[p.fruit].eaten))continue;
        float cx=(p.minX+p.maxX)*.5f,cy=(p.minY+p.maxY)*.5f;auto center=NativePoint({cx,cy},p.body);float r=std::hypot(p.maxX-p.minX,p.maxY-p.minY)*.5f;
        if(p.body>=0)r*=std::max(std::abs(m_nativePoses[p.body].sx),std::abs(m_nativePoses[p.body].sy));
        if(std::hypot(center.x-camera.x,center.y-camera.y)>radius+r)continue;
        float alpha=p.body<0?1.f:m_events.Alpha(BODIES[p.body].source);if(alpha<=0)continue;
        buffer.resize(p.count);for(int j=0;j<p.count;++j){const auto& v=VERTICES[p.first+j];auto q=NativePoint({v.x,v.y},p.body);buffer[j]={q.x,q.y,v.u,v.v,v.r,v.g,v.b,static_cast<unsigned char>(v.a*alpha)};}
        Renderer::DrawWorldTexturedTriangles(TEXTURES[p.texture],buffer.data(),p.count,p.repeat);
    }
}
void Level::DrawBackground(){DrawNativePieces(0);
    const float length=WindLength();if(length<=0)return;
    for(float d=0;d<length;d+=160){auto p=WindPoint(std::fmod(d+m_globalTime*520,length));auto q=WindPoint(std::fmod(d+m_globalTime*520+20,length));Renderer::DrawWorldSprite(SceneSprites::WIND_PARTICLES,p.x,p.y,96,270,{255,255,255,180},std::atan2(q.y-p.y,q.x-p.x)-M_PI*.5f);}}
void Level::DrawTerrain(){DrawNativePieces(1);}
void Level::DrawMasks(){DrawNativePieces(2);}
void Level::DrawForeground(){DrawNativePieces(2);}
void Level::DrawItems(){
    const float flap=.65f+.35f*std::sin(m_globalTime*24);
    for(const auto& fly:m_flies)if(!fly.collected){Renderer::DrawWorldSprite(SceneSprites::PICKIE_WINGS,fly.basePos.x,fly.basePos.y,54,54*flap);Renderer::DrawWorldSprite(SceneSprites::PICKIE_BODY,fly.basePos.x,fly.basePos.y,54,54);}
    const auto& defs=NativeEvents::Definitions();
    for(size_t i=0;i<defs.size();++i)if(defs[i].kind==0){const auto& d=defs[i];const auto& state=m_events.Get(i);
        if(!state.done || state.time<NativeAnimations::MuiDuration("muimui_jump_exit"))
            NativeAnimations::DrawMui(state.done?"muimui_jump_exit":"muimui_jump_idle",state.done?state.time:m_globalTime,!state.done,d.p[0],d.p[1],6.f);
    }
    for(size_t i=0;i<defs.size();++i)if(defs[i].kind==11 && m_events.Get(i).active){const auto& d=defs[i];
        Renderer::DrawWorldCircle(d.p[0],d.p[1],64,{255,255,255,235});
        Renderer::DrawWorldSprite(SceneSprites::HUD_HEAD,d.p[0]-24,d.p[1],40,40);
        const Renderer::Sprite* digits[]={&SceneSprites::HUD_DIGIT0,&SceneSprites::HUD_DIGIT1,&SceneSprites::HUD_DIGIT2,&SceneSprites::HUD_DIGIT3,&SceneSprites::HUD_DIGIT4,&SceneSprites::HUD_DIGIT5,&SceneSprites::HUD_DIGIT6,&SceneSprites::HUD_DIGIT7,&SceneSprites::HUD_DIGIT8,&SceneSprites::HUD_DIGIT9};
        int value=int(d.p[3]);if(value>=10)Renderer::DrawWorldSprite(*digits[(value/10)%10],d.p[0]+12,d.p[1],20,30);
        Renderer::DrawWorldSprite(*digits[value%10],d.p[0]+32,d.p[1],20,30);
    }
}
int Level::CheckFruitCollisions(const LocoData::Vec2& p,float radius){int count=0;for(auto& f:m_fruits)if(!f.eaten){auto q=GetFruitCenter(f);if(std::hypot(q.x-p.x,q.y-p.y)<radius+24){f.eaten=true;++count;++m_eatenFruitsCount;}}return count;}
int Level::CheckFlyCollisions(const LocoData::Vec2& p,float radius){int count=0;for(size_t i=0;i<m_flies.size();++i){auto& f=m_flies[i];if(!f.collected && std::hypot(f.basePos.x-p.x,f.basePos.y-p.y)<radius+24){f.collected=true;count+=FLIES[i].value;m_collectedFliesCount+=FLIES[i].value;}}return count;}
bool Level::CheckMuiEncounter(const LocoData::Vec2& p,float radius){
    int index=m_events.Encounter(p.x,p.y,radius);if(index<0)return false;
    const auto& def=NativeEvents::Definitions()[index];
    if(!IsPathClear(p,{def.p[0],def.p[1]-30}))return false;
    m_events.Begin(index);return true;
}
void Level::ClaimMuiReward(){m_events.Claim();}
float Level::WindLength()const{return m_windDistances.empty()?0.f:m_windDistances.back();}
LocoData::Vec2 Level::WindPoint(float d)const{
    if(WIND_COUNT<2)return WIND_COUNT?WIND[0]:LocoData::Vec2{};
    if(d<=0)return WIND[0];
    auto it=std::lower_bound(m_windDistances.begin()+1,m_windDistances.end(),d);
    if(it==m_windDistances.end())return WIND[WIND_COUNT-1];
    int i=int(it-m_windDistances.begin());float segment=m_windDistances[i]-m_windDistances[i-1];
    float t=std::clamp((d-m_windDistances[i-1])/std::max(segment,.001f),0.f,1.f);
    return {WIND[i-1].x+(WIND[i].x-WIND[i-1].x)*t,WIND[i-1].y+(WIND[i].y-WIND[i-1].y)*t};
}
float Level::WindEntryDistance(const LocoData::Vec2& p,float radius)const{float distance=200+radius,progress=-1,length=0;for(int i=1;i<WIND_COUNT && length<900;++i){auto a=WIND[i-1],b=WIND[i];float dx=b.x-a.x,dy=b.y-a.y,n=m_windDistances[i]-m_windDistances[i-1];float t=std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/std::max(.001f,n*n),0.f,1.f);LocoData::Vec2 q{a.x+t*dx,a.y+t*dy};float d=std::hypot(p.x-q.x,p.y-q.y);if(d<distance && IsPathClear(p,q)){distance=d;progress=length+t*n;}length+=n;}return progress;}
bool Level::IsAtWindEntry(const LocoData::Vec2& p)const{return WindEntryDistance(p)>=0;}
