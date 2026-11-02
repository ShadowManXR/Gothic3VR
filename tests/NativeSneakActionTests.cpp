#include "NativeSneakAction.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Received {uintptr_t actor;NativeGameAction message;};
struct NativeControl {uint16_t key=0;bool pressed=false,before=false;uint32_t duration=0;};
// Exercise the real x86 __thiscall/reference/int-return convention without
// loading game objects into this test process. Decode the native byte offsets.
int __fastcall OriginalOnActionFixture(NativeControl* self,void*,NativeGameAction const& message){
    auto* bytes=reinterpret_cast<unsigned char const*>(&message);
    memcpy(&self->key,bytes,2);memcpy(&self->duration,bytes+8,4);
    self->pressed=(bytes[12]&1)!=0;self->before=(bytes[12]&2)!=0;
    return self->pressed?77:4;
}
int main(){
    NativeSneakAction bridge;std::vector<Received> received;NativeControl control;
    using Original=int(__thiscall*)(NativeControl*,NativeGameAction const&);
    Original original=reinterpret_cast<Original>(&OriginalOnActionFixture);
    bool abi=true;auto send=[&](uintptr_t actor,NativeGameAction const& message){
        int result=original(&control,message);
        abi&=control.key==14&&control.pressed==bool(message.flags&1)&&control.before==bool(message.flags&2)&&control.duration==message.duration&&result==(control.pressed?77:4);
        received.push_back({actor,message});return true;
    };
    bridge.Update(1,false,true,1000,send);CHECK(received.empty());
    bridge.Update(0,true,true,1000,send);CHECK(received.empty());
    bridge.Update(1,true,true,1000,send);CHECK(received.size()==1&&abi&&received.back().message.flags==1&&received.back().message.duration==0&&received.back().message.value==1);
    CHECK(bridge.Owns(1,1000)&&bridge.Owns(1,1100)&&!bridge.Owns(2,1000)&&!bridge.Owns(1,1101)&&!bridge.Owns(1,999));
    for(uint64_t now=1010;now<=1800;now+=10)bridge.Update(1,true,true,now,send);
    CHECK(received.size()==1&&bridge.Owns(1,1800));
    bridge.Update(1,true,false,1810,send);CHECK(received.size()==2&&abi&&received.back().message.flags==2&&received.back().message.duration==810&&received.back().message.value==0&&!bridge.Owns(1,1810));
    bridge.Update(1,true,true,1820,send);bridge.Update(1,false,true,1830,send);
    CHECK(received.size()==4&&received.back().message.flags==2&&!bridge.Owns(1,1830));
    bridge.Update(1,true,true,1840,send);bridge.Update(2,true,true,1850,send);
    CHECK(received.size()==7&&received[5].actor==1&&received[5].message.flags==2&&received[6].actor==2&&received[6].message.flags==1&&bridge.Owns(2,1850));
    // A gap or regressing clock cannot keep an old hold active across sessions.
    bridge.Update(2,true,true,2100,send);CHECK(received.size()==9&&received[7].message.flags==2&&received[8].message.flags==1);
    bridge.Update(2,true,true,2090,send);CHECK(received.size()==11&&received.back().message.flags==1);
    bridge.Release(2100,send);CHECK(received.size()==12&&received.back().message.flags==2&&!bridge.Owns(2,2100));
    bridge.Release(2110,send);CHECK(received.size()==12);
    int failed=0;auto refuse=[&](uintptr_t,NativeGameAction const&){++failed;return false;};
    bridge.Update(3,true,true,2200,refuse);CHECK(failed==1&&!bridge.Owns(3,2200));
    bridge.Update(3,true,true,2210,send);CHECK(bridge.Owns(3,2210));bridge.Release(2220,refuse);CHECK(!bridge.Owns(3,2220));
    // Original native dispatch may re-enter an application callback. No duplicate
    // synthetic press is allowed while that handler is still running.
    int recursion=0;auto reenter=[&](uintptr_t actor,NativeGameAction const& message){++recursion;bridge.Update(actor,true,true,2300,send);return send(actor,message);};
    auto count=received.size();bridge.Update(4,true,true,2300,reenter);CHECK(recursion==1&&received.size()==count+1&&bridge.Owns(4,2300));
    bridge.Release(2310,send);CHECK(abi);
    puts("PASS logical Down14 native x86 ABI, independent of keyboard bindings, sustained hold, release, stale context, actor replacement and re-entry");return 0;
}
