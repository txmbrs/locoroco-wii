#pragma once
#include <box2d/box2d.h>
#include <cmath>

namespace PhysicsGeometry {
// PolygonShape welds vertices nearer than half linearSlop. An area check alone
// misses long, thin triangles and lets Set() assert (or create a fallback box).
inline bool SetTriangle(b2PolygonShape& shape, const b2Vec2* vertices) {
    constexpr float minimumEdge = .5f * b2_linearSlop;
    for(int i=0;i<3;++i) {
        if(!vertices[i].IsValid())return false;
        if((vertices[i]-vertices[(i+1)%3]).LengthSquared() <= minimumEdge*minimumEdge)return false;
    }
    if(std::abs(b2Cross(vertices[1]-vertices[0],vertices[2]-vertices[0])) < .0001f)return false;
    shape.Set(vertices,3);
    return true;
}
}
