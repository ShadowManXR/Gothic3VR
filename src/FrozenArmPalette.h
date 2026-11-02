#pragma once
#include "TrackedPalette.h"
namespace diagnostic {
inline bool FreezeTrackedHero(bool stableBody,bool primary,bool secondary,bool shield){return stableBody||primary||secondary||shield;}
class FrozenArmPalette {
    arms::Mat matrices[TrackedPalette::Limit];uintptr_t player=0,factory=0;unsigned count=0;
public:
    void Reset(){player=factory=0;count=0;}
    bool Capture(uintptr_t actor,uintptr_t owner,arms::Mat const* source,unsigned n){
        Reset();if(!actor||!owner||!source||!n||n>TrackedPalette::Limit)return false;
        for(unsigned i=0;i<n;++i)if(!arms::Affine(source[i]))return false;
        memcpy(matrices,source,n*sizeof(*source));player=actor;factory=owner;count=n;return true;
    }
    bool Matches(uintptr_t actor,uintptr_t owner,unsigned n)const{return count&&actor==player&&owner==factory&&n==count;}
    bool Apply(uintptr_t actor,uintptr_t owner,arms::Mat* destination,unsigned n)const{
        if(!destination||!Matches(actor,owner,n))return false;memcpy(destination,matrices,n*sizeof(*destination));return true;
    }
};
}
