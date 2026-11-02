#pragma once
#include <cstring>
#include <cstdint>
namespace diagnostic {
// An unsuccessful Use can publish Interact for only one native update. It
// must not switch cameras and recenter the headset as if an activity occurred.
class ActivityCameraGate {
    uintptr_t actor=0;uint64_t since=0,last=0;bool candidate=false,active=false;
public:
    void Reset(){actor=0;since=last=0;candidate=active=false;}
    bool Step(uintptr_t player,bool requested,uint64_t now,bool immediate=false){
        if(!player){Reset();return false;}
        if(actor!=player||now<last){Reset();actor=player;}
        last=now;
        if(immediate){candidate=active=true;since=now;return true;}
        if(requested!=candidate){candidate=requested;since=now;}
        uint64_t delay=requested?140:80;
        if(now>=since&&now-since>=delay)active=requested;
        return active;
    }
};
inline bool ActivityCameraImmediate(int action,char const* task,char const* state){
    return action==15||(task&&strstr(task,"FinishingAttack"))||(state&&strstr(state,"FinishingAttack"));
}
inline bool ActivityThirdPerson(int action,int animation,char const* task,char const* state){
    // SDK native gEAction / gEAniState. Sneak, combat, casting, bow and
    // knockdown/get-up are deliberately outside these stationary activities.
    if(ActivityCameraImmediate(action,task,state))return true;
    if(action>=40&&action<=49)return false;
    if(action==73||(action>=78&&action<=118))return true;
    if(animation>=5&&animation<=14)return true;
    auto interaction=[](char const* text){return text&&(strstr(text,"Interact")||strstr(text,"UseMob")||strstr(text,"Use_Camp")||strstr(text,"Sleep"));};
    return interaction(task)||interaction(state);
}
inline bool LockpickControls(int action,char const* task,char const* state){
    if(action==100)return true; // SDK gEAction_Lockpick
    auto lock=[](char const* text){return text&&(strstr(text,"Lockpick")||strstr(text,"LockPick")||strstr(text,"PickLock")||strstr(text,"lockpick"));};
    return lock(task)||lock(state);
}
}
