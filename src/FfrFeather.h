#pragma once
#include "FoveationPattern.h"
namespace diagnostic {
struct FfrFeatherParameters {
    float inverseSize[2]{},center[2]{.5f,.5f};
    float radius=0,band=0,blurPixels=0,enabled=0;
};
static_assert(sizeof(FfrFeatherParameters)==32);
inline FfrFeatherParameters MakeFfrFeather(unsigned width,unsigned height,int mode,int diameter,int transition,float centerX=.5f,float centerY=.5f){
    FfrFeatherParameters p;
    if(!width||!height||!FfrMode(mode)||!FfrTransition(transition)||!std::isfinite(centerX)||!std::isfinite(centerY)||centerX<.1f||centerX>.9f||centerY<.1f||centerY>.9f)return p;
    p.inverseSize[0]=1.f/width;p.inverseSize[1]=1.f/height;p.center[0]=centerX;p.center[1]=centerY;
    p.radius=std::clamp(diameter,40,95)*.01f;p.band=FfrTransition(transition)*.01f;p.blurPixels=mode==3?2.5f:1.5f;p.enabled=1;return p;
}
inline float FfrFeatherAmount(FfrFeatherParameters const& p,float u,float v){
    if(!p.enabled||p.band<=0)return 0;
    float d=std::sqrt(FfrOvalDistanceSquared((u-p.center[0])*2,(v-p.center[1])*2));
    float t=std::clamp((d-(p.radius-p.band*.25f))/p.band,0.f,1.f);return t*t*(3-2*t);
}
}
