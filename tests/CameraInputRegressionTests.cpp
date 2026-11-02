#include "NativeControllerTurn.h"
#include "ActivityCamera.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
// Exercise the real MSVC x86 member return-buffer convention used by the
// camera hook. A wrong calling convention corrupts the stack or result.
struct MatrixFixture {float values[16]{};MatrixFixture(){}~MatrixFixture(){}};
struct CameraFixture {
    float value=7;bool fail=false;
    __declspec(noinline) MatrixFixture Wished(){if(fail)throw std::runtime_error("native camera error");MatrixFixture out;for(unsigned i=0;i<16;++i)out.values[i]=value+float(i);return out;}
};
int main(){
    auto member=&CameraFixture::Wished;void* entry=nullptr;static_assert(sizeof(member)==sizeof(entry));std::memcpy(&entry,&member,sizeof(entry));
    auto original=reinterpret_cast<MatrixFixture*(__thiscall*)(CameraFixture*,MatrixFixture*)>(entry);
    CameraFixture camera;MatrixFixture returned;
    for(unsigned i=0;i<10000;++i){CHECK(original(&camera,&returned)==&returned);for(unsigned j=0;j<16;++j)CHECK(returned.values[j]==camera.value+float(j));}
    camera.fail=true;bool caught=false;try{original(&camera,&returned);}catch(std::runtime_error const&){caught=true;}CHECK(caught);
    for(unsigned rate:{60u,72u,90u,120u,144u}){
        NativeControllerTurn turn;turn.Sample(true,1,651,1000,1,10.0);CHECK(turn.Consume(1,1000)==0);
        double total=0;
        for(unsigned i=1;i<=rate;++i){uint64_t tick=1000+i*1000/rate;
            turn.Sample(true,1,651,tick,i+1,10.0+double(i)/rate);
            total+=turn.Consume(i+1,tick);CHECK(turn.Consume(i+1,tick)==0); // PreRender cannot replay OnProcess
        }
        CHECK(std::fabs(total-3906)<.01);
        turn.Sample(true,0,651,2001,1000,11.001);CHECK(turn.Consume(1000,2001)==0);
        turn.Sample(true,0,651,2010,1001,11.01);CHECK(turn.Consume(1001,2010)==0);
    }
    NativeControllerTurn fine;fine.Sample(true,.401f,100,1000,1,20);
    fine.Sample(true,.401f,100,1004,2,20.004);float tiny=fine.Consume(2,1004);CHECK(tiny>0&&tiny<.01f); // no integer pixel staircase
    fine.Sample(true,-1,100,1010,3,20.01);CHECK(fine.Consume(3,1010)<0);
    fine.Sample(true,1,100,2010,4,21.01);CHECK(std::fabs(fine.Consume(4,2010)-30)<.001f); // stalled frame bounded to50ms
    fine.Sample(false,1,100,2020,5,21.02);CHECK(fine.Consume(5,2020)==0);
    fine.Sample(true,1,100,2030,6,21.03);CHECK(fine.Consume(6,2030)==0); // no hidden catch-up after menu/focus
    fine.Sample(true,1,100,2040,7,21.04);CHECK(fine.Consume(7,2241)>0); // accepted command survives work earlier in the SAME update
    fine.Sample(true,1,100,2250,8,21.25);CHECK(fine.Consume(9,2250)==0); // a new update can never replay it
    fine.Sample(true,1,100,2260,9,21.26);CHECK(fine.Consume(9,2259)==0); // clock regression rejected
    // Camera translation is queried before rotation and can be re-queried
    // INSIDE rotation. Preview count must not change the final camera yaw.
    for(unsigned previews:{0u,1u,2u,8u}){
        NativeControllerTurn turn;NativeTurnEvaluation evaluation;turn.Sample(true,1,100,1000,1,1);
        turn.Sample(true,1,100,1010,2,1.01);
        float filtered=0;
        for(unsigned i=0;i<previews;++i)filtered=evaluation.Filter(1,0); // no Consume at preview boundary
        float delta=turn.Consume(2,1210);CHECK(delta>0&&evaluation.Begin(1,filtered,delta));
        CHECK(!evaluation.Begin(1,0,99)&&!evaluation.Owns(2));
        for(unsigned i=0;i<previews;++i)filtered=evaluation.Filter(1,0);
        CHECK(evaluation.Filter(1,0)==delta); // rotation reads the same angle
        CHECK(evaluation.Filter(2,3)==3); // another camera unaffected
        CHECK(evaluation.Filter(1,.125f)==delta+.125f);CHECK(evaluation.End(1)==.125f);
        CHECK(!evaluation.Owns(1)&&evaluation.Filter(1,0)==0&&turn.Consume(2,1210)==0);
    }
    CHECK(std::fabs(NativeTurnAngle(600,false,1,.1f,.5f)-1.5f)<.0001f);
    CHECK(std::fabs(NativeTurnAngle(-600,false,1,.1f,.5f)+1.5f)<.0001f);
    CHECK(std::fabs(NativeTurnAngle(600,true,1,.1f,.5f)+1.5f)<.0001f);
    CHECK(std::fabs(NativeTurnAngle(-600,true,1,.1f,.5f)-1.5f)<.0001f);
    CHECK(NativeTurnAngle(600,false,NAN,.1f,.5f)==0);
    NativeTurnOutput output;float first[16]{},repeated[16]{};first[0]=first[10]=.5f;first[2]=.866f;first[8]=-.866f;first[5]=first[15]=1;
    output.Store(1,30,first);
    for(unsigned i=0;i<12;++i){std::memset(repeated,0,sizeof(repeated));repeated[0]=repeated[5]=repeated[10]=repeated[15]=1;repeated[12]=float(i); // native zero-input basis, updated position
        CHECK(output.Apply(1,30,repeated));for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)CHECK(repeated[r*4+c]==first[r*4+c]);CHECK(repeated[12]==float(i)&&repeated[15]==1);}
    CHECK(!output.Apply(1,31,repeated)&&!output.Apply(2,30,repeated));output.Reset();CHECK(!output.Apply(1,30,repeated));

    ActivityCameraGate activity;
    uint64_t time=1000;
    for(unsigned i=0;i<100;++i){CHECK(!activity.Step(1,true,time));CHECK(!activity.Step(1,true,time+5));CHECK(!activity.Step(1,false,time+16));CHECK(!activity.Step(1,false,time+200));time+=300;}
    CHECK(!activity.Step(1,true,time));CHECK(!activity.Step(1,true,time+139));CHECK(activity.Step(1,true,time+140));
    for(unsigned i=0;i<100;++i)CHECK(activity.Step(1,true,time+140)); // repeated queries do not advance the timer
    CHECK(activity.Step(1,false,time+150)&&activity.Step(1,false,time+229));CHECK(!activity.Step(1,false,time+230));
    CHECK(!activity.Step(2,true,time+240)&&!activity.Step(0,true,time+500));
    CHECK(ActivityCameraImmediate(15,"PS_Player","PS_Player")&&!ActivityCameraImmediate(73,"PS_Player","PS_Player"));
    CHECK(activity.Step(1,true,time+501,true)); // native finisher remains immediate
    puts("PASS x86 matrix return-buffer ABI/exceptions, continuous float turn at60..144Hz, translation previews, delayed rotation in same update, no replay or menu residual; transient activity camera");return 0;
}
