#pragma once
#include <cmath>
#include <cstdint>
namespace diagnostic {
// Movement permission only. Does not move/recenter the actor or HMD, select
// native Sneak, change collision, or rewrite the controller's direction.
class PhysicalCrouchWalk {
    uintptr_t actor=0;uint64_t session=0,last=0,candidate=0;float reference=0;bool walking=false;
public:
    static constexpr float EnterMeters=.30f,ExitMeters=.18f;
    static constexpr uint64_t DwellMs=120;
    void Reset(){actor=0;session=last=candidate=0;reference=0;walking=false;}
    bool Hold(uintptr_t player,uint64_t tracking,uint64_t now,bool allowed,float standingY){
        if(!allowed||actor!=player||session!=tracking||!last||now<last||now-last>100||!std::isfinite(standingY)||std::fabs(reference-standingY)>.01f){Reset();return false;}
        candidate=0;return walking; // A short cache miss must not create a Run burst.
    }
    bool Update(uintptr_t player,uint64_t tracking,uint64_t now,bool allowed,float standingY,float headY){
        if(!allowed||!player||!tracking||!now||!std::isfinite(standingY)||!std::isfinite(headY)){Reset();return false;}
        if(actor!=player||session!=tracking||now<last||(last&&now-last>250)||std::fabs(reference-standingY)>.01f){Reset();actor=player;session=tracking;reference=standingY;}
        last=now;float depth=standingY-headY;
        bool change=walking?depth<=ExitMeters:depth>=EnterMeters;
        if(!change)candidate=0;
        else{if(!candidate)candidate=now;if(now-candidate>=DwellMs){walking=!walking;candidate=0;}}
        return walking;
    }
    bool Walking()const{return walking;}
};
}
