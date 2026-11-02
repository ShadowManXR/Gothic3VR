#pragma once
#include <cstdint>
namespace diagnostic {
inline bool ReturnedToDifferentHuman(uintptr_t animal,uintptr_t current,bool currentIsAnimal){return animal&&current&&animal!=current&&!currentIsAnimal;}
// Consume X/Y until released so returning human cannot leak a weapon/escape
// press. A held chord fires once; lost focus, stale tracking and menus cancel.
class AnimalReturn {
    uint64_t started=0,session=0,time=0;uintptr_t actor=0;bool timing=false,fired=false,captured=false;
public:
    bool Captured()const{return captured;}
    bool Step(bool allowed,bool animal,uintptr_t player,uint64_t runtimeSession,uint64_t stamp,bool x,bool y){
        if(!x&&!y){timing=fired=captured=false;actor=0;session=runtimeSession;time=stamp;return false;}
        if(!allowed){timing=false;return false;}
        if((actor&&actor!=player)||session!=runtimeSession||stamp<time||stamp-time>100){timing=false;fired=false;}
        actor=player;session=runtimeSession;time=stamp;
        if(animal&&x&&y)captured=true;
        if(!animal||!x||!y){timing=false;return false;}
        if(fired)return false;
        if(!timing){started=stamp;timing=true;}
        if(stamp-started<500)return false;
        fired=true;return true;
    }
};
}
