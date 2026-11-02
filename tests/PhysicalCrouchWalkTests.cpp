#include "PhysicalCrouchWalk.h"
#include "NativeControllerModes.h"
#include "ControllerMoveButtons.h"
#include <cstdio>
#include <limits>
#include <initializer_list>
using namespace diagnostic;
int main(){unsigned failures=0;auto check=[&](bool ok,char const* text){std::printf("%s %s\n",ok?"PASS":"FAIL",text);failures+=!ok;};
    PhysicalCrouchWalk crouch;uint64_t now=1000;
    auto step=[&](float depth,unsigned dt=20){now+=dt;return crouch.Update(1,1,now,true,1.7f,1.7f-depth);};
    bool free=true;for(int i=0;i<30;++i)free&=!step(i%2?.10f:.25f);check(free,"Small head movements and 25 cm leaning do not select Walk");
    check(!step(.4f)&&!step(.4f,40)&&!step(.1f,20),"Brief deep bob does not engage the crouch movement limit");
    check(!step(.35f),"Deliberate crouch first passes the stable-entry delay");for(int i=0;i<6;++i)step(.35f);check(crouch.Walking(),"30 cm threshold held for 120 ms engages native Walk");
    bool stable=true;for(float depth:{.31f,.28f,.23f,.19f,.22f,.29f})stable&=step(depth);check(stable,"Different enter/exit thresholds prevent repeated mode switching");
    check(step(.1f)&&step(.1f,40)&&step(.25f),"Brief upward bob does not release the movement limit");
    step(.1f);for(int i=0;i<6;++i)step(.1f);check(!crouch.Walking(),"Rising within 18 cm for 120 ms releases Walk");
    step(.4f);for(int i=0;i<6;++i)step(.4f);check(crouch.Walking(),"Second crouch re-engages normally");
    check(crouch.Hold(1,1,now+60,true,1.7f)&&!crouch.Hold(1,1,now+101,true,1.7f),"Short pose cache miss retains Walk, but stale tracking expires it");
    step(.4f);for(int i=0;i<6;++i)step(.4f);check(!crouch.Update(1,2,now+20,true,1.3f,1.3f)&&!crouch.Walking(),"Recenter/new tracking session resets the old physical posture");
    check(!crouch.Update(2,2,now+30,true,1.7f,1.2f),"New actor requires its own stable crouch sample");
    check(!crouch.Update(2,2,now+40,false,1.7f,1.2f)&&!crouch.Update(2,2,now+50,true,1.7f,std::numeric_limits<float>::quiet_NaN()),"Menu/context loss and invalid height cannot retain the limit");
    NativeControllerModes modes;modes.Update(true,1,5000,true,false,true,1,true);
    bool walk=true;for(int ground=1;ground<=5;++ground)walk&=modes.Request(1,5000,ground,ground,1)==3;check(walk,"Physical crouching selects native Walk even when Sprint is requested");
    bool untouched=true;for(int special:{0,6,7,8,9,10,11,12,99})untouched&=modes.Request(1,5000,special,4,1)==0&&modes.Request(1,5000,4,special,1)==0;check(untouched,"Crouching does not override jump/fall/swim/special modes");
    modes.Update(true,1,5010,true,true,false,2,true);check(modes.Request(1,5010,2,2,2)==0,"Learned native Sneak retains priority; physical crouch does not toggle it");
    modes.Update(true,1,5020,true,false,false,3,false);check(modes.Request(1,5020,3,3,3)==4,"Ordinary Run returns after rising");
    modes.Update(true,1,5030,false,false,false,4,true);check(modes.Request(1,5030,3,3,4)==0,"Crouching while idle does not create movement or override stopping");
    ControllerMoveButtons buttons;buttons.Step(true,true,0,false,6000,true);buttons.Step(true,true,0,true,6010,true);check(buttons.Step(true,true,0,false,6020,true).sprint,"Existing L3 tap activates Sprint");
    buttons.CancelSprint();check(!buttons.Step(true,true,0,false,6030,true).sprint&&!buttons.Step(true,true,0,false,6040,true).sprint,"Entering physical Walk cancels the sprint latch without automatic resume");
    buttons.Step(true,true,0,true,6050,true);check(buttons.Step(true,true,0,false,6060,true).sprint,"A new L3 tap can select Sprint after rising");
    // Cancelling sprint must not cancel an in-progress double-trigger potion.
    buttons.Step(true,true,0,false,6070,true);buttons.Step(true,true,0,false,6100,true);buttons.Step(true,true,1,false,6110,true);buttons.CancelSprint();buttons.Step(true,true,0,false,6120,true);buttons.Step(true,true,0,false,6150,true);check(buttons.Step(true,true,1,false,6200,true).heal,"Sprint cancellation preserves the independent quick-heal gesture");
    return failures?1:0;
}
