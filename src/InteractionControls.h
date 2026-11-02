#pragma once
#include "ActivityCamera.h"
namespace diagnostic {
// Object interactions are not always native GUI pages. Their script can still
// consume real direction keys while the actor uses the interaction camera.
inline bool DirectionalActivityControls(int action,int animation,char const* task,char const* state){
    if(action==15||(task&&strstr(task,"FinishingAttack"))||(state&&strstr(state,"FinishingAttack")))return false;
    return ActivityThirdPerson(action,animation,task,state);
}
inline bool NativeSneakState(int,int animation,char const*){
    // Native IsSneak helper (Script_Game RVA1f10) checks AniState3. A state
    // named PS_Melee_Sneak executes _AI_Parade and is not stealth by itself.
    return animation==3;
}
}
