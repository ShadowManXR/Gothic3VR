#include "TrackedPalette.h"
#include "ShieldPose.h"
#include <cstdio>
#include <limits>
#include <fstream>
#include <string>
using namespace diagnostic;using namespace diagnostic::arms;
bool Near(Vec a,Vec b,float epsilon=.002f){return Length(a-b)<epsilon;}
bool Near(Mat const& a,Mat const& b){for(int i=0;i<16;++i)if(std::fabs(a.m[i]-b.m[i])>.003f)return false;return true;}
int main(int argc,char** argv){unsigned failures=0;auto check=[&](bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);failures+=!ok;};
    float angles[3]={.25f,.8f,-.4f};Mat a=Rotation(NativeCameraRotation(angles));Position(a,{50,120,-70});Mat inv;
    check(Inverse(a,inv)&&Near(Multiply(a,inv),Mat{})&&Near(Point(inv,Point(a,{3,-4,8})),{3,-4,8}),"raw Genome row matrix and translated inverse round trip");
    a.m[0]*=1.5f;a.m[1]*=1.5f;a.m[2]*=1.5f;a.m[4]*=.7f;a.m[5]*=.7f;a.m[6]*=.7f;
    check(Inverse(a,inv)&&Near(Multiply(a,inv),Mat{}),"inverse preserves native skeletal scale");
    Mat rot;check(Between({1,0,0},{0,0,1},rot)&&Near(Vector(rot,{1,0,0}),{0,0,1}),"bone segment rotation uses the correct row-vector direction");
    check(Between({0,1,0},{0,-1,0},rot)&&Near(Vector(rot,{0,1,0}),{0,-1,0}),"opposite bone directions remain finite");
    bool geometry=true;unsigned limited=0;
    for(int x=-90;x<=90;x+=9)for(int y=-90;y<=90;y+=9)for(int z=-90;z<=90;z+=9){if(!x&&!y&&!z)continue;Solution s;bool ok=Solve({0,0,0},{30,0,0},{50,0,0},{float(x),float(y),float(z)},{.6f,-1,-.3f},s);geometry&=ok&&std::fabs(Length(s.elbow)-30)<.002f&&std::fabs(Length(s.wrist-s.elbow)-20)<.004f&&Length(s.wrist)<=50.01f;if(s.limited)++limited;}
    check(geometry&&limited,"two-bone IK preserves both bone lengths throughout 9260 reachable and unreachable targets");
    bool tracked=true;
    for(unsigned side=0;side<2;++side)for(float reach:{20.f,45.f,60.f,85.f}){
        float sign=side?1.f:-1.f;Vec shoulder{sign*20,120,0},elbow=shoulder+Vec{0,0,30},wrist=shoulder+Vec{0,0,50},target=shoulder+Vec{sign*reach*.2f,10,reach};Solution exact;
        Mat upper,lower,du,dl;Position(upper,shoulder);Position(lower,elbow);
        tracked&=SolveTracked(shoulder,elbow,wrist,target,{sign*.6f,-1,-.3f},exact)&&Near(exact.wrist,target)&&
            TrackedJointDelta(upper,elbow,shoulder,exact.elbow,du)&&TrackedJointDelta(lower,wrist,exact.elbow,exact.wrist,dl)&&
            Near(Point(du,shoulder),shoulder)&&Near(Point(du,elbow),exact.elbow)&&Near(Point(dl,elbow),exact.elbow)&&Near(Point(dl,wrist),target);
        Vec thickness{1,0,0};tracked&=std::fabs(Length(Vector(dl,thickness))-1)<.001f;
    }
    check(tracked,"both tracked bow hands reach the controller beyond native arm limits; skin endpoints meet without changing forearm thickness");
    bool freeReach=true;
    for(float x:{-100.f,-60.f,0.f,60.f,100.f})for(float y:{-40.f,20.f,100.f})for(float z:{-80.f,30.f,120.f}){
        Vec target{x,y,z};Solution exact;freeReach&=SolveTracked({0,0,0},{30,0,0},{50,0,0},target,{0,-1,0},exact)&&Near(exact.wrist,target);
    }
    {Solution folded;freeReach&=SolveTracked({},{30,0,0},{50,0,0},{},{0,-1,0},folded)&&Near(folded.wrist,{});}
    check(freeReach,"full-reach empty/casting hands match controllers beyond both native arm length and minimum fold distance");
    Solution s;check(!Solve({},{},{},{1,0,0},{0,-1,0},s)&&!Solve({},{30,0,0},{50,0,0},{},{0,-1,0},s),"degenerate rig and coincident target refuse the pose");
    Vec bad{std::numeric_limits<float>::quiet_NaN(),0,0};check(!Solve({},{30,0,0},{50,0,0},bad,{0,-1,0},s),"nonfinite tracking cannot reach a palette");
    Mat old;Position(old,{5,10,15});Mat delta;check(JointDelta(old,{35,10,15},{7,8,9},{7,38,9},delta)&&Near(Point(delta,{5,10,15}),{7,8,9})&&Near(Point(delta,{35,10,15}),{7,38,9}),"arm skin and joint endpoints share the same rigid delta");
    Mat frame;check(BodyFrame({1,0,0},{0,1,0},frame)&&Near(Vector(frame,{0,0,1}),{1,0,0}),"finger/thumb landmarks calibrate orientation without Euler guesses");
    bool palms=true;
    for(unsigned side=0;side<2;++side){
        // Open palms facing down in Genome, with thumbs facing each other.
        Vec across{side?-1.f:1.f,0,0},fingers{0,0,1};
        Mat hand=Rotation(NativeCameraRotation(angles));Position(hand,{25,120,40});
        Mat handInverse,calibration;
        palms&=Inverse(hand,handInverse)&&GripCalibration(hand,Position(hand)+fingers*8+across*2,Position(hand)+fingers*8-across*2,calibration);
        Vec posedFingers=Vector(calibration,Vector(handInverse,fingers));
        Vec posedAcross=Vector(calibration,Vector(handInverse,across));
        Vec palm=Cross(posedFingers,posedAcross)*(side?1.f:-1.f);
        palms&=Near(posedFingers,{0,-1,0})&&Near(posedAcross,{0,0,1})&&Near(palm,{side?-1.f:1.f,0,0});
        Mat rolled=Rotation(Matrix({.70710678f,0,0,.70710678f}));
        Mat posed=Multiply(calibration,rolled);
        palms&=Near(Vector(posed,Vector(handInverse,fingers)),Vector(rolled,{0,-1,0}));
    }
    check(palms,"mirrored palms face inward in OpenXR grip space, with correct knuckle direction and controller roll");
    Mat refused;check(!GripCalibration(Mat{},{0,0,8},{0,0,8},refused)&&!GripCalibration(Mat{},{0,0,8},{0,0,4},refused),"coincident or collinear knuckles refuse unstable palm orientation");
    Mat world;float base[3]={100,200,300},origin[3]={0,1.6f,0},zero[3]{};ControllerPose p;p.valid=p.tracked=true;p.position[0]=.3f;p.position[1]=1.2f;p.position[2]=-.5f;
    check(GripWorld(p,NativeCameraRotation(zero),base,{},origin,100,-15,world)&&Near(Position(world),{130,145,350}),"hand tracking uses the stereo origin, world scale and camera height exactly once");
    int calibration[6]{5,2,3,0,0,0};Mat leftGrip,rightGrip;
    check(CalibrateGrip(leftGrip,0,calibration,100)&&CalibrateGrip(rightGrip,1,calibration,100)&&Near(Position(leftGrip),{-5,2,3})&&Near(Position(rightGrip),{5,2,3}),"outward offsets mirror left/right with independent vertical and forward axes");
    Mat rotatedGrip=Rotation(NativeCameraRotation(angles)),reference=rotatedGrip;Position(rotatedGrip,{10,20,30});Position(reference,{10,20,30});
    check(CalibrateGrip(rotatedGrip,1,calibration,125)&&Near(Position(rotatedGrip),Point(reference,{6.25f,2.5f,3.75f})),"device offsets rotate with the grip and respect world scale exactly once");
    calibration[3]=30;calibration[4]=-40;calibration[5]=15;Mat adjusted=reference;auto priorPosition=Point(reference,{5,2,3});
    check(CalibrateGrip(adjusted,1,calibration,100)&&Affine(adjusted)&&Near(Position(adjusted),priorPosition)&&!Near(Vector(adjusted,{0,0,1}),Vector(reference,{0,0,1})),"orientation calibration rotates hand and weapon together without rotating translation twice");
    int none[6]{};Mat unchanged=reference;check(CalibrateGrip(unchanged,0,none,100)&&Near(unchanged,reference),"zero device calibration preserves the original grip pose");
    ControllerFrame sample;sample.focused=true;sample.timeMs=100;sample.hands[1].active=true;sample.hands[1].grip=p;
    check(FreshGrip(sample,1,200)&&!FreshGrip(sample,1,201)&&!FreshGrip(sample,1,99),"100ms freshness and clock reversal gate tracked hands");sample.hands[1].grip.tracked=false;
    check(!FreshGrip(sample,1,100),"valid but inferred controller poses cannot move the combat rig");sample.hands[1].grip.tracked=true;sample.focused=false;check(!FreshGrip(sample,1,100),"focus loss rejects all tracked hand writes");
    auto r=Identify("Bip01 R UpperArm"),l=Identify("Bip01 L Forearm"),f=Identify("Bip01 R Finger1"),t=Identify("Bip01 L Finger0");
    check(r.side==1&&r.part==Upper&&r.joint&&l.side==0&&l.part==Lower&&f.index&&t.thumb&&Identify("Bip01 Head").side<0&&Identify("Bip01 R Clavicle").side<0,"rig names distinguish arms, finger landmarks and unaffected body bones");
    std::ifstream fixture(argc>1?argv[1]:"tests/fixtures/hero-rig-73.txt");std::string name;int boneCount=0,armCount=0,joints[2][3]={{-1,-1,-1},{-1,-1,-1}},landmarks[2][3]={{-1,-1,-1},{-1,-1,-1}};bool otherBonesUntouched=true;
    while(std::getline(fixture,name)){if(!name.empty()&&name.back()=='\r')name.pop_back();auto tag=Identify(name.c_str());if(tag.side>=0){++armCount;if(tag.joint)joints[tag.side][int(tag.part)-1]=boneCount;if(tag.thumb)landmarks[tag.side][0]=boneCount;if(tag.index)landmarks[tag.side][1]=boneCount;if(tag.pinky)landmarks[tag.side][2]=boneCount;}else if((boneCount>=7&&boneCount<=24)||(boneCount>=26&&boneCount<=43))otherBonesUntouched=false;++boneCount;}
    check(boneCount==73&&armCount==36&&joints[0][0]==7&&joints[0][1]==8&&joints[0][2]==9&&joints[1][0]==26&&joints[1][1]==27&&joints[1][2]==28&&landmarks[0][0]==10&&landmarks[0][1]==13&&landmarks[0][2]==22&&landmarks[1][0]==29&&landmarks[1][1]==32&&landmarks[1][2]==41&&otherBonesUntouched,"actual Gothic Hero skeleton resolves both arms, index/pinky calibration and all 30 finger bones while excluding the body and slots");
    Mat data[3]={a,old,frame};Mat before[3];memcpy(before,data,sizeof(data));TrackedPalette palette;bool restore=true;
    for(unsigned eye=0;eye<2;++eye){restore&=palette.Begin(data,3);data[1]=Multiply(data[1],delta);restore&=palette.Restore(data,3)&&!memcmp(data,before,sizeof(data));}
    check(restore,"both stereo eyes start with clean native bones and restore the exact palette bytes");
    Mat replacement[3];check(palette.Begin(data,3)&&!palette.Restore(replacement,3)&&!palette.address,"resource replacement discards borrowed memory without writing a stale palette");
    check(!palette.Begin(data,257),"unbounded skeletons refuse writes");
    ShieldContact shield;Mat shieldItem,nativeHand,trackedHand;Position(shieldItem,{0,120,30});Position(nativeHand,{0,120,30});Position(trackedHand,{50,150,60});
    Vec palm{0,0,6};int shieldSettings[6]{};Mat shieldMoved;
    check(shield.Prepare(123,shieldItem,nativeHand,palm,{-25,-30,-2},{25,30,4})&&shield.Pose(trackedHand,trackedHand,palm,shieldSettings,100,shieldMoved)&&Near(Point(shieldMoved,shield.Contact()),Point(trackedHand,palm)),"shield back grip anchors to actual palm independently of wrist and locomotion");
    Mat unchangedHand=trackedHand;shieldSettings[0]=45;shieldSettings[3]=3;
    check(shield.Pose(trackedHand,trackedHand,palm,shieldSettings,100,shieldMoved)&&Near(Point(shieldMoved,shield.Contact()),Point(trackedHand,palm)+Vector(trackedHand,{0,-3,0}))&&Near(trackedHand,unchangedHand),"shield angles rotate about palm grip without rotating the hand; live offsets preserve contact");
    shield.Reset();check(!shield.Pose(trackedHand,trackedHand,palm,shieldSettings,100,shieldMoved),"weapon or actor reset discards shield contact");
    check(NativeParryCompatible(2,9)&&NativeParryCompatible(2,0)&&NativeParryCompatible(3,0)&&NativeParryCompatible(52,0)&&!NativeParryCompatible(2,2)&&!NativeParryCompatible(6,0),"native shield/1H/2H parry leaves dual-wield power and ranged triggers unchanged");
    ParryLoadout swordShield{2,9};
    check(swordShield.LeftShield()&&swordShield.GuardHand()==0&&swordShield.Pressed(false,true)&&swordShield.Pressed(true,false)&&!swordShield.Pressed(false,false),"1H plus shield accepts either grip and checks the raised left shield");
    for(int type:{2,3,52,12,51})for(int offhand:{0,2,9}){
        ParryLoadout loadout{type,offhand};
        check(loadout.Pressed(false,true)==(type==2&&offhand==9),"left grip blocks exclusively with 1H plus shield; 2H/staff support and dual wield stay separate");
        check(loadout.Pressed(true,false)==NativeParryCompatible(type,offhand),"right grip keeps existing native melee block compatibility");
    }
    for(int ranged:{5,6})for(int other:{0,2,3,9,12,51,52})for(bool right:{false,true})for(bool left:{false,true}){
        check(!ParryLoadout{ranged,other}.Pressed(right,left)&&!ParryLoadout{other,ranged}.Pressed(right,left),"bow/crossbow cannot request block in either hand with either grip");
    }
    check(NativeParryState(16,0)&&NativeParryState(19,0)&&NativeParryState(0,4)&&!NativeParryState(1,0),"native parry inhibits physical swing damage");
    alignas(4) unsigned char msg[16]{16,0};msg[12]=2;uint32_t milliseconds=30;memcpy(msg+8,&milliseconds,4);unsigned char release[16];
    check(ParryReleaseMessage(msg,release)&&release[12]==2&&release[0]==16&&memcmp(release+8,msg+8,4),"short native parry release cannot generate a quick attack");
    msg[12]=3;check(!ParryReleaseMessage(msg,release),"held native parry wind-up is not accelerated");msg[12]=2;msg[0]=15;check(!ParryReleaseMessage(msg,release),"attack and menu messages are not rewritten");
    return failures?1:0;
}
