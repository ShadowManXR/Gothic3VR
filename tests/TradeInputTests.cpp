#include "ControllerInput.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    ControllerTranslator translator;ControllerInput input;input.Initialize(L"Z:\\missing-options.ini");
    ControllerFrame frame{};frame.focused=true;for(auto& h:frame.hands){h.active=true;h.aimInView.valid=true;}
    unsigned downs=0,ups=0;auto emit=[&](unsigned type,unsigned offset,bool down){if(type==1&&offset==4){if(down)++downs;else ++ups;}return true;};
    auto step=[&](bool menu=true,bool trade=true,bool allowed=true){frame.timeMs+=10;auto out=translator.Translate(frame,frame.timeMs,allowed,menu,2,1,2,0,true,false,false,trade);input.Apply(out,emit);return out;};
    frame.timeMs=1000;frame.hands[1].squeeze=1;CHECK(!step().held[UiRightClick]);frame.hands[1].squeeze=0;step();
    frame.hands[1].squeeze=1;auto out=step();CHECK(out.pointer&&out.held[UiRightClick]&&!out.held[UiClick]&&!out.held[Use2]);
    for(int i=0;i<200;++i)step();CHECK(downs==1&&ups==1); // hold never auto-transfers a stack
    frame.hands[1].squeeze=0;step();frame.hands[1].squeeze=1;step();for(int i=0;i<12;++i)step();CHECK(downs==2&&ups==2);
    frame.hands[1].squeeze=0;step();frame.hands[1].trigger=1;out=step();CHECK(out.held[UiClick]&&!out.held[UiRightClick]); // original drag retained
    frame.hands[1].squeeze=1;CHECK(!step().held[UiRightClick]);frame.hands[1].trigger=frame.hands[1].squeeze=0;step();
    frame.hands[1].squeeze=1;step();CHECK(downs==3);out=step(false,false);CHECK(!out.held[UiRightClick]&&!out.held[Use2]&&ups==3); // no RMB leak into world
    frame.hands[1].squeeze=0;step();frame.hands[1].squeeze=1;CHECK(!step(true,false).held[UiRightClick]); // other menu
    frame.hands[1].squeeze=0;step();frame.hands[1].aimInView.position[0]=10;frame.hands[1].squeeze=1;CHECK(!step().held[UiRightClick]);
    frame.hands[1].squeeze=0;frame.hands[1].aimInView.position[0]=0;step();frame.hands[1].squeeze=1;step();CHECK(downs==4);
    frame.focused=false;CHECK(!step().held[UiRightClick]&&ups==4);frame.focused=true;CHECK(!step().held[UiRightClick]);
    MenuSingleClick click;CHECK(!click.Step(true,true,1000));CHECK(!click.Step(true,false,1010));CHECK(click.Step(true,true,1020));
    CHECK(!click.Step(true,true,900));CHECK(!click.Step(true,true,1200));CHECK(!click.Step(false,true,1210));
    puts("PASS trade-only native RMB single-unit input, one transfer per grip press, drag retained, world/menu/pointer/focus isolation and clock reset");return 0;
}
