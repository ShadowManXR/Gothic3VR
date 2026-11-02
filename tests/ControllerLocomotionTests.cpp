#include "ControllerLocomotion.h"
#include "NativeControllerModes.h"
#include "StableLocomotion.h"
#include "ActivityCamera.h"
#include "StereoCameraState.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    WalkingCommand command;float x=0,z=0,out[3]{};uint64_t t=1000;
    for(float angle:{0.f,.8f,1.56f,1.8f,2.8f,3.14f,3.8f,4.7f,5.8f}){
        float euler[3]{0,angle,0};auto view=NativeCameraRotation(euler);float folded[3]{};NativeCameraEuler(view,folded);auto reconstructed=NativeCameraRotation(folded);
        float yaw=0;CHECK(ViewWalkingHeading(reconstructed,{}, {},yaw));CHECK(AnalogWalk(-1,0,x,z));command.Update(true,1,1,t++,x,z);CHECK(command.Direction(yaw,out));
        // Left is left of the complete visible camera at every bearing,
        // including Euler representations with 180-degree pitch/roll.
        CHECK(out[0]*view.m[0]+out[2]*view.m[6]<-.999f);
        CHECK(AnalogWalk(0,1,x,z));command.Update(true,1,1,t++,x,z);CHECK(command.Direction(yaw,out));CHECK(out[0]*view.m[2]+out[2]*view.m[8]>.999f);
        CHECK(AnalogWalk(1,0,x,z));command.Update(true,1,1,t++,x,z);CHECK(command.Direction(yaw,out));CHECK(out[0]*view.m[0]+out[2]*view.m[6]>.999f);
    }
    CHECK(AnalogWalk(.8f,.4f,x,z)&&std::fabs(x/z-2)<.001f);CHECK(!AnalogWalk(.1f,.1f,x,z)&&x==0&&z==0);
    command.Update(true,1,1,t++,1,0);for(int i=0;i<100;++i){command.Update(true,1,1,t,0,0);CHECK(command.Owns(1,1,t)&&!command.Moving());t+=16;}
    command.Update(true,1,1,t++,-1,0);CHECK(command.Direction(0,out)&&out[0]<-.999f);
    CHECK(RunOnlyMode(3,true,false,false)==4&&RunOnlyMode(2,true,false,false)==4&&RunOnlyMode(5,true,false,false)==4);
    CHECK(RunOnlyMode(4,true,false,true)==4&&RunOnlyMode(5,true,false,true)==5&&RunOnlyMode(4,true,true,true)==2);
    CHECK(RunOnlyMode(10,true,false,true)==10&&RunOnlyMode(6,true,false,false)==6&&RunOnlyMode(3,false,false,false)==1);
    GripSneak sneak;CHECK(!sneak.Step(true,true,true,false,true,1000));CHECK(!sneak.Step(true,true,true,false,true,1100));CHECK(sneak.Step(true,true,true,false,true,1120));
    CHECK(sneak.Step(true,true,false,false,true,1130));CHECK(!sneak.Step(true,false,false,false,true,1140));
    CHECK(!sneak.Step(true,true,true,true,true,1200)&&!sneak.Step(true,true,true,false,false,1500));
    CHECK(!sneak.Step(false,true,true,false,true,1800));
    NativeControllerModes modes;
    modes.Update(true,1,2000,false,false,false);CHECK(!modes.Owns(1,2000)&&modes.Request(1,2000,4,4)==0); // untouched keyboard stays native
    modes.Update(true,1,2010,true,false,false);CHECK(modes.Owns(1,2010)&&modes.Request(1,2010,4,4)==4); // Run, without heading/direct motor
    modes.Update(true,1,2020,true,true,true);CHECK(modes.Request(1,2020,4,4)==0); // Sneak overrides Sprint
    modes.Update(true,1,2030,false,true,false);CHECK(modes.Request(1,2030,4,4)==0); // held stationary Sneak
    modes.Update(true,1,2040,true,false,true);CHECK(modes.Request(1,2040,4,4)==0); // Fast never forced; native Shift/perks/stamina decide
    modes.Update(true,1,2050,true,false,false);CHECK(modes.Request(1,2050,4,4)==4); // fresh Run immediately after Sprint
    modes.Update(true,1,2060,false,false,false);CHECK(modes.Request(1,2060,4,4)==0);CHECK(modes.Request(1,2161,4,4)==0); // native stopping, stale releases authority
    // Repeated Run input must never steal special engine modes, even when
    // the other side of the transition still reports an ordinary Run.
    modes.Update(true,1,2070,true,false,false);
    for(int special:{-1,0,6,7,8,9,10,11,12,13,14,15,16,17,99}){
        CHECK(modes.Request(1,2070,special,4)==0);
        CHECK(modes.Request(1,2070,4,special)==0);
    }
    for(int ground:{1,2,3,4,5})CHECK(modes.Request(1,2070,ground,ground)==4);
    modes.Update(true,1,2080,true,false,true);
    CHECK(modes.Request(1,2080,3,4)==4&&modes.Request(1,2080,5,5)==0&&modes.Request(1,2080,4,4)==0);
    CHECK(modes.Request(1,2080,3,6)==0); // Jump during Sprint.
    modes.Update(true,1,2090,false,false,false);CHECK(modes.Owns(1,2090)&&modes.Request(1,2090,3,4)==0);
    modes.Update(true,1,2100,true,false,false);CHECK(modes.Request(1,2100,4,4)==4); // immediate restart
    modes.Update(false,1,2170,true,true,true);CHECK(!modes.Owns(1,2170));
    modes.Update(true,1,2180,true,false,false);modes.Update(true,2,2190,false,false,false);CHECK(!modes.Owns(2,2190)&&!modes.Owns(1,2190));
    modes.Update(true,2,2200,true,false,false);modes.Update(true,2,2400,false,false,false);CHECK(!modes.Owns(2,2400)); // tracking gap cannot keep overriding keyboard
    modes.Update(true,2,2500,true,false,false,80);
    CHECK(modes.Owns(2,2850,80)&&modes.Request(2,2850,4,4,80)==4);
    CHECK(!modes.Owns(2,2850,81)&&modes.Request(2,2850,4,4,81)==0);
    for(int action:{73,79,83,87,89,92,95,98,99,102,106,118})CHECK(ActivityThirdPerson(action,2,"PS_Player","PS_Player"));
    for(int action:{0,1,2,29,33,40,41,43,44,45,47,64,66,77})CHECK(!ActivityThirdPerson(action,2,"PS_Player","PS_Player"));
    CHECK(ActivityThirdPerson(0,10,"TS_Sleep","PS_Player"));CHECK(ActivityThirdPerson(0,2,"TS_UseMob","PS_Player"));
    StereoCameraState state{true,true,true,false,false,false,true};CHECK(!ComfortFirstPerson(state,true,true,false));state.activity=false;CHECK(ComfortFirstPerson(state,true,true,false));
    puts("PASS complete view direction in all quadrants, continuous analog axes, neutral ownership/restart, no Walk, native sprint, skill/support-safe held Sneak and activity third-person");return 0;
}
