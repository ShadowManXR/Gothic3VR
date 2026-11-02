#include "ThirdPersonOrbit.h"
#include "PhysicalBodyCrouch.h"
#include "TrackedPalette.h"
#include "FrozenArmPalette.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;
int main(){unsigned failures=0;auto check=[&](bool ok,char const* label){printf("%s %s\n",ok?"PASS":"FAIL",label);failures+=!ok;};
 for(bool first:{false,true})for(bool selected:{false,true})for(bool crouched:{false,true})
  check(FirstPersonHandsOnly(first,selected,crouched)==(first&&(selected||crouched)),"explicit hands-only and deep crouch hide the body exclusively in first-person");
 ThirdPersonOrbit orbit;Rotation3 rotation;float pivot[3]{0,125,0},eye[3]{0,165,-200};unsigned traces=0;
 auto clear=[&](float const (&)[3],float const (&)[3]){++traces;return 1.f;};
 orbit.Step(1,1,true,true,0,100,1000);check(!orbit.Apply(1,pivot,rotation,eye,100,clear)&&eye[0]==0&&eye[2]==-200,"no stick input leaves third-person framing byte-exact");
 orbit.Step(1,1,true,true,1,100,1050);check(orbit.Apply(1,pivot,rotation,eye,100,clear)&&eye[0]<0&&rotation.m[2]>0&&std::fabs(std::hypot(eye[0],eye[2])-200)<.001f&&eye[1]==165,"right stick orbits around hero, retaining radius and height");
 float held[3];memcpy(held,eye,sizeof(eye));auto look=rotation;auto beforeTraces=traces;
 for(int i=0;i<3;++i){float native[3]{500,900,600};Rotation3 cut;orbit.Apply(1,pivot,cut,native,100,clear);check(!memcmp(native,held,sizeof(held))&&RotationError(cut,look)<.00001f,"repeated stereo preparation and native camera cuts cannot compound orbit");}
 check(traces==beforeTraces,"stationary orbit reuses collision clearance");
 orbit.Step(1,1,true,true,0,100,1100);eye[0]=0;eye[1]=165;eye[2]=-200;orbit.Apply(1,pivot,rotation,eye,100,clear);check(!memcmp(eye,held,sizeof(held)),"releasing stick stops orbit without inertia or recentering");
 orbit.Step(1,1,true,false,1,100,1150);orbit.Step(1,1,true,true,1,100,1200);orbit.Apply(1,pivot,rotation,eye,100,clear);check(!memcmp(eye,held,sizeof(held)),"focus/menu interruption requires joystick neutral before resuming");
 pivot[0]=100;orbit.Apply(1,pivot,rotation,eye,100,clear);check(std::fabs(eye[0]-held[0]-100)<.001f,"framing follows actor translation without changing its heading");
 orbit.Step(1,1,true,true,0,100,1250);orbit.Step(1,1,true,true,1,100,1300);orbit.Apply(1,pivot,rotation,eye,100,[](auto const&,auto const&){return 0.f;});check(std::fabs(eye[0]-held[0]-100)<.001f&&RotationError(rotation,look)<.00001f,"blocked orbit retains last safe view instead of snapping to native camera");
 orbit.Step(1,2,true,true,1,100,1350);check(!orbit.Apply(1,pivot,rotation,eye,100,clear),"entering a different activity cannot inherit previous conversation orbit");
 orbit.Step(1,0,false,true,0,100,1400);check(!orbit.Active(1)&&!orbit.Apply(1,pivot,rotation,eye,100,clear),"return to first-person discards render orbit without touching tracking origin");
 PhysicalBodyCrouch posture;posture.Update(1,1,1000,true,1.7f,1.7f,0);check(posture.DropMeters()==0&&!posture.HandsOnly(),"standing height preserves the accepted body pose");
 float previous=0;bool progressive=true;for(uint64_t t=1010;t<=1600;t+=10){posture.Update(1,1,t,true,1.7f,1.4f,0);progressive&=posture.DropMeters()>=previous&&posture.DropMeters()<=.261f;previous=posture.DropMeters();}
 check(progressive&&std::fabs(posture.DropMeters()-.26f)<.002f&&!posture.HandsOnly(),"physical lowering blends progressively after a small deadzone");
 posture.Update(1,1,1610,true,1.7f,.8f,0);check(posture.HandsOnly()&&posture.DropMeters()<=.5f,"deep physical crouch hides body while limiting visual leg compression");
 posture.Update(1,1,1620,true,1.7f,1.f,0);check(posture.HandsOnly(),"visibility hysteresis prevents body flicker near threshold");posture.Update(1,1,1630,true,1.7f,1.1f,0);check(!posture.HandsOnly(),"body reappears after rising above hysteresis boundary");
 posture.Update(1,2,1700,true,1.1f,1.1f,0);check(posture.DropMeters()==0&&!posture.HandsOnly(),"tracking recenter/session changes do not inherit an old physical crouch");
 posture.Update(1,2,1710,false,1.7f,.8f,0);check(posture.DropMeters()==0&&!posture.HandsOnly(),"disabled option / third-person mode restores native body visibility");
 posture.Update(1,3,1800,true,1.7f,1.2f,.3f);check(posture.DropMeters()<=.2001f&&posture.HandsOnly(),"native sneak plus physical lowering is capped without adding camera height offsets");
 using namespace arms;Mat bone[12],delta[12],body;BodyPart parts[12];for(auto& p:parts)p=BodyPart::Leg;parts[0]=parts[1]=BodyPart::Neutral;
 Position(bone[0],{0,100,0});Position(bone[1],{0,150,0});CrouchLeg legs[2]{{2,3,4,5,6},{7,8,9,10,11}};
 for(unsigned h=0;h<2;++h){float x=h?10.f:-10.f;auto leg=legs[h];Position(bone[leg.root],{x,100,0});Position(bone[leg.upper],{x,95,0});Position(bone[leg.lower],{x,50,1});Position(bone[leg.foot],{x,5,0});Position(bone[leg.toe],{x,0,12});}
 Mat original[12];memcpy(original,delta,sizeof(delta));check(!LowerRenderBody(bone,parts,12,legs,0,body,delta)&&!memcmp(original,delta,sizeof(delta)),"zero physical drop changes no palette matrices");
 check(LowerRenderBody(bone,parts,12,legs,20,body,delta)&&Position(body).y==-20&&Position(Multiply(bone[0],delta[0])).y==80,"pelvis and torso lower only in render space");
 bool grounded=true,anatomical=true;for(auto leg:legs){auto hip=Position(Multiply(bone[leg.upper],delta[leg.upper])),knee=Position(Multiply(bone[leg.lower],delta[leg.lower])),foot=Position(Multiply(bone[leg.foot],delta[leg.foot]));
  grounded&=Length(foot-Position(bone[leg.foot]))<.001f&&Length(Position(Multiply(bone[leg.toe],delta[leg.toe]))-Position(bone[leg.toe]))<.001f;
  anatomical&=knee.z>1&&std::fabs(Length(knee-hip)-Length(Position(bone[leg.lower])-Position(bone[leg.upper])))<.001f&&std::fabs(Length(foot-knee)-Length(Position(bone[leg.foot])-Position(bone[leg.lower])))<.001f;}
 check(grounded,"feet and toes stay at their original ground / gait contacts");check(anatomical,"both knees bend forward with unchanged thigh and calf lengths");
 auto invalid=legs[1];legs[1].foot=-1;memcpy(original,delta,sizeof(delta));auto originalBody=body;check(!LowerRenderBody(bone,parts,12,legs,20,body,delta)&&!memcmp(original,delta,sizeof(delta))&&!memcmp(&originalBody,&body,sizeof(body)),"unknown leg rig refuses crouch without partial palette edits");legs[1]=invalid;
 Mat skin[5];BoneTag tags[5]{};tags[0].side=0;tags[0].part=Hand;tags[1].side=1;tags[1].part=Hand;tags[2].side=0;tags[2].part=Lower;tags[3].side=1;tags[3].part=Upper;
 TrackedPalette saved;saved.Begin(skin,5);Mat hand0=skin[0],hand1=skin[1];bool tracked[2]{true,true};HandsOnlySkin(skin,tags,5,{-30,100,50},{30,120,60},tracked);
 check(!memcmp(&skin[0],&hand0,sizeof(Mat))&&!memcmp(&skin[1],&hand1,sizeof(Mat)),"extreme crouch preserves both hand/finger matrices exactly");
 Vec a=Point(skin[2],{100,30,-50}),b=Point(skin[3],{-80,90,12}),c=Point(skin[4],{0,0,0});check(Length(Cross(b-a,c-a))<.05f&&Length(Vector(skin[2],{0,1,0}))>0,"non-hand skin collapses below pixel size while shader normals remain nonzero");
 FrozenArmPalette frozen;check(frozen.Capture(1,2,skin,5)&&frozen.Matches(1,2,5),"hand-only palette is valid for shared stereo freeze");saved.Restore(skin,5);check(!memcmp(&skin[0],&hand0,sizeof(Mat))&&skin[2].m[0]==1,"scoped palette restore returns the original full body after rendering");
 Mat fullSkin[5];memcpy(fullSkin,skin,sizeof(skin));
 for(bool first:{true,false,true,false})for(unsigned eye=0;eye<2;++eye){
  saved.Begin(skin,5);
  if(FirstPersonHandsOnly(first,true,false))HandsOnlySkin(skin,tags,5,{-30,100,50},{30,120,60},tracked);
  check(!memcmp(&skin[0],&hand0,sizeof(Mat))&&!memcmp(&skin[1],&hand1,sizeof(Mat)),"switching first/third-person preserves both tracked hands in each eye");
  check(first?skin[2].m[0]<.001f:!memcmp(skin,fullSkin,sizeof(skin)),"enabled Hands only hides non-hand skin in first-person and keeps the complete third-person palette");
  saved.Restore(skin,5);check(!memcmp(skin,fullSkin,sizeof(skin)),"every eye restores the complete native body without persistent palette edits");
 }
 printf("Failures: %u\n",failures);return failures?1:0;
}
