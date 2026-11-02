#include "OpenXrClient.h"
#include "TrackingMath.h"
#include "HeadlessFrame.h"
#include <cstdio>
#include <cstring>
#include <vector>

namespace diagnostic
{
namespace
{
class RuntimeManifest
{
public:
    explicit RuntimeManifest(wchar_t const* manifest)
    {
        SetLastError(ERROR_SUCCESS);
        DWORD size = GetEnvironmentVariableW(L"XR_RUNTIME_JSON", nullptr, 0);
        mExisted = size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND;
        if (size) { std::vector<wchar_t> value(size); GetEnvironmentVariableW(L"XR_RUNTIME_JSON", value.data(), size); mPrevious = value.data(); }
        mSet = SetEnvironmentVariableW(L"XR_RUNTIME_JSON", manifest) != FALSE;
    }
    ~RuntimeManifest() { if (mSet) SetEnvironmentVariableW(L"XR_RUNTIME_JSON", mExisted ? mPrevious.c_str() : nullptr); }
    bool Ready() const { return mSet; }
private:
    bool mExisted = false, mSet = false;
    std::wstring mPrevious;
};
}
OpenXrClient::~OpenXrClient() { Disconnect(); if (mModule) FreeLibrary(mModule); }
bool OpenXrClient::Result(XrResult result, char const* operation)
{
    mResult = result;
    if (XR_SUCCEEDED(result)) return true;
    char description[XR_MAX_RESULT_STRING_SIZE]{};
    if (mInstance && mResultString) mResultString(mInstance, result, description);
    sprintf_s(mError, "%.60s: %.100s (%d)", operation, description, static_cast<int>(result));
    return false;
}
bool OpenXrClient::Load(wchar_t const* absoluteLoader)
{
    if (mModule) return true;
    wchar_t programFiles[32768]{};
    if (!GetEnvironmentVariableW(L"ProgramW6432", programFiles, 32768))
        GetEnvironmentVariableW(L"ProgramFiles", programFiles, 32768);
    mManifest = std::wstring(programFiles) + L"\\Virtual Desktop Streamer\\OpenXR\\virtualdesktop-openxr-32.json";
    if (GetFileAttributesW(mManifest.c_str()) == INVALID_FILE_ATTRIBUTES)
    { mLoaderError = ERROR_FILE_NOT_FOUND; strcpy_s(mError, "VDXR Win32 runtime manifest missing"); return false; }
    mModule = LoadLibraryExW(absoluteLoader, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!mModule) { mLoaderError = GetLastError(); return false; }
#define BIND(field, name) field = reinterpret_cast<decltype(field)>(GetProcAddress(mModule, name)); if (!field) { mLoaderError = ERROR_PROC_NOT_FOUND; FreeLibrary(mModule); mModule = nullptr; return false; }
    BIND(mGetProc,"xrGetInstanceProcAddr")
    BIND(mExtensions,"xrEnumerateInstanceExtensionProperties")
    BIND(mCreateInstance,"xrCreateInstance")
    BIND(mDestroyInstance,"xrDestroyInstance")
    BIND(mInstanceProperties,"xrGetInstanceProperties")
    BIND(mGetSystem,"xrGetSystem")
    BIND(mSystemProperties,"xrGetSystemProperties")
    BIND(mViews,"xrEnumerateViewConfigurationViews")
    BIND(mCreateSession,"xrCreateSession")
    BIND(mDestroySession,"xrDestroySession")
    BIND(mBeginSession,"xrBeginSession")
    BIND(mEndSession,"xrEndSession")
    BIND(mPoll,"xrPollEvent")
    BIND(mCreateSpace,"xrCreateReferenceSpace")
    BIND(mDestroySpace,"xrDestroySpace")
    BIND(mLocate,"xrLocateSpace")
    BIND(mResultString,"xrResultToString")
    BIND(mWaitFrame,"xrWaitFrame")
    BIND(mBeginFrame,"xrBeginFrame")
    BIND(mEndFrame,"xrEndFrame")
#undef BIND
    mLoaderError = 0;
    return true;
}
bool OpenXrClient::Instance()
{
    if (mInstance) return true;
    if (!mModule) return false;
    RuntimeManifest selected(mManifest.c_str());
    if (!selected.Ready()) return Result(XR_ERROR_INITIALIZATION_FAILED, "select VDXR manifest");
    uint32_t count = 0;
    if (!Result(mExtensions(nullptr,0,&count,nullptr),"enumerate extensions") || count > 256) return false;
    std::vector<XrExtensionProperties> extensions(count, {XR_TYPE_EXTENSION_PROPERTIES});
    if (!Result(mExtensions(nullptr,count,&count,extensions.data()),"read extensions")) return false;
    bool time = false;
    for (auto const& extension : extensions)
    {
        if (!strcmp(extension.extensionName, XR_MND_HEADLESS_EXTENSION_NAME)) mHeadless = true;
        if (!strcmp(extension.extensionName, XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME)) time = true;
    }
    if (!mHeadless || !time) return Result(XR_ERROR_EXTENSION_NOT_PRESENT,"VDXR headless / QPC extension required");
    char const* enabled[] = {XR_MND_HEADLESS_EXTENSION_NAME, XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME};
    XrInstanceCreateInfo create{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(create.applicationInfo.applicationName,"Gothic3VRDiagnostics");
    strcpy_s(create.applicationInfo.engineName,"Genome diagnostics");
    create.applicationInfo.apiVersion = XR_MAKE_VERSION(1,0,0);
    create.enabledExtensionCount = 2; create.enabledExtensionNames = enabled;
    if (!Result(mCreateInstance(&create,&mInstance),"create VDXR instance")) return false;
    XrInstanceProperties properties{XR_TYPE_INSTANCE_PROPERTIES};
    if (!Result(mInstanceProperties(mInstance,&properties),"runtime properties")) { Disconnect(); return false; }
    strcpy_s(mRuntimeName, properties.runtimeName);
    if (!Result(mGetProc(mInstance,"xrConvertWin32PerformanceCounterToTimeKHR",
        reinterpret_cast<PFN_xrVoidFunction*>(&mConvertTime)),"QPC time function")) { Disconnect(); return false; }
    return true;
}
bool OpenXrClient::Connect()
{
    if (mSession) return true;
    if (!Instance()) return false;
    XrSystemGetInfo get{XR_TYPE_SYSTEM_GET_INFO}; get.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!Result(mGetSystem(mInstance,&get,&mSystem),"get connected VDXR headset")) return false;
    XrSessionCreateInfo create{XR_TYPE_SESSION_CREATE_INFO}; create.systemId = mSystem;
    if (!Result(mCreateSession(mInstance,&create,&mSession),"create headless session")) return false;
    XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO}; space.poseInReferenceSpace.orientation.w = 1;
    space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    XrResult base = mCreateSpace(mSession,&space,&mBase);
    mStage = XR_SUCCEEDED(base);
    if (base == XR_ERROR_REFERENCE_SPACE_UNSUPPORTED)
    { space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL; base = mCreateSpace(mSession,&space,&mBase); }
    if (!Result(base,"create origin space")) { Disconnect(); return false; }
    space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    if (!Result(mCreateSpace(mSession,&space,&mView),"create headset space")) { Disconnect(); return false; }
    return true;
}
bool OpenXrClient::Read(HeadPose& pose)
{
    pose = HeadPose{};
    LARGE_INTEGER qpc; QueryPerformanceCounter(&qpc); pose.qpc = qpc.QuadPart;
    strcpy_s(pose.origin, mStage ? "stage" : "local");
    if (!mSession) return false;
    for (unsigned i=0; i<32; ++i)
    {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        XrResult polled = mPoll(mInstance,&event);
        if (polled == XR_EVENT_UNAVAILABLE) break;
        if (!Result(polled,"poll session")) { Disconnect(); return false; }
        if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) { Disconnect(); return false; }
        if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) continue;
        auto const& changed = reinterpret_cast<XrEventDataSessionStateChanged const&>(event);
        if (changed.state == XR_SESSION_STATE_READY && !mRunning)
        {
            XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO}; begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            if (!Result(mBeginSession(mSession,&begin),"begin headless session")) { Disconnect(); return false; }
            mRunning = true;
        }
        if (changed.state == XR_SESSION_STATE_STOPPING)
        { if (mRunning) mEndSession(mSession); mRunning = false; }
        if (changed.state == XR_SESSION_STATE_LOSS_PENDING || changed.state == XR_SESSION_STATE_EXITING)
        { Disconnect(); return false; }
    }
    pose.runtimeConnected = true;
    pose.deviceConnected = mRunning;
    if (!mRunning) return true;
    if (mPumpFrames)
    {
        char const* operation = nullptr;
        XrResult frame = CompleteHeadlessFrame(mSession,mWaitFrame,mBeginFrame,mEndFrame,operation);
        if (!Result(frame,operation))
        { Disconnect(); return false; }
    }
    QueryPerformanceCounter(&qpc); pose.qpc = qpc.QuadPart;
    XrTime time = 0;
    if (!Result(mConvertTime(mInstance,&qpc,&time),"QPC to OpenXR time")) return true;
    XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
    if (!Result(mLocate(mView,mBase,time,&location),"locate headset")) return true;
    pose.trackingResult = static_cast<int>(location.locationFlags);
    constexpr XrSpaceLocationFlags valid = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if ((location.locationFlags & valid) != valid) return true;
    auto const& p = location.pose.position;
    auto const& r = location.pose.orientation;
    float position[3] = {p.x,p.y,p.z}, rotation[4] = {r.x,r.y,r.z,r.w};
    pose.position[0]=p.x; pose.position[1]=p.y; pose.position[2]=p.z;
    pose.valid = TrackingFromXyzw(position,rotation,pose.transform,pose.quaternion);
    return true;
}
bool OpenXrClient::Info(HeadsetInfo& info)
{
    if (!mInstance || !mSystem) return false;
    info = HeadsetInfo{};
    XrSystemProperties properties{XR_TYPE_SYSTEM_PROPERTIES};
    if (!Result(mSystemProperties(mInstance,mSystem,&properties),"headset properties")) return false;
    strncpy_s(info.model,properties.systemName,_TRUNCATE);
    strncpy_s(info.trackingSystem,mRuntimeName,_TRUNCATE);
    uint32_t count = 0;
    XrViewConfigurationView views[2] = {{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    if (XR_SUCCEEDED(mViews(mInstance,mSystem,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views)) && count == 2)
    { info.recommendedWidth=views[0].recommendedImageRectWidth; info.recommendedHeight=views[0].recommendedImageRectHeight; }
    return true;
}
void OpenXrClient::Disconnect()
{
    if (mView) mDestroySpace(mView);
    if (mBase) mDestroySpace(mBase);
    if (mSession) mDestroySession(mSession);
    if (mInstance) mDestroyInstance(mInstance);
    mView = mBase = XR_NULL_HANDLE; mSession = XR_NULL_HANDLE; mInstance = XR_NULL_HANDLE;
    mSystem = XR_NULL_SYSTEM_ID; mRunning = false; mConvertTime = nullptr;
}
}
