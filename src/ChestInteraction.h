#pragma once
#include <cstring>
#include <cstdint>
namespace diagnostic {
// The native chest task ends before its camera has finished returning. Keep
// the render-only override until first person is stable, with a bounded tail
// for configurations that deliberately leave the native camera in third person.
class ChestViewTransition {
    uintptr_t owner=0;bool active=false,ending=false,settling=false;
    uint64_t ended=0,readySince=0;
public:
    void Reset(){owner=0;active=ending=settling=false;ended=readySince=0;}
    bool Step(uintptr_t player,bool allowed,bool chest,bool cameraReady,uint64_t now){
        if(!allowed||!player){Reset();return false;}
        if(owner!=player){Reset();owner=player;}
        if(chest){active=true;ending=settling=false;return true;}
        if(!active)return false;
        if(!ending){ending=true;ended=now;}
        if(cameraReady){if(!settling){settling=true;readySince=now;}}
        else settling=false;
        if(now<ended||now-ended>=2000||(settling&&now>=readySince&&now-readySince>=200)){
            Reset();return false;
        }
        return true;
    }
};
inline bool SkipChestApproach(bool enabled,bool player,bool chest,char const* parent){
    return enabled&&player&&chest&&parent&&!strcmp(parent,"PS_Interact");
}
inline bool ChestInteraction(bool chestTarget,int action,char const* task,char const* state,bool wasChest=false){
    if(!chestTarget)return wasChest&&((task&&!strcmp(task,"PS_Interact_End"))||(state&&!strcmp(state,"PS_Interact_End")));
    auto chest=[](char const* name){return name&&(!strcmp(name,"PS_Interact_Chest")||!strcmp(name,"PS_Interact_Chest_Loop"));};
    auto transition=[](char const* name){return name&&(!strcmp(name,"PS_Interact")||!strcmp(name,"PS_Interact_End"));};
    return action==99||action==100||chest(task)||chest(state)||transition(task)||transition(state);
}
}
