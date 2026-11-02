#include "PhysicalBow.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
struct Projectile {float speed=0;void SetShootVelocity(float v){speed=v;}};
struct Damage {float multiplier=0;void SetDamageHitMultiplier(float v){multiplier=v;}};
bow::Sample Pose(uint64_t time,float distance,bool trigger){
    bow::Sample s;s.player=1;s.weapon=2;s.session=3;s.time=time;s.valid=s.rightTracked=true;s.trigger=trigger;
    arms::Position(s.left,{0,140,100});arms::Position(s.right,{0,140,100-distance*100});s.trackingLeft={0,0,-1};s.trackingRight={0,0,-1+distance};return s;
}
int main(){
    bow::ChargeProfile native{1.9500000476837158,.20000000298023224,.800000011920929,200.};
    CHECK(native.Valid());CHECK(std::fabs(bow::DrawSpeed(1)-1.4)<.0001);CHECK(std::fabs(bow::DrawSpeed(2)-2.1)<.0001);CHECK(bow::ChargeSeconds(3,native.seconds)==0);Projectile arrow;Damage damage;
    // The first arrow, fully charged native leftovers, and an invalid inherited
    // velocity must all launch identically for the same VR release.
    for(float old:{0.f,40.f,200.f,9000.f,NAN}){arrow.speed=old;damage.multiplier=old;
        CHECK(bow::ApplyArrowCharge(arrow,damage,native,.5f));CHECK(std::fabs(arrow.speed-120)<.001f&&std::fabs(damage.multiplier-.6f)<.001f);}
    for(unsigned i=1;i<=4;++i){float p=float(i)/4;CHECK(bow::ApplyArrowCharge(arrow,damage,native,p));CHECK(std::fabs(arrow.speed-(40+160*p))<.001f);}
    auto saved=arrow.speed;CHECK(!bow::ApplyArrowCharge(arrow,damage,native,NAN)&&arrow.speed==saved);
    // The native charge curve is applied once; full draw retains full damage.
    CHECK(bow::ApplyArrowCharge(arrow,damage,native,1));CHECK(std::fabs(damage.multiplier-1)<.001f&&std::fabs(arrow.speed-200)<.001f);
    bow::Charge charge;float tierPower[3]{};
    for(int tier=1;tier<=3;++tier){charge.Reset();for(int i=0;i<50;++i)tierPower[tier-1]=charge.Step(1,.01,bow::ChargeSeconds(tier,native.seconds));}
    CHECK(tierPower[0]<tierPower[1]&&tierPower[1]<tierPower[2]);
    charge.Reset();for(int i=0;i<400;++i)CHECK(charge.Step(.3f,.01,native.seconds)<=.300001f);
    charge.Reset();CHECK(charge.Step(1,0,native.seconds)==0);CHECK(charge.Step(1,-1,native.seconds)==0);
    // Integration: physically identical pulls produce tier-dependent charge;
    // duplicate samples cannot add charge. Menu/origin loss cancels without fire.
    float released[3]{};
    for(int tier=1;tier<=3;++tier){bow::Draw draw;uint64_t time=1000;double duration=bow::ChargeSeconds(tier,native.seconds);
        draw.Step(Pose(time,.05f,false),true,time,.5f,.2f,duration,.01);time+=10;
        draw.Step(Pose(time,.05f,true),true,time,.5f,.2f,duration,.01);time+=10;
        for(int i=1;i<=50;++i){auto sample=Pose(time,.05f+.5f*std::min(1.f,i/30.f),true);draw.Step(sample,true,time,.5f,.2f,duration,.01);
            float before=draw.power;draw.Step(sample,true,time,.5f,.2f,duration,1);CHECK(draw.power==before);time+=10;}
        auto shot=draw.Step(Pose(time,.55f,false),true,time,.5f,.2f,duration,.01);CHECK(shot.fire);released[tier-1]=shot.power;
        draw.Cancel();CHECK(draw.power==0&&!draw.pulling);time+=500;
        draw.Step(Pose(time,.05f,false),true,time,.5f,.2f,duration,.01);time+=10;
        draw.Step(Pose(time,.05f,true),true,time,.5f,.2f,duration,.01);time+=10;
        CHECK(!draw.Step(Pose(time,.3f,true),false,time,.5f,.2f,duration,.01).fire&&!draw.pulling);
    }
    CHECK(released[0]<released[1]&&released[1]<released[2]&&released[2]==1);
    charge.Reset();CHECK(charge.Step(.25f,0,bow::ChargeSeconds(3,native.seconds))==.25f);CHECK(charge.Step(1,0,bow::ChargeSeconds(3,native.seconds))==1);CHECK(charge.Step(.2f,0,bow::ChargeSeconds(3,native.seconds))==.2f);
    // Master draw/release can be fast without a 100 ms hold or 300 ms repeat
    // timer; physical draw, re-grab and fresh tracking remain mandatory.
    bow::Draw freeDraw;uint64_t fast=30000;
    for(unsigned shot=0;shot<3;++shot){
        freeDraw.Step(Pose(fast,.05f,false),true,fast,.5f,.2f,0,0,true);fast+=10;
        freeDraw.Step(Pose(fast,.05f,true),true,fast,.5f,.2f,0,0,true);fast+=10;
        freeDraw.Step(Pose(fast,.3f,true),true,fast,.5f,.2f,0,0,true);fast+=10;
        freeDraw.Step(Pose(fast,.55f,true),true,fast,.5f,.2f,0,0,true);CHECK(freeDraw.power==1);fast+=10;
        CHECK(freeDraw.Step(Pose(fast,.55f,false),true,fast,.5f,.2f,0,0,true).fire);fast+=10;
        freeDraw.Step(Pose(fast,.3f,false),true,fast,.5f,.2f,0,0,true);fast+=10;
    }
    // String extension is charge-limited, not raw physical hand separation.
    for(unsigned i=0;i<3;++i){CHECK(std::fabs(bow::VisibleDraw(released[i],27)-27*released[i])<.001f);CHECK(bow::VisibleDraw(released[i],27)<=27);}
    CHECK(bow::VisibleDraw(1,27)==27&&bow::VisibleDraw(2,27)==27&&bow::VisibleDraw(-1,27)==0&&bow::VisibleDraw(NAN,27)==0);
    charge.Reset();float visible=0;for(unsigned i=0;i<195;++i){auto next=bow::VisibleDraw(charge.Step(1,.01,1.95),27);CHECK(next>=visible&&next-visible<.14f);visible=next;}CHECK(std::fabs(visible-27)<.001f);
    puts("PASS first/repeated arrow initialization, linear native velocity/damage, tier draw charge and cancellation");return 0;
}
