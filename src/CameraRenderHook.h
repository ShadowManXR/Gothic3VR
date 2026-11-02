#pragma once
#include "VirtualHook.h"
namespace diagnostic
{
// Only the two verified, position-independent prologue instructions are copied.
// No generic instruction decoder or guessed relocation is used.
struct CameraRenderHook
{
    static constexpr unsigned char Prefix[8]={0x83,0xec,0x0c,0x80,0x7c,0x24,0x14,0x01};
    unsigned char* original=nullptr;
    bool Install(void* entry,void* replacement)
    {
        if(original||!Readable(entry,8)||!Executable(entry)||!Executable(replacement)||memcmp(entry,Prefix,8))return false;
        auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,13,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!trampoline)return false;
        memcpy(trampoline,entry,8);trampoline[8]=0xe9;int32_t back=static_cast<int32_t>(reinterpret_cast<uintptr_t>(entry)+8-reinterpret_cast<uintptr_t>(trampoline+13));memcpy(trampoline+9,&back,4);
        DWORD old=0;if(!VirtualProtect(trampoline,13,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        FlushInstructionCache(GetCurrentProcess(),trampoline,13);unsigned char patch[8]={0xe9,0,0,0,0,0x90,0x90,0x90};
        int32_t jump=static_cast<int32_t>(reinterpret_cast<uintptr_t>(replacement)-reinterpret_cast<uintptr_t>(entry)-5);memcpy(patch+1,&jump,4);
        if(!VirtualProtect(entry,8,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
        // Called from ScriptInit before rendering begins; the DLL stays pinned.
        original=trampoline;memcpy(entry,patch,8);DWORD unused=0;VirtualProtect(entry,8,old,&unused);FlushInstructionCache(GetCurrentProcess(),entry,8);return true;
    }
};
}
