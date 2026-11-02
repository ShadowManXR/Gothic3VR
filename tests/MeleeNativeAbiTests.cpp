#include "VirtualHook.h"
#include "MeleeHitPointsHook.h"
#include "TemporaryOverride.h"
#include "PhysicalMelee.h"
#include <cstdio>
struct Target {void** table;int hp=1000,amount=0;};
struct Contact {int magic=7654;};
diagnostic::VirtualHook hook;diagnostic::MeleeHitPointsHook healthHook;
Target* scoped=nullptr;void* sword=nullptr;DWORD thread=0;float factor=.25f;unsigned callbacks=0;
bool arguments=true;
int __stdcall HealthBody(void* unit,void* self,void* other,int delta){
    arguments&=unit==reinterpret_cast<void*>(0x5678)&&other==nullptr;
    auto* target=static_cast<Target*>(self);target->hp+=delta;target->amount=-delta;return 1;
}
// Exact native six-byte prologue and aligned stdcall argument forwarding.
__declspec(naked) int __stdcall NativeHealth(void*,void*,void*,int){
    __asm {
        push ebp
        mov ebp,esp
        and esp,-8
        sub esp,8
        push dword ptr [ebp+20]
        push dword ptr [ebp+16]
        push dword ptr [ebp+12]
        push dword ptr [ebp+8]
        call HealthBody
        mov esp,ebp
        pop ebp
        ret 16
    }
}
int __stdcall Health(void* unit,void* self,void* other,int delta){
    if(self==scoped&&thread==GetCurrentThreadId()&&delta<0)delta=-diagnostic::melee::ScaledDamage(-delta,80,factor);
    return reinterpret_cast<int(__stdcall*)(void*,void*,void*,int)>(healthHook.original)(unit,self,other,delta);
}
void __fastcall NativeDamage(Target* self,void*,void* inflictor,void* other,int amount,int type,Contact& contact){
    ++callbacks;arguments&=inflictor==sword&&other==nullptr&&type==2&&contact.magic==7654;
    // Real OnDamage calls AssessHit again, overwriting the supplied amount.
    (void)amount;
    NativeHealth(reinterpret_cast<void*>(0x5678),self,nullptr,-160);
}
void __fastcall Damage(Target* self,void*,void* inflictor,void* other,int amount,int type,Contact& contact){
    if(self==scoped&&inflictor==sword&&thread==GetCurrentThreadId())amount=diagnostic::melee::ScaledDamage(amount,80,factor);
    reinterpret_cast<void(__thiscall*)(Target*,void*,void*,int,int,Contact&)>(hook.original)(self,inflictor,other,amount,type,contact);
}
void Dispatch(Target* self,int amount){Contact contact;reinterpret_cast<void(__thiscall*)(Target*,void*,void*,int,int,Contact&)>(self->table[96])(self,sword,nullptr,amount,2,contact);}
void __cdecl Fault(void*){RaiseException(0xe0710100,0,0,nullptr);}
void __cdecl Cleanup(void*){scoped=nullptr;thread=0;}
int main(){
    void* table[97]{};table[96]=reinterpret_cast<void*>(&NativeDamage);Target target{table},npc{table};sword=reinterpret_cast<void*>(0x1234);
    if(!hook.Prepare(table,96,reinterpret_cast<void*>(&Damage))||!hook.Install()||!healthHook.Install(reinterpret_cast<void*>(&NativeHealth),reinterpret_cast<void*>(&Health)))return 1;
    scoped=&target;thread=GetCurrentThreadId();Dispatch(&target,160);
    if(target.hp!=960||target.amount!=40||!arguments)return 2;
    Dispatch(&npc,160);if(npc.hp!=840)return 3;
    thread=GetCurrentThreadId()+1;Dispatch(&target,160);if(target.hp!=800)return 4;
    thread=GetCurrentThreadId();factor=1;Dispatch(&target,160);if(target.hp!=640)return 5;
    NativeHealth(reinterpret_cast<void*>(0x5678),&target,nullptr,25);if(target.hp!=665)return 6;
    factor=.25f;target.hp=100;Dispatch(&target,160);if(target.hp!=60)return 7;
    bool caught=false;__try{diagnostic::CallWithRestoration(Fault,Cleanup,nullptr);}__except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
    if(!caught||scoped||thread)return 8;
    Dispatch(&target,40);if(target.hp!=-100||!arguments||callbacks!=6||!hook.Restore())return 9;
    puts("PASS Win32 damage/ChangeHitPoints ABI; recalculated damage capped before HP/death decisions; heal/NPC/thread/fault passthrough");return 0;
}
