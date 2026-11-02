#include "NativeHeroVoice.h"
#include "FocusHook.h"
#include <cstdio>
#include <cstring>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct Channel {
    uint32_t mode=0x00010112;float pan=.35f;
    unsigned reads=0,writes=0;int failRead=0,failMode=0,failPan=0;
};
int __stdcall GetMode(void* p,uint32_t* mode){auto& c=*static_cast<Channel*>(p);++c.reads;if(c.failRead)return 1;*mode=c.mode;return 0;}
int __stdcall SetMode(void* p,uint32_t mode){auto& c=*static_cast<Channel*>(p);++c.writes;if(c.failMode){--c.failMode;return 1;}c.mode=mode;return 0;}
int __stdcall SetPan(void* p,float pan){auto& c=*static_cast<Channel*>(p);++c.writes;if(c.failPan){--c.failPan;return 1;}c.pan=pan;return 0;}
FocusHook startHook,updateHook;unsigned startCalls=0,updateCalls=0;void* expectedSelf=nullptr;void* expectedText=nullptr;bool argumentsOK=true;
using Start=bool(__thiscall*)(void*,void*);
using Update=void(__thiscall*)(void*);
bool __fastcall StartReplacement(void* self,void*,void* text){
    ++startCalls;argumentsOK=argumentsOK&&self==expectedSelf&&text==expectedText;
    return reinterpret_cast<Start>(startHook.original)(self,text);
}
void __fastcall UpdateReplacement(void* self,void*){
    ++updateCalls;argumentsOK=argumentsOK&&self==expectedSelf;
    reinterpret_cast<Update>(updateHook.original)(self);
}
int __cdecl main(){
    NativeVoiceApi api{&GetMode,&SetMode,&SetPan};HeroVoiceChannel voice;Channel c;uintptr_t handle=reinterpret_cast<uintptr_t>(&c);
    auto originalMode=c.mode;auto originalPan=c.pan;
    CHECK(voice.Update(1,handle,false,api)==0&&voice.Empty()&&c.reads+c.writes==0);
    CHECK(voice.Update(1,handle,true,api)==1&&!voice.Empty()&&voice.Tracks(1)&&!voice.Tracks(2));
    CHECK(c.mode==((originalMode&~Voice3D)|Voice2D)&&c.pan==0&&c.reads==1&&c.writes==2);
    for(int i=0;i<100000;++i)CHECK(voice.Update(1,handle,true,api)==0);
    CHECK(c.reads==1&&c.writes==2); // no per-frame FMOD queries or mutations
    CHECK(voice.Update(1,handle,false,api)==2&&voice.Empty()&&c.mode==originalMode&&c.pan==0);
    CHECK(voice.Update(1,handle,false,api)==0&&c.writes==3);
    // Native StartOutput can reuse the same channel address for a new clip.
    c.mode=0x20211;c.pan=-.7f;CHECK(voice.Update(1,handle,true,api)==1);
    CHECK(voice.Update(1,handle,false,api)==2&&c.mode==0x20211&&c.pan==0);
    c.mode=0x400012;c.pan=.1f;CHECK(voice.Update(1,handle,true,api)==1);
    CHECK(voice.Update(1,handle,false,api)==2&&c.mode==0x400012&&c.pan==0);
    // Ended/replaced handles are only identity tokens, never dereferenced.
    CHECK(voice.Update(1,handle,true,api)==1);unsigned before=c.reads+c.writes;
    CHECK(voice.Update(1,0,false,api)==0&&voice.Empty()&&c.reads+c.writes==before);
    c.mode=originalMode;c.pan=originalPan;CHECK(voice.Update(1,handle,true,api)==1);before=c.reads+c.writes;
    Channel other;CHECK(voice.Update(2,reinterpret_cast<uintptr_t>(&other),false,api)==0&&voice.Empty());
    CHECK(c.reads+c.writes==before&&other.reads+other.writes==0);
    c.mode=originalMode; // Native playback initializes a reused channel anew.
    CHECK(voice.Update(1,handle,true,api)==1);before=c.reads+c.writes;voice.Forget();CHECK(c.reads+c.writes==before&&voice.Empty());
    c.mode=originalMode;c.pan=originalPan;c.failRead=1;CHECK(voice.Update(1,handle,true,api)==-1&&voice.Empty()&&c.mode==originalMode);c.failRead=0;
    c.failMode=1;CHECK(voice.Update(1,handle,true,api)==-1&&voice.Empty()&&c.mode==originalMode);
    c.failPan=1;CHECK(voice.Update(1,handle,true,api)==-1&&voice.Empty()&&c.mode==originalMode&&c.pan==originalPan);
    // PUP's pan getter must never be required: NativeVoiceApi has no such
    // callback. Only mode flags are restored for originally-3D speech.
    Channel already2D;already2D.mode=Voice2D|2;already2D.pan=-.5f;
    CHECK(voice.Update(1,reinterpret_cast<uintptr_t>(&already2D),true,api)==0&&voice.Empty());
    CHECK(already2D.reads==1&&already2D.writes==0&&already2D.pan==-.5f&&already2D.mode==(Voice2D|2));
    // Execute fixtures with the exact native prologues. Check this/reference
    // arguments, original results and x86 trampoline stack cleanup.
    unsigned char startCode[]{0x83,0xec,0x0c,0x53,0x8b,0xd9,0x8b,0x44,0x24,0x14,0x89,0x43,0x04,0xb0,0x01,0x5b,0x83,0xc4,0x0c,0xc2,0x04,0x00};
    unsigned char updateCode[]{0x56,0x8b,0xf1,0x83,0x7e,0x6c,0x00,0xff,0x46,0x08,0x5e,0xc3};
    auto page=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(page);
    memcpy(page,startCode,sizeof(startCode));memcpy(page+128,updateCode,sizeof(updateCode));DWORD old=0;CHECK(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),page,4096);
    CHECK(startHook.Install(page,reinterpret_cast<void*>(&StartReplacement),VoiceStartPrefix,sizeof(VoiceStartPrefix)));
    CHECK(updateHook.Install(page+128,reinterpret_cast<void*>(&UpdateReplacement),VoiceUpdatePrefix,sizeof(VoiceUpdatePrefix)));
    alignas(16) uintptr_t unit[32]{};expectedSelf=unit;expectedText=reinterpret_cast<void*>(0x12345678);
    for(unsigned i=0;i<10000;++i){CHECK(reinterpret_cast<Start>(page)(unit,expectedText));reinterpret_cast<Update>(page+128)(unit);}
    CHECK(argumentsOK&&startCalls==10000&&updateCalls==10000&&unit[1]==reinterpret_cast<uintptr_t>(expectedText)&&unit[2]==10000);
    CHECK(VirtualFree(page,0,MEM_RELEASE));
    puts("PASS hero speech centering, native loop/stream flags, same-handle new clips, borrowed lifetime, failure rollback, already-2D untouched, no pan getter, no steady-state FMOD calls and both x86 hook ABIs");return 0;
}
