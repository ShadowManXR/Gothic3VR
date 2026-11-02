#pragma once
#include <g3sdk/Script.h>
#include "NativeApplication.h"
#include "NativeRecovery.h"
namespace diagnostic {
class NativeArmedSneak {
    uintptr_t actor=0;gCScriptRoutine_PS* ownedRoutine=nullptr;gEAniState saved=gEAniState_Stand;
public:
    VirtualHook stateHook;gFScriptAIState normal=nullptr;
    bool Install(void* callback){
        if(stateHook.installed)return true;
        HMODULE module=GetModuleHandleW(L"Script_Game.dll");wchar_t path[MAX_PATH]{};unsigned char actual[32]{};
        constexpr unsigned char hash[]{0x4d,0x29,0x18,0x92,0x81,0xec,0x26,0xea,0xc7,0xc7,0x04,0xfa,0x67,0xed,0x32,0x3d,0xf5,0xc0,0x90,0xd5,0x0b,0x06,0x86,0x26,0xcf,0x60,0x0a,0xec,0xf4,0x26,0x2a,0x24};
        if(!module||!GetModuleFileNameW(module,path,MAX_PATH)||!FileSha256(path,actual)||memcmp(actual,hash,32))return false;
        auto* melee=GetScriptAdminExt().GetScriptAIState("PS_Melee_Sneak");
        auto* unarmed=GetScriptAdminExt().GetScriptAIState("PS_Normal_Sneak");
        if(!Readable(melee,sizeof(*melee))||!Readable(unarmed,sizeof(*unarmed))||
           reinterpret_cast<uintptr_t>(melee->m_funcScriptAIState)!=reinterpret_cast<uintptr_t>(module)+0x809c0||
           reinterpret_cast<uintptr_t>(unarmed->m_funcScriptAIState)!=reinterpret_cast<uintptr_t>(module)+0x82800)return false;
        normal=unarmed->m_funcScriptAIState;
        auto* entry=const_cast<gSScriptAIState*>(melee);
        return stateHook.Prepare(reinterpret_cast<void**>(&entry->m_funcScriptAIState),0,callback)&&stateHook.Install();
    }
    void Abandon(){actor=0;ownedRoutine=nullptr;}
    // Only an idle armed native loop can receive the stance correction. Attack,
    // parry, recovery, finishing and object interaction states stay native.
    void Apply(eCEntity* player,bool requested){
        if(requested&&!stateHook.installed)return;
        if(!Readable(player,sizeof(eCDynamicEntity))){Abandon();return;}
        auto key=reinterpret_cast<uintptr_t>(player);
        if(actor&&actor!=key)Abandon();
        auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(!Readable(routine,sizeof(*routine))){Abandon();return;}
        if(!requested){
            if(actor==key&&routine==ownedRoutine&&routine->GetAniState().GetValue()==gEAniState_Sneak&&
               !strcmp(routine->GetCurrentState().GetText(),"PS_Melee_Loop")){
                routine->SetAniState(bTPropertyContainer<gEAniState>(saved));Entity hero(player);hero.SetMovementMode(gECharMovementMode_Run);
            }Abandon();return;
        }
        if(NativeRecovery(player)||NativeActivity(player)||NativeAnimalForm(player))return;
        if(strcmp(routine->GetCurrentState().GetText(),"PS_Melee_Loop"))return;
        if(!actor){actor=key;ownedRoutine=routine;saved=gEAniState_Stand;}
        bool changed=routine->GetAniState().GetValue()!=gEAniState_Sneak;
        if(changed)routine->SetAniState(bTPropertyContainer<gEAniState>(gEAniState_Sneak));
        // Match native Normal_Sneak: walk translation with Sneak ani/state,
        // rather than a fake mode2 which lacks the native input/perception path.
        if(changed){Entity hero(player);hero.SetMovementMode(gECharMovementMode_Walk);}
    }
    GEBool Enter(bTObjStack<gScriptRunTimeSingleState>& stack,gCScriptProcessingUnit* unit){
        auto* player=unit->GetSelfEntity();GEBool result=normal(stack,unit);
        if(Readable(player,sizeof(eCDynamicEntity))&&gCSession::GetInstance().GetPlayer()==player){
            auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
            if(Readable(routine,sizeof(*routine))&&!strcmp(routine->GetCurrentState().GetText(),"PS_Normal_Loop")){
                Entity hero(player);hero.Routine.SetState("PS_Melee_Loop");Apply(player,true);
            }
        }return result;
    }
};
}
