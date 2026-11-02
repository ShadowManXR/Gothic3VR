#include "CameraRenderHook.h"
#include <cstdio>
struct Camera {unsigned calls=0;bool allow=false,update=false;};
void __cdecl Capture(Camera* self,bool allow,bool update){++self->calls;self->allow=allow;self->update=update;}
// Same eight-byte native prefix, ECX/self and both stack arguments. Nothing
// from the engine DLL is loaded or executed by this trampoline test.
__declspec(naked) void __fastcall Native(Camera*,void*,bool,bool){
    __asm {
        sub esp,0ch
        cmp byte ptr [esp+14h],1
        push dword ptr [esp+14h]
        push dword ptr [esp+14h]
        push ecx
        call Capture
        add esp,18h
        ret 8
    }
}
diagnostic::CameraRenderHook hook;
unsigned forwarded=0;
void __fastcall Replacement(Camera* self,void*,bool allow,bool update){
    ++forwarded;reinterpret_cast<void(__thiscall*)(Camera*,bool,bool)>(hook.original)(self,allow,update);
}
int main(){
    Camera camera;
    Native(&camera,nullptr,false,true);
    if(camera.calls!=1||camera.allow||!camera.update)return 1;
    auto entry=reinterpret_cast<void*>(&Native);
    if(!hook.Install(entry,reinterpret_cast<void*>(&Replacement)))return 2;
    for(unsigned mask=0;mask<4;++mask){
        Native(&camera,nullptr,(mask&1)!=0,(mask&2)!=0);
        if(camera.calls!=mask+2||camera.allow!=bool(mask&1)||camera.update!=bool(mask&2))return 3;
    }
    if(forwarded!=4)return 4;
    diagnostic::CameraRenderHook rejected;
    if(rejected.Install(entry,reinterpret_cast<void*>(&Replacement)))return 5;
    puts("PASS real camera ECX/self and both stack bools through eight-byte trampoline, all flag combinations, original once, changed entry rejection");return 0;
}
