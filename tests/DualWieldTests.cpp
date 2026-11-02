#include "DualWield.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
melee::Sample Pose(unsigned hand,uint64_t time,float x=0){
    melee::Sample s;s.player=10;s.weapon=hand?21:20;s.session=1;s.time=time;s.valid=true;
    s.trackingHand=s.relativeHand={x,0,0};s.hand={x*100,100,0};s.blade={s.hand,s.hand+arms::Vec{0,0,80}};return s;
}
int main(){
    CHECK(melee::DualWieldTypes(2,2,21,20));
    CHECK(!melee::DualWieldTypes(2,9,21,20)&&!melee::DualWieldTypes(3,2,21,20));
    CHECK(!melee::DualWieldTypes(2,2,21,21)&&!melee::DualWieldTypes(2,2,21,0));
    // Different native left/right attachments must follow independent tracked
    // hands. Spatial pose before culling equals the eventual palette pose.
    melee::OneHandPose poses[2];int angles[3]{-25,5,0};
    for(unsigned h=0;h<2;++h){
        arms::Mat nativeHand,nativeMesh,attachment,calibration,grip;
        float r[3]{h?.3f:-.4f,h?.2f:-.1f,.6f};nativeHand=arms::Rotation(NativeCameraRotation(r));arms::Position(nativeHand,{h?30.f:-30.f,100,10});
        CHECK(arms::Between({0,0,1},h?arms::Vec{0,1,0}:arms::Vec{0,-1,0},attachment));arms::Position(attachment,{2,3,4});
        nativeMesh=arms::Multiply(attachment,nativeHand);
        float c[3]{.2f,-.3f,h?.1f:-.1f};calibration=arms::Rotation(NativeCameraRotation(c));
        arms::Position(grip,{h?25.f:-25.f,125,45});
        melee::Blade blade{{0,0,0},{0,0,h?80.f:60.f}};
        CHECK(poses[h].Prepare(nativeMesh,nativeHand,calibration,blade,grip,angles,100,{0,-4.7f,0}));
        auto eventual=poses[h].AtWrist(arms::WristAtPalm(poses[h].grip,calibration,{0,-4.7f,0}));
        for(int i=0;i<16;++i)CHECK(std::fabs(eventual.m[i]-poses[h].mesh.m[i])<.001f);
        auto direction=arms::Vector(poses[h].mesh,{0,0,1});CHECK(arms::Unit(direction));
        CHECK(direction.y<-.4f&&direction.z>.8f);
        auto before=poses[h].mesh;auto shifted=poses[h].AtWrist(arms::WristAtPalm(poses[h].grip,calibration,{0,-4.7f,0})+arms::Vec{150,0,60});
        CHECK(arms::Length(arms::Position(shifted)-arms::Position(before)-arms::Vec{150,0,60})<.001f);
    }
    auto rightBefore=poses[1].mesh;arms::Mat leftHand,leftMesh,leftGrip;
    arms::Position(leftGrip,{-45,155,70});CHECK(poses[0].Prepare(leftMesh,leftHand,arms::Mat{},{{0,0,0},{0,70,0}},leftGrip,angles,100,{0,-4.7f,0}));
    for(int i=0;i<16;++i)CHECK(rightBefore.m[i]==poses[1].mesh.m[i]);
    melee::CombatChannel channels[2];uint64_t t=1000;
    for(int i=0;i<36;++i,t+=10)for(unsigned h=0;h<2;++h)CHECK(!channels[h].Step(Pose(h,t),10,h?21:20,true,t));
    bool left=false,right=false;
    for(int i=1;i<=12;++i,t+=10){left|=channels[0].Step(Pose(0,t,.007f*i),10,20,true,t);right|=channels[1].Step(Pose(1,t),10,21,true,t);}
    CHECK(left&&!right&&channels[0].swing.Hit(99)&&!channels[0].swing.Hit(99)&&!channels[1].swing.Hit(99));
    // Simultaneous samples do not consume the other hand; each weapon can hit
    // the same target once, with its own damage maximum and stroke intensity.
    for(auto& channel:channels)channel.Reset();
    for(int i=0;i<36;++i,t+=10)for(unsigned h=0;h<2;++h)channels[h].Step(Pose(h,t),10,h?21:20,true,t);
    left=right=false;
    for(int i=1;i<=12;++i,t+=10){left|=channels[0].Step(Pose(0,t,.007f*i),10,20,true,t);right|=channels[1].Step(Pose(1,t,-.009f*i),10,21,true,t);}
    CHECK(left&&right&&channels[0].swing.Hit(99)&&channels[1].swing.Hit(99));
    CHECK(!channels[0].swing.Hit(99)&&!channels[1].swing.Hit(99));
    CHECK(melee::ScaledDamage(300,40,channels[0].swing.intensity)<=300);
    CHECK(melee::ScaledDamage(300,90,channels[1].swing.intensity)<=300);
    CHECK(melee::ScaledDamage(300,40,1)==300&&melee::ScaledDamage(300,90,1)==300);
    CHECK(!channels[1].Step(Pose(1,t-10,-.108f),10,21,true,t));CHECK(!channels[1].swing.Hit(99));
    auto generation=channels[1].swing.generation;
    // Left tracking loss and same-timestamp replacement reset only the left.
    CHECK(!channels[0].Step(Pose(0,t),10,20,false,t));CHECK(!channels[0].swing.Hit(99));
    CHECK(channels[1].swing.generation==generation&&channels[1].swing.AlreadyHit(99));
    auto replacement=Pose(0,t);replacement.weapon=22;CHECK(!channels[0].Step(replacement,10,22,true,t));
    CHECK(!channels[0].Step(replacement,10,22,true,t));
    auto stale=Pose(1,t);CHECK(!channels[1].Step(stale,10,21,true,t+101));CHECK(!channels[1].swing.Hit(99));
    for(unsigned h=0;h<2;++h){channels[h].Step(Pose(h,t+200),10,h?21:20,false,t+200);CHECK(channels[h].weapon==0);}
    puts("PASS dual wield independent spatial poses, left/right strokes, simultaneous target hits, per-weapon caps, duplicate eyes, tracking/equipment/menu resets");return 0;
}
