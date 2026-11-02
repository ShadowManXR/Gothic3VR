#pragma once
#include <windows.h>
#include <unknwn.h>
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "TrackingData.h"
#include <string>

namespace diagnostic
{
class OpenXrClient
{
public:
    ~OpenXrClient();
    bool Load(wchar_t const* absoluteLoader);
    bool Connect();
    bool Read(HeadPose& pose);
    bool Info(HeadsetInfo& info);
    void Disconnect();
    DWORD LoaderError() const { return mLoaderError; }
    int InitError() const { return static_cast<int>(mResult); }
    char const* ErrorDescription() const { return mError; }
    bool HeadlessSupported() const { return mHeadless; }
    char const* RuntimeName() const { return mRuntimeName; }
    void EnableFramePump(bool enabled) { mPumpFrames = enabled; }
private:
    bool Instance();
    bool Result(XrResult result, char const* operation);
    HMODULE mModule = nullptr;
    DWORD mLoaderError = 0;
    XrResult mResult = XR_SUCCESS;
    char mError[192] = "OpenXR loader not initialized", mRuntimeName[XR_MAX_RUNTIME_NAME_SIZE]{};
    std::wstring mManifest;
    XrInstance mInstance = XR_NULL_HANDLE;
    XrSystemId mSystem = XR_NULL_SYSTEM_ID;
    XrSession mSession = XR_NULL_HANDLE;
    XrSpace mBase = XR_NULL_HANDLE, mView = XR_NULL_HANDLE;
    bool mRunning = false, mHeadless = false, mStage = false, mPumpFrames = true;
    PFN_xrWaitFrame mWaitFrame = nullptr;
    PFN_xrBeginFrame mBeginFrame = nullptr;
    PFN_xrEndFrame mEndFrame = nullptr;
    PFN_xrGetInstanceProcAddr mGetProc = nullptr;
    PFN_xrEnumerateInstanceExtensionProperties mExtensions = nullptr;
    PFN_xrCreateInstance mCreateInstance = nullptr;
    PFN_xrDestroyInstance mDestroyInstance = nullptr;
    PFN_xrGetInstanceProperties mInstanceProperties = nullptr;
    PFN_xrGetSystem mGetSystem = nullptr;
    PFN_xrGetSystemProperties mSystemProperties = nullptr;
    PFN_xrEnumerateViewConfigurationViews mViews = nullptr;
    PFN_xrCreateSession mCreateSession = nullptr;
    PFN_xrDestroySession mDestroySession = nullptr;
    PFN_xrBeginSession mBeginSession = nullptr;
    PFN_xrEndSession mEndSession = nullptr;
    PFN_xrPollEvent mPoll = nullptr;
    PFN_xrCreateReferenceSpace mCreateSpace = nullptr;
    PFN_xrDestroySpace mDestroySpace = nullptr;
    PFN_xrLocateSpace mLocate = nullptr;
    PFN_xrResultToString mResultString = nullptr;
    PFN_xrConvertWin32PerformanceCounterToTimeKHR mConvertTime = nullptr;
};
}
