#include "StableLocomotion.h"
#include "DirectLocomotion.h"
#include "AnimalReturn.h"
#include "TemporaryOverride.h"
#include <windows.h>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
struct Movement {
    float acceleration=140,deceleration=-120,inertia=.8f;bool sensorDirection=true;
    bool GetSensorAffectsDirection(){return sensorDirection;}void SetSensorAffectsDirection(bool value){sensorDirection=value;}
    float GetMoveAcceleration(){return acceleration;}float GetMoveDecceleration(){return deceleration;}float GetAniVelocityInertia(){return inertia;}
    float GetForwardSpeedMax(){return 300;}float GetStrafeSpeedMax(){return 200;}float GetBackwardSpeedMax(){return 150;}
    void SetMoveAcceleration(float v){acceleration=v;}void SetMoveDecceleration(float v){deceleration=v;}void SetAniVelocityInertia(float v){inertia=v;}
};
WalkingProperties<Movement> properties;Movement actor;
void __cdecl Raise(void*){RaiseException(0xe0424242,0,0,nullptr);}
void __cdecl Restore(void*){properties.Restore();}
bool ExceptionTrial(){__try{CallWithRestoration(Raise,Restore,nullptr);}__except(EXCEPTION_EXECUTE_HANDLER){return true;}return false;}
int __cdecl main(){
    WalkingCommand input;float direction[3]{};
    input.Update(true,1,7,1000,0,0);CHECK(!input.Owns(1,7,1000));
    input.Update(true,1,7,1010,0,1);CHECK(input.Owns(1,7,1010)&&input.Direction(1.570796327f,direction));CHECK(direction[0]>.999f&&std::fabs(direction[2])<.001f);
    input.Update(true,1,7,1020,1,1);CHECK(input.Direction(0,direction));CHECK(std::fabs(std::hypot(direction[0],direction[2])-1)<.0001f);
    input.Update(true,1,7,1030,0,0);CHECK(input.Owns(1,7,1030));
    CHECK(!input.Owns(1,7,1300));input.Update(true,1,7,1040,0,1);input.Update(true,2,7,1050,0,0);CHECK(!input.Owns(2,7,1050));
    input.Update(true,2,7,1060,0,1);input.Update(false,2,7,1070,0,0);CHECK(!input.Owns(2,7,1070));
    // Restart/reverse on the very next controller sample; no stop action can
    // remain queued or require an animation to finish.
    input.Update(true,2,7,1080,0,1);input.Update(true,2,7,1090,0,0);CHECK(input.Direction(0,direction)&&direction[2]==1);
    input.Update(true,2,7,1100,0,-1);CHECK(input.Moving()&&input.Direction(0,direction)&&direction[2]==-1);
    float native[3]{18,-12,240},out[3]{};CHECK(RedirectHorizontalVelocity(native,direction,out));
    CHECK(out[1]==-12&&out[2]<0&&std::fabs(std::hypot(out[0],out[2])-std::hypot(native[0],native[2]))<.0001f);
    // Native short-braking speed passes unchanged: no second velocity filter
    // can disagree with native goals/history and produce a correction snap.
    for(float v:{200.f,100.f,20.f}){native[0]=0;native[2]=v;CHECK(RedirectHorizontalVelocity(native,direction,out)&&out[2]==-v&&out[1]==-12);}
    native[2]=0;CHECK(!RedirectHorizontalVelocity(native,direction,out));
    for(int mode=0;mode<18;++mode)CHECK(ComfortWalkingMode(mode)==(mode==1||(mode>=2&&mode<=5)||mode==10));
    properties.Begin(&actor,60);CHECK(std::fabs(actor.acceleration-5000)<.001f&&std::fabs(actor.deceleration+5000)<.001f&&actor.inertia==0&&!actor.sensorDirection);CHECK(ExceptionTrial());CHECK(actor.acceleration==140&&actor.deceleration==-120&&actor.inertia==.8f&&actor.sensorDirection);
    properties.Begin(&actor,120);CHECK(std::fabs(actor.acceleration-2500)<.001f);properties.Restore();
    CHECK(ReturnedToDifferentHuman(1,2,false));CHECK(!ReturnedToDifferentHuman(1,1,false)&&!ReturnedToDifferentHuman(1,0,false)&&!ReturnedToDifferentHuman(1,2,true));
    puts("PASS controller ownership, immediate restart/direction, unchanged native speed, unchanged Y, native property exception restoration and animal cleanup ownership");return 0;
}

