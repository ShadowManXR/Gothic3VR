#pragma once
#include <g3sdk/Script.h>
#include <cstdio>
#include "NativeApplication.h"
#include "PickupPause.h"
namespace diagnostic {
class NativePickup {
    VirtualHook wait,animation;uintptr_t scriptBase=0;
    static bool Profile(wchar_t const* module,char const* hex){
        auto image=GetModuleHandleW(module);wchar_t path[MAX_PATH];unsigned char actual[32]{},expected[32]{};
        for(unsigned i=0;i<32;++i){unsigned value=0;if(sscanf_s(hex+2*i,"%2x",&value)!=1)return false;expected[i]=static_cast<unsigned char>(value);}
        return image&&GetModuleFileNameW(image,path,MAX_PATH)&&FileSha256(path,actual)&&!memcmp(actual,expected,32);
    }
    bool CallSite(uintptr_t callRva,uintptr_t iatRva)const{
        auto* code=reinterpret_cast<unsigned char const*>(scriptBase+callRva);uintptr_t operand=0;
        if(!Readable(code,6)||code[0]!=0xff||code[1]!=0x15)return false;
        memcpy(&operand,code+2,4);return operand==scriptBase+iatRva;
    }
public:
    using Instruction=GEBool(__stdcall*)(void*,gCScriptProcessingUnit*,GEBool);
    bool Install(void* waitReplacement,void* animationReplacement){
        if(!Profile(L"Game.dll","4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00")||
           !Profile(L"Script_Game.dll","4d29189281ec26eac7c704fa67ed323df5c090d50b068626cf600aecf4262a24"))return false;
        scriptBase=reinterpret_cast<uintptr_t>(GetModuleHandleW(L"Script_Game.dll"));
        auto game=GetModuleHandleW(L"Game.dll");
        auto* waitSlot=reinterpret_cast<void**>(scriptBase+0xe4140);
        auto* animationSlot=reinterpret_cast<void**>(scriptBase+0xe412c);
        if(!CallSite(0x82342,0xe4140)||!CallSite(0x4aeed,0xe412c)||!CallSite(0x4bb8d,0xe412c)||
           !Readable(waitSlot,4)||!Readable(animationSlot,4)||
           *waitSlot!=GetProcAddress(game,"?sAIWaitInstr@gCScriptProcessingUnit@@SG_NPAXPAV1@_N@Z")||
           *animationSlot!=GetProcAddress(game,"?sAIPlayAniInstr@gCScriptProcessingUnit@@SG_NPAXPAV1@_N@Z")||
           !wait.Prepare(waitSlot,0,waitReplacement)||!animation.Prepare(animationSlot,0,animationReplacement))return false;
        if(wait.Install()&&animation.Install())return true;
        animation.Restore();wait.Restore();return false;
    }
    bool Matches(uintptr_t caller,PickupPause kind)const{
        return wait.installed&&animation.installed&&caller>=scriptBase&&PickupPauseSite(caller-scriptBase,kind);
    }
    bool Skip(uintptr_t caller,PickupPause kind,void* args,gCScriptProcessingUnit* spu,GEBool fullStop,eCEntity* player)const{
        static_assert(sizeof(gCScriptProcessingUnit::sAIPlayAniInstr_Args)==28&&sizeof(gCScriptProcessingUnit::sAIWaitInstr_Args)==8);
        if(!Matches(caller,kind)||fullStop||!Readable(player,sizeof(eCEntity))||!Readable(spu,sizeof(*spu))||
           !Readable(args,kind==PickupPause::Wait?8:28))return false;
        eCEntity* actor=nullptr;memcpy(&actor,args,sizeof(actor));unsigned waitMs=500;
        if(kind==PickupPause::Wait)memcpy(&waitMs,static_cast<unsigned char*>(args)+4,4);
        return SkipPickupPause(kind,caller-scriptBase,true,true,false,reinterpret_cast<uintptr_t>(actor),
            reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(spu->GetSelfEntity()),waitMs);
    }
    GEBool Dispatch(PickupPause kind,void* args,gCScriptProcessingUnit* spu,GEBool fullStop)const{
        return reinterpret_cast<Instruction>(kind==PickupPause::Wait?wait.original:animation.original)(args,spu,fullStop);
    }
};
}
