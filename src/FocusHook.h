#pragma once
#include "VirtualHook.h"
namespace diagnostic {
// Game.dll 1.75 / PUP 1.1.1: supplied complete prologues contain no relative
// instructions. The caller also validates the entire native module hash.
struct FocusHook {
    static constexpr unsigned char FindPrefix[6]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
    static constexpr unsigned char AnglesPrefix[5]={0x83,0xec,0x54,0x53,0x56};
    unsigned char* original=nullptr;
    bool Install(void* entry,void* replacement,unsigned char const* prefix,size_t bytes) {
        if(original||bytes<5||bytes>16||!Readable(entry,bytes)||!Executable(entry)||!Executable(replacement)||memcmp(entry,prefix,bytes))return false;
        auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,bytes+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return false;
        memcpy(trampoline,entry,bytes);trampoline[bytes]=0xe9;int32_t back=int32_t(reinterpret_cast<uintptr_t>(entry)+bytes-reinterpret_cast<uintptr_t>(trampoline+bytes+5));memcpy(trampoline+bytes+1,&back,4);
        DWORD old=0;if(!VirtualProtect(trampoline,bytes+5,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        FlushInstructionCache(GetCurrentProcess(),trampoline,bytes+5);unsigned char patch[16];memset(patch,0x90,sizeof(patch));patch[0]=0xe9;
        int32_t jump=int32_t(reinterpret_cast<uintptr_t>(replacement)-reinterpret_cast<uintptr_t>(entry)-5);memcpy(patch+1,&jump,4);
        if(!VirtualProtect(entry,bytes,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        original=trampoline;memcpy(entry,patch,bytes);DWORD unused=0;VirtualProtect(entry,bytes,old,&unused);FlushInstructionCache(GetCurrentProcess(),entry,bytes);return true;
    }
};
}
