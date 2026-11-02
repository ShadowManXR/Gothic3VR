#pragma once
#include "VirtualHook.h"
namespace diagnostic {
// CP 1.75 ToVirtualCoords starts with SUB ESP,14h and CMP [ECX+9Ch],0.
// These complete instructions contain no relative addresses. Engine hash is
// checked by InstallStereoHook before this entry is patched.
struct UiMouseHook {
    static constexpr unsigned char Prefix[10]={0x83,0xec,0x14,0x80,0xb9,0x9c,0,0,0,0};
    unsigned char* original=nullptr;
    bool Install(void* entry,void* replacement) {
        if(original||!Readable(entry,10)||!Executable(entry)||!Executable(replacement)||memcmp(entry,Prefix,10))return false;
        auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,15,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return false;
        memcpy(trampoline,entry,10);trampoline[10]=0xe9;
        int32_t jump=int32_t(uintptr_t(entry)+10-uintptr_t(trampoline+15));memcpy(trampoline+11,&jump,4);
        DWORD old;if(!VirtualProtect(trampoline,15,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        FlushInstructionCache(GetCurrentProcess(),trampoline,15);
        unsigned char patch[10];memset(patch,0x90,10);patch[0]=0xe9;jump=int32_t(uintptr_t(replacement)-uintptr_t(entry)-5);memcpy(patch+1,&jump,4);
        if(!VirtualProtect(entry,10,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        original=trampoline;memcpy(entry,patch,10);DWORD ignored;VirtualProtect(entry,10,old,&ignored);FlushInstructionCache(GetCurrentProcess(),entry,10);return true;
    }
};
}
