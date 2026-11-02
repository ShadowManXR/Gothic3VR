#include "BowStringGuidance.h"
#include "PhysicalBow.h"
#include <cstdio>
#include <cstring>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
arms::Vec Forward(arms::Mat const& p){return arms::Vector(p,{0,0,1});}
float Angle(arms::Vec d){return std::atan2(std::hypot(d.x,d.y),d.z);}
arms::Mat Settle(arms::Vec hand,int percent=20,bool drawing=true){
    bow::StringGuidance guide;arms::Mat result;
    for(unsigned i=0;i<70;++i){result={};if(!guide.Apply(result,{},hand,100,drawing,true,1000+i*11,1,2,percent,80))return {};}
    return result;
}
int main(){
    auto neutral=Settle({0,0,-50});CHECK(arms::Length(Forward(neutral)-arms::Vec{0,0,1})<.0001f);
    auto right=Settle({20,0,-50}),up=Settle({0,20,-50}),both=Settle({80,80,-50},40);
    CHECK(Forward(right).x<-.06f&&std::fabs(Forward(right).y)<.0001f);
    CHECK(Forward(up).y<-.06f&&std::fabs(Forward(up).x)<.0001f);
    CHECK(Angle(Forward(both))<=bow::StringGuidance::LimitRadians+.00001f&&Angle(Forward(both))>.10f);
    CHECK(Angle(Forward(Settle({20,0,-50},10)))<Angle(Forward(right)));
    CHECK(Angle(Forward(Settle({20,0,-50},0)))==0);
    CHECK(Angle(Forward(Settle({20,0,-50},20,false)))==0);
    CHECK(Angle(Forward(Settle({20,0,10})))==0&&Angle(Forward(Settle({20,0,-1})))==0);
    CHECK(arms::Length(arms::Position(right))==0);
    bow::StringGuidance guide;arms::Mat current;
    for(unsigned i=0;i<30;++i){current={};CHECK(guide.Apply(current,{}, {20,10,-50},100,true,true,1000+i*11,1,2,20,80));}
    auto duplicate=arms::Mat{};CHECK(guide.Apply(duplicate,{}, {20,10,-50},100,true,true,1319,1,2,20,80)&&!memcmp(current.m,duplicate.m,sizeof(current.m)));
    for(unsigned reason=0;reason<5;++reason){current={};uint64_t now=reason==0?1500:reason==1?900:1320;
        CHECK(guide.Apply(current,{}, {20,10,-50},100,true,reason!=4,now,reason==2?4:1,reason==3?9:2,20,80));CHECK(Angle(Forward(current))==0);}
    current={};CHECK(!guide.Apply(current,{}, {},NAN,true,true,2000,1,2,20,80));
    // World translation and native turns are common to both hands. They must
    // not be smoothed along with the relative string correction.
    for(float yaw:{-2.f,-.5f,1.5f}){
        float angles[3]{.4f,yaw,.2f};auto transform=arms::Rotation(NativeCameraRotation(angles));
        bow::StringGuidance world,local;
        for(unsigned i=0;i<70;++i){auto a=transform,b=arms::Mat{};arms::Vec p{50000.f+i*2.f,5500,-90000};arms::Position(a,p);
            CHECK(world.Apply(a,p,p+arms::Vector(transform,{20,10,-50}),100,true,true,2000+i*11,1,2,20,80));
            CHECK(local.Apply(b,{}, {20,10,-50},100,true,true,2000+i*11,1,2,20,80));
            CHECK(arms::Length(Forward(a)-arms::Vector(transform,Forward(b)))<.0002f&&arms::Length(arms::Position(a)-p)<.0001f);
        }
    }
    guide.Reset();double raw=0,filtered=0;
    for(unsigned i=0;i<100;++i){current={};float jitter=i%2?.25f:-.25f;CHECK(guide.Apply(current,{}, {jitter,0,-50},100,true,true,4000+i*11,1,2,20,80));
        if(i>5){raw+=std::atan2(std::fabs(jitter),50.f)*.2f;filtered+=Angle(Forward(current));}}
    CHECK(filtered<raw*.3);
    // Aiming changes must not create draw distance, alter native tier timing
    // or change draw power. Release uses the same last held direction as sight.
    for(bool lateralOnly:{false,true}){
        bow::Draw baseline,guided;bow::StringGuidance steering;bow::Shot plainShot,guidedShot;arms::Vec lastDirection;uint64_t time=7000;
        for(unsigned i=0;i<64;++i){bool trigger=i>0&&i<63;float travel=lateralOnly?0:std::min(.5f,float(i)*.01f);
            bow::Sample sample;sample.player=1;sample.weapon=2;sample.session=1;sample.time=time;sample.valid=sample.rightTracked=true;sample.trigger=trigger;sample.drawAimValid=true;
            arms::Position(sample.left,{0,140,100});sample.leftAim=sample.left;sample.drawAim=sample.leftAim;sample.nockLocal={0,13,-27};
            auto rest=bow::DrawRest(sample);arms::Position(sample.right,rest+arms::Vec{10,0,-5-travel*100});
            sample.trackingLeft={0,1,-1};sample.trackingRight={.1f,1,-1+travel};auto steered=sample;
            CHECK(steering.Apply(steered.leftAim,rest,arms::Position(steered.right),100,guided.pulling,true,time,1,2,20,80));
            sample.nativeNock=steered.nativeNock=true;sample.launchNock=bow::StringRest(sample);steered.launchNock=bow::StringRest(steered);
            sample.launchDirection=Forward(sample.leftAim);steered.launchDirection=Forward(steered.leftAim);
            plainShot=baseline.Step(sample,true,time,.5f,.2f,1.3,.011);guidedShot=guided.Step(steered,true,time,.5f,.2f,1.3,.011);
            CHECK(std::fabs(baseline.drawMeters-guided.drawMeters)<.00001f&&std::fabs(baseline.power-guided.power)<.00001f);
            if(i==62)lastDirection=steered.launchDirection;time+=11;
        }
        CHECK(plainShot.fire==guidedShot.fire);
        if(lateralOnly)CHECK(!guidedShot.fire);
        else CHECK(guidedShot.fire&&guidedShot.direction.x<-.01f&&arms::Length(guidedShot.direction-lastDirection)<.0001f&&std::fabs(plainShot.power-guidedShot.power)<.0001f);
    }
    puts("PASS bounded string-hand aim, fixed grip, near-string fade, tremor, world-turn invariance, resets, native charge and last-held projectile direction");return 0;
}
