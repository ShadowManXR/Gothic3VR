#pragma once
#include <cstring>
namespace diagnostic {
inline bool LecternInteraction(bool lecternTarget,int action,char const* task,char const* state,bool wasLectern=false){
    auto named=[](char const* value,char const* expected){return value&&!strcmp(value,expected);};
    if(!lecternTarget)return wasLectern&&(named(task,"PS_Interact_End")||named(state,"PS_Interact_End"));
    return action==96||named(task,"PS_Interact_Bookstand")||named(state,"PS_Interact_Bookstand")||
        named(task,"PS_Interact")||named(state,"PS_Interact")||named(task,"PS_Interact_End")||named(state,"PS_Interact_End");
}
}
