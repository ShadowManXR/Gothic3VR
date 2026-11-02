#include "ArmTwist.h"
#include "NaturalWrist.h"
#include "FrozenArmPalette.h"
#include "LockedMeleeAttachment.h"
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace diagnostic;using namespace diagnostic::arms;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Near(float a,float b,float epsilon=.0005f){return std::fabs(a-b)<=epsilon;}
bool Near(Vec a,Vec b,float epsilon=.004f){return Length(a-b)<=epsilon;}
bool Near(Mat const& a,Mat const& b,float epsilon=.0008f){for(unsigned i=0;i<16;++i)if(!Near(a.m[i],b.m[i],epsilon))return false;return true;}
Mat Roll(Vec axis,float angle){Unit(axis);float half=angle*.5f;return Rotation(Matrix({std::cos(half),axis.x*std::sin(half),axis.y*std::sin(half),axis.z*std::sin(half)}));}
float RotationDifference(Mat const& a,Mat const& b){float out=0;for(unsigned i:{0u,1u,2u,4u,5u,6u,8u,9u,10u})out=std::max(out,std::fabs(a.m[i]-b.m[i]));return out;}
int main(){
    CHECK(FreezeTrackedHero(true,false,false,false)&&!FreezeTrackedHero(false,false,false,false));
    CHECK(FreezeTrackedHero(false,true,false,false)&&FreezeTrackedHero(false,false,true,false)&&FreezeTrackedHero(false,false,false,true));
    // Static small elbow drop: no wrist-tilt-driven compensation or shoulder
    // translation. Exact wrist/palm and original progressive axial twist remain.
    for(float units:{50.f,100.f,200.f})for(float yaw:{0.f,.8f,2.7f})for(float side:{-1.f,1.f}){
        Mat world=Roll({0,1,0},yaw);Position(world,{40000,130,-25000});
        Mat upper=world,lower=world,hand=world;
        Position(upper,Point(world,{side*.18f*units,.35f*units,0}));
        Position(lower,Point(world,{side*.18f*units,0,.15f*units}));
        Position(hand,Point(world,{side*.18f*units,0,.55f*units}));
        for(float tilt:{-120.f,-60.f,0.f,60.f,120.f}){
            Solution solved{Position(lower),Position(hand)};auto before=solved;
            CHECK(LowerTrackedForearm(units,solved));
            CHECK(Near(solved.elbow.x,before.elbow.x)&&Near(solved.elbow.z,before.elbow.z));
            CHECK(Near(before.elbow.y-solved.elbow.y,.03f*units,.01f)&&Near(solved.wrist,before.wrist));
            Mat du,dl;CHECK(TrackedJointDelta(upper,Position(lower),Position(upper),solved.elbow,du));
            CHECK(TrackedJointDelta(lower,Position(hand),solved.elbow,solved.wrist,dl));
            Mat target=Multiply(Roll({1,0,0},tilt*ArmDegree),world);Position(target,solved.wrist);auto exact=target;
            ArmTwistChain chain;TwistAngles distributed;
            CHECK(ProgressiveArmTwist(upper,lower,upper,lower,hand,target,solved.elbow,solved.wrist,du,dl,chain,1,2,10000,distributed));
            CHECK(Near(Point(du,Position(upper)),Position(upper),.05f));
            CHECK(Near(Point(du,Position(lower)),solved.elbow,.05f));
            CHECK(Near(Point(dl,Position(lower)),solved.elbow,.05f));
            CHECK(Near(Point(dl,Position(hand)),solved.wrist,.05f)&&Near(target,exact));
        }
    }
    Solution shortArm{{0,0,0},{0,0,1}};CHECK(LowerTrackedForearm(100,shortArm));CHECK(Near(shortArm.elbow.y,-.1f));
    Solution invalid{{0,0,0},{0,0,0}};CHECK(!LowerTrackedForearm(100,invalid));
    invalid={{0,0,0},{0,0,25}};CHECK(!LowerTrackedForearm(0,invalid)&&!LowerTrackedForearm(std::numeric_limits<float>::quiet_NaN(),invalid));
    for(float sign:{-1.f,1.f}){
        auto slight=DistributeTwist(sign*8*ArmDegree),medium=DistributeTwist(sign*55*ArmDegree),large=DistributeTwist(sign*160*ArmDegree);
        CHECK(Near(slight.wrist,sign*8*ArmDegree)&&Near(slight.forearm,0)&&Near(slight.upper,0));
        CHECK(std::fabs(medium.wrist)<22*ArmDegree&&std::fabs(medium.forearm)>30*ArmDegree&&Near(medium.upper,0));
        CHECK(std::fabs(large.wrist)<=22*ArmDegree&&std::fabs(large.forearm)<85*ArmDegree&&std::fabs(large.upper)>50*ArmDegree);
        for(float degrees=0;degrees<=300;degrees+=.25f){auto a=DistributeTwist(sign*degrees*ArmDegree),b=DistributeTwist(sign*(degrees+.001f)*ArmDegree);
            CHECK(Near(a.wrist+a.forearm+a.upper,sign*degrees*ArmDegree));
            CHECK(std::fabs(b.wrist-a.wrist)<.0001f&&std::fabs(b.forearm-a.forearm)<.0001f&&std::fabs(b.upper-a.upper)<.0001f);
        }
    }
    ArmTwistChain wrap;TwistAngles angles;uint64_t time=1000;float previousUpper=0,previousLower=0;
    for(int i=0;i<=720;++i){float continuous=float(i)*ArmDegree,wrapped=std::atan2(std::sin(continuous),std::cos(continuous));
        CHECK(wrap.Step(wrapped,true,1,2,time,angles));
        CHECK(Near(angles.wrist+angles.forearm+angles.upper,continuous,.001f));
        CHECK(std::fabs(angles.upper-previousUpper)<1.1f*ArmDegree&&std::fabs(angles.forearm+angles.upper-previousLower)<1.1f*ArmDegree);
        previousUpper=angles.upper;previousLower=angles.forearm+angles.upper;time+=10;
    }
    CHECK(wrap.Step(0,false,1,2,time,angles));CHECK(Near(angles.wrist+angles.forearm+angles.upper,4*ArmPi,.001f));
    CHECK(wrap.Step(0,true,1,2,time+200,angles)&&Near(angles.upper,0));
    CHECK(wrap.Step(170*ArmDegree,true,1,2,time+210,angles));CHECK(wrap.Step(0,true,1,3,time+220,angles)&&Near(angles.upper,0));
    CHECK(wrap.Step(170*ArmDegree,true,1,3,time+230,angles));CHECK(wrap.Step(0,true,9,3,time+240,angles)&&Near(angles.upper,0));
    CHECK(wrap.Step(170*ArmDegree,true,9,3,time+250,angles));CHECK(wrap.Step(0,true,9,3,10,angles)&&Near(angles.upper,0));
    float nan=std::numeric_limits<float>::quiet_NaN();CHECK(!wrap.Step(nan,true,9,3,20,angles));
    // Left/right, bent/fully extended arms and moving body transforms. Native
    // idle forearm roll must not alter the controller's progressive chain.
    for(float sign:{-1.f,1.f})for(float reach:{35.f,55.f,95.f})for(float bodyYaw:{0.f,.8f,2.1f}){
        Mat body=Roll({0,1,0},bodyYaw);Position(body,{40000,130,-25000});
        Mat refUpper=body,refLower=body,refHand=body;
        Position(refUpper,Point(body,{sign*18,0,0}));Position(refLower,Point(body,{sign*18,0,30}));Position(refHand,Point(body,{sign*18,0,55}));
        Mat upper=refUpper,lower=Multiply(Roll({0,0,1},.55f),refLower);Position(lower,Position(refLower));
        Vec target=Point(body,{sign*18+sign*reach*.1f,12,reach});Solution solution;
        CHECK(SolveTracked(Position(upper),Position(lower),Position(refHand),target,Vector(body,{sign*.6f,-1,-.3f}),solution));
        Mat du,dl;CHECK(TrackedJointDelta(upper,Position(lower),Position(upper),solution.elbow,du));
        CHECK(TrackedJointDelta(lower,Position(refHand),solution.elbow,solution.wrist,dl));
        Mat swing;Vec axis=solution.wrist-solution.elbow;
        CHECK(Between(Position(refHand)-Position(refLower),axis,swing));
        Mat neutral=Multiply(refHand,swing);Position(neutral,solution.wrist);
        ArmTwistChain chain;time=10000;Mat previousForearm;bool havePrevious=false;
        melee::LockedMeleeAttachment locked;Mat attachment;Position(attachment,{0,0,4});
        auto initialMesh=Multiply(attachment,neutral);
        CHECK(locked.Capture(11,2,initialMesh,neutral)&&locked.LockProfile(initialMesh,neutral));
        for(int d=0;d<=220;++d){
            Mat hand=Multiply(neutral,Roll(axis,sign*float(d)*ArmDegree));Position(hand,solution.wrist);Mat exact=hand;
            Mat upperDelta=du,lowerDelta=dl;
            CHECK(ProgressiveArmTwist(upper,lower,refUpper,refLower,refHand,hand,solution.elbow,solution.wrist,upperDelta,lowerDelta,chain,1,2,time,angles));
            CHECK(Near(hand,exact));CHECK(Near(angles.wrist+angles.forearm+angles.upper,sign*float(d)*ArmDegree,.001f));
            CHECK(Near(Point(upperDelta,Position(upper)),Position(upper),.06f));
            CHECK(Near(Point(upperDelta,Position(lower)),solution.elbow,.06f));
            CHECK(Near(Point(lowerDelta,Position(lower)),solution.elbow,.06f));
            CHECK(Near(Point(lowerDelta,Position(refHand)),solution.wrist,.06f));
            Mat forearm=Multiply(lower,lowerDelta);float residual=0;bool defined=false;
            CHECK(AxialTwist(forearm,hand,axis,residual,defined)&&defined&&Near(residual,angles.wrist,.001f));
            if(havePrevious)CHECK(RotationDifference(previousForearm,forearm)<.08f);previousForearm=forearm;havePrevious=true;
            Vec transverse=Cross(axis,Vector(body,{1,0,0}));CHECK(Unit(transverse));Mat inverse;
            CHECK(Inverse(dl,inverse));Vec before=Vector(inverse,transverse);
            CHECK(Near(Length(Vector(lowerDelta,before)),Length(transverse),.001f));
            // Weapon and finger contacts are driven by the unchanged hand.
            Mat weapon,expected=Multiply(attachment,exact);
            CHECK(locked.ProfileAtHand(hand,weapon)&&Near(weapon,expected,.02f));
            Mat nativePalette[3]{upper,lower,refHand},posed[3]{Multiply(upper,upperDelta),forearm,hand};
            FrozenArmPalette stereo;CHECK(stereo.Capture(1,2,posed,3));TrackedPalette snapshot;
            for(int eye=0;eye<2;++eye){
                Mat original[3];memcpy(original,nativePalette,sizeof(original));CHECK(snapshot.Begin(nativePalette,3));
                CHECK(stereo.Apply(1,2,nativePalette,3)&&!memcmp(nativePalette,posed,sizeof(posed)));
                CHECK(snapshot.Restore(nativePalette,3)&&!memcmp(original,nativePalette,sizeof(original)));
            }
            // A duplicated XR timestamp (the other eye) cannot integrate twist twice.
            Mat du2=du,dl2=dl;TwistAngles same;
            CHECK(ProgressiveArmTwist(upper,lower,refUpper,refLower,refHand,hand,solution.elbow,solution.wrist,du2,dl2,chain,1,2,time,same));
            CHECK(Near(upperDelta,du2)&&Near(lowerDelta,dl2)&&Near(same.upper,angles.upper));time+=10;
        }
    }
    // Pure wrist bending has no longitudinal twist; an ambiguous half-turn
    // must preserve the last valid twist rather than inventing a new roll.
    float angle=0;bool defined=false;
    CHECK(AxialTwist(Mat{},Roll({1,0,0},.8f),{0,0,1},angle,defined)&&defined&&Near(angle,0));
    CHECK(AxialTwist(Mat{},Roll({1,0,0},ArmPi),{0,0,1},angle,defined)&&!defined);
    Mat bad;bad.m[0]=nan;CHECK(!AxialTwist(bad,Mat{},{0,0,1},angle,defined));
    puts("PASS progressive wrist/forearm/upper-arm limits, continuous turns and resets, mirrored/bent/stretched arms, native roll rejection, exact endpoints/hands/thickness and stereo reuse");
    return 0;
}
