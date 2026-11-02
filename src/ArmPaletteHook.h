#pragma once
#include "VirtualHook.h"
namespace diagnostic {
struct ArmPaletteHook {
    // Engine 1.75 / PUP 1.1.1, complete position-independent instructions.
    static constexpr unsigned char Prefix[9]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x14};
    unsigned char* original=nullptr;
    bool Install(void* entry,void* replacement){
        if(original||!Readable(entry,9)||!Executable(entry)||!Executable(replacement)||memcmp(entry,Prefix,9))return false;
        auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,14,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return false;
        memcpy(trampoline,entry,9);trampoline[9]=0xe9;int32_t back=int32_t(reinterpret_cast<uintptr_t>(entry)+9-reinterpret_cast<uintptr_t>(trampoline+14));memcpy(trampoline+10,&back,4);
        DWORD old=0;if(!VirtualProtect(trampoline,14,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),trampoline,14);
        unsigned char patch[9]={0xe9,0,0,0,0,0x90,0x90,0x90,0x90};int32_t jump=int32_t(reinterpret_cast<uintptr_t>(replacement)-reinterpret_cast<uintptr_t>(entry)-5);memcpy(patch+1,&jump,4);
        if(!VirtualProtect(entry,9,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        original=trampoline;memcpy(entry,patch,9);DWORD unused=0;VirtualProtect(entry,9,old,&unused);FlushInstructionCache(GetCurrentProcess(),entry,9);return true;
    }
};
}
