#pragma once
#include "CombatRefinement.h"
namespace diagnostic { namespace combat {
// Only a continuous, sufficiently fast portion of the physical weapon path
// can qualify damage or stamina. Slow accumulated travel plus a tiny flick
// cannot qualify. Endpoints are already in XR metres, excluding locomotion.
class SwingVelocity {
    melee::Blade previous{};uint64_t time=0;unsigned stroke=0;int minimum=0;
    StrokeDistance fast;float speed=0;bool fastNow=false;
public:
    void Reset(){previous={};time=0;stroke=0;minimum=0;fast.Reset();speed=0;fastNow=false;}
    float Speed()const{return speed;}
    bool Ready(int centimetres)const{return fastNow&&fast.Ready(centimetres);}
    void Step(melee::Blade const& blade,uint64_t stamp,unsigned generation,bool moving,int minimumCmPerSecond){
        int setting=std::clamp(minimumCmPerSecond,50,800);
        if(!stamp||!generation||!arms::Finite(blade.root)||!arms::Finite(blade.tip)){Reset();return;}
        if(!time||generation!=stroke||setting!=minimum||stamp<time||stamp-time>100){Reset();time=stamp;stroke=generation;minimum=setting;previous=blade;return;}
        if(stamp==time)return; // reused XR sample cannot erase the qualified path
        auto before=previous;auto began=time;float root=arms::Length(blade.root-before.root),tip=arms::Length(blade.tip-before.tip);
        previous=blade;time=stamp;speed=std::max(root,tip)/(float(stamp-began)*.001f);
        if(root>.6f||tip>1.5f||!std::isfinite(speed)){Reset();return;}
        fastNow=moving&&speed+.00001f>=float(setting)*.01f;
        if(!fastNow){fast.Reset();return;}
        fast.Step(before,began,generation,true);fast.Step(blade,stamp,generation,true);
    }
};
} }
