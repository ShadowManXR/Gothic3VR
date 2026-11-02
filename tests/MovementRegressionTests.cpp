#include "NativeControllerActions.h"
#include "NativeMovementInput.h"
#include "NativeDirectionGoal.h"
#include "ControllerInput.h"
#include "InteractionTargetBox.h"
#include <vector>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    NativeControllerActions actions;ControllerTranslator translator;ControllerInput keyboard;
    wchar_t temp[MAX_PATH],options[MAX_PATH];GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"gma",0,options);
    wchar_t const* names[]={L"SessionKey.Forward",L"SessionKey.Backward",L"SessionKey.StrafeLeft",L"SessionKey.StrafeRight",L"SessionKey.Walk"};
    wchar_t const* scans[]={L"17",L"31",L"30",L"32",L"54"};
    for(unsigned i=0;i<5;++i){WritePrivateProfileStringW(names[i],L"Key1.Type",L"0",options);WritePrivateProfileStringW(names[i],L"Key1.Offset",scans[i],options);}
    keyboard.Initialize(options);DeleteFileW(options);
    ControllerFrame frame;frame.focused=true;for(auto& hand:frame.hands)hand.active=true;
    uint64_t now=1000;std::vector<NativeMovementAction> messages;
    unsigned windowsMovement=0;bool nativeSprint=false,nativeInvalid=false;float x=0,z=0;
    // Emulate verified native per-frame timer consumption, including OnAction
    // adding directions irrespective of flags. A direction release would be a
    // spurious extra step; a missing clear would continue for all neutral ticks.
    auto nativeTick=[&](bool allowed=true){
        x=z=0;messages.clear();
        actions.Dispatch(1,now,allowed,[&](NativeMovementAction const& m){
            messages.push_back(m);
            if(m.key<=4){if(m.duration==UINT32_MAX)nativeInvalid=true;if(m.key==1)z+=1;if(m.key==2)z-=1;if(m.key==3)x-=1;if(m.key==4)x+=1;}
            else if(m.key==11)nativeSprint=(m.flags&1)!=0;
            return 0;
        });
        return 0;
    };
    auto step=[&](float axisX,float axisZ){
        now+=10;frame.timeMs=now;frame.hands[0].stick[0]=axisX;frame.hands[0].stick[1]=axisZ;
        auto commands=translator.Translate(frame,now,true,false,2,1,2,0);
        actions.Sample(1,now,true,commands.held[Forward],commands.held[Backward],commands.held[Left],commands.held[Right],commands.held[Sprint]);
        ControllerInput::NativeMovementOnly(commands);
        keyboard.Apply(commands,[&](unsigned type,unsigned offset,bool){if(type==0&&(offset==0xc8||offset==0xd0||offset==0xcb||offset==0xcd||offset==17||offset==31||offset==30||offset==32||offset==42||offset==54))++windowsMovement;return true;});
        nativeTick();
    };
    step(0,0);step(0,1);CHECK(x==0&&z==1&&messages.size()==1&&messages[0].flags==1);
    CHECK(actions.OwnsPoll(1,now,1)&&actions.OwnsPoll(1,now,2)&&actions.OwnsPoll(1,now,3)&&actions.OwnsPoll(1,now,4)&&!actions.OwnsPoll(1,now,11));
    for(unsigned i=0;i<200;++i){step(.12f,1);CHECK(x==0&&z==1&&messages.size()==1&&messages[0].flags==3);}
    step(1,0);CHECK(x==1&&z==0&&messages.size()==1); // old forward is absent in this very tick
    step(0,-1);CHECK(x==0&&z==-1&&messages.size()==1);
    step(-1,0);CHECK(x==-1&&z==0&&messages.size()==1);
    step(-1,1);CHECK(x==-1&&z==1&&messages.size()==2);
    for(unsigned i=0;i<300;++i){step(0,0);CHECK(x==0&&z==0&&messages.empty());}
    CHECK(windowsMovement==0&&!nativeInvalid);
    CHECK(!actions.OwnsPoll(1,now,1)); // hardware keyboard is preserved at neutral
    actions.Sample(1,now,true,true,false,false,false,true);nativeTick();CHECK(nativeSprint&&z==1);
    CHECK(actions.OwnsPoll(1,now,11)&&!actions.OwnsPoll(1,now,14)); // released Shift polls cannot cancel Sprint; Sneak independent
    now+=101;nativeTick();CHECK(!nativeSprint&&x==0&&z==0&&messages.size()==1&&messages[0].key==11&&messages[0].flags==2&&messages[0].duration==UINT32_MAX);
    now+=10;nativeTick();CHECK(messages.empty());
    actions.Sample(1,now,true,true,false,false,false,true);nativeTick();CHECK(nativeSprint);
    nativeTick(false);CHECK(!nativeSprint&&x==0&&z==0);
    actions.Sample(1,now,true,true,true,true,true,true);nativeTick();CHECK(messages.empty()); // opposite directions cannot leak
    actions.Sample(2,now,true,true,false,false,false,false);nativeTick();CHECK(messages.empty()); // old player ignored
    actions.Reset();actions.Dispatch(2,now,true,[&](auto const&){++windowsMovement;});CHECK(windowsMovement==0&&!nativeInvalid);

    // A long native update must not hand joystick ownership back to an
    // unrelated keyboard/script poll in the middle of the same input frame.
    actions.Sample(1,3000,true,true,false,false,false,true,77);
    CHECK(actions.Fresh(1,3400,77)&&actions.OwnsPoll(1,3400,2,77)&&actions.OwnsPoll(1,3400,11,77));
    CHECK(!actions.Fresh(1,3400,78)&&!actions.Fresh(1,2999,77));
    messages.clear();actions.Dispatch(1,3400,true,[&](auto const& m){messages.push_back(m);},77);
    CHECK(messages.size()==2&&messages[0].key==11&&messages[1].key==1);
    messages.clear();actions.Dispatch(1,3410,true,[&](auto const& m){messages.push_back(m);},78);
    CHECK(messages.size()==1&&messages[0].key==11&&messages[0].value==0); // no fresh sample => release, not a cached step
    actions.Sample(1,3450,true,false,true,false,false,false,80);
    actions.Dispatch(1,3450,true,[](auto const&){},80);
    actions.Sample(1,3460,true,false,false,false,false,false,81);
    CHECK(actions.NeedsDirectionReset(1,3460,81)&&!actions.OwnsPoll(1,3460,1,81)); // final native timer clear, keyboard resumes
    actions.Dispatch(1,3460,true,[](auto const&){},81);CHECK(!actions.NeedsDirectionReset(1,3460,81));
    actions.Sample(1,3470,true,true,false,false,false,false,82);actions.Dispatch(1,3470,true,[](auto const&){},82);
    CHECK(actions.NeedsDirectionReset(1,3600,83)); // stale tracking must also remove held direction
    actions.Dispatch(1,3600,false,[](auto const&){},83);CHECK(!actions.NeedsDirectionReset(1,3600,83));

    // Seed actual CP input offsets with old Back/StrafeRight timers. These
    // timers, unlike the wished-vector accumulator, choose the native goal.
    unsigned char control[0xb0],before[0xb0];std::memset(control,0x5a,sizeof(control));std::memcpy(before,control,sizeof(control));
    CHECK(!ClearControllerDirectionFrame(control,0xaf));CHECK(!std::memcmp(before,control,sizeof(control)));
    CHECK(ClearControllerDirectionFrame(control,sizeof(control)));
    for(unsigned i=0;i<sizeof(control);++i){bool timer=i>=0x3c&&i<0x4c,vector=i>=0x60&&i<0x6c;
        CHECK(control[i]==(timer?0xff:vector?0:before[i]));}
    // Current Forward then runs through original OnAction. No old Backward
    // timer remains to cancel it, or side timer to turn it into a diagonal.
    actions.Sample(1,3500,true,true,false,false,false,false,79);
    actions.Dispatch(1,3500,true,[&](auto const& m){if(m.key<=4){constexpr unsigned offsets[]{0x3c,0x48,0x40,0x44};std::memcpy(control+offsets[m.key-1],&m.duration,4);}},79);
    uint32_t durations[4]{};std::memcpy(durations,control+0x3c,sizeof(durations));
    CHECK(durations[0]!=UINT32_MAX&&durations[1]==UINT32_MAX&&durations[2]==UINT32_MAX&&durations[3]==UINT32_MAX);

    ControllerReferenceLease reference;int restore=-1;
    CHECK(!reference.Begin(false,1)&&!reference.Begin(true,0));
    for(float yaw:{0.f,1.2f,3.14159265f,-2.8f}){
        // Native target faces opposite the headset. Controller forward must
        // use the visible view even with target-relative combat configured.
        int frameOfReference=1;CHECK(reference.Begin(true,frameOfReference));frameOfReference=0;
        float viewX=std::sin(yaw),viewZ=std::cos(yaw),targetX=-viewX,targetZ=-viewZ;
        float stepX=frameOfReference?targetX:viewX,stepZ=frameOfReference?targetZ:viewZ;
        CHECK(stepX*viewX+stepZ*viewZ>.999f);CHECK(reference.End(frameOfReference,restore)&&restore==1);
        frameOfReference=restore;CHECK(frameOfReference==1&&!reference.End(0,restore));
    }
    CHECK(reference.Begin(true,1)&&!reference.End(1,restore)); // a native reference change takes priority
    ControllerReferenceLease outer,inner;int referenceMode=1;
    CHECK(outer.Begin(true,referenceMode));referenceMode=0;
    referenceMode=1; // original OnAction script changes its reference
    CHECK(inner.Begin(true,referenceMode));referenceMode=0;
    CHECK(referenceMode==0); // actual ProcessMovements consumes camera-relative input
    CHECK(inner.End(referenceMode,restore));referenceMode=restore;
    CHECK(referenceMode==1&&!outer.End(referenceMode,restore)); // preserve native script choice after callback

    CHECK(HeadMayAlignNativeBody(false,0,0)&&!HeadMayAlignNativeBody(true,0,0)&&!HeadMayAlignNativeBody(false,0,-1));
    for(float side:{-1.f,1.f})for(float drift:{-.30f,-.13f,.13f,.30f}){
        ControllerTranslator replay;ControllerFrame sample;sample.focused=true;sample.timeMs=10000;
        for(auto& h:sample.hands)h.active=true;sample.hands[0].stick[0]=side*.95f;sample.hands[0].stick[1]=drift;
        auto command=replay.Translate(sample,sample.timeMs,true,false,2,1,2,0);
        CHECK(!command.held[Forward]&&!command.held[Backward]&&command.held[Left]==(side<0)&&command.held[Right]==(side>0));
        NativeControllerActions input;input.Sample(1,10000,true,command.held[Forward],command.held[Backward],command.held[Left],command.held[Right],false,1);
        unsigned sends=0;input.Dispatch(1,10000,true,[&](auto const& message){++sends;if(message.key!=(side<0?3:4))sends+=100;},1);
        CHECK(sends==1);
        sample.timeMs+=10;sample.hands[0].stick[0]=sample.hands[0].stick[1]=0;
        command=replay.Translate(sample,sample.timeMs,true,false,2,1,2,0);
        input.Sample(1,sample.timeMs,true,command.held[Forward],command.held[Backward],command.held[Left],command.held[Right],false,2);
        input.Dispatch(1,sample.timeMs,true,[&](auto const&){++sends;},2);CHECK(sends==1);
    }
    float root[3]{1000,200,3000},lo[3]{-9999,-9999,-9999},hi[3]{9999,9999,9999};
    WorldAim aim;float rating=0;aim.origin[0]=1000;aim.origin[1]=350;aim.origin[2]=2900;
    float dy=-120,dz=100,norm=std::hypot(dy,dz);aim.direction[1]=dy/norm;aim.direction[2]=dz/norm;
    CHECK(InteractionAimBox(aim,88,root,100,lo,hi,120,rating)); // standing hand, fire at feet
    CHECK(hi[0]-lo[0]==110&&hi[1]-lo[1]==75); // smoke cannot steal the whole scene
    aim.origin[2]=2600;dy=-120;dz=400;norm=std::hypot(dy,dz);aim.direction[1]=dy/norm;aim.direction[2]=dz/norm;
    CHECK(!InteractionAimBox(aim,88,root,100,lo,hi,120,rating)); // far fire still excluded
    aim.origin[2]=2900;aim.direction[0]=1;aim.direction[1]=aim.direction[2]=0;
    CHECK(!InteractionAimBox(aim,89,root,100,lo,hi,120,rating)); // not a radius-only focus grab
    float itemLo[3]{1080,340,2890},itemHi[3]{1090,350,2910},copyLo[3],copyHi[3];
    for(unsigned i=0;i<3;++i){copyLo[i]=itemLo[i];copyHi[i]=itemHi[i];}
    CHECK(InteractionAimBox(aim,2,root,100,itemLo,itemHi,120,rating,true));
    for(unsigned i=0;i<3;++i)CHECK(copyLo[i]==itemLo[i]&&copyHi[i]==itemHi[i]);
    puts("PASS continuous native actions, immediate axis replacement, 300 neutral ticks, tracking/pause release, no Windows movement keys, reachable nearby hearth without distant focus theft");return 0;
}
