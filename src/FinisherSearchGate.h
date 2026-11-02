#pragma once
#include <cstdint>
namespace diagnostic::combat {
// Full ROI scans have an 80ms cadence. A live-ROI hint revalidates positive
// focus every frame; an execution press or lost hint requests an immediate scan.
class FinisherSearchGate {
    uintptr_t player=0,weapon=0;uint64_t session=0,previous=0,next=0;bool chord=false;
public:
    static constexpr uint64_t MissIntervalMs=80;
    uint64_t searches=0,skipped=0;
    void Reset(){player=weapon=0;session=previous=next=0;searches=skipped=0;chord=false;}
    bool Pressed(bool down){bool edge=down&&!chord;chord=down;return edge;}
    bool Due(uintptr_t owner,uintptr_t item,uint64_t origin,uint64_t now,bool urgent){
        if(!owner||!item){Reset();return false;}
        if(player!=owner||weapon!=item||session!=origin||now<previous){next=0;player=owner;weapon=item;session=origin;}
        previous=now;
        if(!urgent&&next&&now<next){++skipped;return false;}
        ++searches;return true;
    }
    void Complete(bool,uint64_t now){next=now+MissIntervalMs;}
};
}
