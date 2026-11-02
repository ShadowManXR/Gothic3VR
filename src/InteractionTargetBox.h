#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>
#include "WorldAim.h"
namespace diagnostic {
inline bool CampfireTarget(int type){return type==88||type==89;}
inline void CampfireInteractionBox(int type,float const (&root)[3],float units,float (&lo)[3],float (&hi)[3]){
    if(!CampfireTarget(type)||!std::isfinite(units)||units<=0)return;
    for(float v:root)if(!std::isfinite(v))return;
    // Flame/smoke bounds cover a much larger area than the usable hearth.
    lo[0]=root[0]-.55f*units;hi[0]=root[0]+.55f*units;
    lo[2]=root[2]-.55f*units;hi[2]=root[2]+.55f*units;
    lo[1]=root[1]-.10f*units;hi[1]=root[1]+.65f*units;
    // Keep the configured range. A metre measured from a standing hand to
    // a ground-level hearth previously rejected even a player beside it.
}
inline bool InteractionAimBox(WorldAim const& aim,int type,float const (&root)[3],float units,float (&lo)[3],float (&hi)[3],float reach,float& rating,bool pickup=false){
    CampfireInteractionBox(type,root,units,lo,hi);
    if(!CampfireTarget(type))return WorldAimBox(aim,lo,hi,reach,units,rating,pickup);
    if(!std::isfinite(reach)||reach<=0)return false;
    float vertical=std::max(lo[1]-aim.origin[1],std::max(0.f,aim.origin[1]-hi[1]));
    float rayReach=std::hypot(reach,vertical);
    if(!WorldAimBox(aim,lo,hi,rayReach,units,rating))return false;
    // Count horizontal reach independently of the hand's height; still require
    // the ray to intersect the hearth, never the huge flame/smoke boundary.
    float entry=rating<=.05f?0.f:(rating-.05f)/.95f*rayReach;
    float horizontal=entry*std::hypot(aim.direction[0],aim.direction[2]);
    if(horizontal>reach)return false;
    rating=.05f+.95f*horizontal/reach;return true;
}
inline bool SignpostName(char const* name){return name&&!std::strncmp(name,"Sign_",5)&&name[5];}
// IsFocus is called while drawing labels for many ordinary world entities.
// Reject their native names before querying player/menu state or VR geometry.
template<class Extension> inline bool ExtraSignpostFocus(char const* name,Extension&& extension){
    return SignpostName(name)&&extension();
}
inline bool SignpostInView(float const (&head)[3],float const (&forward)[3],float const (&point)[3],float units){
    if(!std::isfinite(units)||units<=0)return false;
    float length=0,dot=0,norm=0;
    for(unsigned i=0;i<3;++i){if(!std::isfinite(head[i])||!std::isfinite(forward[i])||!std::isfinite(point[i]))return false;
        float d=point[i]-head[i];length+=d*d;dot+=d*forward[i];norm+=forward[i]*forward[i];}
    return std::fabs(norm-1)<.02f&&length>.01f*units*units&&length<=64*units*units&&dot>=.94f*std::sqrt(length);
}
}
