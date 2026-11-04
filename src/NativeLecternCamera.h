#pragma once
#include <g3sdk/Script.h>
#include "VirtualHook.h"
#include "LecternInteraction.h"
namespace diagnostic {
inline bool NativeLecternInteraction(eCEntity* player,bool wasLectern=false){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    if(!Readable(routine,sizeof(*routine)))return false;
    auto* target=routine->GetSPU().GetTargetEntity();
    bool lectern=Readable(target,sizeof(eCDynamicEntity))&&gCEntity::GetUseType(target)==gEUseType_Bookstand;
    return LecternInteraction(lectern,int(routine->GetAction().GetValue()),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText(),wasLectern);
}
}
