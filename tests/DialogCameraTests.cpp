#include "DialogCameraLock.h"
#include "GameStereoMath.h"
#include "StereoCameraState.h"
#include <cstdio>
#include <cstring>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool Same(float const (&a)[3],float const (&b)[3]){for(unsigned i=0;i<3;++i)if(std::fabs(a[i]-b[i])>.001f)return false;return true;}
int Framing(){
    // Reproduce the transition frame at the hero's head, at different headings,
    // world scales and nonzero room-scale displacement. Use the real stereo
    // transform again afterwards: testing the stored base alone would miss the
    // original inside-head bug when the player has moved in the real room.
    for(float units:{50.f,100.f,200.f})for(float yaw:{-2.4f,0.f,1.2f}){
        float angles[3]{0,yaw,0},root[3]{800,220,-450},origin[3]{800,220+1.65f*units,-450};
        auto native=NativeCameraRotation(angles);Orientation center{};float xrOrigin[3]{0,1.65f,0};
        StereoEyePose head{};head.position[0]=.22f;head.position[1]=1.75f;head.position[2]=-.1f;
        head.quaternion[1]=std::sin(.2f);head.quaternion[3]=std::cos(.2f);
        float eye[3]{},euler[3]{},offset[3]{},base[3]{};
        CHECK(GameEyeTransform(native,origin,center,xrOrigin,head,units,eye,euler));
        for(unsigned i=0;i<3;++i)offset[i]=eye[i]-origin[i];
        int rays=0;auto clear=[&](float const (&)[3],float const (&)[3]){++rays;return 1.f;};
        auto entryView=NativeCameraRotation(euler);
        CHECK(FrameDialogCamera(origin,root,offset,entryView,units,clear,base)&&rays==1);
        CHECK(GameEyeTransform(native,base,center,xrOrigin,head,units,eye,euler));
        CHECK(std::fabs(std::hypot(eye[0]-root[0],eye[2]-root[2])/units-std::hypot(2.f,.55f))<.001f);
        CHECK(std::fabs((eye[1]-root[1])/units-1.65f)<.001f);
        CHECK((eye[0]-root[0])*entryView.m[2]+(eye[2]-root[2])*entryView.m[8]<-1.99f*units);
        CHECK(center.w==1&&xrOrigin[1]==1.65f); // no recenter
        DialogCameraLock lock;CHECK(lock.Capture(true,1,native,base)&&lock.Active(1)&&!lock.Active(2));
        float locked[3]{};Rotation3 rotation;CHECK(lock.Apply(1,rotation,locked));
        head.position[0]+=.1f;float moved[3]{};
        CHECK(GameEyeTransform(rotation,locked,center,xrOrigin,head,units,moved,euler));
        float displacement=0;for(unsigned i=0;i<3;++i)displacement+=(moved[i]-eye[i])*(moved[i]-eye[i]);
        CHECK(std::fabs(std::sqrt(displacement)-.1f*units)<.002f);
    }
    Rotation3 view;float root[3]{},origin[3]{0,165,0},offset[3]{},base[3]{};
    int attempts=0;
    auto wall=[&](float const (&)[3],float const (&)[3]){return ++attempts==1?.2f:1.f;};
    CHECK(FrameDialogCamera(origin,root,offset,view,100,wall,base)&&attempts==2);
    CHECK(base[2]<-100&&std::hypot(base[0],base[2])>200); // clear rear-side alternative
    auto narrow=[](float const (&)[3],float const (&)[3]){return .5f;};
    CHECK(FrameDialogCamera(origin,root,offset,view,100,narrow,base));
    CHECK(std::hypot(base[0],base[2])>80&&std::hypot(base[0],base[2])<110);
    auto blocked=[](float const (&)[3],float const (&)[3]){return .1f;};
    CHECK(!FrameDialogCamera(origin,root,offset,view,100,blocked,base)); // never lock back inside the head
    auto invalid=[](float const (&)[3],float const (&)[3]){return std::numeric_limits<float>::quiet_NaN();};
    CHECK(!FrameDialogCamera(origin,root,offset,view,100,invalid,base));
    origin[2]=-300;int queries=0;
    auto unused=[&](float const (&)[3],float const (&)[3]){++queries;return 1.f;};
    CHECK(FrameDialogCamera(origin,root,offset,view,100,unused,base)&&Same(base,origin)&&queries==0);
    CHECK(!FrameDialogCamera(origin,root,offset,view,0,unused,base));
    origin[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!FrameDialogCamera(origin,root,offset,view,100,unused,base));
    return 0;
}
int PlayerView(){
    for(float units:{50.f,100.f,200.f}){
        DialogCameraLock camera;float root[3]{10000,300,-12000},base[3]{root[0]+.12f*units,root[1]+1.7f*units,root[2]+.2f*units};
        float angles[3]{0,.65f,0};auto gameplay=NativeCameraRotation(angles);
        CHECK(camera.RememberPlayer(1,gameplay,base,root,units));
        float sideAngles[3]{.4f,2.2f,.3f};auto sideView=NativeCameraRotation(sideAngles);
        CHECK(camera.CapturePlayer(1,root,sideView,9*units,4*units,units));
        float out[3]{};Rotation3 view;CHECK(camera.ApplyPlayer(1,root,view,out));
        CHECK(Same(out,base)&&RotationError(view,gameplay)<.0001f);
        for(unsigned frame=0;frame<300;++frame){
            float movedRoot[3]{root[0]+frame*.02f*units,root[1]+std::sin(frame*.1f)*.1f*units,root[2]-frame*.01f*units};
            CHECK(camera.CapturePlayer(1,movedRoot,sideView,12*units,8*units,units));
            CHECK(camera.ApplyPlayer(1,movedRoot,view,out)&&RotationError(view,gameplay)<.0001f);
            for(unsigned i=0;i<3;++i)CHECK(std::fabs((out[i]-movedRoot[i])-(base[i]-root[i]))<.003f);
        }
        CHECK(!camera.ApplyPlayer(2,root,view,out));
        Orientation center{};float xrOrigin[3]{0,1.65f,0};StereoEyePose eye{};eye.position[1]=1.65f;eye.quaternion[3]=1;
        float before[3],after[3],euler[3];CHECK(camera.ApplyPlayer(1,root,view,out));
        CHECK(GameEyeTransform(view,out,center,xrOrigin,eye,units,before,euler));
        eye.position[0]=.15f;eye.position[1]-=.1f;eye.quaternion[1]=std::sin(.2f);eye.quaternion[3]=std::cos(.2f);
        CHECK(GameEyeTransform(view,out,center,xrOrigin,eye,units,after,euler));
        float distance=0;for(unsigned i=0;i<3;++i)distance+=(after[i]-before[i])*(after[i]-before[i]);
        CHECK(std::fabs(std::sqrt(distance)-std::sqrt(.15f*.15f+.1f*.1f)*units)<.003f);
        CHECK(RotationError(NativeCameraRotation(euler),view)>.1f&&center.w==1&&xrOrigin[1]==1.65f);
        camera.Release();CHECK(!camera.Active());
        base[1]+=.1f*units;CHECK(camera.RememberPlayer(1,gameplay,base,root,units));
        CHECK(camera.CapturePlayer(1,root,sideView,9*units,4*units,units)&&camera.ApplyPlayer(1,root,view,out)&&Same(base,out));
        // World reset/cold-load cannot reuse a prior hero's pose or a native side shot.
        camera.Reset();CHECK(camera.CapturePlayer(2,root,gameplay,1.65f*units,.2f*units,units));
        CHECK(camera.ApplyPlayer(2,root,view,out)&&RotationError(view,gameplay)<.0001f);
        CHECK(std::fabs(out[1]-root[1]-1.65f*units)<.003f&&std::hypot(out[0]-root[0],out[2]-root[2])<.21f*units);
        camera.Reset();CHECK(!camera.CapturePlayer(2,root,gameplay,165,20,0));
        root[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!camera.RememberPlayer(2,gameplay,base,root,units));
        CHECK(!camera.CapturePlayer(2,root,gameplay,165,20,units));
    }
    return 0;
}
int main(){
    CHECK(PlayerView()==0);
    CHECK(Framing()==0);
    DialogCameraLock camera;float position[3]{100,180,-240},angles[3]{.12f,.65f,0};auto native=NativeCameraRotation(angles);
    auto original=native;float originalPosition[3]{100,180,-240},base[3]{};Rotation3 rotation;
    CHECK(!camera.Capture(false,1,native,position)&&!camera.Active()&&!camera.Apply(1,rotation,base));
    CHECK(camera.Capture(true,1,native,position)&&camera.Apply(1,rotation,base));
    CHECK(Same(position,originalPosition)&&RotationError(native,original)<.0001f); // input/native snapshot stays untouched
    for(unsigned frame=0;frame<600;++frame){
        angles[0]=std::sin(frame*.2f);angles[1]=frame*.1f;angles[2]=.3f;native=NativeCameraRotation(angles);
        position[0]=std::cos(frame*.1f)*400;position[1]=100+frame;position[2]=std::sin(frame*.1f)*400;
        CHECK(camera.Capture(true,1,native,position)&&camera.Apply(1,rotation,base));
        CHECK(RotationError(rotation,original)<.0001f&&Same(base,originalPosition));
    }
    // Both eyes consume the SAME fixed native base, followed by ordinary
    // tracked head pose. The lock must not freeze eyes in space or remove IPD.
    Orientation center{};float origin[3]{0,1.65f,0};
    StereoEyePose left{},right{};left.position[0]=-.032f;right.position[0]=.032f;
    left.position[1]=right.position[1]=1.65f;left.quaternion[3]=right.quaternion[3]=1;
    float lp[3]{},rp[3]{},le[3]{},re[3]{};
    CHECK(GameEyeTransform(rotation,base,center,origin,left,100,lp,le)&&GameEyeTransform(rotation,base,center,origin,right,100,rp,re));
    float ipd=0;for(unsigned i=0;i<3;++i)ipd+=(rp[i]-lp[i])*(rp[i]-lp[i]);CHECK(std::fabs(std::sqrt(ipd)-6.4f)<.002f);
    float before[3]{lp[0],lp[1],lp[2]},beforeEuler[3]{le[0],le[1],le[2]};
    left.position[0]+=.1f;left.position[1]-=.15f;left.position[2]-=.2f;
    left.quaternion[1]=std::sin(.2f);left.quaternion[3]=std::cos(.2f);
    CHECK(GameEyeTransform(rotation,base,center,origin,left,100,lp,le));
    float moved=0;for(unsigned i=0;i<3;++i)moved+=(lp[i]-before[i])*(lp[i]-before[i]);
    CHECK(std::fabs(std::sqrt(moved)-std::sqrt(100.f+225.f+400.f))<.003f);
    CHECK(RotationError(NativeCameraRotation(le),NativeCameraRotation(beforeEuler))>.1f);
    // Native camera cuts, different speakers and FOV changes do not own or
    // mutate the shared XR center. The lock stores no projection/eye state.
    CHECK(center.w==1&&center.x==0&&center.y==0&&center.z==0&&origin[1]==1.65f);
    StereoCameraState state{true,true,true,false,true};CHECK(!ComfortFirstPerson(state,true,true,false));
    state.dialog=false;CHECK(ComfortFirstPerson(state,true,true,false));
    CHECK(!camera.Capture(false,1,native,position)&&!camera.Apply(1,rotation,base));
    CHECK(camera.Capture(true,1,native,position));CHECK(camera.Capture(true,2,original,originalPosition));
    CHECK(!camera.Apply(1,rotation,base)&&camera.Apply(2,rotation,base)&&Same(base,originalPosition));
    camera.Reset();CHECK(!camera.Active());CHECK(camera.Capture(true,2,native,position)&&camera.Apply(2,rotation,base)&&Same(base,position));
    camera.Reset();position[1]=std::numeric_limits<float>::quiet_NaN();CHECK(!camera.Capture(true,2,native,position));
    position[1]=150;native.m[0]=2;CHECK(!camera.Capture(true,2,native,position));
    CHECK(camera.Capture(true,2,original,position)&&camera.Apply(2,rotation,base));
    CHECK(!camera.Capture(true,0,original,position)&&!camera.Active());
    puts("PASS player dialogue view, cold entry, root movement, native cuts ignored, stereo IPD, immediate 6DoF, intact XR center, exit/actor/world reset and invalid initial data");return 0;
}
