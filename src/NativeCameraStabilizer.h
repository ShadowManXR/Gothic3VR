#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace diagnostic {
// Filter only the native actor origin, never an eye/controller pose. The
// accepted horizontal room transfer is a change of origin, not locomotion to
// filter: it must be rebased immediately before the shared XR origin consumes
// it. Otherwise smoothing the actor introduces a backwards head movement.
class NativeCameraStabilizer {
    uintptr_t actor=0;
    double previousTime=0;
    float scale=0,raw[3]{},filtered[3]{};
    static bool Finite(float const (&v)[3]) {
        for(float x:v)if(!std::isfinite(x))return false;
        return true;
    }
    void Seed(uintptr_t player,float const (&position)[3],double time,float units) {
        actor=player;previousTime=time;scale=units;
        for(unsigned i=0;i<3;++i)raw[i]=filtered[i]=position[i];
    }
public:
    void Reset(){actor=0;previousTime=0;}
    bool Update(uintptr_t player,float const (&position)[3],double time,float units,int horizontalMs,int verticalMs) {
        if(!player||!Finite(position)||!std::isfinite(time)||!std::isfinite(units)||units<=0){Reset();return false;}
        double dt=time-previousTime;
        float displacement=0;
        for(unsigned i=0;i<3;++i)displacement=std::max(displacement,std::fabs(position[i]-raw[i]));
        // Loads, teleports, tracking pauses and scale changes establish a new
        // origin. Never slide the view across a whole world transition.
        if(actor!=player||scale!=units||dt<0||dt>.25||displacement>3*units){Seed(player,position,time,units);return true;}
        if(dt==0)return true; // Duplicate render query cannot advance the filter.
        previousTime=time;
        for(unsigned i=0;i<3;++i) {
            int milliseconds=std::clamp(i==1?verticalMs:horizontalMs,0,250);
            float alpha=milliseconds?float(-std::expm1(-dt/(double(milliseconds)*.001))):1.f;
            filtered[i]+=alpha*(position[i]-filtered[i]);raw[i]=position[i];
        }
        // Keep the view near its collider even during a large native impulse.
        float x=filtered[0]-raw[0],z=filtered[2]-raw[2],distance=std::hypot(x,z),limit=.5f*units;
        if(distance>limit){filtered[0]=raw[0]+x*limit/distance;filtered[2]=raw[2]+z*limit/distance;}
        filtered[1]=raw[1]+std::clamp(filtered[1]-raw[1],-.3f*units,.3f*units);
        return true;
    }
    bool Apply(uintptr_t player,float const (&position)[3],float (&out)[3]) const {
        if(!actor||actor!=player||!Finite(position))return false;
        // Focus queries between render frames reuse the current correction,
        // rather than advancing the filter for an individual hand or eye.
        for(unsigned i=0;i<3;++i)out[i]=position[i]+(filtered[i]-raw[i]);
        return true;
    }
    void RebaseRoomTransfer(uintptr_t player,float const (&accepted)[3]) {
        if(actor!=player||!actor||!Finite(accepted))return;
        // XR consumes X/Z only. Terrain Y remains native vertical movement and
        // is smoothed; physical headset crouching remains entirely optical.
        for(unsigned i:{0u,2u}){raw[i]+=accepted[i];filtered[i]+=accepted[i];}
    }
};
}
