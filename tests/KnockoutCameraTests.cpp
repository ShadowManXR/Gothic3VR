#include "KnockoutCamera.h"
#include "FirstPersonCameraLock.h"
#include "StereoCameraState.h"
#include "FirstPersonHeadVisibility.h"
#include <cstdio>
#include <initializer_list>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int __cdecl main(){
    KnockoutCamera camera;
    CHECK(!camera.Update(1,0,2,false,false));
    for(int hit:{20,21,22,23,24,25,26,27,28,34})CHECK(!camera.Update(1,hit,2,false,false));
    for(int fall:{29,30,31,32,33,37}){
        CHECK(camera.Update(1,fall,2,false,false)); // action precedes animation/down flag
        CHECK(camera.Update(1,0,15,true,false));
        CHECK(camera.Update(1,30,16,false,false));
        CHECK(camera.Update(1,0,16,false,false)); // no early release during get-up
        CHECK(!camera.Update(1,0,2,false,false));
    }
    CHECK(camera.Update(1,0,2,true,false)); // native down flag alone also owns 3P
    CHECK(!camera.Update(1,0,2,false,false));
    CHECK(camera.Update(1,32,16,true,false));CHECK(!camera.Update(1,77,3,false,false)); // upright sneak return
    CHECK(camera.Update(1,29,15,true,false));CHECK(!camera.Update(1,16,4,false,false)); // upright guard return
    CHECK(camera.Update(1,33,17,true,false)); // health exhausted; still alive
    for(int i=0;i<300;++i)CHECK(camera.Update(1,0,17,true,false));
    CHECK(camera.Update(1,30,16,true,false)); // native get-up, no early FP jump
    CHECK(camera.Update(1,30,15,false,false));
    CHECK(!camera.Update(1,0,2,false,false)); // native standing releases 3P
    CHECK(camera.Update(1,32,16,true,false)); // later ordinary fall also uses 3P
    CHECK(camera.Update(1,37,20,true,false)); // pierced nonlethal KO
    CHECK(!camera.Update(2,0,2,false,false)); // new actor/save cannot inherit KO
    CHECK(camera.Update(2,35,18,true,true)); // real death also uses native 3P
    CHECK(camera.Update(2,36,19,true,true));
    camera.Reset();CHECK(!camera.Update(2,0,2,false,false));
    CHECK(camera.Update(2,33,17,true,false));CHECK(!camera.Update(0,0,0,false,false));CHECK(!camera.Update(2,0,2,false,false));
    // Every stereo/head/camera-permission decision shares this exception.
    StereoCameraState state{true,true,true,false,false,false,false};
    state.activity=camera.Update(2,33,17,true,false);CHECK(!ComfortFirstPerson(state,true,true,false));
    CHECK(CameraMode(state)==StereoCameraMode::NativeOther&&StereoCameraAllowed(state,true));
    CHECK(!HideHeadInCamera(state)); // even if native FP flag has not updated yet
    int originalCalls=0;auto original=[&](int*,char const* const&,int*,int*,int){++originalCalls;return 0;};
    auto permit=[&](char const* name,int*){return IsFirstPersonPermission(name)&&!state.activity;};
    int admin=0,actor=0;char const* name="IsFirstPersonCameraAllowed";
    CHECK(CallCameraScript(original,&admin,name,&actor,&actor,0,permit)==0&&originalCalls==1);
    state.activity=camera.Update(2,0,2,false,false);CHECK(ComfortFirstPerson(state,true,true,false));
    CHECK(CallCameraScript(original,&admin,name,&actor,&actor,0,permit)==1&&originalCalls==2);
    puts("PASS all native knockdown/KO/death states use third-person with visible head, get-up completion, calibrated first-person return and save lifecycle resets");return 0;
}
