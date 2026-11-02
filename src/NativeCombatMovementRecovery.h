#pragma once
#include <g3sdk/Script.h>
#include "CombatMovementRecovery.h"
#include "NativeApplication.h"
namespace diagnostic {
class NativeCombatMovementRecovery {
    CombatMovementRecovery recovery;bool attempted=false,ready=false;
    uintptr_t combatCallback=0;
public:
    void Reset(){recovery.Reset();}
    // Caller requires NativeMelee::ready, whose installation verifies the full
    // Game/Engine/Script/Script_Game hashes. These entry points and the private
    // instruction-action read are additionally verified against that binary.
    bool Initialize(){
        if(attempted)return ready;attempted=true;
        auto game=GetModuleHandleW(L"Game.dll");auto base=reinterpret_cast<uintptr_t>(game);
        combatCallback=reinterpret_cast<uintptr_t>(GetProcAddress(game,"?sAICombatMoveInstr@gCScriptProcessingUnit@@SG_NPAXPAV1@_N@Z"));
        auto stop=reinterpret_cast<uintptr_t>(GetProcAddress(game,"?AIStopCombatMove@gCScriptRoutine_PS@@QAEXXZ"));
        unsigned char actionStore[]{0x89,0x96,0x54,0x01,0,0};
        unsigned char stopPrefix[]{0x8b,0x81,0xd8,0,0,0,0x83,0xc1,0x64};
        ready=game&&combatCallback==base+0x1696e0&&stop==base+0x1644d0&&
            Readable(reinterpret_cast<void*>(base+0x16abed),sizeof(actionStore))&&
            !memcmp(reinterpret_cast<void*>(base+0x16abed),actionStore,sizeof(actionStore))&&
            Readable(reinterpret_cast<void*>(stop),sizeof(stopPrefix))&&!memcmp(reinterpret_cast<void*>(stop),stopPrefix,sizeof(stopPrefix));
        return ready;
    }
    CombatMoveRepair Update(eCEntity* player,gCCharacterMovement_PS* movement,gCScriptRoutine_PS* routine,bool allowed,bool physicalMelee,uint64_t now){
        static_assert(offsetof(gCScriptProcessingUnit,m_pfInstrCallback)==0x74,"verified SPU instruction owner");
        if(!allowed||!Initialize()||!Readable(player,sizeof(eCDynamicEntity))||!Readable(movement,sizeof(*movement))||!Readable(routine,sizeof(*routine))){Reset();return CombatMoveRepair::None;}
        auto& spu=routine->GetSPU();if(!Readable(&spu,sizeof(spu))){Reset();return CombatMoveRepair::None;}
        CombatMoveState state;state.player=reinterpret_cast<uintptr_t>(player);state.movement=reinterpret_cast<uintptr_t>(movement);state.routine=reinterpret_cast<uintptr_t>(routine);
        state.allowed=allowed;state.physicalMelee=physicalMelee;state.combat=movement->IsCombatMovementEnabledFromSPU()!=GEFalse;
        state.mode=int(movement->GetMovementMode());state.action=routine->GetAction().GetValue();state.animation=routine->GetAniState().GetValue();
        auto const* name=routine->GetCurrentState().GetText();state.idle=name&&(!strcmp(name,"PS_Melee_Loop")||!strcmp(name,"PS_Normal_Loop"));
        state.callback=reinterpret_cast<uintptr_t>(spu.m_pfInstrCallback);state.combatCallback=combatCallback;
        memcpy(&state.instructionAction,reinterpret_cast<unsigned char const*>(&spu)+0x154,sizeof(state.instructionAction));
        auto repair=recovery.Step(state,now);
        if(repair==CombatMoveRepair::PhysicalAttack){
            // Native cancellation retires the instruction, releases motion
            // ownership, combat translation/rotation and movement inhibition.
            routine->AIStopCombatMove();
        }else if(repair==CombatMoveRepair::Orphan){
            // No instruction remains to run its native epilogue. Match that
            // epilogue's three SDK setters; preserve position, velocity, goals,
            // movement mode, input, routine, HP, stamina and damage calculation.
            eCEntityProxy none;none.SetEntity(nullptr);bCVector zero(0,0,0);
            movement->EnableMovementFromSPU(GETrue);
            movement->EnableCombatRotationFromSPU(GEFalse,0.f);
            movement->EnableCombatMovementFromSPU(GEFalse,none,zero);
        }
        return repair;
    }
};
}
