#pragma once
#include "InteractionTargetBox.h"
#include <cstdint>
#include <limits>
namespace diagnostic {
inline uintptr_t FinalInteractionFocus(uintptr_t nativeChoice,bool character,uintptr_t rankedObject){
    return nativeChoice&&character?nativeChoice:rankedObject?rankedObject:nativeChoice;
}
inline bool PreciseInteractionRay(WorldAim const& aim,float const (&lo)[3],float const (&hi)[3],float reach){
    float enter=0,leave=reach;
    for(unsigned i=0;i<3;++i){
        float a=lo[i]-aim.origin[i],b=hi[i]-aim.origin[i],d=aim.direction[i];
        if(std::fabs(d)<.00001f){if(a>0||b<0)return false;continue;}
        a/=d;b/=d;if(a>b)std::swap(a,b);enter=std::max(enter,a);leave=std::min(leave,b);if(enter>leave)return false;
    }
    return leave>=0&&enter<=reach;
}
inline bool RankedInteractionBox(WorldAim const& aim,int type,float const (&root)[3],float units,float (&lo)[3],float (&hi)[3],float reach,float& score,bool pickup){
    float legacy=0;if(!InteractionAimBox(aim,type,root,units,lo,hi,reach,legacy,pickup))return false;
    float near2=0,size2=0,center2=0,along=0;
    for(unsigned i=0;i<3;++i){
        float d=std::max(lo[i]-aim.origin[i],std::max(0.f,aim.origin[i]-hi[i]));near2+=d*d;
        float extent=hi[i]-lo[i];size2+=extent*extent;
        float center=(lo[i]+hi[i])*.5f-aim.origin[i];center2+=center*center;along+=center*aim.direction[i];
    }
    bool precise=PreciseInteractionRay(aim,lo,hi,CampfireTarget(type)?std::hypot(reach,std::sqrt(near2)):reach);
    bool contact=near2<=(pickup?.15f*.15f:.10f*.10f)*units*units;
    float angle=std::sqrt(std::max(0.f,center2-along*along))/std::max(.1f*units,std::sqrt(center2));
    // Prefer compact, well-centred hits over a bed/shrine's huge bounds.
    // Retain limited pickup assistance for tiny offerings/ground weapons;
    // the generous furniture padding must never beat an exact chest hit.
    // All candidates still pass the unchanged reach and native LOS checks.
    score=(precise?0.f:contact?.15f:pickup?.25f:1.f)+.35f*std::min(1.f,std::sqrt(size2)/(2*units))+
        .2f*std::min(1.f,std::sqrt(near2)/reach)+.2f*std::min(1.f,angle);
    return std::isfinite(score);
}
class InteractionFocusRanking {
    uintptr_t entity=0;float score=std::numeric_limits<float>::max();
public:
    void Reset(){entity=0;score=std::numeric_limits<float>::max();}
    bool Better(float value)const{return std::isfinite(value)&&value>=0&&value<score;}
    void Accept(uintptr_t id,float value){if(id&&Better(value)){entity=id;score=value;}}
    uintptr_t Selected()const{return entity;}
};
}
