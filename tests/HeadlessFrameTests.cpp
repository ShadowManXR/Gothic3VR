#include "HeadlessFrame.h"
#include <cstdio>
#include <cstring>
namespace
{
unsigned stage = 0, failAt = 0, failures = 0;
void Check(bool condition, char const* name)
{ std::printf("%s %s\n",condition ? "PASS":"FAIL",name); if (!condition) ++failures; }
XrResult XRAPI_CALL Wait(XrSession session, XrFrameWaitInfo const* info, XrFrameState* state)
{
    if (session != 42 || stage != 0 || info->type != XR_TYPE_FRAME_WAIT_INFO || state->type != XR_TYPE_FRAME_STATE)
        return XR_ERROR_VALIDATION_FAILURE;
    stage = 1;
    state->predictedDisplayTime = 123456;
    state->shouldRender = XR_TRUE;
    return failAt == 1 ? XR_ERROR_SESSION_LOST : XR_SUCCESS;
}
XrResult XRAPI_CALL Begin(XrSession session, XrFrameBeginInfo const* info)
{
    if (session != 42 || stage != 1 || info->type != XR_TYPE_FRAME_BEGIN_INFO) return XR_ERROR_CALL_ORDER_INVALID;
    stage = 2;
    return failAt == 2 ? XR_ERROR_SESSION_LOST : XR_SUCCESS;
}
XrResult XRAPI_CALL End(XrSession session, XrFrameEndInfo const* info)
{
    if (session != 42 || stage != 2 || info->type != XR_TYPE_FRAME_END_INFO || info->displayTime != 123456 ||
        info->environmentBlendMode != XR_ENVIRONMENT_BLEND_MODE_OPAQUE || info->layerCount != 0 || info->layers != nullptr)
        return XR_ERROR_VALIDATION_FAILURE;
    stage = 3;
    return failAt == 3 ? XR_ERROR_SESSION_LOST : XR_SUCCESS;
}
}
int __cdecl main()
{
    char const* operation = nullptr;
    Check(diagnostic::CompleteHeadlessFrame(42,Wait,Begin,End,operation) == XR_SUCCESS && stage == 3,
        "wait/begin/end protocol, timestamp, zero layers despite shouldRender");
    stage = 0; failAt = 1;
    Check(diagnostic::CompleteHeadlessFrame(42,Wait,Begin,End,operation) == XR_ERROR_SESSION_LOST && stage == 1 && !strcmp(operation,"wait headless frame"),
        "wait failure stops before begin and end");
    stage = 0; failAt = 2;
    Check(diagnostic::CompleteHeadlessFrame(42,Wait,Begin,End,operation) == XR_ERROR_SESSION_LOST && stage == 2 && !strcmp(operation,"begin headless frame"),
        "begin failure stops before end");
    stage = 0; failAt = 3;
    Check(diagnostic::CompleteHeadlessFrame(42,Wait,Begin,End,operation) == XR_ERROR_SESSION_LOST && stage == 3 && !strcmp(operation,"end headless frame (zero layers)"),
        "end failure is reported to the client for session recovery");
    return failures ? 1:0;
}
