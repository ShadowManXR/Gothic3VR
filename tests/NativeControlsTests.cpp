#include "InteractionControls.h"
#include "ControllerInput.h"
#include "NativeControllerModes.h"
#include "NativeSneakAction.h"
#include "StabDistance.h"
#include "SwingVelocity.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    combat::StabDistance stab;combat::SwingVelocity swing;uint64_t stamp=1000;
    for(int i=0;i<=40;++i,stamp+=10){melee::Blade blade{{0,0,i*.006f},{0,0,1+i*.006f}};
        stab.Step(blade,stamp,1,true,50);swing.Step(blade,stamp,1,true,250);}
    CHECK(stab.Ready(15)&&!swing.Ready(15)&&stab.Speed()>.59f&&stab.Speed()<.61f);
    stab.Reset();for(int i=0;i<=40;++i,stamp+=10){melee::Blade blade{{0,0,i*.002f},{0,0,1+i*.002f}};stab.Step(blade,stamp,2,true,50);CHECK(!stab.Ready(5));}
    puts("PASS short0.6m/s stab independent of2.5m/s swing gate, low-speed movement rejected");
    CHECK(DirectionalActivityControls(99,2,"PS_Interact","PS_Interact")&&DirectionalActivityControls(100,2,"PS_Player","PS_Player"));
    CHECK(DirectionalActivityControls(0,2,"TS_UseMob","PS_Interact")&&!DirectionalActivityControls(15,2,"PS_Player","PS_Player"));
    CHECK(!DirectionalActivityControls(0,2,"PS_Player","PS_Player"));
    CHECK(!NativeSneakState(0,2,"PS_Normal")&&!NativeSneakState(77,2,"PS_Player")&&NativeSneakState(0,3,"PS_Normal"));
    CHECK(!NativeSneakState(0,4,"PS_Melee_Sneak")&&NativeSneakState(0,3,"PS_Melee_Loop"));
    ControllerTranslator translator;ControllerInput input;input.Initialize(L"Z:\\no-options.ini");
    ControllerFrame frame;frame.focused=true;frame.timeMs=10000;for(auto& hand:frame.hands)hand.active=true;
    //Observed failure: the interaction is active but both GUI and Lockpick flags
    //are false. Do not discard directions as ordinary suppressed locomotion.
    translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true);
    frame.timeMs+=10;frame.hands[0].stick[0]=-.9f;auto commands=translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true);
    CHECK(commands.held[MenuLeft]&&!commands.held[Left]&&!commands.held[Forward]&&commands.moveX==0&&commands.moveZ==0&&!commands.pointer);
    int aDown=0,aUp=0,dDown=0,ctrlDown=0,ctrlUp=0;bool mouseReceived=false;
    auto emit=[&](unsigned type,unsigned key,bool down){mouseReceived|=type==1;if(type==0){if(key==30){if(down)++aDown;else ++aUp;}if(key==32&&down)++dDown;if(key==157||key==29){if(down)++ctrlDown;else ++ctrlUp;}}return true;};
    input.Apply(commands,emit);CHECK(aDown==1&&!mouseReceived);
    frame.timeMs+=10;frame.hands[1].trigger=.9f;commands=translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true);
    CHECK(commands.held[Use1]&&!commands.pointer&&!commands.held[UiClick]); //native activity exit/action remains available
    frame.hands[1].trigger=0;frame.timeMs+=80;commands=translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true);input.Apply(commands,emit);CHECK(aUp==1);
    frame.timeMs+=10;frame.hands[0].stick[0]=.9f;commands=translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true);input.Apply(commands,emit);CHECK(dDown==1);
    frame.timeMs+=10;frame.focused=false;input.Apply(translator.Translate(frame,frame.timeMs,true,false,2,1,2,0,true,false,true),emit);
    puts("PASS non-GUI native activity delivers A/D without movement, preserves trigger action and releases keys");
    wchar_t temp[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"gsc",0,path);
    WritePrivateProfileStringW(L"SessionKey.Down",L"Key1.Type",L"0",path);WritePrivateProfileStringW(L"SessionKey.Down",L"Key1.Offset",L"157",path);
    input.Initialize(path);CHECK(input.Binding(Sneak).type==0&&input.Binding(Sneak).offset==157);DeleteFileW(path);
    NativeControllerModes modes;GripSneak grip;NativeSneakAction nativeSneak;int presses=0,releases=0;
    auto logical=[&](uintptr_t actor,NativeGameAction const& message){if(actor!=1||message.key!=14)return false;if(message.flags&1)++presses;else ++releases;return true;};
    uint64_t now=20000;CHECK(!grip.Step(true,true,true,false,true,now));now+=120;
    bool requested=grip.Step(true,true,true,false,true,now);CHECK(requested);commands={};
    modes.Update(true,1,now,true,requested,false);CHECK(modes.Request(1,now,4,4)==0);nativeSneak.Update(1,true,requested,now,logical);input.Apply(commands,emit);CHECK(presses==1&&ctrlDown==0);
    for(int i=0;i<200;++i){now+=10;requested=grip.Step(true,true,i%2,false,true,now);
        modes.Update(true,1,now,i%2,requested,false);CHECK(requested&&modes.Request(1,now,4,4)==0);nativeSneak.Update(1,true,requested,now,logical);input.Apply(commands,emit);}CHECK(presses==1&&releases==0&&ctrlDown==0&&ctrlUp==0);
    // Releasing grip releases the native logical action, independent of Ctrl.
    now+=10;requested=grip.Step(true,false,true,false,true,now);nativeSneak.Update(1,true,requested,now,logical);input.Apply(commands,emit);modes.Update(true,1,now,true,requested,false);
    CHECK(!requested&&releases==1&&ctrlUp==0&&modes.Request(1,now,4,4)==4);
    CHECK(!grip.Step(true,true,true,true,true,now+10)&&!grip.Step(true,true,true,false,false,now+20));
    puts("PASS logical native Down action for sustained Sneak, no physical key, no mode override, release, support and skill gates");return 0;
}
