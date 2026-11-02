#include "OpenVrClient.h"
#include "TrackingMath.h"
#include <cstring>
#include <vector>

namespace diagnostic
{
namespace
{
std::wstring InstalledSteamVr()
{
    for (REGSAM view : {KEY_WOW64_64KEY, KEY_WOW64_32KEY})
    {
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 250820",
            0, KEY_QUERY_VALUE | view, &key) != ERROR_SUCCESS) continue;
        wchar_t path[32768]{};
        DWORD size = sizeof(path), type = 0;
        LSTATUS result = RegQueryValueExW(key, L"InstallLocation", nullptr, &type,
            reinterpret_cast<BYTE*>(path), &size);
        RegCloseKey(key);
        if (result != ERROR_SUCCESS || type != REG_SZ || size >= sizeof(path)) continue;
        std::wstring runtime(path);
        DWORD attributes = GetFileAttributesW((runtime + L"\\bin\\vrclient.dll").c_str());
        if (!runtime.empty() && attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
            return runtime;
    }
    return {};
}
// Valve's loader reads VR_OVERRIDE during initialization. Preserve the caller's
// environment and change only this process while choosing the SteamVR client.
class RuntimeOverride
{
public:
    explicit RuntimeOverride(wchar_t const* path)
    {
        DWORD size = GetEnvironmentVariableW(L"VR_OVERRIDE", nullptr, 0);
        mExisted = size != 0 || GetLastError() != ERROR_ENVVAR_NOT_FOUND;
        if (size) { std::vector<wchar_t> value(size); GetEnvironmentVariableW(L"VR_OVERRIDE", value.data(), size); mPrevious = value.data(); }
        mChanged = SetEnvironmentVariableW(L"VR_OVERRIDE", path) != FALSE;
    }
    ~RuntimeOverride() { if (mChanged) SetEnvironmentVariableW(L"VR_OVERRIDE", mExisted ? mPrevious.c_str() : nullptr); }
    bool Ready() const { return mChanged; }
private:
    std::wstring mPrevious;
    bool mExisted = false, mChanged = false;
};
}
OpenVrClient::~OpenVrClient()
{
    Disconnect();
    if (mModule) FreeLibrary(mModule);
}
bool OpenVrClient::Load(wchar_t const* absolutePath)
{
    if (mModule) return true;
    mRuntimePath = InstalledSteamVr();
    if (mRuntimePath.empty()) { mLoaderError = ERROR_PATH_NOT_FOUND; return false; }
    mModule = LoadLibraryExW(absolutePath, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!mModule) { mLoaderError = GetLastError(); return false; }
    mInit = reinterpret_cast<Init>(GetProcAddress(mModule, "VR_InitInternal2"));
    mShutdown = reinterpret_cast<Shutdown>(GetProcAddress(mModule, "VR_ShutdownInternal"));
    mInterface = reinterpret_cast<Interface>(GetProcAddress(mModule, "VR_GetGenericInterface"));
    mErrorText = reinterpret_cast<ErrorText>(GetProcAddress(mModule, "VR_GetVRInitErrorAsEnglishDescription"));
    if (!mInit || !mShutdown || !mInterface || !mErrorText)
    {
        mLoaderError = ERROR_PROC_NOT_FOUND; FreeLibrary(mModule); mModule = nullptr; return false;
    }
    mLoaderError = ERROR_SUCCESS;
    return true;
}
bool OpenVrClient::Connect()
{
    if (mSystem) return true;
    if (!mModule) return false;
    RuntimeOverride runtime(mRuntimePath.c_str());
    if (!runtime.Ready()) { mInitError = vr::VRInitError_Init_Internal; return false; }
    mInitError = vr::VRInitError_None;
    mInit(&mInitError, vr::VRApplication_Background, nullptr);
    if (mInitError != vr::VRInitError_None) return false;
    mInitialized = true;
    mSystem = static_cast<vr::IVRSystem*>(mInterface(vr::IVRSystem_Version, &mInitError));
    if (!mSystem || mInitError != vr::VRInitError_None)
    {
        if (mInitError == vr::VRInitError_None) mInitError = vr::VRInitError_Init_InterfaceNotFound;
        Disconnect(); return false;
    }
    return true;
}
void OpenVrClient::Disconnect()
{
    mSystem = nullptr;
    if (mInitialized) mShutdown();
    mInitialized = false;
}
char const* OpenVrClient::ErrorDescription() const
{
    return mErrorText ? mErrorText(mInitError) : "OpenVR loader unavailable";
}
bool OpenVrClient::Read(HeadPose& pose)
{
    pose = HeadPose{};
    LARGE_INTEGER now; QueryPerformanceCounter(&now); pose.qpc = now.QuadPart;
    if (!mSystem) return false;
    vr::VREvent_t event{};
    for (unsigned i = 0; i < 16 && mSystem->PollNextEvent(&event, sizeof(event)); ++i)
        if (event.eventType == vr::VREvent_Quit)
        {
            mSystem->AcknowledgeQuit_Exiting(); Disconnect(); return false;
        }
    vr::TrackedDevicePose_t hmd{};
    mSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0.0f, &hmd, 1);
    QueryPerformanceCounter(&now); pose.qpc = now.QuadPart;
    pose.runtimeConnected = true;
    pose.deviceConnected = hmd.bDeviceIsConnected;
    pose.trackingResult = static_cast<int>(hmd.eTrackingResult);
    std::memcpy(pose.transform, hmd.mDeviceToAbsoluteTracking.m, sizeof(pose.transform));
    pose.position[0] = pose.transform[3]; pose.position[1] = pose.transform[7]; pose.position[2] = pose.transform[11];
    pose.valid = hmd.bPoseIsValid && hmd.bDeviceIsConnected && TrackingQuaternion(pose.transform, pose.quaternion);
    return true;
}
bool OpenVrClient::Info(HeadsetInfo& info)
{
    if (!mSystem) return false;
    info = HeadsetInfo{};
    mSystem->GetStringTrackedDeviceProperty(0, vr::Prop_ModelNumber_String, info.model, sizeof(info.model));
    mSystem->GetStringTrackedDeviceProperty(0, vr::Prop_ManufacturerName_String, info.manufacturer, sizeof(info.manufacturer));
    mSystem->GetStringTrackedDeviceProperty(0, vr::Prop_TrackingSystemName_String, info.trackingSystem, sizeof(info.trackingSystem));
    info.model[127] = info.manufacturer[127] = info.trackingSystem[127] = 0;
    mSystem->GetRecommendedRenderTargetSize(&info.recommendedWidth, &info.recommendedHeight);
    for (unsigned eye = 0; eye < 2; ++eye)
    {
        auto type = static_cast<vr::EVREye>(eye);
        auto transform = mSystem->GetEyeToHeadTransform(type);
        std::memcpy(info.eyeToHead + 12*eye, transform.m, sizeof(transform.m));
        float* raw = info.projectionRaw + 4*eye;
        mSystem->GetProjectionRaw(type, raw, raw+1, raw+2, raw+3);
    }
    return true;
}
}
