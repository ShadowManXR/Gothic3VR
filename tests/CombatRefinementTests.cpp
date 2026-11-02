#include "CombatRefinement.h"
#include "CombatGestures.h"
#include "StaffMelee.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    combat::StrokeDistance distance;combat::StrokeBudget stamina;unsigned spent=0;uint64_t t=1000;
    auto gate=[&](float x,unsigned generation=1){melee::Blade b{{x,0,0},{x,0,1}};bool ready=distance.Step(b,t,generation,true);t+=10;if(ready)stamina.Begin(generation,false,10,20,100,false,[&](int){++spent;return true;});return ready;};
    for(int i=0;i<200;++i)CHECK(!gate((i%2)*.02f));CHECK(spent==0);
    distance.Reset();stamina.Reset();CHECK(!gate(0,2));for(int i=1;i<=49;++i)CHECK(!gate(.01f*i,2));CHECK(spent==0);
    CHECK(gate(.51f,2)&&spent==1);CHECK(gate(.52f,2)&&spent==1);CHECK(!gate(.52f,3));
    // Vertical strokes qualify identically; no flattening or camera translation.
    distance.Reset();bool ready=false;for(int i=0;i<=52;++i,t+=10)ready|=distance.Step({{0,i*.01f,0},{0,i*.01f,1}},t,4,true);CHECK(ready);
    distance.Reset();for(int i=0;i<60;++i,t+=10)CHECK(!distance.Step({{0,0,0},{0,0,1}},t,5,true));
    // A wrist arc sweeps a real metre-long blade without translating the hand.
    distance.Reset();ready=false;for(int i=0;i<=60;++i,t+=10){float a=.01f*i;ready|=distance.Step({{0,0,0},{std::sin(a),0,std::cos(a)}},t,6,true);}CHECK(ready);
    distance.Reset();CHECK(!distance.Step({{0,0,0},{0,0,1}},t,7,true));t+=10;CHECK(!distance.Step({{2,0,0},{2,0,1}},t,7,true));t+=10;
    CHECK(!distance.Step({{0,0,0},{0,0,1}},t,8,true));CHECK(!distance.Step({{1,0,0},{1,0,1}},t,8,true)); // duplicate eye cannot accumulate
    // Endpoint conversion is independent of world origin/yaw and does not
    // require the other controller for a one-handed weapon.
    ControllerFrame frame;frame.hands[1].grip.valid=frame.hands[1].grip.tracked=true;frame.hands[1].grip.quaternion[3]=1;frame.hands[1].grip.position[0]=.2f;
    for(float yaw:{0.f,.9f,2.f}){float angles[3]{0,yaw,0};auto world=arms::Rotation(NativeCameraRotation(angles));arms::Position(world,{1000,150,-900});
        melee::Blade b{arms::Point(world,{0,0,0}),arms::Point(world,{0,0,100})},raw;CHECK(melee::TrackingWeaponBlade(frame,1,world,b,100,raw));CHECK(arms::Length(raw.root-arms::Vec{.2f,0,0})<.001f&&arms::Length(raw.tip-arms::Vec{.2f,0,-1})<.001f);}
    // Damage and debit thresholds can differ without per-contact charges.
    combat::StrokeBudget deferred;int cost=0;auto pay=[&](int amount){cost+=amount;return true;};
    CHECK(!deferred.Decide(1,false,false,false,10,20,100,false,pay)&&cost==0&&!deferred.ForStroke(1));
    CHECK(!deferred.Decide(1,false,false,true,10,20,100,false,pay)&&cost==10&&deferred.Paid());
    CHECK(deferred.Decide(1,false,true,true,10,20,100,false,pay)&&cost==10);
    CHECK(deferred.Decide(2,false,true,false,10,20,100,false,pay)&&cost==10&&!deferred.Paid());
    CHECK(deferred.Decide(2,false,true,true,10,20,100,false,pay)&&cost==20&&deferred.Paid());
    CHECK(!deferred.Decide(3,true,true,true,10,20,0,false,pay)&&cost==20);
    distance.Reset();for(int i=0;i<=31;++i,t+=10)distance.Step({{.01f*i,0,0},{.01f*i,0,1}},t,9,true);
    CHECK(distance.Ready(20)&&!distance.Ready(50));
    WorldAim aim;aim.origin[1]=130;aim.direction[1]=-1;float rating=0;float lo[3]{-30,0,-60},hi[3]{30,45,60};
    CHECK(combat::FinisherBounds(aim,lo,hi,100,rating));aim.direction[1]=0;aim.direction[2]=1;CHECK(!combat::FinisherBounds(aim,lo,hi,100,rating));
    distance.Reset();for(int i=0;i<=20;++i,t+=10)distance.Step({{0,0,i*.01f},{0,0,1+i*.01f}},t,10,true);CHECK(distance.Thrust());
    distance.Reset();for(int i=0;i<=20;++i,t+=10)distance.Step({{i*.01f,0,0},{i*.01f,0,1}},t,11,true);CHECK(!distance.Thrust());
    distance.Reset();for(int i=0;i<200;++i,t+=10)distance.Step({{0,0,(i%2)*.02f},{0,0,1+(i%2)*.02f}},t,12,true);CHECK(!distance.Thrust());
    float sight[3];aim.origin[1]=130;aim.direction[2]=0;aim.direction[1]=-1;CHECK(combat::FinisherBounds(aim,lo,hi,100,rating));
    CHECK(combat::FinisherSightPoint(aim,lo,hi,100,rating,sight)&&sight[1]>=15&&sight[1]<=45&&sight[0]>=lo[0]&&sight[0]<=hi[0]);
    hi[1]=4;CHECK(combat::FinisherSightPoint(aim,lo,hi,100,rating,sight)&&sight[1]>=2&&sight[1]<=4);
    puts("PASS 50 cm weapon-path gate, tiny-swing no stamina, wrist/vertical strokes, identity/jump/duplicate frames and downward finisher bounds");return 0;
}
