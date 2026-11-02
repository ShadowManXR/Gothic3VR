#pragma once
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <utility>
namespace diagnostic::combat {
// Read the game's currently owned ROI synchronously on the application
// thread. An invalid entry is rejected at the actual access; avoid a kernel
// VirtualQuery for every pointer in an otherwise valid native collection.
template<class Read> bool ReadOwnedRoiEntry(Read const& read) noexcept {
    __try {return read();}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct RoiIdentity {uintptr_t interaction=0,entity=0;};
class FinisherRoiHint {
    uintptr_t player=0,weapon=0;uint64_t session=0;
    size_t index=0;RoiIdentity identity{};
public:
    uint64_t reuses=0;
    void Clear(){index=0;identity={};}
    void Reset(){Clear();player=weapon=0;session=reuses=0;}
    void Context(uintptr_t owner,uintptr_t item,uint64_t origin){
        if(!owner||!item){Reset();return;}
        if(player!=owner||weapon!=item||session!=origin){Clear();player=owner;weapon=item;session=origin;}
    }
    bool HasSelection()const{return identity.interaction&&identity.entity;}
    void Remember(size_t entry,RoiIdentity current){if(!current.interaction||!current.entity){Clear();return;}index=entry;identity=current;}
    // Only integers survive between calls. Resolve through the live native
    // collection again; stored pointer identities are never dereferenced.
    template<class Read,class Validate> uintptr_t Reuse(size_t count,Read&& read,Validate&& validate){
        if(!HasSelection()||index>=count){Clear();return 0;}
        auto current=read(index);
        if(current.interaction!=identity.interaction||current.entity!=identity.entity||!validate(current.entity)){Clear();return 0;}
        ++reuses;return current.entity;
    }
};
}
