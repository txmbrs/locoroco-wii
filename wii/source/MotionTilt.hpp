#pragma once
#include <algorithm>
#include <cmath>

// Degrees in, normalized symmetric steering out. No startup auto-zero: holding
// the remote tilted while launching must not change the neutral reference.
class MotionTilt {
public:
    float Update(float roll, float dt) {
        if (!std::isfinite(roll)) return Map(m_stable);
        m_samples[m_index] = std::clamp(roll,-90.0f,90.0f);
        m_index=(m_index+1)%3;
        float sorted[3]={m_samples[0],m_samples[1],m_samples[2]};
        std::sort(sorted,sorted+3);
        float median=sorted[1];
        dt=std::clamp(dt,0.0f,1.0f/15.0f);
        m_filtered+=(median-m_filtered)*(1.0f-std::exp(-dt/0.060f));
        float change=m_filtered-m_stable;
        // Backlash suppresses hand tremor even away from the center.
        if (std::abs(change)>0.35f) m_stable+=std::copysign(std::abs(change)-0.35f,change);
        if (median>=45.0f && m_filtered>=44.5f) m_stable=45.0f;
        if (median<=-45.0f && m_filtered<=-44.5f) m_stable=-45.0f;
        return Map(m_stable);
    }
private:
    static float Map(float degrees) {
        constexpr float deadZone=2.0f;
        float magnitude=std::clamp((std::abs(degrees)-deadZone)/(45.0f-deadZone),0.0f,1.0f);
        return std::copysign(magnitude,degrees);
    }
    float m_samples[3]={},m_filtered=0,m_stable=0;
    int m_index=0;
};
