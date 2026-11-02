#pragma once
#include "HeadControlMath.h"
namespace diagnostic {
inline bool BodyLocomotionIntent(float x,float z){return std::isfinite(x)&&std::isfinite(z)&&std::hypot(x,z)>.05f;}
// Physical head yaw follows a body-relative free-look cone. Joystick/native
// camera turns remain in the world/tracking base and are applied only once.
class HeadBodyHeading {
    uintptr_t actor=0;uint64_t session=0;float relativeYaw=0;
    static float Wrap(float angle){return std::atan2(std::sin(angle),std::cos(angle));}
public:
    static constexpr float FreeLookDegrees=30.f;
    static constexpr float FreeLookRadians=FreeLookDegrees*.01745329252f;
    void Reset(){actor=0;session=0;relativeYaw=0;}
    bool Step(uintptr_t player,Rotation3 const& native,Orientation center,HeadPose const& head,int64_t now,int64_t frequency,Rotation3& out,bool locomotion=false){
        if(!player||!FreshOrientation(head,now,frequency,100))return false;
        float baseYaw;if(!HorizontalHeading(native,baseYaw))return false;
        Rotation3 relative;if(!RelativeHeadRotation(center,{head.quaternion[0],head.quaternion[1],head.quaternion[2],head.quaternion[3]},relative))return false;
        if(actor!=player||session!=head.session){Reset();actor=player;session=head.session;}
        float headYaw;
        if(HorizontalHeading(relative,headYaw)){
            if(locomotion)relativeYaw=headYaw; // Face forward while walking, including backward/strafe gait.
            else {float difference=Wrap(headYaw-relativeYaw);
                if(std::fabs(difference)>FreeLookRadians)relativeYaw=Wrap(relativeYaw+difference-std::copysign(FreeLookRadians,difference));}
        } // Looking straight up/down retains the last horizontal body heading.
        float angles[3]{0,Wrap(baseYaw+relativeYaw),0};out=NativeCameraRotation(angles);return true;
    }
};
// A short native alignment lease spans character input -> native physics.
// Release it only when a freshly supplied owner still has our exact vector;
// native scripts/targets that changed it retain priority.
class BodyFacingLease {
    uintptr_t actor=0,movement=0;float previous[3]{},requested[3]{};
public:
    void Forget(){actor=movement=0;}
    bool Empty()const{return !movement;}
    bool Matches(uintptr_t player,uintptr_t property)const{return actor==player&&movement==property&&movement;}
    void Begin(uintptr_t player,uintptr_t property,float const (&before)[3],float const (&desired)[3]){
        actor=player;movement=property;for(unsigned i=0;i<3;++i){previous[i]=before[i];requested[i]=desired[i];}
    }
    bool Take(uintptr_t player,uintptr_t property,bool targeted,float const (&current)[3],float (&restore)[3]){
        if(!Matches(player,property))return false;
        Forget();if(targeted)return false;
        for(unsigned i=0;i<3;++i)if(!std::isfinite(current[i])||std::fabs(current[i]-requested[i])>.00001f)return false;
        for(unsigned i=0;i<3;++i)restore[i]=previous[i];return true;
    }
};
}
