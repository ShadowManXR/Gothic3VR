#pragma once
#include "HeroVoiceChannel.h"
#include "VirtualHook.h"
namespace diagnostic {
// Full Game.dll SHA256 is checked by InstallMovementHook before installation.
// Both prologues consist of complete instructions without relative operands.
inline constexpr uint32_t VoiceStartRva=0x16fc50,VoiceUpdateRva=0x16fd90;
inline constexpr unsigned char VoiceStartPrefix[]{0x83,0xec,0x0c,0x53,0x8b,0xd9};
inline constexpr unsigned char VoiceUpdatePrefix[]{0x56,0x8b,0xf1,0x83,0x7e,0x6c,0x00};
struct NativeVoiceApi {
    // Legacy C++ FMOD exports use __stdcall, including the explicit Channel*
    // first argument on x86. PUP forwards these SAME exports to its backend.
    using GetModeFn=int(__stdcall*)(void*,uint32_t*);
    using SetModeFn=int(__stdcall*)(void*,uint32_t);
    using SetPanFn=int(__stdcall*)(void*,float);
    GetModeFn getMode=nullptr;SetModeFn setMode=nullptr;SetPanFn setPan=nullptr;
    bool Resolve(){
        auto module=GetModuleHandleW(L"fmodex.dll");if(!module)return false;
        getMode=reinterpret_cast<GetModeFn>(GetProcAddress(module,"?getMode@Channel@FMOD@@QAG?AW4FMOD_RESULT@@PAI@Z"));
        setMode=reinterpret_cast<SetModeFn>(GetProcAddress(module,"?setMode@Channel@FMOD@@QAG?AW4FMOD_RESULT@@I@Z"));
        setPan=reinterpret_cast<SetPanFn>(GetProcAddress(module,"?setPan@Channel@FMOD@@QAG?AW4FMOD_RESULT@@M@Z"));
        return Executable(reinterpret_cast<void*>(getMode))&&Executable(reinterpret_cast<void*>(setMode))&&
            Executable(reinterpret_cast<void*>(setPan));
    }
    int GetMode(uintptr_t h,uint32_t& m){return getMode(reinterpret_cast<void*>(h),&m);}
    int SetMode(uintptr_t h,uint32_t m){return setMode(reinterpret_cast<void*>(h),m);}
    int SetPan(uintptr_t h,float p){return setPan(reinterpret_cast<void*>(h),p);}
};
}
