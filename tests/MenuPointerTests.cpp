#include "MenuPointer.h"
#include "ControllerControls.h"
#include <cstdio>
int main() {
    unsigned failures=0;auto check=[&](bool value,char const* name){printf("%s %s\n",value?"PASS":"FAIL",name);failures+=!value;};
    diagnostic::MenuPointer pointer;float u=.5f,v=.5f;pointer.Filter(u,v,1000,false);
    float maxJitter=0;
    for(unsigned i=1;i<=100;++i){u=.5f+(i%2?.004f:-.004f);v=.5f+(i%2?-.003f:.003f);pointer.Filter(u,v,1000+i*10,false);if(i>20)maxJitter=std::max(maxJitter,std::hypot(u-.5f,v-.5f));}
    check(maxJitter<.0015f,"stationary controller tremor is reduced by more than 70 percent without resolution dependence");
    float before=u;
    u=.99f;v=.5f;pointer.Filter(u,v,2010,false);
    check(u>before&&u<.99f,"deliberate movement responds immediately while remaining filtered");
    for(unsigned i=2;i<=12;++i){u=.99f;v=.5f;pointer.Filter(u,v,2000+i*10,false);}
    check(u>.98f,"fast aiming reaches a distant menu option within 120 ms");
    pointer.Reset();u=.25f;v=.35f;pointer.Filter(u,v,3000,false);
    u=.29f;v=.4f;pointer.Filter(u,v,3010,true);
    check(std::fabs(u-.25f)<.0001f&&std::fabs(v-.35f)<.0001f,"trigger squeeze clicks the previously indicated target, not wrist jerk");
    for(unsigned i=2;i<10;++i){u=.7f;v=.8f;pointer.Filter(u,v,3000+i*10,true);}
    check(std::fabs(u-.25f)<.0001f,"click stability lasts through the initial 100 ms");
    u=.7f;v=.8f;pointer.Filter(u,v,3110,true);check(u>.6f&&v>.7f,"holding trigger still permits inventory dragging after initial stabilization");
    pointer.Reset();u=1;v=0;pointer.Filter(u,v,4000,false);check(u==1&&v==0,"full menu edges remain reachable");
    u=0;v=1;pointer.Filter(u,v,4200,false);check(u==0&&v==1,"tracking gaps snap to the newly indicated point without traversing old options");
    float oldU=u;u=1;v=0;pointer.Filter(u,v,4200,false);check(u==oldU,"duplicate tracking samples do not advance the filter or create artificial velocity");
    diagnostic::ControllerCameraReset reset;
    check(!reset.Step(true,true,true,1000)&&!reset.Step(true,true,true,2000),"a chord held during startup cannot trigger a reset");
    reset.Step(true,false,false,2001);reset.Step(true,true,true,2010);
    check(!reset.Step(true,true,true,2509)&&reset.Step(true,true,true,2510)&&!reset.Step(true,true,true,3200),"both joystick buttons held 500 ms trigger exactly once");
    reset.Step(true,true,false,3210);check(!reset.Step(true,true,true,4000),"both buttons must be released before repeating camera reset");
    reset.Step(true,false,false,4010);reset.Step(true,true,true,4020);reset.Step(false,true,true,4500);
    check(!reset.Step(true,true,true,5000),"focus loss cancels a partially held chord");
    reset.Step(true,false,false,5010);reset.Step(true,true,true,5020);reset.Step(true,true,false,5400);reset.Step(true,true,true,5410);
    check(!reset.Step(true,true,true,5909)&&reset.Step(true,true,true,5910),"interrupted simultaneous press starts a fresh 500 ms interval");
    diagnostic::ControllerFrame frame{};frame.focused=true;frame.timeMs=6000;frame.hands[0].active=frame.hands[1].active=true;
    diagnostic::ControllerTranslator translator;translator.Translate(frame,6000,true,false,2.6f,1.5f,2,0);
    frame.hands[0].stickClick=frame.hands[1].stickClick=true;frame.hands[0].stick[1]=1;frame.hands[1].stick[0]=1;frame.hands[1].trigger=1;
    frame.timeMs=6010;translator.Translate(frame,6010,true,false,2.6f,1.5f,2,0);frame.timeMs=6510;
    auto commands=translator.Translate(frame,6510,true,false,2.6f,1.5f,2,0);
    check(commands.recenter&&!commands.held[diagnostic::Forward]&&!commands.held[diagnostic::Use1]&&commands.turnX==0,"reset chord cannot walk, rotate, attack or click while pressing both sticks");
    printf("Pointer/reset failures: %u\n",failures);return failures?1:0;
}
