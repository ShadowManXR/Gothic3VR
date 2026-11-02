#include "FinisherRoiHint.h"
#include "VirtualHook.h"
#include "CombatRefinement.h"
#include "FramePerformance.h"
#include <array>
#include <cstdio>
using namespace diagnostic::combat;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    FinisherRoiHint hint;std::array<RoiIdentity,4> live{{{10,100},{20,200},{30,300},{40,400}}};unsigned reads=0,checks=0;
    auto read=[&](size_t i){++reads;return live[i];};bool eligible=true;
    auto validate=[&](uintptr_t value){++checks;return eligible&&value==300;};
    hint.Context(1,2,3);hint.Remember(2,live[2]);
    for(unsigned i=0;i<1000;++i)CHECK(hint.Reuse(live.size(),read,validate)==300);
    CHECK(reads==1000&&checks==1000&&hint.reuses==1000); // one current entry, never a full scan
    live[2]={50,500};CHECK(!hint.Reuse(live.size(),read,validate)&&!hint.HasSelection()&&checks==1000);
    hint.Remember(2,{30,300});CHECK(!hint.Reuse(2,read,validate)&&reads==1001);
    live[2]={30,300};hint.Remember(2,live[2]);eligible=false;CHECK(!hint.Reuse(4,read,validate)&&!hint.HasSelection());
    eligible=true;hint.Remember(2,live[2]);hint.Context(1,9,3);CHECK(!hint.HasSelection());
    hint.Remember(2,live[2]);hint.Context(9,9,3);CHECK(!hint.HasSelection());
    hint.Remember(2,live[2]);hint.Context(9,9,4);CHECK(!hint.HasSelection());
    hint.Remember(2,live[2]);hint.Reset();CHECK(!hint.HasSelection()&&!hint.reuses);
    auto* page=static_cast<unsigned*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(page);*page=17;
    CHECK(ReadOwnedRoiEntry([&](){return *page==17;}));DWORD previous=0;
    CHECK(VirtualProtect(page,4096,PAGE_NOACCESS,&previous));
    CHECK(!ReadOwnedRoiEntry([&](){return *static_cast<volatile unsigned*>(page)==17;}));
    CHECK(VirtualFree(page,0,MEM_RELEASE));
    CHECK(FinisherWithinReach(0,0,0,180,120,0,100));
    CHECK(!FinisherWithinReach(0,0,0,180.01f,0,0,100)&&!FinisherWithinReach(0,0,0,0,120.01f,0,100));
    CHECK(!FinisherWithinReach(0,0,0,NAN,0,0,100)&&!FinisherWithinReach(0,0,0,0,0,0,0));
    volatile unsigned datum=7;constexpr unsigned iterations=100000;unsigned accepted=0;
    auto start=diagnostic::performance::Now();
    for(unsigned i=0;i<iterations;++i)if(diagnostic::Readable(const_cast<unsigned*>(&datum),sizeof(datum))&&datum==7)++accepted;
    auto queryTicks=diagnostic::performance::Now()-start;CHECK(accepted==iterations);accepted=0;start=diagnostic::performance::Now();
    for(unsigned i=0;i<iterations;++i)if(ReadOwnedRoiEntry([&](){return datum==7;}))++accepted;
    auto guardedTicks=diagnostic::performance::Now()-start;CHECK(accepted==iterations);
    printf("PASS live-index resolution, reordering/removal/context invalidation, changed eligibility/LOS, guarded unreadable page and native reach boundaries; %u reads VirtualQuery=%.3fms guarded=%.3fms (not game FPS)\n",iterations,double(queryTicks)*1000/diagnostic::performance::Frequency(),double(guardedTicks)*1000/diagnostic::performance::Frequency());return 0;
}
