#pragma once
#include <algorithm>
#include <cstdint>
namespace diagnostic::pup {
// PUP's optional queue maintenance already yields between complete items.
// Share its allowance across both VR eyes and nested render passes. No native
// queue/item/entity pointer is stored here, and an item is never interrupted.
class PrefetchBudget {
    uint64_t frequency,frame=0,used=0,started=0;bool valid=false,active=false;
public:
    static constexpr unsigned BudgetMs=2,NativeBudgetMs=5;
    explicit PrefetchBudget(uint64_t hz):frequency(hz){}
    void Reset(){frame=used=started=0;valid=active=false;}
    unsigned RemainingMs()const{
        if(!frequency)return 0;
        auto elapsedMs=(used*1000+frequency-1)/frequency;
        return elapsedMs>=BudgetMs?0:BudgetMs-unsigned(elapsedMs);
    }
    bool Admit(uint64_t nextFrame,uint64_t now,bool rightEye=false){
        // Publishing another optional item after the left image was rendered
        // can change the resident scene only for the right eye. Leave that
        // queue work intact for the next pair; forced loading bypasses this
        // policy in the caller, as do non-world and non-stereo calls.
        if(rightEye||active)return false;
        if(!valid||frame!=nextFrame||now<started){valid=true;frame=nextFrame;used=0;}
        return RemainingMs()!=0;
    }
    uint32_t Start(uint64_t now,uint32_t nativeStarted){
        auto remaining=RemainingMs();active=true;started=now;
        // Preserve time already spent by the native caller, including modulo
        // 32-bit timer wrap. PUP's own post-item check remains authoritative.
        return nativeStarted-(NativeBudgetMs-(std::min)(NativeBudgetMs,remaining));
    }
    void Finish(uint64_t now){
        if(!active)return;
        if(now>=started)used+=(std::min)(now-started,frequency*60);else used=0;
        active=false;
    }
};
inline constexpr unsigned WorkRva=0x111c0,QueueLockRva=0x994e0;
inline constexpr unsigned char WorkPrefix[]={0x55,0x8b,0xec,0x83,0xec,0x20};
inline constexpr unsigned char FileHash[]={0x97,0x76,0x0f,0xfc,0xd3,0xee,0xf0,0x9b,0x7d,0xbc,0xd8,0xef,0x92,0x5b,0x99,0x95,0xa1,0x98,0xcb,0x8b,0xba,0x96,0x95,0x12,0x98,0x96,0x9c,0x48,0xa8,0x4c,0xd8,0xf0};
}
