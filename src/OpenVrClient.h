#pragma once
#include <windows.h>
#include <openvr.h>
#include <cstdint>
#include <string>
#include "TrackingData.h"

namespace diagnostic
{
class OpenVrClient
{
public:
    ~OpenVrClient();
    bool Load(wchar_t const* absolutePath);
    bool Connect();
    void Disconnect();
    bool Read(HeadPose& pose);
    bool Info(HeadsetInfo& info);
    DWORD LoaderError() const { return mLoaderError; }
    int InitError() const { return static_cast<int>(mInitError); }
    char const* ErrorDescription() const;
private:
    using Init = uint32_t(__cdecl*)(vr::EVRInitError*, vr::EVRApplicationType, char const*);
    using Shutdown = void(__cdecl*)();
    using Interface = void*(__cdecl*)(char const*, vr::EVRInitError*);
    using ErrorText = char const*(__cdecl*)(vr::EVRInitError);
    HMODULE mModule = nullptr;
    Init mInit = nullptr;
    Shutdown mShutdown = nullptr;
    Interface mInterface = nullptr;
    ErrorText mErrorText = nullptr;
    vr::IVRSystem* mSystem = nullptr;
    vr::EVRInitError mInitError = vr::VRInitError_None;
    DWORD mLoaderError = 0;
    bool mInitialized = false;
    std::wstring mRuntimePath;
};
}
