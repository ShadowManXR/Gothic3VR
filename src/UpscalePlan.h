#pragma once
#include <algorithm>
namespace diagnostic {
enum class UpscaleMethod { Off=0, NIS=1, CAS=2 };
inline UpscaleMethod ValidUpscaleMethod(int n){return n>=0&&n<=2?UpscaleMethod(n):UpscaleMethod::Off;}
inline int ValidSmaaQuality(int n){return n>=0&&n<=2?n:0;}
struct UpscalePlan {
    unsigned inputWidth=0,inputHeight=0,outputWidth=0,outputHeight=0;
    UpscaleMethod method=UpscaleMethod::Off;
    int sharpness=25;
    bool fxaa=false,smaa=false;
    int smaaQuality=0; // Medium, High, Ultra
    bool Scaled()const{return inputWidth!=outputWidth||inputHeight!=outputHeight;}
};
// Explicit output selection avoids repeatedly scaling the last native INI value.
// With "Use game settings", apply sharpening at the existing native resolution.
inline UpscalePlan MakeUpscalePlan(unsigned width,unsigned height,UpscaleMethod method,int percent,int sharpness,bool explicitOutput=true,bool fxaa=false,bool smaa=false,int smaaQuality=0){
    UpscalePlan p{width,height,width,height,ValidUpscaleMethod(int(method)),std::clamp(sharpness,0,100),fxaa&&!smaa,smaa,ValidSmaaQuality(smaaQuality)};
    if(p.method!=UpscaleMethod::Off&&explicitOutput){
        percent=std::clamp(percent,50,100);
        auto scale=[&](unsigned n){if(percent==100)return n;return std::min(n,std::max(((n+3)/4)*2,((n*unsigned(percent)+100)/200)*2));};
        p.inputWidth=scale(width);p.inputHeight=scale(height);
    }
    return p;
}
// If the game was not restarted after a settings change, never silently
// allocate mismatched swapchains or reduce a full-size image for fake savings.
inline UpscalePlan MatchUpscaleInput(UpscalePlan p,unsigned width,unsigned height){
    if(p.inputWidth!=width||p.inputHeight!=height)p={width,height,width,height,p.method,p.sharpness,p.fxaa,p.smaa,p.smaaQuality};
    return p;
}
}
