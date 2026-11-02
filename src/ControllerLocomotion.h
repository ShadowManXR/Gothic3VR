#pragma once
#include "CameraComfort.h"
#include <algorithm>
namespace diagnostic {
inline bool AnalogWalk(float x,float z,float& outX,float& outZ){
    outX=outZ=0;if(!std::isfinite(x)||!std::isfinite(z))return false;
    float length=std::hypot(x,z);if(length<=.25f)return false;
    outX=x/length;outZ=z/length;return true;
}
// Native locomotion receives digital keys. Gate BOTH axes: a small backward
// component during lateral movement must not become a full Backward action.
// Enter the secondary axis at35deg, leave at25deg to avoid edge chatter.
class NativeStickDirections {
    int lateral=0,longitudinal=0;
public:
    void Reset(){lateral=longitudinal=0;}
    bool Step(float x,float z,float& outX,float& outZ){
        if(!AnalogWalk(x,z,outX,outZ)){Reset();return false;}
        int side=x<0?-1:1,front=z<0?-1:1;
        float sideRatio=lateral==side?.4663077f:.7002075f;
        float frontRatio=longitudinal==front?.4663077f:.7002075f;
        lateral=std::fabs(x)>std::fabs(z)*sideRatio?side:0;
        longitudinal=std::fabs(z)>std::fabs(x)*frontRatio?front:0;
        if(!lateral)outX=0;if(!longitudinal)outZ=0;
        if(!lateral&&longitudinal)outZ=float(longitudinal);
        if(!longitudinal&&lateral)outX=float(lateral);
        return true;
    }
};
inline bool ViewWalkingHeading(Rotation3 const& native,Orientation center,Orientation head,float& heading){
    Rotation3 relative;if(!RelativeHeadRotation(center,head,relative))return false;
    return HorizontalHeading(Product(UprightCamera(native),relative),heading);
}
inline int RunOnlyMode(int current,bool moving,bool sneak,bool sprint){
    if(current<1||current>5)return current; // water, air, recovery remain native
    if(sneak)return 2;
    if(!moving)return current==2||current==3||current==5?1:current;
    return sprint&&current==5?5:4; // native decides if sprint stamina/state permits
}
class GripSneak {
    uint64_t began=0;bool active=false;
public:
    void Reset(){began=0;active=false;}
    bool Step(bool context,bool held,bool moving,bool support,bool learned,uint64_t now){
        if(!context||!held||support||!learned){Reset();return false;}
        if(!began&&moving)began=now;
        if(began&&now>=began&&now-began>=120)active=true;
        return active;
    }
};
}
