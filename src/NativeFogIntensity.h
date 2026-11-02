#pragma once
#include <algorithm>
#include <cmath>
namespace diagnostic::fog {
// Native shader uniform: transmittance = saturate(x + distance * y).
// Scale opacity, not camera far planes, ROI, streaming or weather/save data.
// Work happens only when the engine updates fog constants, never per draw.
inline int Percent(int value) noexcept {return std::clamp((value+2)/5*5,0,100);}
inline bool Scale(float& x,float& y,int percent) noexcept {
    percent=Percent(percent);
    if(percent==100||!std::isfinite(x)||!std::isfinite(y)||y>0)return false;
    float scale=percent*.01f;
    x=1.0f+(x-1.0f)*scale;y*=scale;
    return true;
}
}
