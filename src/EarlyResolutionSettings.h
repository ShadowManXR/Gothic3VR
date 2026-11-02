#pragma once
#include "VrSettings.h"
namespace diagnostic {
// "From game" leaves the INI intact, but a custom windowed mode saved by a
// previous launcher run still needs the early context patch on the next run.
inline bool EarlyResolutionTarget(VrSettings const& settings,wchar_t const* nativeOptions,unsigned& width,unsigned& height) {
    if(ResolutionIndex(settings.renderWidth,settings.renderHeight)){auto p=MakeUpscalePlan(settings.renderWidth,settings.renderHeight,settings.upscaleMethod,settings.upscaleScalePercent,settings.upscaleSharpness);width=p.inputWidth;height=p.inputHeight;return true;}
    if(!nativeOptions||GetPrivateProfileIntW(L"Options.Video",L"WindowMode",-1,nativeOptions)!=0)return false;
    int x=GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,nativeOptions),y=GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,nativeOptions);
    if(!ResolutionIndex(x,y))return false;
    width=x;height=y;return true;
}
}
