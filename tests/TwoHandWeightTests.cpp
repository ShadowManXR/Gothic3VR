#include "TwoHandWeight.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    melee::TwoHandWeight weight;ControllerPose pose;pose.valid=pose.tracked=true;pose.quaternion[3]=1;
    CHECK(weight.Step(1,1,1000,pose,false));pose.quaternion[1]=std::sin(.75f);pose.quaternion[3]=std::cos(.75f);
    auto raw=pose;CHECK(weight.Step(1,1,1011,pose,false));CHECK(melee::Angle(weight.Rotation(),Orientation{})>.03f&&melee::Angle(weight.Rotation(),Orientation{})<.04f);
    auto first=weight.Rotation();CHECK(weight.Step(1,1,1011,pose,false));CHECK(melee::Angle(first,weight.Rotation())<.001f);
    pose=raw;CHECK(weight.Step(1,1,1022,pose,true));CHECK(melee::Angle(weight.Rotation(),{raw.quaternion[3],0,raw.quaternion[1],0})<.001f);
    pose=raw;CHECK(weight.Step(1,2,1033,pose,false));CHECK(melee::Angle(weight.Rotation(),{raw.quaternion[3],0,raw.quaternion[1],0})<.001f);
    arms::Mat mesh,controller,weighted,out;arms::Position(mesh,{500,120,300});arms::Position(controller,{500,120,300});weighted=arms::Rotation(Matrix({.9238795f,0,.3826834f,0}));arms::Position(weighted,{500,120,300});
    arms::Vec local{2,3,4},palm=arms::Point(mesh,local);CHECK(melee::WeightedMesh(mesh,controller,weighted,palm,out));CHECK(arms::Length(arms::Point(out,local)-palm)<.001f);
    // Identical tracking in a moving world keeps the same relative weapon pose.
    auto before=out;arms::Vec walk{70,0,50};arms::Position(mesh,arms::Position(mesh)+walk);arms::Position(controller,arms::Position(controller)+walk);arms::Position(weighted,arms::Position(weighted)+walk);
    CHECK(melee::WeightedMesh(mesh,controller,weighted,palm+walk,out));CHECK(arms::Length(arms::Position(out)-arms::Position(before)-walk)<.001f);
    CHECK(melee::UnsupportedDamageFraction(1,true)==.2f&&melee::UnsupportedDamageFraction(1,false)==1);
    CHECK(melee::UnsupportedHitReady(1000,1000,true)&&!melee::UnsupportedHitReady(1000,1001,true)&&melee::UnsupportedHitReady(1000,9999,false));
    CHECK(melee::UnsupportedNextHit(1000)==1900);pose.tracked=false;CHECK(!weight.Step(1,1,1044,pose,false));
    puts("PASS 2H one-hand inertia, actual support release, duplicate samples, world movement, palm anchor and spam restriction");return 0;
}
