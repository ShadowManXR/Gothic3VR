#include "HeadBodyHeading.h"
#include "GameStereoMath.h"
#include "TrackedBodyMath.h"
#include "NativeSprintPolicy.h"
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
float radians(float degrees){return degrees*.01745329252f;}
Orientation PhysicalYaw(float angle){return {std::cos(angle/2),0,-std::sin(angle/2),0};}
void Set(HeadPose& h,Orientation q){h.quaternion[0]=q.w;h.quaternion[1]=q.x;h.quaternion[2]=q.y;h.quaternion[3]=q.z;}
float Yaw(Rotation3 const& rotation){float yaw=0;return HorizontalHeading(rotation,yaw)?yaw:999;}
bool Near(float a,float b){return std::fabs(std::atan2(std::sin(a-b),std::cos(a-b)))<.0001f;}
int main(){
    HeadBodyHeading following;HeadPose h;h.qpc=1000;h.session=1;h.runtimeConnected=h.deviceConnected=h.valid=true;h.trackingResult=4;Set(h,{});
    Rotation3 native,out;CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),0));
    for(float angle:{3.f,8.f,-8.f,15.f,-15.f,25.f,-25.f,29.9f,-29.9f}){Set(h,PhysicalYaw(radians(angle)));CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),0));}
    Set(h,PhysicalYaw(radians(50)));CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),radians(20)));
    for(int i=0;i<1000;++i)CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),radians(20))); // both eyes / extra render passes do not drift
    float base[3]{0,radians(90),0};native=NativeCameraRotation(base);
    CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),radians(110))); // controller turn + physical yaw once
    CHECK(Near(out.m[4],1)&&Near(out.m[1],0)&&Near(out.m[7],0));
    CHECK(BodyLocomotionIntent(0,1)&&BodyLocomotionIntent(0,-1)&&BodyLocomotionIntent(1,0)&&!BodyLocomotionIntent(0,0));
    CHECK(following.Step(1,native,{},h,1000,1000,out,true)&&Near(Yaw(out),radians(140))); // Walking removes the torso/forward-direction gap.
    CHECK(following.Step(1,native,{},h,1000,1000,out,true)&&Near(Yaw(out),radians(140))); // No accumulation across render/physics.
    // Looking vertically or rolling the head does not tip the body over.
    Set(h,{std::cos(radians(90)/2),std::sin(radians(90)/2),0,0});
    CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),radians(140))&&out.m[4]==1);
    // Full physical revolutions and yaw wrap retain a bounded free-look cone.
    following.Reset();native={};
    for(int degree=0;degree<=1080;++degree){Set(h,PhysicalYaw(radians(float(degree))));CHECK(following.Step(1,native,{},h,1000,1000,out));
        CHECK(std::fabs(std::atan2(std::sin(radians(float(degree))-Yaw(out)),std::cos(radians(float(degree))-Yaw(out))))<=HeadBodyHeading::FreeLookRadians+.0001f);}
    auto center=PhysicalYaw(radians(70));following.Reset();Set(h,center);CHECK(following.Step(1,native,center,h,1000,1000,out)&&Near(Yaw(out),0));
    Set(h,Product(center,PhysicalYaw(radians(-60))));CHECK(following.Step(1,native,center,h,1000,1000,out)&&Near(Yaw(out),radians(-30)));
    ++h.session;Set(h,{});CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),0));
    // Backward and sideways input changes gait, not facing by 180/90 degrees.
    for(float direction:{-1.f,1.f}){Set(h,PhysicalYaw(radians(80)));
        CHECK(following.Step(1,native,{},h,1000,1000,out,BodyLocomotionIntent(0,direction))&&Near(Yaw(out),radians(80)));}
    Set(h,PhysicalYaw(radians(100)));CHECK(following.Step(1,native,{},h,1000,1000,out)&&Near(Yaw(out),radians(80))); // Rest resumes 30-degree free look around aligned torso.
    Set(h,PhysicalYaw(radians(80)));CHECK(following.Step(1,native,{},h,1000,1000,out));Set(h,{});
    CHECK(following.Step(2,native,{},h,1000,1000,out)&&Near(Yaw(out),0));
    CHECK(!following.Step(2,native,{},h,1101,1000,out)&&!following.Step(2,native,{},h,999,1000,out));
    h.trackingResult=0;CHECK(!following.Step(2,native,{},h,1000,1000,out));h.trackingResult=4;
    h.quaternion[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!following.Step(2,native,{},h,1000,1000,out));
    // Body heading is a separate parameter: its deadzone never feeds into
    // camera/HMD/controller transforms, scale, position or weapon calibration.
    following.Reset();Set(h,PhysicalYaw(radians(90)));StereoEyePose eye{};eye.quaternion[3]=h.quaternion[0];eye.quaternion[1]=h.quaternion[2];
    float position[3]{1000,2000,-3000},origin[3]{},beforeP[3],beforeE[3],afterP[3],afterE[3];
    CHECK(GameEyeTransform(native,position,{},origin,eye,100,beforeP,beforeE));
    CHECK(following.Step(1,native,{},h,1000,1000,out));
    arms::Mat entity,body;arms::Position(entity,{1000,2000,-3000});CHECK(arms::BodyWorld(entity,out,body));
    CHECK(arms::Length(arms::Position(entity)-arms::Position(body))<.001f);
    CHECK(GameEyeTransform(native,position,{},origin,eye,100,afterP,afterE));
    for(int i=0;i<3;++i)CHECK(beforeP[i]==afterP[i]&&beforeE[i]==afterE[i]);
    // A native script/target may change alignment between input and physics;
    // ownership checks must never restore over it or into another actor.
    BodyFacingLease lease;float old[3]{0,0,1},desired[3]{1,0,0},restore[3]{};lease.Begin(1,2,old,desired);
    CHECK(!lease.Take(2,2,false,desired,restore)&&lease.Matches(1,2));CHECK(lease.Take(1,2,false,desired,restore)&&lease.Empty());
    for(int i=0;i<3;++i)CHECK(old[i]==restore[i]);
    lease.Begin(1,2,old,desired);CHECK(!lease.Take(1,2,true,desired,restore)&&lease.Empty());
    lease.Begin(1,2,old,desired);CHECK(!lease.Take(1,2,false,old,restore)&&lease.Empty());
    lease.Begin(1,2,old,desired);lease.Forget();CHECK(!lease.Take(1,2,false,desired,restore));
    puts("PASS physical yaw + native turn once, 30-degree free-look, upright body, vertical look, wrapping, recenter/session/save resets, native alignment ownership and unchanged eye transform");return 0;
}
