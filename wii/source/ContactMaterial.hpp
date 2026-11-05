#pragma once
#include <box2d/box2d.h>
#include <algorithm>
#include <cmath>

class MiniContactListener final: public b2ContactListener {
public:
    void PreSolve(b2Contact* contact,const b2Manifold*) override {
        auto* a=contact->GetFixtureA();auto* b=contact->GetFixtureB();
        if(a->GetUserData().pointer && b->GetUserData().pointer)contact->SetFriction(.08f);
        else if(a->GetUserData().pointer || b->GetUserData().pointer)contact->ResetFriction();
    }
};
