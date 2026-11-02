#include "ControllerControls.h"
#include "ControllerInput.h"
#include "ControllerSneakToggle.h"
#include "NativeSneakAction.h"
#include "NativeControllerModes.h"
#include "InteractionControls.h"
#include "SwingVelocity.h"
#include "StabDistance.h"
#include <string>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    ControllerTranslator translator;ControllerInput input;input.Initialize(L"Z:\\missing-options.ini");
    ControllerFrame f;f.focused=true;f.timeMs=10000;for(auto& hand:f.hands)hand.active=true;
    std::string guesses;int ups=0;bool lockSignature=true;auto send=[&](unsigned type,unsigned key,bool down){if(type==0&&(key==0x1e||key==0x20)){if(down)guesses+=key==0x1e?'L':'R';else ++ups;}return true;};
    auto sample=[&](float x,float y=0,bool focused=true){f.timeMs+=10;f.focused=focused;f.hands[0].stick[0]=x;f.hands[0].stick[1]=y;
        auto commands=translator.Translate(f,f.timeMs,true,false,2,1,2,0,true,lockSignature,true);input.Apply(commands,send);return commands;};
    auto rest=[&](){for(int i=0;i<12;++i)sample(0);};
    sample(-.9f);for(int i=0;i<300;++i)sample(-.9f);CHECK(guesses.empty()); //held before puzzle entry
    rest();sample(-.9f);for(int i=0;i<200;++i)sample(-.9f);CHECK(guesses=="L"&&ups==1);
    // No centre stop: sign changes, deadzone chatter and micro-centre samples
    // cannot turn one physical deflection into multiple wasted lockpicks.
    for(int i=0;i<120;++i)sample(i%3==0?0.f:i%3==1?.9f:-.9f);
    CHECK(guesses=="L");for(int i=0;i<25;++i)sample(0,.7f);sample(.9f);CHECK(guesses=="L");
    lockSignature=false;for(int i=0;i<100;++i)sample(i%2?-.9f:.9f);CHECK(guesses=="L");lockSignature=true;
    rest();sample(.9f);for(int i=0;i<12;++i)sample(.9f);CHECK(guesses=="LR"&&ups==2);
    for(char guess:std::string("LRRRLL")){rest();sample(guess=='L'?-.9f:.9f);for(int i=0;i<12;++i)sample(guess=='L'?-.9f:.9f);}
    CHECK(guesses=="LRLRRRLL"&&ups==8); //LRLR and RRLL, including repeated directions
    rest();sample(-.9f);sample(-.9f,0,false);CHECK(guesses=="LRLRRRLLL"&&ups==9);
    for(int i=0;i<100;++i)sample(-.9f);CHECK(guesses.size()==9);rest();sample(.9f);CHECK(guesses.size()==10);
    puts("PASS actual A/D emits one guess per neutral-separated lockpick step, long holds/sign changes/jitter and focus recovery cannot repeat");
    ControllerSneakToggle toggle;NativeSneakAction native;NativeControllerModes modes;int presses=0,releases=0;uint64_t time=20000;
    auto logical=[&](uintptr_t actor,NativeGameAction const& action){if(actor!=1||action.key!=14)return false;if(action.flags&1)++presses;else ++releases;return true;};
    auto y=[&](bool button,bool game=true,bool fresh=true,bool learned=true){time+=40;bool requested=toggle.Step(1,fresh,game,button,learned,time);native.Update(1,fresh&&game,requested,time,logical);modes.Update(fresh&&game,1,time,false,requested,false);return requested;};
    auto doubleY=[&](){y(false);y(false);y(true);y(false);y(false);return y(true);};
    CHECK(!y(true)&&!y(true)&&doubleY());for(int i=0;i<100;++i)CHECK(y(true));CHECK(presses==1&&releases==0);
    for(int i=0;i<100;++i)CHECK(y(false)&&modes.Request(1,time,4,4)==0);CHECK(!doubleY()&&presses==1&&releases==1);
    CHECK(doubleY());CHECK(!y(true,false)&&toggle.Enabled());CHECK(y(true)&&toggle.Enabled()); //held radial Y cannot toggle on return
    CHECK(!doubleY());CHECK(!y(false,true,true,false)&&!y(true,true,true,false));
    CHECK(doubleY());CHECK(!y(false,true,false)&&!toggle.Enabled());CHECK(!toggle.Step(2,true,true,true,true,time+10));
    CHECK(!NativeSneakState(77,4,"PS_Melee_Sneak")&&NativeSneakState(0,3,"PS_Melee_Loop"));
    puts("PASS double Y toggle, sustained/stationary armed Sneak ownership, native-mode priority, menu/radial suspension, held-button/skill/actor gates and actual stance3");
    ControllerTranslator remap;f={};f.focused=true;f.timeMs=30000;for(auto& hand:f.hands)hand.active=true;
    auto controls=[&](){f.timeMs+=10;return remap.Translate(f,f.timeMs,true,false,2,1,2,0);};
    controls();f.hands[0].secondary=true;auto c=controls();CHECK(c.sneakToggleButton&&!c.held[Back]);f.hands[0].secondary=false;controls();
    f.hands[0].stick[1]=1;f.hands[0].squeeze=.9f;c=controls();CHECK(!c.held[Sprint]&&!c.quickHeal);f.hands[0].stickClick=true;controls();f.hands[0].stickClick=false;c=controls();CHECK(c.held[Sprint]&&!c.quickHeal&&c.held[Forward]&&!c.radial.visible);
    f.hands[0].trigger=1;c=controls();CHECK(c.held[Sprint]&&!c.quickHeal);for(int i=0;i<4;++i)controls();
    f.hands[0].trigger=0;for(int i=0;i<4;++i)controls();f.hands[0].trigger=1;c=controls();CHECK(c.quickHeal&&c.held[Sprint]&&f.hands[0].squeeze==.9f);
    CHECK(!controls().quickHeal);f.hands[0].trigger=0;c=controls();CHECK(c.held[Sprint]&&c.held[Forward]);
    f.hands[0].stick[1]=0;c=controls();CHECK(!c.held[Sprint]);f.hands[0].stick[1]=1;CHECK(!controls().held[Sprint]);f.hands[0].squeeze=0;f.hands[0].menu=true;c=controls();CHECK(c.held[Back]);f.hands[0].menu=false;controls();
    f.hands[1].stickClick=true;c=controls();CHECK(c.radial.visible&&c.captured&&!c.held[Sprint]&&!c.sneakToggleButton);
    f.hands[1].stickClick=false;f.hands[1].stick[1]=1;c=controls();CHECK(c.radial.visible&&c.radial.selected==0);
    f.hands[1].stick[0]=1;f.hands[1].stick[1]=0;c=controls();CHECK(c.radial.selected!=0);
    f.hands[1].stick[0]=0;f.hands[1].primary=true;c=controls();CHECK(c.pulse>=Quick1&&c.pulse<=Quick0&&!c.radial.visible&&!c.held[Jump]);
    puts("PASS Menu button Back, L3 Sprint until stop and double-left-trigger heal, R3 radial / right joystick selection / A confirmation and captured inputs");
    combat::SwingVelocity velocity;combat::StabDistance stab;uint64_t t=40000;
    auto blade=[](float x){return melee::Blade{{x,0,0},{x,0,1}};};
    for(int i=0;i<=20;++i,t+=10){velocity.Step(blade(i*.07f),t,1,true,800);CHECK(!velocity.Ready(10));}
    velocity.Reset();for(int i=0;i<=20;++i,t+=10)velocity.Step(blade(i*.09f),t,2,true,800);CHECK(velocity.Ready(50));
    for(int i=0;i<=10;++i,t+=10){stab.Step({{0,0,i*.07f},{0,0,1+i*.07f}},t,3,true,800);CHECK(!stab.Ready(5));}
    stab.Reset();for(int i=0;i<=10;++i,t+=10)stab.Step({{0,0,i*.09f},{0,0,1+i*.09f}},t,4,true,800);CHECK(stab.Ready(15));
    puts("PASS 800cm/s is enforced in runtime gates:7m/s rejected,9m/s accepted for swing and stab");return 0;
}
