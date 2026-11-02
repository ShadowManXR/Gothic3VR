#include "HeadControlMath.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;
int __cdecl main()
{
    unsigned failures=0,checks=0;
    auto check=[&](bool ok,char const* name){++checks;if(!ok)++failures;std::printf("%s %s\n",ok?"PASS":"FAIL",name);};
    auto near=[](float a,float b){return std::fabs(a-b)<.0001f;};
    constexpr float h=.70710678f,pi=3.14159265359f;
    Rotation3 delta;
    check(RelativeHeadRotation({h,0,h,0},{h,0,h,0},delta)&&RotationError(delta,{})<.0001f,"non-identity recenter is neutral");
    check(RelativeHeadRotation({h,0,h,0},{-h,0,-h,0},delta)&&RotationError(delta,{})<.0001f,"quaternion sign does not cause a flip");
    RelativeHeadRotation({}, {h,0,-h,0},delta);
    check(near(delta.m[2],1)&&near(delta.m[8],0),"physical right turn looks along Genome +X");
    RelativeHeadRotation({}, {h,h,0,0},delta);
    check(near(delta.m[5],1)&&near(delta.m[8],0),"physical up look aims along Genome +Y");
    RelativeHeadRotation({}, {h,0,0,h},delta);
    check(near(delta.m[1],-1)&&near(delta.m[3],1),"roll retained through handedness conversion");
    Orientation center{h,h,0,0},local{h,0,-h,0};
    RelativeHeadRotation(center,Product(center,local),delta);
    Rotation3 direct; RelativeHeadRotation({},local,direct);
    check(RotationError(delta,direct)<.0001f,"relative rotation is in recentered head space, not world space");
    float recordedEuler[3]={-.0263317544f,-1.06658518f,0};
    Rotation3 recorded{{.483116895f,-.0230522566f,-.875252366f,0,.999653339f,-.0263287108f,.875555873f,.0127198454f,.482949406f}};
    check(RotationError(NativeCameraRotation(recordedEuler),recorded)<.00001f,"independent real first-person view fixture");
    unsigned badRoundTrips=0;
    for(float pitch:{-1.5f,-.4f,0.f,.8f,1.5f}) for(float yaw:{-pi,-2.f,-pi/2,0.f,pi/2,2.f,pi}) for(float roll:{-1.2f,0.f,.7f})
    {
        float original[3]={pitch,yaw,roll},out[3];auto rotation=NativeCameraRotation(original);
        NativeCameraEuler(rotation,out);
        if(RotationError(rotation,NativeCameraRotation(out))>.0001f)++badRoundTrips;
    }
    check(badRoundTrips==0,"105 pitch/yaw/roll cases including both yaw singularities");
    float cameraYaw[3]={0,pi/2,0}; auto composed=Product(NativeCameraRotation(cameraYaw),direct);
    check(near(composed.m[2],0)&&near(composed.m[8],-1),"mouse yaw plus head yaw compose in camera-local axes");
    float output[3]; NativeCameraEuler(composed,output);
    check(RotationError(NativeCameraRotation(output),composed)<.0001f,"composition survives SDK Euler representation");
    auto initial=NativeCameraRotation(recordedEuler);
    check(RotationError(Product(initial,direct),Product(direct,initial))>.001f,"head rotation composition differs from world-axis rotation");
    check(!RelativeHeadRotation({}, {0,0,0,0},delta),"zero quaternion rejected");
    check(!RelativeHeadRotation({}, {std::numeric_limits<float>::quiet_NaN(),0,0,0},delta),"non-finite quaternion rejected");
    HeadPose pose;pose.qpc=1000;pose.runtimeConnected=pose.deviceConnected=pose.valid=true;pose.trackingResult=15;pose.quaternion[0]=1;
    check(FreshOrientation(pose,1049,1000,50),"fresh tracked orientation accepted");
    check(!FreshOrientation(pose,1051,1000,50),"stale pose rejected");
    check(!FreshOrientation(pose,999,1000,50),"future timestamp rejected");
    pose.trackingResult=3;check(!FreshOrientation(pose,1000,1000,50),"valid but untracked orientation rejected");
    pose.trackingResult=15;pose.deviceConnected=false;check(!FreshOrientation(pose,1000,1000,50),"disconnected headset rejected");
    std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
