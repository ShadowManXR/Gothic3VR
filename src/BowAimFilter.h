#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic { namespace bow {
// Filter in tracking space: turning/walking the hero does not trail the aim.
// Quaternion interpolation handles roll and the +/-180 degree seam together.
class AimFilter {
    Orientation held;uint64_t time=0,session=0;uintptr_t weapon=0;bool have=false;
public:
    void Reset(){have=false;time=0;}
    bool Apply(ControllerPose& pose,uint64_t now,uint64_t origin,uintptr_t item,int milliseconds){
        Orientation raw{pose.quaternion[3],pose.quaternion[0],pose.quaternion[1],pose.quaternion[2]};
        if(!pose.valid||!pose.tracked||!Normalize(raw)){Reset();return false;}
        bool restart=!have||now<time||now-time>100||session!=origin||weapon!=item||milliseconds<=0;
        if(restart){held=raw;have=true;time=now;session=origin;weapon=item;}
        else if(now!=time){
            float dot=held.w*raw.w+held.x*raw.x+held.y*raw.y+held.z*raw.z;
            if(dot<0){raw={-raw.w,-raw.x,-raw.y,-raw.z};dot=-dot;}
            float angle=2*std::acos(std::clamp(dot,0.f,1.f))*57.2957795f;
            // Fine tremor gets the configured damping. A deliberate turn
            // catches up in ~12 ms, and large discontinuities reset instantly.
            if(angle>45)held=raw;
            else if(angle>.08f){float response=std::clamp((angle-.5f)/2.5f,0.f,1.f);float tau=float(milliseconds)+(12-float(milliseconds))*response;
                float alpha=1-std::exp(-float(now-time)/std::max(1.f,tau));
                held={held.w+(raw.w-held.w)*alpha,held.x+(raw.x-held.x)*alpha,held.y+(raw.y-held.y)*alpha,held.z+(raw.z-held.z)*alpha};Normalize(held);}
            time=now;
        }
        pose.quaternion[0]=held.x;pose.quaternion[1]=held.y;pose.quaternion[2]=held.z;pose.quaternion[3]=held.w;return true;
    }
};
}}
