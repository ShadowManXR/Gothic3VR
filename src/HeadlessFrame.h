#pragma once
#define XR_NO_PROTOTYPES
#include <openxr/openxr.h>
namespace diagnostic
{
inline XrResult CompleteHeadlessFrame(XrSession session, PFN_xrWaitFrame wait,
    PFN_xrBeginFrame begin, PFN_xrEndFrame end, char const*& operation)
{
    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState state{XR_TYPE_FRAME_STATE};
    operation = "wait headless frame";
    XrResult result = wait(session,&waitInfo,&state);
    if (XR_FAILED(result)) return result;
    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    operation = "begin headless frame";
    result = begin(session,&beginInfo);
    if (XR_FAILED(result)) return result;
    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime = state.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    // Headless sessions always end with zero composition layers, even when
    // shouldRender is set by a runtime. No game textures enter this path.
    operation = "end headless frame (zero layers)";
    return end(session,&endInfo);
}
}
