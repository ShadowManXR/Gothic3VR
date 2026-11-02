#pragma once
#include <g3sdk/Script.h>
#include "VirtualHook.h"
#include "PhysicalMelee.h"
#include "ActivityCamera.h"
#include "InteractionControls.h"
#include "ShieldPose.h"
#include "KnockoutCamera.h"
namespace diagnostic {
inline bool NativeKnockoutThirdPerson(eCEntity* player,KnockoutCamera& camera){
    if(!Readable(player,sizeof(eCDynamicEntity))){camera.Reset();return false;}
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    if(!Readable(routine,sizeof(*routine))){camera.Reset();return false;}
    static_assert(gEAction_LieKnockOut==33&&gEAction_LiePiercedKO==37&&gEAniState_LieKnockOut==17&&gEAniState_LiePiercedKO==20);
    return camera.Update(reinterpret_cast<uintptr_t>(player),routine->GetAction().GetValue(),routine->GetAniState().GetValue(),gCEntity::IsDown(player),gCEntity::IsDead(player));
}
inline bool NativeActivity(eCEntity* player,bool* immediate=nullptr){
    if(immediate)*immediate=false;
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));if(!Readable(routine,sizeof(*routine)))return false;
    if(immediate)*immediate=ActivityCameraImmediate(routine->GetAction().GetValue(),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText());
    return ActivityThirdPerson(routine->GetAction().GetValue(),routine->GetAniState().GetValue(),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText());
}
inline bool NativeLockpick(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    static_assert(gEAction_Lockpick==100&&gEAction_FinishingAttack==15);
    return Readable(routine,sizeof(*routine))&&LockpickControls(routine->GetAction().GetValue(),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText());
}
inline bool NativeDirectionalActivity(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    return Readable(routine,sizeof(*routine))&&DirectionalActivityControls(routine->GetAction().GetValue(),routine->GetAniState().GetValue(),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText());
}
inline bool NativeLockpickSteps(eCEntity* player){
    if(NativeLockpick(player))return true;
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    if(!Readable(routine,sizeof(*routine))||!strstr(routine->GetCurrentState().GetText(),"PS_Interact_Chest"))return false;
    auto* target=routine->GetSPU().GetTargetEntity();
    if(!Readable(target,sizeof(eCDynamicEntity))||!target->HasPropertySet(eEPropertySetType_Lock))return false;
    Entity chest(target);return chest.Lock.IsLocked();
}
inline bool NativeSneaking(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    static_assert(gEAction_Sneak==77&&gEAniState_Sneak==3);
    return Readable(routine,sizeof(*routine))&&NativeSneakState(routine->GetAction().GetValue(),routine->GetAniState().GetValue(),routine->GetCurrentState().GetText());
}
inline bool NativeAnimalForm(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity))||!player->HasPropertySet(eEPropertySetType_NPC))return false;
    Entity hero(player);
    // Query the installed native wrapper rather than guessing a status bit.
    // Swapped creature player entities are also detected through native species.
    return hero.NPC.IsTransformed()||gESpecies(hero.NPC.Species)!=gESpecies_Human;
}
inline bool NativeBlocking(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    return Readable(routine,sizeof(*routine))&&NativeParryState(routine->GetAction().GetValue(),routine->GetAniState().GetValue());
}
inline bool NativeRecovery(eCEntity* player){
    if(!Readable(player,sizeof(eCDynamicEntity)))return true;
    static_assert(gEAction_SitKnockDown==29&&gEAction_LieKnockOut==33&&gEAniState_SitKnockDown==15&&gEAniState_LieKnockOut==17);
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    return melee::RecoveryState(Readable(routine,sizeof(*routine))?routine->GetAction().GetValue():-1,
        Readable(routine,sizeof(*routine))?routine->GetAniState().GetValue():-1,gCEntity::IsDown(player),gCEntity::IsDead(player));
}
}
