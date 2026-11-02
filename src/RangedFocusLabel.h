#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace diagnostic {
inline int ScaleLabelHeight(int height,int percent){
    if(!height||height<-4096||height>4096||percent<1||percent>200)return 0;
    int magnitude=height<0?-height:height;
    int scaled=std::max(1,(magnitude*percent+50)/100);
    return height<0?-scaled:scaled;
}
inline int HalfLabelHeight(int height){
    if(!height||height<-4096||height>4096)return 0;
    return height<0?-std::max(1,(-height+1)/2):std::max(1,(height+1)/2);
}
class RangedFocusLabel {
    uintptr_t target=0;uint64_t serial=0,capturedAt=0;
public:
    static constexpr uint64_t FadeLimitMs=2000;
    void Reset(){target=0;serial=0;capturedAt=0;}
    bool IsTarget(uintptr_t entity)const{return target&&target==entity;}
    // The native HUD can still draw the previous ranged name while the weapon
    // is being sheathed and focus fades out. A new interaction target cancels
    // that style immediately; an empty focus must not enlarge the fading name.
    void InteractionFocus(uintptr_t entity){if(entity)Reset();}
    void Capture(uintptr_t entity,uint64_t frame,uint64_t now,bool ranged,bool character,float const (&origin)[3],float const (&lo)[3],float const (&hi)[3],float units,float closeReach){
        Reset();if(!entity||!ranged||!character||!std::isfinite(units)||units<=0||!std::isfinite(closeReach))return;
        float d2=0,nearRadius=std::max(2*units,closeReach);
        for(unsigned i=0;i<3;++i){
            if(!std::isfinite(origin[i])||!std::isfinite(lo[i])||!std::isfinite(hi[i])||lo[i]>hi[i])return;
            float d=std::max(lo[i]-origin[i],std::max(0.f,origin[i]-hi[i]));d2+=d*d;
        }
        if(d2>nearRadius*nearRadius){target=entity;serial=frame;capturedAt=now;}
    }
    bool Applies(uintptr_t entity,uint64_t frame,uint64_t now,bool allowed)const{
        return allowed&&target&&target==entity&&frame>=serial&&now>=capturedAt&&
            (frame==serial||now-capturedAt<=FadeLimitMs);
    }
};
}
