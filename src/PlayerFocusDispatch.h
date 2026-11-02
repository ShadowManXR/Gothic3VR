#pragma once
#include <cstdint>
namespace diagnostic {
// Global focus hooks also receive every NPC's targeting calls. Resolve the
// player's component once per application frame; only compare identities on
// subsequent calls. A cached component pointer is never dereferenced here.
class PlayerFocusDispatch {
    uintptr_t player=0,focus=0;uint64_t frame=0;bool valid=false;
public:
    void Reset(){player=focus=0;frame=0;valid=false;}
    template<class Resolve> bool Matches(uintptr_t candidate,uintptr_t owner,uint64_t serial,Resolve&& resolve){
        if(!owner){Reset();return false;}if(!candidate)return false;
        if(!valid||player!=owner||frame!=serial){auto next=uintptr_t(resolve());player=owner;focus=next;frame=serial;valid=true;}
        return focus&&candidate==focus;
    }
};
}
