#include "RoomMotionScope.h"
#include "VirtualHook.h"
#include "TemporaryOverride.h"
#include <cstdio>
struct Movement {void** table;int mode=4;float speed=300;bool nativeGuard=false;};
diagnostic::RoomMotionScope scope;
diagnostic::VirtualHook hook;
unsigned nativeCalls=0;
// Reproduce the verified CP callback contract: +0x264 protects StopMovement
// but does not protect the later SetMovementMode(Standing) for a living actor.
void __fastcall NativeWorldUpdate(Movement* self,void*){++nativeCalls;if(!self->nativeGuard)self->speed=0;self->mode=1;}
void __fastcall WorldUpdate(Movement* self,void*){
    if(scope.Suppress(self,GetCurrentThreadId()))return;
    reinterpret_cast<void(__thiscall*)(Movement*)>(hook.original)(self);
}
void Dispatch(Movement& movement){reinterpret_cast<void(__thiscall*)(Movement*)>(movement.table[68])(&movement);}
void __cdecl TransferFault(void* context){Dispatch(*static_cast<Movement*>(context));RaiseException(0xe0710096,0,0,nullptr);}
void __cdecl Cleanup(void*){scope.End();}
int main(){
    void* table[69]{};table[68]=reinterpret_cast<void*>(&NativeWorldUpdate);Movement player{table},npc{table};
    player.nativeGuard=true;Dispatch(player);if(player.mode!=1||player.speed!=300)return 1;
    if(!hook.Prepare(table,68,reinterpret_cast<void*>(&WorldUpdate))||!hook.Install())return 2;
    player.mode=4;scope.Begin(&player,GetCurrentThreadId());Dispatch(player);
    if(player.mode!=4||player.speed!=300||scope.suppressed!=1)return 3;
    Dispatch(npc);if(npc.mode!=1||npc.speed!=0||nativeCalls!=2)return 4;
    scope.End();Dispatch(player);if(player.mode!=1)return 5;
    player.mode=5;scope.Begin(&player,GetCurrentThreadId()+1);Dispatch(player);if(player.mode!=1)return 6;scope.End();
    player.mode=4;scope.Begin(&player,GetCurrentThreadId());bool caught=false;
    __try{diagnostic::CallWithRestoration(TransferFault,Cleanup,&player);}__except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
    if(!caught||scope.movement||player.mode!=4)return 7;
    Dispatch(player);if(player.mode!=1)return 8;
    if(!hook.Restore())return 9;
    puts("PASS native standing-reset regression, scoped player virtual callback, NPC/thread passthrough and exception cleanup");return 0;
}
