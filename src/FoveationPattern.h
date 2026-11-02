#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace diagnostic {
inline int FfrMode(int n){return n>=0&&n<=3?n:0;}
inline int FfrTransition(int n){return std::clamp(n,0,30);}
// Landscape oval in each eye's normalized view (3:2 horizontal/vertical axes).
// Reciprocal axis scales keep the same nominal area as the previous diameter,
// before viewport clipping and conservative tile coverage. Render-target pixel
// aspect is deliberately not used: XR projects this texture onto the eye FOV.
inline constexpr float FfrHorizontalDistanceScale=.816496581f; // 1 / sqrt(1.5)
inline constexpr float FfrVerticalDistanceScale=1.224744871f;   // sqrt(1.5)
inline float FfrOvalDistanceSquared(float dx,float dy){
    dx*=FfrHorizontalDistanceScale;dy*=FfrVerticalDistanceScale;return dx*dx+dy*dy;
}
// The existing percentage scales both axes. A tile touching the central oval
// always stays 1x1; no hidden-area, geometry or texture culling is introduced.
inline uint8_t FfrTile(unsigned x,unsigned y,unsigned tileW,unsigned tileH,unsigned width,unsigned height,float centerX,float centerY,int mode,int diameter){
    if(!FfrMode(mode))return 0;
    float x0=float(x*tileW)/width,x1=std::min(1.f,float((x+1)*tileW)/width);
    float y0=float(y*tileH)/height,y1=std::min(1.f,float((y+1)*tileH)/height);
    float dx=std::max({x0-centerX,centerX-x1,0.f})*2,dy=std::max({y0-centerY,centerY-y1,0.f})*2;
    float d=FfrOvalDistanceSquared(dx,dy),r=std::clamp(diameter,40,95)*.01f;
    if(d<=r*r)return 0;
    if(mode==3&&d>1.f)return 2; // Only extreme periphery may use 4x4.
    return 1; // 2x2 shading, geometry and depth stay native resolution.
}
inline std::vector<uint8_t> FfrPattern(unsigned width,unsigned height,unsigned tileW,unsigned tileH,float centerX,float centerY,int mode,int diameter){
    if(!width||!height||!tileW||!tileH)return {};
    unsigned w=(width+tileW-1)/tileW,h=(height+tileH-1)/tileH;std::vector<uint8_t> pixels(w*h);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)pixels[y*w+x]=FfrTile(x,y,tileW,tileH,width,height,centerX,centerY,mode,diameter);
    return pixels;
}
}
