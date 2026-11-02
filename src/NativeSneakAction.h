#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
namespace diagnostic {
// Hash-verified Game.dll OnAction layout, also used by the native parry bridge.
// This is a logical SessionKey.Down event, with no keyboard scan code or binding.
struct NativeGameAction {
    uint16_t key=14,reserved=0;float value=0;uint32_t duration=0;uint8_t flags=0,padding[3]{};
};
static_assert(sizeof(NativeGameAction)==16&&offsetof(NativeGameAction,key)==0&&offsetof(NativeGameAction,duration)==8&&offsetof(NativeGameAction,flags)==12);
class NativeSneakAction {
    uintptr_t actor=0;uint64_t began=0,sampled=0;bool pressed=false,dispatching=false;
    NativeGameAction Message(bool down,uint64_t now)const{
        NativeGameAction out;out.value=down?1.f:0.f;out.flags=(down?1:0)|(pressed?2:0);
        out.duration=pressed&&now>=began?uint32_t(std::min(now-began,uint64_t(UINT32_MAX))):0;return out;
    }
public:
    void Abandon(){actor=0;began=sampled=0;pressed=false;}
    bool Owns(uintptr_t player,uint64_t now)const{return pressed&&actor==player&&now>=sampled&&now-sampled<=100;}
    template<class Send> void Update(uintptr_t player,bool allowed,bool wanted,uint64_t now,Send send){
        if(dispatching)return;
        struct DispatchScope {bool& flag;DispatchScope(bool& value):flag(value){flag=true;}~DispatchScope(){flag=false;}} scope(dispatching);
        if(actor&&(!allowed||player!=actor||now<sampled||(sampled&&now-sampled>100))){
            if(pressed)send(actor,Message(false,now));Abandon();
        }
        if(!allowed||!player)return;
        actor=player;sampled=now;if(wanted==pressed)return;
        if(send(actor,Message(wanted,now))){pressed=wanted;began=wanted?now:0;}
    }
    template<class Send> void Release(uint64_t now,Send send){Update(0,false,false,now,send);}
};
}
