#include "CrossbowHandling.h"
#include "BowAimFilter.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
arms::Vec Forward(ControllerPose const& p){return arms::Vector(arms::Rotation(Matrix({p.quaternion[3],p.quaternion[0],p.quaternion[1],p.quaternion[2]})),{0,0,-1});}
int main(){
    crossbow::SupportedGuidance support;uint64_t time=1000;auto aimSupport=[&](ControllerPose& p,arms::Mat const& l,arms::Mat const& r){time+=10;return support.Step(p,l,r,1,2,time);};
    arms::Mat left,right;arms::Position(left,{0,1,-.4f});arms::Position(right,{0,1,0});ControllerPose aim;
    CHECK(aimSupport(aim,left,right));CHECK(arms::Dot(Forward(aim),{0,0,-1})>.9999f);auto before=aim;
    float angle[3]{.8f,-1.f,.6f};right=arms::Rotation(NativeCameraRotation(angle));arms::Position(right,{0,1,0});
    CHECK(aimSupport(aim,left,right));for(unsigned i=0;i<4;++i)CHECK(std::fabs(aim.quaternion[i]-before.quaternion[i])<.00001f);
    arms::Position(left,{.3f,1,-.4f});CHECK(aimSupport(aim,left,right));CHECK(arms::Dot(Forward(aim),{.6f,0,-.8f})>.9999f);
    before=aim;auto step=arms::Vec{200,50,-300};arms::Position(left,arms::Position(left)+step);arms::Position(right,arms::Position(right)+step);
    CHECK(aimSupport(aim,left,right));CHECK(arms::Dot(Forward(aim),Forward(before))>.9999f);
    // Stable support orientation remains valid while pointing vertically.
    arms::Position(right,{0,0,0});arms::Position(left,{0,.4f,0});CHECK(aimSupport(aim,left,right));CHECK(arms::Dot(Forward(aim),{0,1,0})>.9999f);
    arms::Position(left,{0,.01f,0});CHECK(!aimSupport(aim,left,right));
    // A rolled support wrist at attachment preserves the original gun frame.
    support.Reset();float rolled[3]{0,0,.78539816f};left=arms::Rotation(NativeCameraRotation(rolled));arms::Position(left,{0,1,-.4f});right=arms::Mat{};arms::Position(right,{0,1,0});aim=ControllerPose{};
    CHECK(aimSupport(aim,left,right));CHECK(std::fabs(aim.quaternion[2])<.00001f&&std::fabs(aim.quaternion[3]-1)<.00001f);
    support.Reset();CHECK(aimSupport(aim,left,right));CHECK(std::fabs(aim.quaternion[2])<.00001f);
    // Closed loops return to the same pose without frame-history roll drift.
    auto initialLeft=left,initialRight=right;auto expected=aim;
    for(unsigned i=0;i<4000;++i){float t=float(i%100)*.06283185f;auto movedLeft=initialLeft;arms::Position(movedLeft,arms::Position(initialLeft)+arms::Vec{.15f*std::sin(t),.1f*std::cos(t)-.1f,0});auto raw=ControllerPose{};
        CHECK(support.Step(raw,movedLeft,initialRight,1,2,10000+i*10));}
    auto returned=ControllerPose{};CHECK(support.Step(returned,initialLeft,initialRight,1,2,51000));
    for(unsigned i=0;i<4;++i)CHECK(std::fabs(returned.quaternion[i]-expected.quaternion[i])<.00001f);
    // A slow frame must not recapture the primary-hand aim while support is held.
    auto moved=initialLeft;arms::Position(moved,arms::Position(initialLeft)+arms::Vec{.15f,.1f,0});auto immediate=ControllerPose{},delayed=ControllerPose{};
    CHECK(support.Step(immediate,moved,initialRight,1,2,51010));delayed.quaternion[1]=.5f;delayed.quaternion[3]=.8660254f;
    CHECK(support.Step(delayed,moved,initialRight,1,2,51310));for(unsigned i=0;i<4;++i)CHECK(std::fabs(immediate.quaternion[i]-delayed.quaternion[i])<.00001f);
    // Sway is visible, deterministic at a duplicated XR time, and broad enough
    // to make unsupported aim substantially less precise than supported aim.
    ControllerPose raw,a,b;CHECK(crossbow::OneHandSway(a,1000)&&crossbow::OneHandSway(b,1000));
    for(unsigned i=0;i<4;++i)CHECK(a.quaternion[i]==b.quaternion[i]);float spread=0;
    for(uint64_t t=1000;t<3000;t+=10){auto q=raw;CHECK(crossbow::OneHandSway(q,t));spread=std::max(spread,arms::Length(Forward(q)-Forward(raw)));}
    CHECK(spread>.07f);auto q=raw;CHECK(crossbow::OneHandSway(q,2000));CHECK(arms::Length(Forward(q)-Forward(a))>.01f);
    puts("PASS crossbow left support guidance, right wrist independence, tracking-space locomotion and visible one-hand sway");return 0;
}
