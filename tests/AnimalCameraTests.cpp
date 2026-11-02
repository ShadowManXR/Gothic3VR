#include "AnimalCamera.h"
#include "AnimalReturn.h"
#include "CameraComfort.h"
#include "StereoCameraState.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
// Mirrors verified SetMode's guard: even mode Normal returns to FP if a
// persistent, temporary or requested FP latch survives the transition.
int NativeMode(unsigned char* camera){return AnimalCamera::FirstPerson(camera,0x558)?11:0;}
int main(){
    unsigned char camera[0x558],before[0x558];
    for(auto flag:{AnimalCamera::Active,AnimalCamera::Request,AnimalCamera::Temporary}){
        memset(camera,0,sizeof(camera));camera[flag]=1;camera[0x10]=0x67;memcpy(before,camera,sizeof(camera));
        CHECK(NativeMode(camera)==11);CHECK(AnimalCamera::Release(camera,sizeof(camera)));CHECK(NativeMode(camera)==0);
        for(size_t i=0;i<sizeof(camera);++i)CHECK(camera[i]==(i==flag?0:before[i]));
    }
    CHECK(!AnimalCamera::Release(camera,AnimalCamera::Size-1));CHECK(!AnimalCamera::Release(nullptr,sizeof(camera)));
    StereoCameraState state{true,true,true,true,false,true};CHECK(!ComfortFirstPerson(state,true,true,false));
    state.animal=false;CHECK(ComfortFirstPerson(state,true,true,false));
    CameraHeightAnchor anchor;CHECK(anchor.Height(1,100,280)==280);float human;
    CHECK(anchor.Snapshot(human)&&human==180);anchor.Reset();CHECK(anchor.Height(2,100,145)==145);
    anchor.Reset();CHECK(anchor.Restore(3,human)&&anchor.Height(3,500,545)==680);
    float root[3]{30,500,20},native[3]{90,545,80};anchor.Position(3,root,native);CHECK(native[0]==30&&native[1]==680&&native[2]==20);
    float hmd[3]{.1f,1.6f,-.2f},origin[3]{};Orientation center;
    CHECK(RecenterTracking({1,0,0,0},hmd,true,center,origin)&&origin[1]==1.6f&&anchor.Height(3,500,545)==680);
    CHECK(!anchor.Restore(0,human)&&!anchor.Restore(4,NAN));
    AnimalReturn chord;CHECK(!chord.Step(true,true,1,7,1000,false,false));
    for(uint64_t t=1010;t<1510;t+=10)CHECK(!chord.Step(true,true,1,7,t,true,true));
    CHECK(chord.Step(true,true,1,7,1510,true,true)&&chord.Captured());
    CHECK(!chord.Step(true,false,2,7,1520,true,true)&&chord.Captured());
    CHECK(!chord.Step(true,false,2,7,1530,false,false)&&!chord.Captured());
    CHECK(!chord.Step(true,true,1,7,1540,true,true));CHECK(!chord.Step(false,true,1,7,1550,true,true));
    for(uint64_t t=1560;t<2060;t+=10)CHECK(!chord.Step(true,true,1,7,t,true,true));CHECK(chord.Step(true,true,1,7,2060,true,true));
    CHECK(!chord.Step(true,true,1,7,2070,false,false));CHECK(!chord.Step(true,true,1,7,2080,true,true));
    CHECK(!chord.Step(true,true,1,8,2580,true,true)); // stale session cannot immediately return
    puts("PASS animal transition releases all native first-person latches and humanoid view resumes only after transformation ends");return 0;
}
