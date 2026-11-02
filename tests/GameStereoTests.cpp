#include "GameStereoMath.h"
#include "StereoCameraState.h"
#include "CameraComfort.h"
#include <cstdio>
#include <cmath>
#include <array>
#include <cstring>
int main()
{
    unsigned failures=0,checks=0;auto check=[&](bool v,char const* s){++checks;if(!v){++failures;printf("FAIL %s\n",s);}};
    auto close=[](float a,float b){return std::fabs(a-b)<.0001f;};
    float origin[3]={100,200,300},centerPosition[3]={0,1.6f,0},out[3]{},euler[3]{};diagnostic::Orientation center;
    diagnostic::StereoEyePose left{{-.032f,1.6f,0},{0,0,0,1},{-.8f,.6f,.7f,-.5f}},right=left;right.position[0]=.032f;
    diagnostic::Rotation3 identity;
    check(diagnostic::GameEyeTransform(identity,origin,center,centerPosition,left,100,out,euler)&&close(out[0],96.8f)&&close(out[1],200)&&close(out[2],300),"left eye offset in centimeters");
    check(diagnostic::GameEyeTransform(identity,origin,center,centerPosition,right,100,out,euler)&&close(out[0],103.2f),"right eye separation");
    right.position[2]=-.25f;check(diagnostic::GameEyeTransform(identity,origin,center,centerPosition,right,100,out,euler)&&close(out[2],325),"OpenXR forward translation maps to Genome positive Z");
    float yaw[3]={0,1.57079632679f,0};auto rotated=diagnostic::NativeCameraRotation(yaw);
    check(diagnostic::GameEyeTransform(rotated,origin,center,centerPosition,left,100,out,euler)&&close(out[2],303.2f),"eye separation rotates with native heading");
    center={std::cos(.4f),0,std::sin(.4f),0};left.quaternion[1]=center.y;left.quaternion[3]=center.w;
    check(diagnostic::GameEyeTransform(identity,origin,center,centerPosition,left,100,out,euler)&&close(euler[1],0),"recenter removes headset yaw without removing native heading");
    float p[16];diagnostic::StereoProjection(left.fov,13,10000,p);
    auto project=[&](float x,float y,float z,int c){return (x*p[c]+y*p[4+c]+z*p[8+c]+p[12+c])/z;};
    check(close(project(std::tan(left.fov[0]),0,1,0),-1)&&close(project(std::tan(left.fov[1]),0,1,0),1),"asymmetric horizontal FOV edges");
    check(close(project(0,std::tan(left.fov[3]),1,1),-1)&&close(project(0,std::tan(left.fov[2]),1,1),1),"vertical FOV edges");
    check(close(project(0,0,13,2),0)&&close(project(0,0,10000,2),1),"native near/far depth preserved");
    // Native RenderLowPoly switches from the detailed range to a disjoint
    // distant range and then back. Reusing the detailed matrix hides the
    // entire low-poly pass, even when native/EDD streaming loaded its geometry.
    float mainProjection[16];memcpy(mainProjection,p,sizeof(p));
    check(project(0,0,50000,2)>1,"regression: frozen main depth clips distant terrain");
    for(auto range: {std::array<float,2>{6300,85000},std::array<float,2>{36300,238000}}){
        check(diagnostic::StereoProjectionDepth(range[0],range[1],p),"native and EDD low-poly ranges accepted");
        check(close(project(0,0,range[0],2),0)&&close(project(0,0,range[1],2),1),"low-poly pass maps native clip planes to depth zero and one");
        check(project(0,0,50000,2)>=0&&project(0,0,50000,2)<=1,"distant terrain remains inside the low-poly depth range");
        bool lens=true;for(int i=0;i<16;++i)if(i!=10&&i!=14&&p[i]!=mainProjection[i])lens=false;
        check(lens,"depth changes preserve eye asymmetry, perspective and handedness");
    }
    check(diagnostic::StereoProjectionDepth(13,10000,p)&&close(project(0,0,13,2),0)&&close(project(0,0,10000,2),1),"main projection restored after low-poly terrain");
    for(auto range: {std::array<float,2>{0,10000},std::array<float,2>{13,13},std::array<float,2>{-1,10000},std::array<float,2>{13,INFINITY}}){
        float before[16];memcpy(before,p,sizeof(p));
        check(!diagnostic::StereoProjectionDepth(range[0],range[1],p)&&!memcmp(before,p,sizeof(p)),"invalid depth range leaves the last valid eye projection intact");
    }
    // Replay transitions seen during the live 0.5.1 trial: native first-person
    // disappears for several seconds after combat/interaction. Those frames
    // must still reach OpenXR once VR is connected, with the same eye geometry.
    diagnostic::StereoCameraState sequence[]={
        {true,true,true,false,false}, {true,true,false,false,false},
        {true,true,false,true,false}, {true,true,false,false,true},
        {true,true,true,false,false}, {true,false,false,false,false},
        {false,false,false,false,false}};
    bool connected=false;unsigned submitted=0;
    for(auto state:sequence)
    {
        bool allowed=diagnostic::StereoCameraAllowed(state,connected);
        check(allowed,"active stereo continues across native camera mode transitions");
        if(allowed){connected=true;++submitted;
            float neutral[3]={0,0,0};diagnostic::Orientation recentered;
            diagnostic::StereoEyePose pose{{-.032f,1.6f,0},{0,0,0,1},{-.8f,.6f,.7f,-.5f}};
            check(diagnostic::GameEyeTransform(diagnostic::NativeCameraRotation(neutral),origin,recentered,centerPosition,pose,100,out,euler)&&close(out[0],96.8f),"eye separation preserved in every native camera mode");}
    }
    check(submitted==7,"no dropped stereo frames during camera mode replay");
    check(!diagnostic::StereoCameraAllowed(sequence[1],false)&&!diagnostic::StereoCameraAllowed(sequence[3],false)&&
        !diagnostic::StereoCameraAllowed(sequence[6],false),"initial activation still requires native first-person in the loaded game");
    check(diagnostic::CameraMode(sequence[2])==diagnostic::StereoCameraMode::Temporary&&
        diagnostic::CameraMode(sequence[3])==diagnostic::StereoCameraMode::Dialog,"temporary and dialogue modes identified for diagnostics");
    float tilted[3]={.25f,1.8f,.15f};auto upright=diagnostic::UprightCamera(diagnostic::NativeCameraRotation(tilted));
    check(close(upright.m[1],0)&&close(upright.m[3],0)&&close(upright.m[4],1)&&close(upright.m[5],0)&&close(upright.m[7],0),"native animation pitch/roll cannot tilt the VR horizon");
    auto recenter=diagnostic::UprightCenter({std::cos(.2f),std::sin(.2f),0,0});
    check(close(recenter.x,0)&&close(recenter.z,0)&&close(recenter.w,1),"recentring with a tilted head keeps the world vertical");
    diagnostic::StereoEyePose headPitch{{0,1.6f,0},{std::sin(.15f),0,0,std::cos(.15f)},{}};
    check(diagnostic::GameEyeTransform(upright,origin,recenter,centerPosition,headPitch,100,out,euler)&&diagnostic::RotationError(upright,diagnostic::NativeCameraRotation(euler))>.1f,"physical headset pitch remains present with upright native base");
    diagnostic::CameraHeightAnchor anchor;
    check(close(anchor.Height(1,100,270),270)&&close(anchor.Height(1,100,280),270),"native head animation cannot bob the eye height");
    check(close(anchor.Height(1,120,290),290),"terrain and jump movement are preserved by player root height");
    anchor.Reset();check(close(anchor.Height(2,50,225),225),"new player/session recalibrates the eye height");
    anchor.Context(true,true); // Dialogue uses the native camera, not Height().
    anchor.Context(true,true); // First-person returns while native camera is high.
    check(close(anchor.Height(2,50,700),225),"dialogue exit cannot recalibrate eye height from elevated transition camera");
    check(close(anchor.Height(2,80,730),255),"terrain height remains tracked after returning from dialogue");
    anchor.Context(false,true);check(close(anchor.Height(2,80,250),250),"leaving the world discards the previous height even if actor address is reused");
    anchor.Context(true,false);check(close(anchor.Height(2,80,240),240),"disabling camera stabilization resets its height calibration");
    float root[3]{100,80,300},animated[3]{130,900,270};anchor.Position(2,root,animated);
    check(close(animated[0],100)&&close(animated[1],240)&&close(animated[2],300),"drawing sword cannot move the first-person eye sideways, backwards or vertically");
    root[0]+=30;root[1]+=5;root[2]-=20;animated[0]=700;animated[1]=950;animated[2]=-800;anchor.Position(2,root,animated);
    check(close(animated[0],130)&&close(animated[1],245)&&close(animated[2],280),"joystick, accepted room walking and terrain move the eye with the actor while head animation is ignored");
    diagnostic::StereoEyePose leaning{{.2f,1.4f,-.3f},{0,0,0,1},{}};float tracked[3],rotation[3];float resetOrigin[3]{0,1.6f,0};
    check(diagnostic::GameEyeTransform(identity,animated,{},resetOrigin,leaning,100,tracked,rotation)&&close(tracked[0],150)&&close(tracked[1],225)&&close(tracked[2],310),"body anchored camera still adds true headset translation and crouching");
    float offset[3]{0,10,0};diagnostic::ForwardCameraOffset(rotated,25,offset);
    check(close(offset[0],25)&&close(offset[1],10)&&std::fabs(offset[2])<.0001f,"forward camera offset follows horizontal heading without changing height");
    float movedHead[3]={3.5f,1.8f,-4.f},recenterPosition[3]{};diagnostic::Orientation recenteredHead;
    diagnostic::Orientation headYaw{std::cos(.6f),0,std::sin(.6f),0};
    check(diagnostic::RecenterTracking(headYaw,movedHead,true,recenteredHead,recenterPosition),"valid full camera reset captures current headset translation and yaw");
    diagnostic::StereoEyePose resetEye{{movedHead[0],movedHead[1],movedHead[2]},{0,headYaw.y,0,headYaw.w},{}};
    check(diagnostic::GameEyeTransform(identity,origin,recenteredHead,recenterPosition,resetEye,100,out,euler)&&close(out[0],origin[0])&&close(out[1],origin[1])&&close(out[2],origin[2])&&close(euler[1],0),"reset cancels meters of origin displacement and rotation instead of leaving the player body");
    // Replay the actual failing order: L3+R3 -> native movement update ->
    // stereo render, while looking along a slope with nonzero pitch/roll.
    diagnostic::Orientation tiltedHead=diagnostic::Product(headYaw,diagnostic::Product(diagnostic::Orientation{std::cos(.2f),std::sin(.2f),0,0},diagnostic::Orientation{std::cos(.1f),0,0,std::sin(.1f)}));
    auto oldCenter=recenteredHead;float oldPosition[3]={recenterPosition[0],recenterPosition[1],recenterPosition[2]},newPosition[3]={4,2,-5};
    check(!diagnostic::RecenterTracking(tiltedHead,newPosition,false,recenteredHead,recenterPosition,diagnostic::RecenterPhase::LegacyCamera,true)&&
        close(recenteredHead.w,oldCenter.w)&&close(recenteredHead.x,oldCenter.x)&&close(recenteredHead.z,oldCenter.z)&&close(recenterPosition[0],oldPosition[0])&&close(recenterPosition[2],oldPosition[2]),"movement cannot consume stereo reset, tilt its center, or leave translation partially reset");
    check(diagnostic::RecenterTracking(tiltedHead,newPosition,true,recenteredHead,recenterPosition)&&close(recenteredHead.x,0)&&close(recenteredHead.z,0)&&close(recenterPosition[0],4)&&close(recenterPosition[2],-5),"following stereo frame consumes complete reset with yaw-only origin on a slope");
    diagnostic::Rotation3 relative;check(diagnostic::RelativeHeadRotation(recenteredHead,tiltedHead,relative)&&diagnostic::RotationError(relative,identity)>.1f,"slope reset retains physical headset pitch and roll instead of baking them into world origin");
    check(diagnostic::RecenterTracking(tiltedHead,newPosition,false,recenteredHead,recenterPosition,diagnostic::RecenterPhase::LegacyCamera,false),"legacy non-stereo camera still owns its original reset path");
    anchor.Context(true,true);check(close(anchor.Height(2,80,900),240),"manual tracking reset must not recalibrate character height from a dialogue transition");
    printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
