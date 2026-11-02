#pragma once
#include "TwoHandMelee.h"
namespace diagnostic { namespace melee {
enum class HandlingProfile : unsigned {OneHand,SupportedTwoHand,StaffOneHand,StaffTwoHand};
inline float StaffOneHandAngularSpeed(int weightPercent){
    // A long shaft needs a real rotational speed limit when unsupported. The
    // old angle-gap clamp could skip straight towards a fast controller and
    // erase most of the selected weight. Existing 0..200% slider remains valid.
    return 720.f/(1.f+1.2f*float(std::clamp(weightPercent,0,200))*.01f)*.01745329252f;
}
// All history is in XR metres/orientation, never native world coordinates.
// The reflection matches GameEyeTransform's OpenXR (-Z)/Genome (+Z) basis.
inline arms::Mat ReflectTrackingRotation(arms::Mat pose){
    arms::Position(pose,{});for(unsigned i:{2u,6u,8u,9u})pose.m[i]=-pose.m[i];return pose;
}
inline bool HandlingToTracking(arms::Mat const& world,arms::Mat const& anchor,float units,arms::Mat& out){
    arms::Mat inverse;if(!arms::Affine(world)||!std::isfinite(units)||units<=0||!arms::Inverse(anchor,inverse))return false;
    auto relative=arms::Multiply(world,inverse);auto p=arms::Position(relative)*(1.f/units);
    out=ReflectTrackingRotation(relative);arms::Position(out,{p.x,p.y,-p.z});return arms::Affine(out);
}
inline bool HandlingToWorld(arms::Mat const& tracking,arms::Mat const& anchor,float units,arms::Mat& out){
    if(!arms::Affine(tracking)||!arms::Affine(anchor)||!std::isfinite(units)||units<=0)return false;
    auto p=arms::Position(tracking);out=arms::Multiply(ReflectTrackingRotation(tracking),anchor);
    arms::Position(out,arms::Point(anchor,{p.x*units,p.y*units,-p.z*units}));return arms::Affine(out);
}
inline Orientation HandlingBlend(Orientation a,Orientation b,float amount){
    float dot=a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z;
    if(dot<0){b={-b.w,-b.x,-b.y,-b.z};dot=-dot;}
    float x=1-amount,y=amount;
    if(dot<.9995f){float angle=std::acos(std::clamp(dot,0.f,1.f)),denom=std::sin(angle);
        x=std::sin((1-amount)*angle)/denom;y=std::sin(amount*angle)/denom;}
    Orientation out{a.w*x+b.w*y,a.x*x+b.x*y,a.y*x+b.y*y,a.z*x+b.z*y};Normalize(out);return out;
}
class MeleeHandling {
    uintptr_t weapon=0;uint64_t session=0,time=0;unsigned profile=0;bool valid=false;
    arms::Vec previous,position;Orientation rotation;
public:
    void Reset(){weapon=0;session=time=0;valid=false;}
    bool Step(uintptr_t item,uint64_t origin,uint64_t stamp,HandlingProfile mode,arms::Mat const& target,int weightPercent,int smoothingMs,arms::Mat& out){
        Orientation raw;auto p=arms::Position(target);
        if(!item||!stamp||!arms::Finite(p)||!PoseOrientation(target,raw)){Reset();return false;}
        float weight=float(std::clamp(weightPercent,0,200))*.01f,smoothing=float(std::clamp(smoothingMs,0,150))*.001f;
        if(weight==0&&smoothing==0){Reset();out=target;return true;}
        bool fresh=!valid||item!=weapon||origin!=session||stamp<time||stamp-time>100||arms::Length(p-previous)>.5f;
        if(fresh){position=p;rotation=raw;}
        else if(stamp!=time){
            float dt=float(stamp-time)*.001f,tauRotation=smoothing+weight*.08f,tauPosition=smoothing+weight*.035f;
            float angular=tauRotation>0?-std::expm1(-dt/tauRotation):1.f;
            float linear=tauPosition>0?-std::expm1(-dt/tauPosition):1.f;
            auto previousRotation=rotation;
            rotation=HandlingBlend(rotation,raw,angular);position=position+(p-position)*linear;
            if(mode==HandlingProfile::StaffOneHand&&weight>0){
                float maximum=StaffOneHandAngularSpeed(weightPercent)*dt,step=Angle(previousRotation,rotation);
                if(step>maximum)rotation=HandlingBlend(previousRotation,rotation,maximum/step);
            }else{
                // Preserve the accepted response of all other profiles, including
                // staff/halberd held with both hands. No spring overshoot.
                float maxAngle=(8+18*weight)*.01745329252f,angle=Angle(raw,rotation);
                if(angle>maxAngle)rotation=HandlingBlend(raw,rotation,maxAngle/angle);
            }
            // Translation stays close to the physical hand even when the heavy
            // unsupported shaft's orientation trails during a fast swing.
            auto lag=position-p;float distance=arms::Length(lag),maximum=.02f+.035f*weight;
            if(distance>maximum)position=p+lag*(maximum/distance);
        }
        weapon=item;session=origin;time=stamp;profile=unsigned(mode);previous=p;valid=true;
        out=arms::Rotation(Matrix(rotation));arms::Position(out,position);return arms::Affine(out);
    }
};
// Transform the whole supported hand/weapon assembly by ONE rigid delta.
// Independent filters on the two attached palms would recreate grip fighting.
inline bool HandlingAssembly(arms::Mat const& before,arms::Mat const& after,arms::Mat& delta){
    arms::Mat inverse;if(!arms::Affine(after)||!arms::Inverse(before,inverse))return false;
    delta=arms::Multiply(inverse,after);return arms::Affine(delta);
}
}}
