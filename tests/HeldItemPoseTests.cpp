#include "HeldItemPose.h"
#include "TemporaryOverride.h"
#include "CastingGrip.h"
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    arms::Mat wrist,item,grip,calibration,posed,inverse;
    arms::Position(wrist,{500,190,300});arms::Position(item,{504,191,305});
    float angles[3]{.4f,-.7f,.2f};grip=arms::Rotation(NativeCameraRotation(angles));arms::Position(grip,{600,250,450});
    CHECK(arms::HeldItemPose(item,wrist,calibration,grip,100,posed));
    CHECK(arms::Inverse(wrist,inverse));auto attachment=arms::Multiply(item,inverse);
    auto target=arms::Multiply(calibration,grip);arms::Position(target,arms::Point(grip,{0,0,-8}));
    CHECK(arms::Inverse(target,inverse));auto actual=arms::Multiply(posed,inverse);
    for(unsigned i=0;i<16;++i)CHECK(std::fabs(actual.m[i]-attachment.m[i])<.0001f);
    auto before=posed;arms::Vec step{20,0,30};arms::Position(wrist,arms::Position(wrist)+step);arms::Position(item,arms::Position(item)+step);arms::Position(grip,arms::Position(grip)+step);
    CHECK(arms::HeldItemPose(item,wrist,calibration,grip,100,posed));CHECK(arms::Length(arms::Position(posed)-arms::Position(before)-step)<.001f);
    CHECK(!arms::HeldItemPose(item,wrist,calibration,grip,0,posed));
    // Idle and walking carry offsets differ natively. Neither may move the
    // visible scroll/rune centre away from the tracked palm.
    arms::CastingContact contact;arms::Vec minimum{-1,-7,-2},maximum{1,7,2},palm{0,-4,0};
    CHECK(contact.Prepare(1,item,wrist,minimum,maximum));CHECK(contact.Pose(target,palm,posed));
    CHECK(arms::Length(arms::Point(posed,(minimum+maximum)*.5f)-arms::Point(target,palm))<.001f);
    before=posed;arms::Position(item,arms::Position(item)+arms::Vec{40,-20,30});
    CHECK(contact.Prepare(1,item,wrist,minimum,maximum)&&contact.Pose(target,palm,posed));
    for(unsigned i=0;i<16;++i)CHECK(std::fabs(posed.m[i]-before.m[i])<.001f);
    arms::Position(target,arms::Position(target)+step);CHECK(contact.Pose(target,palm,posed));CHECK(arms::Length(arms::Position(posed)-arms::Position(before)-step)<.001f);
    CHECK(contact.Prepare(2,item,wrist,{-2,-2,-1},{2,2,1})&&contact.Pose(target,palm,posed));
    CHECK(arms::Length(arms::Point(posed,{0,0,0})-arms::Point(target,palm))<.001f);
    CHECK(!contact.Prepare(3,item,wrist,{2,0,0},{1,1,1})&&!contact.Pose(target,palm,posed));
    CHECK(contact.Prepare(4,item,wrist,minimum,maximum));auto handBefore=target;
    arms::Vec correction{4,2,-1};CHECK(contact.Pose(target,palm,posed,correction));
    CHECK(arms::Length(arms::Point(posed,(minimum+maximum)*.5f)-arms::Point(target,palm)-correction)<.001f);
    CHECK(!memcmp(&handBefore,&target,sizeof(target)));CHECK(!contact.Pose(target,palm,posed,{NAN,0,0}));
    // Finger chains close independently of the native idle/walk pose. Segment
    // lengths and base knuckles stay unchanged; curl mirrors between hands.
    for(int side=0;side<2;++side){arms::Mat local[3],closed[3];arms::Position(local[0],{0,-7,1});arms::Position(local[1],{0,-10,1});arms::Position(local[2],{0,-12,1});
        float sign=side?-1.f:1.f;CHECK(arms::CastingFinger(local,{sign,0,0},palm,false,closed));
        CHECK(arms::Length(arms::Position(closed[0])-arms::Position(local[0]))<.001f);
        CHECK(std::fabs(arms::Length(arms::Position(closed[1])-arms::Position(closed[0]))-3)<.001f);
        CHECK(std::fabs(arms::Length(arms::Position(closed[2])-arms::Position(closed[1]))-2)<.001f);
        CHECK(arms::Position(closed[2]).x*sign>0);
        for(auto const& finger:closed)CHECK(arms::Affine(finger));
        auto animated=local[1];arms::Position(animated,{10,10,10}); // walking cannot enter the bind-based solver
        CHECK(arms::CastingFinger(local,{sign,0,0},palm,false,closed));CHECK(arms::Length(arms::Position(closed[1])-arms::Position(animated))>1);
    }
    auto tag=arms::Identify("Hero_Right_Hand_Finger3_2");CHECK(tag.side==1&&tag.finger==2&&tag.segment==1);
    puts("PASS cast mesh palm contact under idle/walk, calibration/locomotion, and closed finger segment geometry");return 0;
}
