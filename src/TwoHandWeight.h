#pragma once
#include "TwoHandMelee.h"
namespace diagnostic { namespace melee {
// Inertia lives in tracking space. Walking/turning the hero cannot drag the
// weapon backward. Hands remain immediate; only the unsupported mesh turns slowly.
class TwoHandWeight {
    Orientation held;uintptr_t weapon=0;uint64_t session=0,time=0;Vec position;bool have=false;
public:
    void Reset(){have=false;weapon=0;session=time=0;}
    Orientation Rotation()const{return held;}
    bool Step(uintptr_t item,uint64_t origin,uint64_t stamp,ControllerPose& pose,bool supported){
        Orientation raw{pose.quaternion[3],pose.quaternion[0],pose.quaternion[1],pose.quaternion[2]};Vec p{pose.position[0],pose.position[1],pose.position[2]};
        if(!item||!stamp||!pose.valid||!pose.tracked||!arms::Finite(p)||!Normalize(raw)){Reset();return false;}
        bool restart=supported||!have||item!=weapon||session!=origin||stamp<time||stamp-time>100||arms::Length(p-position)>.4f;
        if(restart){held=raw;have=true;}
        else if(stamp!=time){
            float dot=held.w*raw.w+held.x*raw.x+held.y*raw.y+held.z*raw.z;
            if(dot<0){raw={-raw.w,-raw.x,-raw.y,-raw.z};dot=-dot;}
            float angle=2*std::acos(std::clamp(dot,0.f,1.f)),dt=float(stamp-time)*.001f;
            float amount=angle>.000001f?std::min(1-std::exp(-dt/.14f),3.2f*dt/angle):1;
            held={held.w+(raw.w-held.w)*amount,held.x+(raw.x-held.x)*amount,held.y+(raw.y-held.y)*amount,held.z+(raw.z-held.z)*amount};if(!Normalize(held)){Reset();return false;}
        }
        weapon=item;session=origin;time=stamp;position=p;
        pose.quaternion[0]=held.x;pose.quaternion[1]=held.y;pose.quaternion[2]=held.z;pose.quaternion[3]=held.w;return true;
    }
};
inline bool WeightedMesh(arms::Mat const& desired,arms::Mat const& controller,arms::Mat const& weighted,Vec palm,arms::Mat& out){
    arms::Mat inverse,meshInverse;if(!arms::Affine(desired)||!arms::Affine(weighted)||!arms::Finite(palm)||!arms::Inverse(controller,inverse)||!arms::Inverse(desired,meshInverse))return false;
    auto turn=arms::Multiply(inverse,weighted);arms::Position(turn,{});
    out=arms::Multiply(desired,turn);Vec contact=arms::Point(meshInverse,palm);arms::Position(out,palm-arms::Vector(out,contact));return arms::Affine(out);
}
inline float UnsupportedDamageFraction(float nativeStroke,bool unsupported){return nativeStroke*(unsupported?.2f:1.f);}
inline bool UnsupportedHitReady(uint64_t now,uint64_t next,bool unsupported){return !unsupported||now>=next;}
inline uint64_t UnsupportedNextHit(uint64_t now){return now+900;}
} }
