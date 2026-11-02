#include "FocusHook.h"
#include "NativeComfortProfile.h"
#include <cstdio>
struct State {unsigned char data[0x220]{};};
struct Vector {float x,y,z;};
__declspec(naked) void __fastcall NativeVelocity(State*,void*,Vector const&){
    __asm {
        push esi
        mov esi,ecx
        cmp dword ptr [esi+170h],0
        push edi
        mov edi,[esp+0ch]
        fld dword ptr [edi]
        fstp dword ptr [esi+134h]
        fld dword ptr [edi+4]
        fstp dword ptr [esi+138h]
        fld dword ptr [edi+8]
        fstp dword ptr [esi+13ch]
        pop edi
        pop esi
        ret 4
    }
}
__declspec(naked) void __fastcall NativeMouse(State*,void*,float){
    __asm {
        push ebp
        mov ebp,esp
        and esp,0fffffff8h
        fld dword ptr [ebp+8]
        fadd dword ptr [ecx+218h]
        fstp dword ptr [ecx+218h]
        mov esp,ebp
        pop ebp
        ret 4
    }
}
diagnostic::FocusHook velocityHook,mouseHook;bool instant=false;
void __fastcall Velocity(State* self,void*,Vector const& value){Vector wanted{0,value.y,100};reinterpret_cast<void(__thiscall*)(State*,Vector const&)>(velocityHook.original)(self,wanted);}
void __fastcall Mouse(State* self,void*,float input){if(instant)*reinterpret_cast<float*>(self->data+0x218)=input;else reinterpret_cast<void(__thiscall*)(State*,float)>(mouseHook.original)(self,input);}
int main(){
    State state;auto v=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeVelocity)));auto m=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeMouse)));
    if(!velocityHook.Install(v,reinterpret_cast<void*>(&Velocity),diagnostic::CurrentVelocityPrefix,10)||!mouseHook.Install(m,reinterpret_cast<void*>(&Mouse),diagnostic::FocusHook::FindPrefix,6))return 1;
    Vector native{20,-17,120};NativeVelocity(&state,nullptr,native);auto saved=reinterpret_cast<Vector*>(state.data+0x134);if(saved->x!=0||saved->y!=-17||saved->z!=100)return 2;
    for(unsigned i=0;i<10000;++i){instant=true;NativeMouse(&state,nullptr,3);NativeMouse(&state,nullptr,0);if(*reinterpret_cast<float*>(state.data+0x218)!=0)return 3;}
    instant=false;NativeMouse(&state,nullptr,2);NativeMouse(&state,nullptr,3);if(*reinterpret_cast<float*>(state.data+0x218)!=5)return 4;
    diagnostic::FocusHook rejected;if(rejected.Install(v,reinterpret_cast<void*>(&Velocity),diagnostic::CurrentVelocityPrefix,10))return 5;
    puts("PASS 10-byte native velocity trampoline, Y/cache consistency, void/float mouse ABI, immediate neutral input and native filter fallback");return 0;
}
