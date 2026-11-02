#pragma once
#include "TwoHandMelee.h"
namespace diagnostic { namespace crossbow {
// Keep the weapon and support-palm orientation at attachment. Changes in
// the left hand then guide the weapon relative to that captured pose. The
// right wrist contributes position only; no absolute left-wrist roll snaps
// the gun (and attached palm) when the support grip first becomes active.
class SupportedGuidance {
    arms::Mat initial,leftAtGrab;arms::Vec lineAtGrab;uintptr_t item=0;uint64_t session=0;bool captured=false;
public:
    void Reset(){captured=false;item=0;session=0;}
    bool Step(ControllerPose& aim,arms::Mat const& left,arms::Mat const& right,uintptr_t weapon,uint64_t origin,uint64_t stamp){
        arms::Vec line=arms::Position(left)-arms::Position(right);
        if(!weapon||!stamp||!arms::Affine(left)||!arms::Affine(right)||arms::Length(line)<.08f||!arms::Unit(line)){Reset();return false;}
        Orientation q{aim.quaternion[3],aim.quaternion[0],aim.quaternion[1],aim.quaternion[2]};if(!Normalize(q)){Reset();return false;}
        if(item!=weapon||session!=origin){Reset();item=weapon;session=origin;}
        if(!captured){initial=arms::Rotation(Matrix(q));leftAtGrab=left;arms::Position(leftAtGrab,{});lineAtGrab=line;captured=true;}
        // Solve from the captured frame, never from last frame's solved gun.
        // This avoids accumulated twist/feedback and right-hand recaptures
        // after an isolated slow render. External focus/tracking gates own loss.
        arms::Mat transport,inverse;if(!arms::Between(lineAtGrab,line,transport)||!arms::Inverse(arms::Multiply(leftAtGrab,transport),inverse))return false;
        auto currentLeft=left;arms::Position(currentLeft,{});Orientation delta;
        if(!melee::PoseOrientation(arms::Multiply(inverse,currentLeft),delta))return false;
        if(delta.w<0){delta.w=-delta.w;delta.x=-delta.x;delta.y=-delta.y;delta.z=-delta.z;}
        float along=delta.x*line.x+delta.y*line.y+delta.z*line.z;
        float angle=std::fabs(delta.w)+std::fabs(along)<.00001f?0:2*std::atan2(along,delta.w);
        Orientation roll{std::cos(angle*.5f),line.x*std::sin(angle*.5f),line.y*std::sin(angle*.5f),line.z*std::sin(angle*.5f)};
        auto result=arms::Multiply(arms::Multiply(initial,transport),arms::Rotation(Matrix(roll)));
        if(!melee::PoseOrientation(result,q))return false;
        aim.quaternion[0]=q.x;aim.quaternion[1]=q.y;aim.quaternion[2]=q.z;aim.quaternion[3]=q.w;return true;
    }
};
// Deliberate visible one-hand handling penalty, shared by mesh, reticle and
// projectile. It never changes native damage, velocity or reload progression.
inline bool OneHandSway(ControllerPose& aim,uint64_t time){
    Orientation q{aim.quaternion[3],aim.quaternion[0],aim.quaternion[1],aim.quaternion[2]};if(!Normalize(q))return false;
    double t=double(time%600000)*.001;constexpr float radians=.01745329252f;
    float pitch=float(3.0*std::sin(t*4.1)+.8*std::sin(t*17.3))*radians;
    float yaw=float(4.0*std::sin(t*3.3)+.8*std::cos(t*19.1))*radians;
    Orientation turn=Product(Orientation{std::cos(yaw*.5f),0,std::sin(yaw*.5f),0},Orientation{std::cos(pitch*.5f),std::sin(pitch*.5f),0,0});
    q=Product(q,turn);if(!Normalize(q))return false;
    aim.quaternion[0]=q.x;aim.quaternion[1]=q.y;aim.quaternion[2]=q.z;aim.quaternion[3]=q.w;return true;
}
} }
