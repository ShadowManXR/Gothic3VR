#pragma once
#include "HeadControlMath.h"
#include <algorithm>
namespace diagnostic {
// Render-only camera control. Never writes actor heading, camera AI state or
// tracking origins, so native dialogue/recovery facing remains authoritative.
class ThirdPersonOrbit {
    uintptr_t actor=0;unsigned context=0;uint64_t last=0;float angle=0;
    bool neutral=false,captured=false;Rotation3 entry;float offset[3]{};
    float cachedPivot[3]{},cachedEye[3]{},fraction=1;bool clearanceReady=false;
    float safeAngle=0,safeOffset[3]{};Rotation3 safeRotation;bool safe=false;
public:
    void Reset(){actor=0;context=0;last=0;angle=safeAngle=0;neutral=captured=clearanceReady=safe=false;}
    bool Active(uintptr_t player)const{return actor&&actor==player&&captured;}
    bool Requested(uintptr_t player)const{return actor==player&&actor&&(captured||std::fabs(angle)>.00001f);}
    void Step(uintptr_t player,unsigned mode,bool enabled,bool inputAllowed,float axis,int speed,uint64_t now){
        if(!enabled||!player||!mode){Reset();return;}
        if(actor!=player||context!=mode||now<last){Reset();actor=player;context=mode;}
        float dt=last&&now>=last?float(std::min(now-last,uint64_t(50)))*.001f:0;last=now;
        if(!inputAllowed||!std::isfinite(axis)){neutral=false;return;}
        if(std::fabs(axis)<.25f)neutral=true;
        if(!neutral||std::fabs(axis)<=.4f)return;
        float magnitude=(std::min(std::fabs(axis),1.f)-.4f)/.6f;
        angle=std::remainder(angle+std::copysign(magnitude,axis)*2.0943951f*float(std::clamp(speed,0,2000))*.01f*dt,6.2831853f);
    }
    template<class Clearance>
    bool Apply(uintptr_t player,float const (&pivot)[3],Rotation3& rotation,float (&position)[3],float units,Clearance&& clearance){
        if(actor!=player||!actor||!std::isfinite(units)||units<=0)return false;
        if(!captured){if(std::fabs(angle)<.00001f)return false;
            for(unsigned i=0;i<3;++i){if(!std::isfinite(position[i])||!std::isfinite(pivot[i]))return false;offset[i]=position[i]-pivot[i];}
            if(std::hypot(offset[0],offset[2])<.65f*units)return false;
            entry=rotation;captured=true;
        }
        float c=std::cos(angle),s=std::sin(angle),desired[3]{pivot[0]+c*offset[0]+s*offset[2],pivot[1]+offset[1],pivot[2]-s*offset[0]+c*offset[2]};
        bool changed=!clearanceReady;for(unsigned i=0;i<3;++i)changed|=std::fabs(pivot[i]-cachedPivot[i])>.001f*units||std::fabs(desired[i]-cachedEye[i])>.001f*units;
        if(changed){float value=clearance(pivot,desired);if(!std::isfinite(value))return false;fraction=std::clamp(value,0.f,1.f);clearanceReady=true;for(unsigned i=0;i<3;++i){cachedPivot[i]=pivot[i];cachedEye[i]=desired[i];}}
        // Refuse a collision correction that would put the view inside the hero.
        if(fraction*std::hypot(offset[0],offset[2])<.55f*units){
            angle=safeAngle;clearanceReady=false;
            if(!safe){captured=false;angle=0;return false;}
            for(unsigned i=0;i<3;++i)position[i]=pivot[i]+safeOffset[i];rotation=safeRotation;return true;
        }
        for(unsigned i=0;i<3;++i)position[i]=pivot[i]+fraction*(desired[i]-pivot[i]);
        // Native dialogue/shoulder shots can look beside the subject. Rotating
        // that entry direction preserves the framing error throughout the orbit.
        // Aim the render base at the actual pivot; XR head look stays separate.
        float dx=pivot[0]-position[0],dy=pivot[1]-position[1],dz=pivot[2]-position[2];
        float look[3]{std::atan2(dy,std::hypot(dx,dz)),std::atan2(dx,dz),0};
        rotation=NativeCameraRotation(look);
        safe=true;safeAngle=angle;safeRotation=rotation;for(unsigned i=0;i<3;++i)safeOffset[i]=position[i]-pivot[i];return true;
    }
};
}
