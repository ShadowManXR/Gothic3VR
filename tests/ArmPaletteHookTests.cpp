#include "FrozenArmPalette.h"
#include "FocusHook.h"
#include "ArmPaletteHook.h"
#include <cstdio>
diagnostic::ArmPaletteHook hook;unsigned calls=0;
using Update=unsigned(__thiscall*)(void*,unsigned,void const*);
unsigned __fastcall Replacement(void* self,void*,unsigned frame,void const* matrix){++calls;return reinterpret_cast<Update>(hook.original)(self,frame,matrix);}
int main(){
    // EBP-based ABI fixture with the engine's exact nine-byte aligned-stack
    // prologue. Return a checksum of ECX, frame ID and matrix reference.
    unsigned char code[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x14,0x8b,0x45,0x08,0x33,0x45,0x0c,0x33,0xc1,0x8b,0xe5,0x5d,0xc2,0x08,0x00};
    auto* entry=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(code),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!entry)return 1;memcpy(entry,code,sizeof(code));DWORD old=0;VirtualProtect(entry,sizeof(code),PAGE_EXECUTE_READ,&old);
    bool ok=hook.Install(entry,reinterpret_cast<void*>(&Replacement));unsigned checksum=0;
    if(ok){unsigned sentinel=0xabcdef;checksum=reinterpret_cast<Update>(entry)(reinterpret_cast<void*>(0x12345),0x6789,&sentinel);ok=checksum==(0x12345^0x6789^unsigned(reinterpret_cast<uintptr_t>(&sentinel)))&&calls==1;}
    printf("%s native ECX, arguments, return value and aligned-stack trampoline\n",ok?"PASS":"FAIL");
    if(hook.original)VirtualFree(hook.original,0,MEM_RELEASE);VirtualFree(entry,0,MEM_RELEASE);diagnostic::FrozenArmPalette frozen;diagnostic::arms::Mat prepared[3],left[3],right[3];
    diagnostic::arms::Position(prepared[0],{100,150,200});diagnostic::arms::Position(prepared[1],{60,130,240});diagnostic::arms::Position(prepared[2],{90,135,255});
    ok&=frozen.Capture(1,2,prepared,3);diagnostic::arms::Position(left[0],{-900,0,0});diagnostic::arms::Position(right[0],{900,0,0});
    // Both eyes see one prepared tracked rig even when their native cache
    // supplies different carry palettes or reports no update on the second eye.
    ok&=frozen.Apply(1,2,left,3)&&frozen.Apply(1,2,right,3)&&!memcmp(left,right,sizeof(left))&&!memcmp(left,prepared,sizeof(left));
    ok&=!frozen.Apply(1,9,right,3)&&!frozen.Apply(8,2,right,3)&&!frozen.Apply(1,2,right,2);
    frozen.Reset();ok&=!frozen.Apply(1,2,right,3);printf("%s both eyes share frozen 2H rig; owner/count/reset validation\n",ok?"PASS":"FAIL");
    unsigned char filmCode[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x10,0x8b,0x45,0x08,0x8b,0x00,0x89,0x01,0x8b,0xe5,0x5d,0xc2,0x04,0x00};
    auto* video=static_cast<unsigned char*>(VirtualAlloc(nullptr,sizeof(filmCode),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!video)return 1;memcpy(video,filmCode,sizeof(filmCode));VirtualProtect(video,sizeof(filmCode),PAGE_EXECUTE_READ,&old);
    diagnostic::FocusHook film;using NativeFilm=void(__thiscall*)(void*,void const*);unsigned movieResult=0,argument=0x12345678;
    // Exact verified Engine PlayVideo prologue: aligned stack, ECX owner,
    // one const reference argument, void return and ret 4 cleanup.
    ok&=film.Install(video,reinterpret_cast<void*>(&Replacement),diagnostic::FocusHook::FindPrefix,6);
    if(film.original){for(unsigned i=0;i<100;++i){argument+=1;reinterpret_cast<NativeFilm>(film.original)(&movieResult,&argument);ok&=movieResult==argument;}VirtualFree(film.original,0,MEM_RELEASE);}VirtualFree(video,0,MEM_RELEASE);
    printf("%s native PlayVideo trampoline preserves ECX/reference/void ABI over 100 calls\n",ok?"PASS":"FAIL");
    return ok?0:1;
}
