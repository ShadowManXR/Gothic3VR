#include "FocusHook.h"
#include <cstdio>
struct NativeFocus {int value=40;};
// Fastcall's unused EDX parameter leaves exactly the same ECX/stack ABI as
// these native thiscall methods; naked stubs must be non-member functions.
__declspec(naked) int __fastcall NativeFind(NativeFocus*,void*,bool) {
        __asm {
            push ebp
            mov ebp,esp
            and esp,0fffffff8h
            mov eax,[ecx]
            movzx edx,byte ptr [ebp+8]
            add eax,edx
            mov esp,ebp
            pop ebp
            ret 4
        }
}
__declspec(naked) int __fastcall NativeAngles(NativeFocus*,void*,int,int,int) {
        __asm {
            sub esp,54h
            push ebx
            push esi
            mov eax,[ecx]
            pop esi
            pop ebx
            add esp,54h
            add eax,[esp+4]
            add eax,[esp+8]
            add eax,[esp+0ch]
            ret 0ch
        }
}
__declspec(naked) bool __fastcall NativeCheck(NativeFocus*,void*,int,int,float,float&) {
    __asm {
        push ebp
        mov ebp,esp
        and esp,0fffffff8h
        mov eax,[ebp+10h]
        mov edx,[ebp+14h]
        mov [edx],eax
        mov eax,[ecx]
        add eax,[ebp+8]
        add eax,[ebp+0ch]
        cmp eax,43
        sete al
        mov esp,ebp
        pop ebp
        ret 10h
    }
}
diagnostic::FocusHook findHook,angleHook,checkHook;
diagnostic::FocusHook stopHook;unsigned stopImportCalls=0;bool stopEntered=false,stopReturned=false;
void __cdecl StopImport(){++stopImportCalls;}
void* stopImportCell=reinterpret_cast<void*>(&StopImport);
__declspec(naked) void __fastcall NativeStop(NativeFocus*,void*) {
    __asm {
        push ecx
        push ebx
        push ebp
        mov ebp,dword ptr [stopImportCell]
        call ebp
        mov ecx,[esp+8]
        inc dword ptr [ecx]
        pop ebp
        pop ebx
        pop ecx
        ret
    }
}
void __fastcall Stop(NativeFocus* self,void*){stopEntered=true;reinterpret_cast<void(__thiscall*)(NativeFocus*)>(stopHook.original)(self);stopReturned=true;}
int __fastcall Find(NativeFocus* self,void*,bool flag) {return reinterpret_cast<int(__thiscall*)(NativeFocus*,bool)>(findHook.original)(self,flag)+100;}
int __fastcall Angles(NativeFocus* self,void*,int x,int y,int z) {return reinterpret_cast<int(__thiscall*)(NativeFocus*,int,int,int)>(angleHook.original)(self,x,y,z)+200;}
bool __fastcall Check(NativeFocus* self,void*,int x,int y,float best,float& rating){auto result=reinterpret_cast<bool(__thiscall*)(NativeFocus*,int,int,float,float&)>(checkHook.original)(self,x,y,best,rating);rating+=.25f;return result;}
int main() {
    NativeFocus focus;if(NativeFind(&focus,nullptr,false)!=40||NativeFind(&focus,nullptr,true)!=41||NativeAngles(&focus,nullptr,1,2,3)!=46)return 1;
    auto find=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeFind)));
    auto angles=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeAngles)));
    if(!findHook.Install(find,reinterpret_cast<void*>(&Find),diagnostic::FocusHook::FindPrefix,6)||
        !angleHook.Install(angles,reinterpret_cast<void*>(&Angles),diagnostic::FocusHook::AnglesPrefix,5))return 2;
    if(NativeFind(&focus,nullptr,false)!=140||NativeFind(&focus,nullptr,true)!=141||NativeAngles(&focus,nullptr,1,2,3)!=246||focus.value!=40)return 3;
    diagnostic::FocusHook rejected;if(rejected.Install(find,reinterpret_cast<void*>(&Find),diagnostic::FocusHook::FindPrefix,6))return 4;
    auto nativeCheck=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeCheck)));
    if(!checkHook.Install(nativeCheck,reinterpret_cast<void*>(&Check),diagnostic::FocusHook::FindPrefix,6))return 5;
    float rating=0;if(!NativeCheck(&focus,nullptr,1,2,.5f,rating)||rating!=.75f||NativeCheck(&focus,nullptr,2,2,.5f,rating))return 6;
    auto nativeStop=const_cast<void*>(diagnostic::ResolveJumpChain(reinterpret_cast<void*>(&NativeStop)));
    unsigned char prefix[9]{0x51,0x53,0x55,0x8b,0x2d};uintptr_t operand=reinterpret_cast<uintptr_t>(&stopImportCell);memcpy(prefix+5,&operand,4);
    if(!stopHook.Install(nativeStop,reinterpret_cast<void*>(&Stop),prefix,9))return 7;
    for(unsigned i=0;i<100;++i)NativeStop(&focus,nullptr);
    if(focus.value!=140||stopImportCalls!=100||!stopEntered||!stopReturned)return 8;
    puts("PASS native focus/stop complete prologues, copied absolute IAT operand, void thiscall callback, stack/callee registers and changed-prologue rejection");return 0;
}
