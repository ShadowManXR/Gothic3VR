#include "PickupPause.h"
#include "VirtualHook.h"
#include <cstdio>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using Instruction=bool(__stdcall*)(void*,void*,bool);
VirtualHook hook;unsigned calls=0;void* expectedArgs=nullptr;void* expectedSpu=nullptr;bool expectedStop=false,forwarded=true;
bool __stdcall Native(void* args,void* spu,bool stop){
    ++calls;forwarded&=args==expectedArgs&&spu==expectedSpu&&stop==expectedStop;return stop;
}
bool __stdcall Replacement(void* args,void* spu,bool stop){
    return reinterpret_cast<Instruction>(hook.original)(args,spu,stop);
}
int main(){
    for(auto kind:{PickupPause::Wait,PickupPause::Animation}){
        uintptr_t sites[]{0x82348,0x4aef3,0x4bb93};
        for(auto site:sites){bool target=PickupPauseSite(site,kind);
            CHECK(SkipPickupPause(kind,site,true,true,false,1,1,1)==target);
            CHECK(!SkipPickupPause(kind,site,false,true,false,1,1,1));
            CHECK(!SkipPickupPause(kind,site,true,false,false,1,1,1));
            CHECK(!SkipPickupPause(kind,site,true,true,true,1,1,1));
            CHECK(!SkipPickupPause(kind,site,true,true,false,2,1,2));
            CHECK(!SkipPickupPause(kind,site,true,true,false,1,1,2));
            CHECK(!SkipPickupPause(kind,site,true,true,false,0,0,0));
            CHECK(!SkipPickupPause(kind,site+1,true,true,false,1,1,1));}
        // Chests, mining, crafting, combat, waits and instruction continuation
        // from every other call site always retain their native callback.
        for(uintptr_t site:{uintptr_t(0),uintptr_t(0x4c1a2),uintptr_t(0x4c405),uintptr_t(0x4c765)})
            CHECK(!SkipPickupPause(kind,site,true,true,false,1,1,1));
    }
    CHECK(!SkipPickupPause(PickupPause::Wait,0x82348,true,true,false,1,1,1,501));
    CHECK(!SkipPickupPause(PickupPause::Wait,0x82348,true,true,false,1,1,1,0));
    void* slot=reinterpret_cast<void*>(&Native);
    CHECK(hook.Prepare(&slot,0,reinterpret_cast<void*>(&Replacement))&&hook.Install());
    expectedArgs=reinterpret_cast<void*>(0x12345678);expectedSpu=reinterpret_cast<void*>(0x87654321);
    for(unsigned i=0;i<10000;++i){expectedStop=(i&1)!=0;CHECK(reinterpret_cast<Instruction>(slot)(expectedArgs,expectedSpu,expectedStop)==expectedStop);}
    CHECK(calls==10000&&forwarded&&hook.Restore()&&slot==reinterpret_cast<void*>(&Native));
    puts("PASS only fresh VR hero pickup pauses are bypassed; continuation/cancellation/NPC/unrelated instructions and stdcall forwarding are preserved");return 0;
}
