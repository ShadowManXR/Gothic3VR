#pragma once
namespace diagnostic {
enum class OpenXrRuntimeChoice { Vdxr=0, MetaLink=1, SteamVR=2, System=3 };
inline OpenXrRuntimeChoice ValidOpenXrRuntime(int value){return value>=0&&value<=3?OpenXrRuntimeChoice(value):OpenXrRuntimeChoice::Vdxr;}
inline wchar_t const* OpenXrRuntimeLabel(OpenXrRuntimeChoice value){
    switch(value){case OpenXrRuntimeChoice::MetaLink:return L"Meta Quest Link / Air Link (64-bit host)";
    case OpenXrRuntimeChoice::SteamVR:return L"SteamVR (OpenXR)";
    case OpenXrRuntimeChoice::System:return L"System default OpenXR (32-bit)";
    default:return L"Virtual Desktop / VDXR";}
}
}
