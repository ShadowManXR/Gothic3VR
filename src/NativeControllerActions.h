#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
namespace diagnostic {
// Exact Game.dll OnAction layout. The analog member at +16 is also read by
// native rotation actions, so provide the full message even for digital input.
struct NativeMovementAction {
    uint16_t key=0,reserved=0;float value=0;uint32_t duration=0;
    uint8_t flags=0,padding[3]{};float analog=0;
};
static_assert(sizeof(NativeMovementAction)==20&&offsetof(NativeMovementAction,duration)==8&&offsetof(NativeMovementAction,flags)==12&&offsetof(NativeMovementAction,analog)==16);
class NativeControllerActions {
    uintptr_t actor=0;uint64_t sampled=0,frame=0,started[5]{};bool valid=false,wanted[5]{},held[5]{};
public:
    void Reset(){actor=0;sampled=frame=0;valid=false;for(unsigned i=0;i<5;++i){wanted[i]=held[i]=false;started[i]=0;}}
    void Sample(uintptr_t player,uint64_t stamp,bool allowed,bool forward,bool backward,bool left,bool right,bool sprint,uint64_t update=0){
        if(actor!=player)Reset();actor=player;sampled=stamp;frame=update;valid=allowed&&player&&stamp;
        wanted[0]=forward&&!backward;wanted[1]=backward&&!forward;
        wanted[2]=left&&!right;wanted[3]=right&&!left;
        wanted[4]=sprint&&(wanted[0]||wanted[1]||wanted[2]||wanted[3]);
    }
    bool Fresh(uintptr_t player,uint64_t now,uint64_t update=0)const{return valid&&actor==player&&now>=sampled&&
        (frame?(update==frame):(now-sampled<=100));}
    bool OwnsPoll(uintptr_t player,uint64_t now,unsigned key,uint64_t update=0)const{
        if(!Fresh(player,now,update))return false;
        if(key==11)return wanted[4];
        return key>=1&&key<=4&&(wanted[0]||wanted[1]||wanted[2]||wanted[3]);
    }
    bool NeedsDirectionReset(uintptr_t player,uint64_t now,uint64_t update=0)const{
        if(!player||actor!=player)return false;
        // Include the final release/tracking-loss frame. ProcessMovements can
        // return early during native combat states without clearing old timers.
        for(unsigned i=0;i<4;++i)if(held[i]||(Fresh(player,now,update)&&wanted[i]))return true;
        return false;
    }
    unsigned DirectionMask(uintptr_t player,uint64_t now,uint64_t update=0)const{
        if(!Fresh(player,now,update))return 0;unsigned mask=0;for(unsigned i=0;i<4;++i)if(wanted[i])mask|=1u<<i;return mask;
    }
    // Run once at the native ProcessMovements boundary, inside the SAME camera
    // scope for all keys. Native ProcessMovements clears its per-key timers to
    // -1 every tick: no directional event means release. Sending a synthetic
    // release through OnAction would still add its direction to the accumulator!
    template<class Send> void Dispatch(uintptr_t player,uint64_t now,bool allowed,Send send,uint64_t update=0){
        if(!player||actor!=player)return;
        bool fresh=allowed&&Fresh(player,now,update);
        for(unsigned index:{4u,0u,1u,2u,3u}){
            bool down=fresh&&wanted[index];
            if(!down&&!held[index])continue;
            NativeMovementAction message;message.key=uint16_t(index==4?11:index+1);
            message.flags=uint8_t((down?1:0)|(held[index]?2:0));message.value=down?1.f:0.f;
            if(down&&!held[index])started[index]=now;
            message.duration=down?uint32_t(std::min(now-started[index],uint64_t(UINT32_MAX-1))):UINT32_MAX;
            if(down||index==4)send(message); // Sprint has native press/release state.
            held[index]=down;if(!down)started[index]=0;
        }
    }
};
}
