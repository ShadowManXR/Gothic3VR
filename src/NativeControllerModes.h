#pragma once
#include <cstdint>
namespace diagnostic {
// Controller input selects a native wished mode. It never generates velocity,
// writes the native wished direction or changes animation/physics properties.
class NativeControllerModes {
    uintptr_t actor=0;uint64_t time=0,frame=0;bool owned=false,moving=false,sneaking=false,sprinting=false,crouchWalking=false;
public:
    void Reset(){actor=0;time=frame=0;owned=moving=sneaking=sprinting=crouchWalking=false;}
    void Update(bool allowed,uintptr_t player,uint64_t stamp,bool move,bool sneak,bool sprint,uint64_t update=0,bool physicalWalk=false){
        if(!allowed||!player||!stamp){Reset();return;}
        if(actor!=player||stamp<time||(time&&stamp-time>100))Reset();
        actor=player;time=stamp;frame=update;owned=owned||move||sneak;moving=move;
        sneaking=sneak;crouchWalking=physicalWalk;sprinting=sprint&&!sneak&&!physicalWalk;
    }
    bool Owns(uintptr_t player,uint64_t now,uint64_t update=0)const{return owned&&actor==player&&now>=time&&
        (frame?(frame==update):(now-time<=100));}
    // 0 means let the native action handler decide (including Sprint permission,
    // exhaustion and skills). Never request Fast directly.
    int Request(uintptr_t player,uint64_t now,int nativeWish,int nativeMode,uint64_t update=0)const{
        // Run/Sprint selection is only a ground-walking preference. Never
        // turn a native jump, fall, swim, slide, ragdoll or unknown mode into
        // Run. Released input must also leave native stopping untouched.
        if(!Owns(player,now,update)||!moving||sneaking||nativeWish<1||nativeWish>5||nativeMode<1||nativeMode>5)return 0;
        if(crouchWalking)return 3; // Native Walk, only for deliberate physical crouching.
        // A delayed native Walk poll cannot introduce a third movement mode. Leave
        // native Sprint5 and its refusal/exhaustion decisions untouched.
        if(sprinting)return nativeWish==3?4:0;
        return 4; // Sneak belongs to the native Down/OnPlayerSneak input path.
    }
};
}
