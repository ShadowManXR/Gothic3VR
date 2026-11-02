#pragma once
#include "PhysicalMelee.h"
#include "WorldAim.h"
namespace diagnostic { namespace combat {
// Measure the actual weapon endpoints in XR space, excluding locomotion and
// camera/body animation. A small repeated tremor cannot accumulate a stroke.
class StrokeDistance {
    uint64_t time=0;unsigned stroke=0;melee::Blade start{},previous{};
    float rootTravel=0,tipTravel=0,extent=0;bool qualified=false;
public:
    void Reset(){time=0;stroke=0;start=previous={};rootTravel=tipTravel=extent=0;qualified=false;}
    float Travel()const{return std::max(rootTravel,tipTravel);}
    bool Thrust()const{arms::Vec axis=previous.tip-previous.root,motion=previous.root-start.root;float length=arms::Length(motion);return length>=.15f&&arms::Unit(axis)&&arms::Dot(motion,axis)>=.75f*length;}
    bool Ready(int minimumCm)const{float minimum=float(std::clamp(minimumCm,10,200))*.01f;return Travel()+.00001f>=minimum&&extent+.00001f>=minimum*.7f;}
    bool Step(melee::Blade const& blade,uint64_t stamp,unsigned generation,bool moving){
        if(!moving||!stamp||!generation||!arms::Finite(blade.root)||!arms::Finite(blade.tip))return false;
        if(stroke!=generation||!time||stamp<time||stamp-time>100){Reset();stroke=generation;time=stamp;start=previous=blade;return false;}
        if(stamp==time)return qualified;
        float root=arms::Length(blade.root-previous.root),tip=arms::Length(blade.tip-previous.tip);
        if(root>.6f||tip>1.5f){Reset();return false;} // tracking discontinuity
        rootTravel+=root;tipTravel+=tip;extent=std::max(extent,std::max(arms::Length(blade.root-start.root),arms::Length(blade.tip-start.tip)));
        time=stamp;previous=blade;
        // 50 cm of weapon path, plus a coherent excursion. This still permits
        // curved wrist strokes, while 2 cm oscillations remain ineligible.
        qualified=Ready(50);return qualified;
    }
};
// Sight ends inside the grounded body, not at its former standing eye point
// or at the padded ray entry below the floor. Selection still uses the ray.
inline bool FinisherSightPoint(WorldAim const& aim,float const (&lo)[3],float const (&hi)[3],float units,float rating,float (&point)[3]){
    if(!std::isfinite(units)||units<=0||!std::isfinite(rating))return false;
    float entry=std::clamp((rating-.05f)/.95f*2.f*units,0.f,2.f*units);
    for(unsigned i=0;i<3;++i){if(!std::isfinite(lo[i])||!std::isfinite(hi[i])||lo[i]>hi[i]||!std::isfinite(aim.origin[i])||!std::isfinite(aim.direction[i]))return false;
        point[i]=std::clamp(aim.origin[i]+entry*aim.direction[i],lo[i],hi[i]);}
    point[1]=std::max(point[1],lo[1]+std::min(.15f*units,(hi[1]-lo[1])*.5f));return true;
}
inline bool FinisherBounds(WorldAim const& aim,float const (&lo)[3],float const (&hi)[3],float units,float& rating){
    return WorldAimBox(aim,lo,hi,2.f*units,units,rating,true);
}
inline bool FinisherWithinReach(float ax,float ay,float az,float bx,float by,float bz,float units){
    return std::isfinite(units)&&units>0&&std::hypot(ax-bx,az-bz)<=1.8f*units&&std::fabs(ay-by)<=1.2f*units;
}
} }
