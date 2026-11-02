#include "MagicAim.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    ControllerFrame f;f.focused=true;f.timeMs=1000;auto& hand=f.hands[1];hand.active=true;
    hand.grip.valid=hand.grip.tracked=hand.aim.valid=hand.aim.tracked=true;hand.grip.position[0]=.3f;hand.grip.position[1]=1.2f;hand.grip.position[2]=-.5f;hand.aim=hand.grip;
    HeadPose head;head.valid=true;head.session=1;float base[3]{100,200,300},center[3]{0,1.6f,0};int calibration[6]{};
    magic::Aim aim;CHECK(aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1000));
    arms::Vec origin,direction;float separation;CHECK(bow::Aim(aim.sample,origin,direction,separation));
    CHECK(arms::Length(origin-arms::Vec{130,135,350})<.001f&&arms::Dot(direction,{0,0,1})>.999f);
    // Locomotion must translate the filtered ray immediately; smoothing acts
    // in tracking space, and does not trail the game/world origin.
    base[0]+=500;f.timeMs+=11;CHECK(aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1011));
    CHECK(arms::Length(aim.sample.launchNock-origin-arms::Vec{500,0,0})<.001f);
    auto raw=hand.aim;hand.aim.quaternion[1]=.004f;hand.aim.quaternion[3]=1;f.timeMs+=11;
    CHECK(aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1022));
    CHECK(std::fabs(aim.sample.leftAim.m[2])<.008f);
    f.timeMs+=11;head.session++;CHECK(aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,0,1033));
    CHECK(std::fabs(aim.sample.leftAim.m[2])>.007f);
    CHECK(!aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1134)&&!aim.sample.valid);
    hand.aim=raw;f.timeMs=1200;CHECK(!aim.Prepare(1,0,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1200));
    hand.grip.tracked=false;CHECK(!aim.Prepare(1,2,f,head,Rotation3{},base,Orientation{},center,100,-25,calibration,80,1200));
    aim.Reset();CHECK(!aim.sample.valid);puts("PASS magic stabilized right-controller launch/focus/reticle aim, locomotion, reset and tracking gates");return 0;
}
