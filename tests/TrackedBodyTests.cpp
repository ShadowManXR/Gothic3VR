#include "TrackedBodyMath.h"
#include "TrackedPalette.h"
#include <cstdio>
#include <fstream>
#include <string>
using namespace diagnostic;using namespace diagnostic::arms;
// At Gothic's ~100000cm world coordinates, a float position has ~0.008cm
// spacing. Allow 0.05cm for composed translations; retain tight axis checks.
bool Near(Vec a,Vec b){return Length(a-b)<.05f;}
bool Near(Mat const& a,Mat const& b){for(int i=0;i<16;++i)if(std::fabs(a.m[i]-b.m[i])>(i>=12&&i<=14?.05f:.005f))return false;return true;}
int main(int argc,char** argv){unsigned failures=0;auto check=[&](bool ok,char const* message){printf("%s %s\n",ok?"PASS":"FAIL",message);failures+=!ok;};
    std::ifstream fixture(argc>1?argv[1]:"tests/fixtures/hero-rig-73.txt");std::string name;unsigned count=0,neutral=0,legs=0;bool correct=true;
    while(std::getline(fixture,name)){if(!name.empty()&&name.back()=='\r')name.pop_back();auto part=IdentifyBody(name.c_str());if(part==BodyPart::Neutral)++neutral;if(part==BodyPart::Leg)++legs;
        if((count>=7&&count<=24)||(count>=26&&count<=43)||count>=58)correct&=part==BodyPart::Other;++count;}
    check(count==73&&neutral==12&&legs==10&&correct,"actual Hero rig distinguishes neutral torso/clavicles/head, animated legs, controller arms and unrelated slots");
    check(IdentifyBody("NPC_Spine_Spine_1")==BodyPart::Other&&IdentifyBody("Slot_Bow")==BodyPart::Other,"NPCs and weapon attachment names are not body candidates");
    float actorAngles[3]{.2f,1.1f,.3f},baseAngles[3]{-.3f,.6f,.2f};Mat actor=Rotation(NativeCameraRotation(actorAngles));Position(actor,{87600,5300,-9500});
    Mat actorBefore=actor,body;check(BodyWorld(actor,NativeCameraRotation(baseAngles),body)&&Near(Position(body),Position(actor))&&Near(Vector(body,{0,1,0}),{0,1,0}),"body stays at actor position and follows base yaw without camera pitch, roll or height offset");
    check(Near(actor,actorBefore),"body construction never moves the native gameplay entity");
    Mat scaled=actor;for(int i=0;i<3;++i)scaled.m[i]*=1.2f;for(int i=4;i<7;++i)scaled.m[i]*=.8f;
    Mat scaledBody;check(BodyWorld(scaled,NativeCameraRotation(baseAngles),scaledBody)&&std::fabs(Length(Vector(scaledBody,{1,0,0}))-1.2f)<.001f&&std::fabs(Length(Vector(scaledBody,{0,1,0}))-.8f)<.001f,"upright body retains native model scale");
    Mat bindPelvis;Position(bindPelvis,{0,95,0});Mat nativeShift=Rotation(NativeCameraRotation(actorAngles));Position(nativeShift,{18,2,-14});
    Mat pelvis=Multiply(Multiply(bindPelvis,nativeShift),actor),desired=Multiply(bindPelvis,body),delta;
    check(Rebase(pelvis,desired,delta)&&Near(Multiply(pelvis,delta),desired),"sword stance lateral displacement and rotation are removed at the pelvis");
    Mat foot;Position(foot,{12,0,8});Mat nativeFoot=Multiply(Multiply(foot,nativeShift),actor);
    Mat walked=Multiply(nativeFoot,delta);float before=Length(Position(nativeFoot)-Position(pelvis)),after=Length(Position(walked)-Position(desired));
    check(std::fabs(before-after)<.01f,"pelvis correction keeps leg segment geometry rather than replacing walking with bind pose");
    Position(foot,{17,6,20});Mat nextStep=Multiply(Multiply(Multiply(foot,nativeShift),actor),delta);
    check(Length(Position(nextStep)-Position(walked))>10,"animated feet continue moving after torso stabilization");
    Mat upper;Position(upper,{20,145,0});Mat nativeUpper=Multiply(Multiply(upper,nativeShift),actor),wantedUpper=Multiply(upper,body),armBase;
    bool armOk=Rebase(nativeUpper,wantedUpper,armBase);Mat lower;Position(lower,{45,140,0});Mat hand;Position(hand,{65,130,0});
    Mat nativeLower=Multiply(Multiply(lower,nativeShift),actor),nativeHand=Multiply(Multiply(hand,nativeShift),actor);
    Mat rebasedLower=Multiply(nativeLower,armBase),rebasedHand=Multiply(nativeHand,armBase);Vec target=Point(body,{30,135,35});Solution solve;
    armOk&=Solve(Position(wantedUpper),Position(rebasedLower),Position(rebasedHand),target,Vector(body,{.6f,-1,-.3f}),solve);
    Mat joint;armOk&=JointDelta(wantedUpper,Position(rebasedLower),Position(wantedUpper),solve.elbow,joint);
    Mat combined=Multiply(armBase,joint);
    armOk&=Near(Point(combined,Position(nativeUpper)),Position(wantedUpper))&&Near(Point(combined,Position(nativeLower)),solve.elbow)&&Near(solve.wrist,target);
    check(armOk,"tracked arm delta composes with shoulder rebase while keeping IK targets in the shared stereo world");
    Mat paletteData[3]{pelvis,nativeFoot,nativeUpper},original[3];memcpy(original,paletteData,sizeof(original));TrackedPalette saved;bool restored=true;
    for(unsigned eye=0;eye<2;++eye){restored&=saved.Begin(paletteData,3);paletteData[0]=Multiply(paletteData[0],delta);paletteData[1]=Multiply(paletteData[1],delta);paletteData[2]=Multiply(paletteData[2],combined);restored&=saved.Restore(paletteData,3)&&!memcmp(original,paletteData,sizeof(original));}
    check(restored,"body and arm changes restore exact native palette bytes after each stereo eye");
    Mat invalid;invalid.m[0]=0;invalid.m[5]=0;invalid.m[10]=0;Mat rejected;
    check(!Rebase(invalid,desired,rejected)&&!BodyWorld(invalid,NativeCameraRotation(baseAngles),rejected),"singular body data refuses a render override");
    return failures?1:0;
}
