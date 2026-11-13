#pragma once
#include <cstring>
namespace diagnostic {
inline bool CookingProp(char const* name){return name&&(!strcmp(name,"It_Stick")||!strcmp(name,"It_Meat"));}
inline bool CookingTarget(int type){return type==61||type==88;}
inline bool CookingInteraction(bool target,int action,char const* task,char const* state,bool wasCooking=false){
    auto ending=[](char const* name){return name&&!strcmp(name,"PS_Interact_End");};
    if(!target)return wasCooking&&(ending(task)||ending(state));
    auto cooking=[](char const* name){return name&&(!strcmp(name,"PS_Interact_Campfire")||
        !strcmp(name,"PS_Interact_Barbecue")||!strcmp(name,"PS_Interact_Loop")||
        !strcmp(name,"PS_Interact")||!strcmp(name,"PS_Interact_End"));};
    return action==73||action==94||cooking(task)||cooking(state);
}
}
