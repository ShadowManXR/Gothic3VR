#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic { namespace bow {
// The bow hand remains authoritative. While drawing, the string-hand position
// adds a small angular correction towards the string-to-bow line. Filter only
// that relative correction: locomotion and body turns must never trail behind.
class StringGuidance {
    arms::Vec held;uint64_t time=0,session=0;uintptr_t weapon=0;bool have=false;
public:
    static constexpr float LimitRadians=6.f*.01745329252f;
    void Reset(){held={};time=session=0;weapon=0;have=false;}
    bool Apply(arms::Mat& aim,arms::Vec rest,arms::Vec stringHand,float units,bool pulling,bool tracked,
               uint64_t now,uint64_t origin,uintptr_t item,int influence,int smoothing){
        if(!arms::Affine(aim)||!arms::Finite(rest)||!std::isfinite(units)||units<=0){Reset();return false;}
        influence=std::clamp(influence,0,40);
        if(!item||!origin||!now||!influence||!tracked||!arms::Finite(stringHand)){Reset();return true;}
        bool restart=!have||now<time||now-time>100||session!=origin||weapon!=item;
        if(restart){Reset();have=true;time=now;session=origin;weapon=item;}
        arms::Vec target;
        if(pulling){
            auto line=(rest-stringHand)*(1.f/units);
            arms::Vec local{arms::Dot(line,arms::Vector(aim,{1,0,0})),arms::Dot(line,arms::Vector(aim,{0,1,0})),arms::Dot(line,arms::Vector(aim,{0,0,1}))};
            float lateral=std::hypot(local.x,local.y);
            // Fade near the nocking point; an almost zero-length line must
            // not flip the aim or produce a sudden correction at string grab.
            if(local.z>.05f&&arms::Length(local)<1.5f&&lateral>1.e-6f){
                float fade=std::clamp((local.z-.05f)/.15f,0.f,1.f);fade=fade*fade*(3-2*fade);
                float angle=std::min(LimitRadians,std::atan2(lateral,local.z)*float(influence)*.01f*fade);
                target={local.x/lateral*angle,local.y/lateral*angle,0};
            }
        }
        if(now!=time){
            float tau=float(std::clamp(smoothing,40,160));float alpha=1-std::exp(-float(now-time)/tau);
            held=held+(target-held)*alpha;time=now;
        }
        float angle=std::hypot(held.x,held.y);if(angle<.0001f){held={};return true;}
        float bounded=std::min(angle,LimitRadians),scale=std::sin(bounded)/angle;
        arms::Vec forward=arms::Vector(aim,{held.x*scale,held.y*scale,std::cos(bounded)});
        arms::Mat correction;if(!arms::Between(arms::Vector(aim,{0,0,1}),forward,correction)){Reset();return false;}
        auto position=arms::Position(aim);aim=arms::Multiply(aim,correction);arms::Position(aim,position);return arms::Affine(aim);
    }
};
} }
