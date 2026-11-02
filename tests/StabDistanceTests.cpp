#include "StabDistance.h"
#include "SwingVelocity.h"
#include "CombatGestures.h"
#include "ControllerInput.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    combat::StabDistance stab;combat::SwingVelocity swing;combat::StrokeBudget budget;uint64_t stamp=1000;int spent=0;
    auto forward=[](float z){return melee::Blade{{0,0,z},{0,0,z+1}};};
    for(int i=0;i<=10;++i,stamp+=10){auto blade=forward(i*.02f);stab.Step(blade,stamp,1,true,100);swing.Step(blade,stamp,1,true,100);
        bool ready=stab.Ready(15);budget.Decide(1,false,ready,ready,10,20,100,false,[&](int cost){spent+=cost;return true;});}
    CHECK(stab.Ready(15)&&!swing.Ready(150)&&spent==10); //short fast thrust, no150cm swing needed
    // The same qualifying 20cm thrust remains physical damage for animals,
    // even with a much longer swing threshold; humanoids retain native Pierce.
    melee::Swing contacts;contacts.intensity=.6f;unsigned dispatched=0;int actions[4]{};
    int species[4]{0,10,5,11}; // human, wolf, orc, boar in one stroke
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<4;++i){
        if(!stab.Ready(15)||!contacts.Hit(100+i))continue;
        actions[i]=melee::PhysicalAttackAction(false,true,2,2,species[i]);++dispatched;
    }
    CHECK(dispatched==4&&actions[0]==11&&actions[1]==1&&actions[2]==11&&actions[3]==1&&spent==10);
    stab.Step(forward(.2f),stamp-10,1,false,100);CHECK(stab.Ready(15)); //same XR sample preserves gate
    stab.Step(forward(.2f),stamp,1,true,100);CHECK(!stab.Ready(15));stamp+=10;
    stab.Reset();for(int i=0;i<=250;++i,stamp+=20){stab.Step(forward(i*.005f),stamp,2,true,100);CHECK(!stab.Ready(5));}
    stab.Step(forward(1.28f),stamp,2,true,100);CHECK(!stab.Ready(5));stamp+=20; //slow1.25m then3cm flick
    stab.Reset();for(int i=0;i<=20;++i,stamp+=10){float x=i*.03f;stab.Step({{x,0,0},{x,0,1}},stamp,3,true,100);CHECK(!stab.Ready(5));}
    //Sideways travel cannot feed a later short forward twitch.
    stab.Step({{.6f,0,.02f},{.6f,0,1.02f}},stamp,3,true,100);CHECK(!stab.Ready(5));stamp+=10;
    stab.Reset();for(int i=0;i<100;++i,stamp+=10){stab.Step(forward(i%2*.02f),stamp,4,true,100);CHECK(!stab.Ready(5));}
    stab.Reset();for(int i=0;i<=50;++i,stamp+=10){float a=i*.03f;stab.Step({{0,0,0},{std::sin(a),0,std::cos(a)}},stamp,5,true,100);CHECK(!stab.Ready(5));}
    stab.Reset();for(int i=0;i<=5;++i,stamp+=10)stab.Step(forward(i*.02f),stamp,6,true,100);CHECK(stab.Ready(5)&&!stab.Ready(15));
    stamp+=101;stab.Step(forward(.15f),stamp,6,true,100);CHECK(!stab.Ready(5));stamp+=10;
    for(int i=0;i<=10;++i,stamp+=10)stab.Step(forward(.15f+i*.02f),stamp,6,true,100);CHECK(stab.Ready(15));
    stab.Step(forward(.4f),stamp,7,true,100);CHECK(!stab.Ready(5));stamp+=10;
    for(int i=0;i<=10;++i,stamp+=10)stab.Step(forward(.4f+i*.02f),stamp,7,true,100);CHECK(stab.Ready(15));
    stab.Step(forward(.64f),stamp,7,true,200);CHECK(!stab.Ready(5));
    puts("PASS separate short fast stab distance, shared stroke debit, slow/flick/wrist/cut/tremor rejection, duplicate sample and context reset");
    ControllerTranslator translator;ControllerInput input;input.Initialize(L"Z:\\missing-user-options.ini");
    CHECK(input.Binding(MenuLeft).type==0&&input.Binding(MenuLeft).offset==0x1e&&input.Binding(MenuRight).offset==0x20);
    ControllerFrame frame;frame.focused=true;frame.timeMs=10000;for(auto& hand:frame.hands)hand.active=true;
    //Actual minigame has no native routine100/task signature. Still send A/D.
    translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);frame.timeMs+=10;frame.hands[0].stick[0]=-.9f;
    auto commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);
    CHECK(commands.held[MenuLeft]&&!commands.held[Left]&&!commands.held[Forward]&&!commands.lockpick&&commands.moveX==0&&commands.moveZ==0);
    int aDown=0,aUp=0,dDown=0;bool keyboardOnly=true;auto emit=[&](unsigned type,unsigned key,bool down){keyboardOnly&=type==0;if(key==0x1e){if(down)++aDown;else ++aUp;}if(key==0x20&&down)++dDown;return true;};
    input.Apply(commands,emit);CHECK(aDown==1);
    //Native routine changes while the same page remains open cannot erase a fresh pulse.
    frame.timeMs+=20;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);input.Apply(commands,emit);CHECK(aDown==1&&commands.held[MenuLeft]);
    frame.timeMs+=80;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);input.Apply(commands,emit);CHECK(aUp==1&&!commands.held[MenuLeft]);
    frame.timeMs+=100;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);input.Apply(commands,emit);CHECK(aDown==1); //hold cannot repeat guesses
    frame.timeMs+=10;frame.hands[0].stick[0]=0;input.Apply(translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false),emit);
    frame.timeMs+=10;frame.hands[0].stick[0]=-.9f;input.Apply(translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false),emit);CHECK(aDown==2);
    frame.timeMs+=10;frame.hands[0].stick[0]=.9f;input.Apply(translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false),emit);CHECK(aUp==2&&dDown==1);
    frame.timeMs+=10;frame.focused=false;commands=translator.Translate(frame,frame.timeMs,true,true,2,1,2,0,true,false);input.Apply(commands,emit);CHECK(!commands.held[MenuRight]);
    CHECK(keyboardOnly);puts("PASS actual A/D menu keys without routine detection, neutral re-arm, one guess per deflection, native-state transition and focus release");return 0;
}
