#pragma once
#include "VrSettings.h"
#include "VideoOptionsWriter.h"
namespace diagnostic {
// Apply the internal scene size before launch, so native postprocessing and UI
// are recreated coherently. NIS/CAS enlarge the scene to the selected XR output.
// No desktop mode, DSR or headset streaming setting is changed.
inline bool ApplyGameResolution(wchar_t const* userOptions,VrSettings const& s,DWORD* failure=nullptr) {
    if(failure)*failure=0;
    DWORD error=0;
    if(!RecoverVideoOptions(userOptions,error)){if(failure)*failure=error;return false;}
    if(s.renderWidth==0&&s.renderHeight==0)return true;
    if(!ResolutionIndex(s.renderWidth,s.renderHeight)){if(failure)*failure=ERROR_INVALID_PARAMETER;return false;}
    auto plan=MakeUpscalePlan(s.renderWidth,s.renderHeight,s.upscaleMethod,s.upscaleScalePercent,s.upscaleSharpness);
    bool ok=WriteVideoOptions(userOptions,plan.inputWidth,plan.inputHeight,error);if(failure)*failure=error;return ok;
}
}
