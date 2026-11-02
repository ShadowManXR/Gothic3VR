#pragma once
#include "VirtualHook.h"
namespace diagnostic {
struct MeleeHitPointsHook {
    // Script_Game 1.75: complete, position-independent prologue of ChangeHitPoints.
    static constexpr unsigned char Prefix[6]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
    unsigned char* original=nullptr;
    bool Install(void* entry,void* replacement){
        if(original||!Readable(entry,6)||!Executable(entry)||!Executable(replacement)||memcmp(entry,Prefix,6))return false;
        auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,11,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return false;
        memcpy(trampoline,entry,6);trampoline[6]=0xe9;int32_t back=int32_t(reinterpret_cast<uintptr_t>(entry)+6-reinterpret_cast<uintptr_t>(trampoline+11));memcpy(trampoline+7,&back,4);
        DWORD old=0;if(!VirtualProtect(trampoline,11,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),trampoline,11);
        unsigned char patch[6]={0xe9,0,0,0,0,0x90};int32_t jump=int32_t(reinterpret_cast<uintptr_t>(replacement)-reinterpret_cast<uintptr_t>(entry)-5);memcpy(patch+1,&jump,4);
        if(!VirtualProtect(entry,6,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        original=trampoline;memcpy(entry,patch,6);DWORD unused=0;VirtualProtect(entry,6,old,&unused);FlushInstructionCache(GetCurrentProcess(),entry,6);return true;
    }
};
}
