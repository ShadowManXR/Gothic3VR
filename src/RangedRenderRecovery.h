#pragma once
#include <cstdint>
namespace diagnostic { namespace ranged {
// A native cache hit is a valid render source, not a request to restore the
// carry pose. The actual Apply routines validate owner and matrix buffers.
inline bool ApplyTrackedFrame(bool ours,bool nativeUpdated,bool crossbow){return ours&&(nativeUpdated||crossbow);}
inline bool KeepCrossbowNock(bool gameplay,bool isCrossbow,bool samePlayer,bool sameSlot,bool bolt){return gameplay&&isCrossbow&&samePlayer&&sameSlot&&bolt;}
struct Identity {
    uintptr_t player=0,weapon=0;uint64_t session=0;
    bool operator==(Identity const& other)const{return player==other.player&&weapon==other.weapon&&session==other.session;}
    bool Valid()const{return player&&weapon&&player!=weapon;}
};
// An equipment/load transition may temporarily expose an incomplete actor.
// Retry the same binding at a bounded rate; a new weapon/player/session must
// never inherit another binding's failure. This affects render state only.
class Retry {
    Identity identity;uint64_t lastFailure=0;bool failed=false;
public:
    static constexpr uint64_t DelayMs=500;
    bool Observe(Identity key,uint64_t now){
        if(!key.Valid()){Reset();return false;}
        if(!(key==identity)){identity=key;failed=false;}
        return !failed||now<lastFailure||now-lastFailure>=DelayMs;
    }
    void Fail(uint64_t now){lastFailure=now;failed=true;}
    void Success(){failed=false;}
    void Reset(){identity={};failed=false;lastFailure=0;}
};
struct PaletteOwner {
    uintptr_t factory=0,wrapper=0,actor=0,nodes=0;unsigned count=0;
    bool Same(PaletteOwner const& other)const{
        return factory&&wrapper&&actor&&nodes&&count&&factory==other.factory&&wrapper==other.wrapper&&
            actor==other.actor&&nodes==other.nodes&&count==other.count;
    }
};
}}
