#include "ProceduralLocomotion.h"
#include "StableLocomotion.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    GroundMotor motor;float direction[3]{0,0,1},velocity[3]{};float position=0;
    for(uint64_t i=1;i<=100;++i){CHECK(motor.Step(1,i,true,direction,300,1,1,1,.01,60,velocity));CHECK(velocity[0]==0&&velocity[1]==0&&velocity[2]<=300);position+=velocity[2]*.01f;}
    GroundMotor isolated;float straight[3]{0,0,1},result[3]{};
    for(uint64_t f=1;f<=30;++f){CHECK(isolated.Step(11,f,true,straight,300,1,1,1,.01,150,result));isolated.ResetFor(22);}
    CHECK(std::fabs(isolated.Speed()-300)<.001f);isolated.ResetFor(11);CHECK(isolated.Speed()==0);
    float stopped=position;CHECK(motor.Step(1,101,false,direction,300,1,1,1,.01,60,velocity)&&velocity[2]==0&&motor.Speed()==0);
    // Native animation may still propose extra footsteps. The procedural
    // contribution must remain zero for every neutral frame, without a tail.
    for(uint64_t i=102;i<=200;++i){CHECK(motor.Step(1,i,false,direction,300,1,1,1,.01,60,velocity));position+=velocity[2]*.01f;CHECK(position==stopped);}
    direction[2]=-1;CHECK(motor.Step(1,201,true,direction,300,1,1,1,.01,60,velocity)&&velocity[2]<0);float before=velocity[2];
    CHECK(motor.Step(1,201,true,direction,300,1,1,1,.01,60,velocity)&&velocity[2]==before);
    CHECK(motor.Step(1,202,true,direction,300,1.5f,.8f,.5f,.01,0,velocity));CHECK(std::fabs(velocity[2]+180)<.001f);
    direction[0]=direction[2]=1;CHECK(motor.Step(2,1,true,direction,200,1,1,1,.01,0,velocity));CHECK(std::fabs(std::hypot(velocity[0],velocity[2])-200)<.001f);
    CHECK(motor.Step(2,2,false,direction,200,1,1,1,0,0,velocity)&&velocity[0]==0&&velocity[2]==0);
    CHECK(!motor.Step(2,3,true,direction,NAN,1,1,1,.01,60,velocity));
    WalkingCommand command;command.Update(true,1,1,1000,0,1);
    for(uint64_t t=1010;t<2000;t+=10){command.Update(true,1,1,t,0,0);CHECK(command.Owns(1,1,t)&&!command.Moving());}
    command.Update(true,1,1,2000,1,0);CHECK(command.Moving());command.Update(false,1,1,2010,0,0);CHECK(!command.Owns(1,1,2010));
    GroundMotor resolved;float forward[3]{0,0,1};CHECK(resolved.StepNative(33,1,true,forward,600,.01,0,result));CHECK(std::fabs(result[2]-600)<.001f); // 6 m/s, not the erroneous 600*1.5*1.5
    CHECK(resolved.StepNative(33,2,true,forward,300,.01,0,result));CHECK(std::fabs(result[2]-300)<.001f); // native slow/slope result already resolved
    CHECK(resolved.StepNative(33,3,false,forward,600,.01,0,result)&&result[2]==0);
    puts("PASS procedural ground stop without animation tail, immediate restart, duplicate-frame idempotence, native speed modifiers, diagonals and retained controller ownership");return 0;
}
