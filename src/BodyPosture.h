#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace diagnostic {
inline bool PickupLowerBody(int action,char const* task,char const* state){
    // Native PS_Normal_Take delegates inventory transfer to _AI_TakeItem.
    // Mining, campfires, chests and knockdowns are separate native activities.
    return action==66||(state&&!strcmp(state,"PS_Normal_Take"))||
        (task&&!strcmp(task,"_AI_TakeItem"));
}
// Optical stance offset only. It never changes the actor origin, movement
// mode, animation state, inventory task or tracked headset/controller pose.
class BodyPosture {
    uintptr_t actor=0,rig=0;uint64_t last=0,entered=0,left=0,pickupUntil=0;
    float scale=0,camera=0,target=0,native=0;bool sneak=false,pickup=false,crouched=false;
public:
    void Reset(){actor=rig=0;last=entered=left=pickupUntil=0;scale=camera=target=native=0;sneak=pickup=crouched=false;}
    void Update(uintptr_t player,uintptr_t skeleton,uint64_t now,float units,bool nativeSneak,bool taking,float nativeDrop){
        if(!player||!skeleton||!std::isfinite(units)||units<=0||!std::isfinite(nativeDrop)){Reset();return;}
        bool fresh=actor!=player||rig!=skeleton||scale!=units||now<last||(last&&now-last>250);
        if(fresh){Reset();actor=player;rig=skeleton;scale=units;last=now;}
        if(taking)pickupUntil=now+200;
        pickup=taking||now<pickupUntil;
        bool wanted=nativeSneak||(pickup&&sneak);
        float drop=std::clamp(nativeDrop,-.7f*units,0.f);
        if(wanted&&!sneak){entered=now;target=drop;}
        if(!wanted&&sneak)left=now;
        // Learn the native crouch's depth during its entry blend, then hold
        // the view steady instead of feeding footstep/pelvis bob to the HMD.
        if(wanted&&!pickup){native=drop;if(now-entered<=1000)target=std::min(target,drop);}
        if(!wanted){target=0;native=0;}
        float dt=float(now-last)*.001f;
        if(fresh)camera=target;
        else if(dt>0)camera+=(target-camera)*float(-std::expm1(-dt/.12f));
        if(std::fabs(camera-target)<.0005f*units)camera=target;
        crouched=wanted||(!wanted&&now-left<500&&camera<-.005f*units);
        sneak=wanted;last=now;
    }
    float CameraHeight()const{return camera;}
    float NativeHeight()const{return native;}
    bool Crouched()const{return crouched;}
    bool Pickup()const{return pickup;}
};
}
