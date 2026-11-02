#include "PhysicalMelee.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
melee::Sample Pose(uint64_t time,float x=0){
    melee::Sample s;s.player=1;s.weapon=2;s.session=3;s.time=time;s.valid=true;
    s.relativeHand={x,0,0};s.trackingHand={x,0,0};s.hand={x*100,100,0};s.blade={{x*100,100,5},{x*100,100,105}};return s;
}
void Settle(melee::Swing& swing,uint64_t& time){for(unsigned i=0;i<36;++i){swing.Step(Pose(time));time+=10;}}
int main(){
    melee::Blade b;
    CHECK(melee::LocalBlade({-2,-10,-2},{2,90,2},100,b));CHECK(b.tip.y==90&&b.root.y>0);
    CHECK(melee::LocalBlade({-110,-2,-2},{10,2,2},100,b));CHECK(b.tip.x==-110);
    CHECK(!melee::LocalBlade({0,0,0},{0,0,0},100,b));CHECK(!melee::LocalBlade({0,0,0},{0,400,0},100,b));
    CHECK(melee::ScaledDamage(120,80,1)==120);CHECK(melee::ScaledDamage(40,80,.5f)==20);
    CHECK(melee::ScaledDamage(0,80,1)==0);CHECK(melee::ScaledDamage(80,80,INFINITY)==0);
    CHECK(melee::ScaledDamage(120,80,9)==120);CHECK(melee::ScaledDamage(60,80,1)==60);
    CHECK(melee::ScaledDamage(170,80,.5f)==85);CHECK(melee::ScaledDamage(2147483647,80,1)==2147483647);
    CHECK(melee::BlockAttack(true,false,true,15)&&melee::BlockAttack(true,false,true,16));
    CHECK(!melee::BlockAttack(false,false,true,15)&&!melee::BlockAttack(true,true,true,15)&&!melee::BlockAttack(true,false,false,15));
    CHECK(!melee::BlockAttack(true,false,true,20));
    CHECK(!melee::BlockAttack(true,false,true,15,true)&&!melee::BlockAttack(true,false,true,16,true));
    for(int action=29;action<=33;++action)CHECK(melee::RecoveryState(action,0,false,false));
    for(int animation=15;animation<=17;++animation)CHECK(melee::RecoveryState(0,animation,false,false));
    CHECK(!melee::RecoveryState(0,0,false,false)&&melee::RecoveryState(0,0,true,false)&&melee::RecoveryState(0,0,false,true));
    melee::Swing swing;uint64_t time=1000;Settle(swing,time);
    // Common room/joystick movement never creates swing energy.
    for(int i=0;i<100;++i){auto s=Pose(time);s.hand.x=float(i)*3;s.blade.root.x=s.blade.tip.x=s.hand.x;CHECK(!swing.Step(s));time+=10;}
    swing.Reset();Settle(swing,time);
    for(int i=0;i<40;++i){auto s=Pose(time,-.005f*i);s.trackingHand={};CHECK(!swing.Step(s));time+=10;} // Head-only motion.
    swing.Reset();Settle(swing,time);
    for(int i=0;i<60;++i){CHECK(!swing.Step(Pose(time,(i%2)*.0005f)));time+=10;}
    swing.Reset();Settle(swing,time);
    bool weak=false;float weakDamage=0;
    for(int i=1;i<=8;++i){weak|=swing.Step(Pose(time,.005f*i));weakDamage=swing.intensity;time+=10;}
    CHECK(weak&&weakDamage>0&&weakDamage<.5f);CHECK(swing.Hit(7)&&!swing.Hit(7)&&swing.Hit(8));
    CHECK(!swing.Step(Pose(time-10,.04f))); // Same controller sample / second eye.
    for(int i=9;i<=72;++i){swing.Step(Pose(time,.005f*i));time+=10;}
    CHECK(swing.intensity>.99f);CHECK(!swing.Hit(7));
    float x=.36f;for(int i=0;i<18;++i){swing.Step(Pose(time,x));time+=10;}
    bool second=false;for(int i=1;i<=8;++i){second|=swing.Step(Pose(time,x+.005f*i));time+=10;}CHECK(second&&swing.Hit(7));
    // Discontinuities and weapon/session changes must settle before hitting.
    CHECK(!swing.Step(Pose(time,1.5f)));time+=10;auto changed=Pose(time,1.5f);changed.weapon=99;CHECK(!swing.Step(changed));
    changed.time+=10;changed.session=77;CHECK(!swing.Step(changed));
    changed.time+=10;changed.valid=false;CHECK(!swing.Step(changed));CHECK(!swing.Hit(7));
    melee::Blade old{{-20,0,0},{-20,100,0}},now{{20,0,0},{20,100,0}};
    auto sweep=melee::BladeSweep(old,now,100);CHECK(sweep.count>20&&sweep.count<=96);
    bool crossing=false;for(unsigned i=0;i<sweep.count;++i){auto a=sweep.segments[i].a,c=sweep.segments[i].b;if(a.x<0&&c.x>0&&a.y>=45&&a.y<=55)crossing=true;}
    CHECK(crossing); // Target crossed entirely between two frames.
    // Capsule endpoints equal the actual blade on every axis, including inside a target.
    for(auto direction:{arms::Vec{80,0,0},arms::Vec{0,80,0},arms::Vec{0,0,80},arms::Vec{30,-40,50}}){
        melee::Blade blade{{3,4,5},arms::Vec{3,4,5}+direction};melee::BladeCapsule shape;
        CHECK(melee::Capsule(blade,100,shape)&&std::fabs(shape.radius-2)<.001f);
        // Engine CollectEntitiesInCapsule halves the SDK's full height.
        auto axis=arms::Vector(shape.orientation,{0,melee::NativeCapsuleHeight(shape)*.5f,0});
        CHECK(arms::Length(shape.center-axis-blade.root)<.001f&&arms::Length(shape.center+axis-blade.tip)<.001f);
        CHECK(arms::Length(melee::ClosestPoint(blade,shape.center)-shape.center)<.001f);
    }
    CHECK(melee::CapsuleSteps(old,now,100)>1&&melee::CapsuleSteps(old,now,0)==0);
    melee::Blade enclosed{{-5,0,0},{5,0,0}};melee::BladeCapsule enclosedShape;CHECK(melee::Capsule(enclosed,100,enclosedShape));
    CHECK(arms::Length(melee::ClosestPoint(enclosed,{0,1,0})-arms::Vec{0,1,0})<enclosedShape.radius); // Both ray origins are inside a close collider.
    // Pitch/yaw/roll-only strokes must rearm on angular reversal without hand translation.
    for(unsigned axis=0;axis<3;++axis){
        swing.Reset();Settle(swing,time);bool hit=false;unsigned generation=0;
        for(int i=1;i<=30;++i){auto p=Pose(time);float angle=.03f*i;p.orientation={std::cos(angle*.5f),axis==0?std::sin(angle*.5f):0,axis==1?std::sin(angle*.5f):0,axis==2?std::sin(angle*.5f):0};hit|=swing.Step(p);time+=10;}
        CHECK(hit&&swing.Hit(123));generation=swing.generation;hit=false;
        for(int i=29;i>=20;--i){auto p=Pose(time);float angle=.03f*i;p.orientation={std::cos(angle*.5f),axis==0?std::sin(angle*.5f):0,axis==1?std::sin(angle*.5f):0,axis==2?std::sin(angle*.5f):0};hit|=swing.Step(p);time+=10;}
        CHECK(hit&&swing.generation>generation&&swing.Hit(123));
    }
    // Each translated axis has the same thresholds; vertical motion is not flattened.
    for(unsigned axis=0;axis<3;++axis){swing.Reset();Settle(swing,time);bool hit=false;
        for(int i=1;i<=12;++i){auto p=Pose(time);arms::Vec delta{axis==0?.007f*i:0,axis==1?.007f*i:0,axis==2?.007f*i:0};p.relativeHand=p.trackingHand=delta;p.hand=arms::Vec{0,100,0}+delta*100;p.blade.root=p.hand;p.blade.tip=p.hand+arms::Vec{0,0,80};hit|=swing.Step(p);time+=10;}CHECK(hit&&swing.Hit(99));}
    // Real attachment orientations and hand calibration must all aim the mesh forward.
    int swordAngles[3]{};melee::Blade local{{0,0,0},{0,0,80}};
    for(auto nativeAxis:{arms::Vec{0,1,0},arms::Vec{0,-1,0},arms::Vec{1,0,0},arms::Vec{0,0,-1}}){
        arms::Mat attachment,handCalibration,grip,forwardGrip;CHECK(arms::Between({0,0,1},nativeAxis,attachment));
        float handAngles[3]{.35f,.15f,-.2f};handCalibration=arms::Rotation(NativeCameraRotation(handAngles));
        arms::Position(grip,{23,120,45});CHECK(melee::SwordGrip(attachment,handCalibration,local,grip,swordAngles,forwardGrip));
        auto displayed=arms::Multiply(arms::Multiply(attachment,handCalibration),forwardGrip);auto direction=arms::Vector(displayed,local.tip-local.root);CHECK(arms::Unit(direction));
        CHECK(arms::Length(direction-arms::Vec{0,0,1})<.001f&&arms::Length(arms::Position(forwardGrip)-arms::Position(grip))<.001f);
    }
    swordAngles[0]=-25;arms::Mat pitched;CHECK(melee::SwordGrip(arms::Mat{},arms::Mat{},local,arms::Mat{},swordAngles,pitched));CHECK(arms::Vector(pitched,{0,0,1}).y<-.4f);
    puts("PASS blade pivots, light/full capped damage, click/menu gates, room motion, jitter, swing deduplication, rest rearm, tracking changes and swept contacts");return 0;
}
