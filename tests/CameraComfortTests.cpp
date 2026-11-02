#include "NativeCameraStabilizer.h"
#include "CameraComfort.h"
#include "StereoCameraState.h"
#include "GameStereoMath.h"
#include "RoomScale.h"
#include "WorldAim.h"
#include "TrackedBodyMath.h"
#include <cstdio>
#include <limits>

int main() {
    unsigned failures=0,checks=0;
    auto check=[&](bool ok,char const* text){++checks;if(!ok){++failures;printf("FAIL %s\n",text);}};
    auto close=[](float a,float b){return std::fabs(a-b)<.002f;};
    diagnostic::NativeCameraStabilizer camera;
    float root[3]{100,50,300},out[3]{};
    check(camera.Update(1,root,1,100,40,100)&&camera.Apply(1,root,out)&&close(out[0],100)&&close(out[1],50)&&close(out[2],300),"first render establishes an exact actor origin");
    root[0]+=20;root[1]+=10;
    camera.Update(1,root,1.01,100,40,100);camera.Apply(1,root,out);
    check(out[0]>100&&out[0]<105&&out[1]>50&&out[1]<51,"a native action impulse is attenuated, with slower terrain Y");
    float first[3]{out[0],out[1],out[2]};
    camera.Update(1,root,1.01,100,40,100);camera.Apply(1,root,out);
    check(close(out[0],first[0])&&close(out[1],first[1]),"duplicate eye/focus queries do not advance the native filter");
    bool monotonic=true;
    for(unsigned frame=2;frame<=100;++frame){camera.Update(1,root,1+frame*.01,100,40,100);camera.Apply(1,root,out);monotonic&=out[0]>=first[0]&&out[0]<=root[0]&&out[1]>=first[1]&&out[1]<=root[1];first[0]=out[0];first[1]=out[1];}
    check(monotonic&&close(out[0],root[0])&&close(out[1],root[1]),"an isolated native jump converges without oscillation or overshoot");

    // Replay root noise superimposed on a normal 390 cm/s run. Measure changes
    // around the intended velocity, not just distance to a filtered fixture.
    for(float hz:{45.f,90.f,120.f}) {
        camera.Reset();float previousRaw=0,previousView=0;double rawJitter=0,viewJitter=0;
        for(unsigned frame=0;frame<=unsigned(hz*3);++frame) {
            double time=1+frame/double(hz);root[0]=100;root[1]=50+2*std::sin(float(time)*75);root[2]=300+390*float(time-1)+(frame%2?4.f:-4.f);
            camera.Update(1,root,time,100,40,100);camera.Apply(1,root,out);
            if(frame>unsigned(hz)) {rawJitter+=std::pow(root[2]-previousRaw-390/hz,2);viewJitter+=std::pow(out[2]-previousView-390/hz,2);}
            previousRaw=root[2];previousView=out[2];
        }
        check(viewJitter<rawJitter*.1,"native forward/backward jitter during running is reduced at different render cadences");
        check(root[2]-out[2]<25&&root[2]-out[2]>0,"continuous native running has a small bounded trailing origin, not a frozen camera");
    }
    // A paused world, teleport or new player resets instead of dragging the
    // view from the old save through geometry; this never recenters XR.
    root[0]=1000;camera.Update(1,root,5,100,40,100);camera.Apply(1,root,out);
    check(close(out[0],root[0]),"teleport/long pause immediately establishes the new native origin");
    root[1]+=30;camera.Update(2,root,5.01,100,40,100);camera.Apply(2,root,out);
    check(close(out[1],root[1])&&!camera.Apply(1,root,out),"a new actor cannot inherit a previous actor's correction");
    root[2]+=10;camera.Update(2,root,5.02,100,0,0);camera.Apply(2,root,out);
    check(close(out[2],root[2]),"zero time constants bypass native smoothing");
    float bad[3]{0,std::numeric_limits<float>::quiet_NaN(),0};
    check(!camera.Update(2,bad,5.03,100,40,100)&&!camera.Apply(2,root,out),"invalid native data cannot poison subsequent stereo frames");
    camera.Update(2,root,5.04,100,40,100);camera.Apply(2,root,out);
    check(close(out[2],root[2]),"valid data recovers with a clean origin");

    // Replay the actual transfer order: actor+body move, XR consumes X/Z,
    // then the renderer filters the native origin. Nonzero center and camera
    // yaw ensure the cancellation isn't limited to the forward Z axis.
    for(float yaw:{0.f,1.2f})for(float direction:{-1.f,1.f}) {
        camera.Reset();float nativeAngles[3]{0,yaw,0};auto native=diagnostic::NativeCameraRotation(nativeAngles);
        diagnostic::Orientation center{std::cos(.35f),0,std::sin(.35f),0};float trackingOrigin[3]{0,1.6f,0};
        diagnostic::StereoEyePose eye{{.16f,1.35f,-.3f},{0,center.y,0,center.w},{}};
        diagnostic::ControllerPose hand{true,{.25f,1.2f,-.55f},{0,center.y,0,center.w}};
        root[0]=100;root[1]=50;root[2]=300;camera.Update(1,root,1,100,40,100);
        for(unsigned frame=1;frame<=180;++frame) {
            root[2]+=390.f/90;camera.Update(1,root,1+frame/90.,100,40,100);camera.Apply(1,root,out);
            float before[3]{},after[3]{},euler[3]{};diagnostic::WorldAim aimBefore,aimAfter;
            diagnostic::GameEyeTransform(native,out,center,trackingOrigin,eye,100,before,euler);
            diagnostic::ControllerWorldAim(native,out,center,trackingOrigin,hand,100,aimBefore);
            float accepted[3]{direction*1.5f,0,direction*2.5f};
            for(unsigned i=0;i<3;++i)root[i]+=accepted[i];
            camera.RebaseRoomTransfer(1,accepted);
            check(diagnostic::ConsumeRoomStep(center,native,accepted,100,trackingOrigin),"room transfer consumes the shared XR origin");
            camera.Apply(1,root,out);diagnostic::GameEyeTransform(native,out,center,trackingOrigin,eye,100,after,euler);
            diagnostic::ControllerWorldAim(native,out,center,trackingOrigin,hand,100,aimAfter);
            check(close(before[0],after[0])&&close(before[1],after[1])&&close(before[2],after[2]),"forward/backward room consumption never drags the stabilized eye backwards");
            check(close(aimBefore.origin[0],aimAfter.origin[0])&&close(aimBefore.origin[2],aimAfter.origin[2]),"controller targeting remains continuous through the same transfer");
        }
        // No filter is applied to physical head/hand movement after the base.
        float before[3]{},after[3]{},euler[3]{},newPose[3]{eye.position[0]+.08f,eye.position[1]-.2f,eye.position[2]-.12f};
        diagnostic::GameEyeTransform(native,out,center,trackingOrigin,eye,100,before,euler);
        for(unsigned i=0;i<3;++i)eye.position[i]=newPose[i];
        diagnostic::GameEyeTransform(native,out,center,trackingOrigin,eye,100,after,euler);
        check(close(after[1]-before[1],-20)&&close(std::hypot(after[0]-before[0],after[2]-before[2]),std::hypot(8.f,12.f)),"physical stepping and crouching reach the rendered eye in full immediately");
        float neutralEuler[3]{euler[0],euler[1],euler[2]};
        diagnostic::Orientation tilted=diagnostic::Product(center,{std::cos(.15f),std::sin(.15f),0,0});
        eye.quaternion[0]=tilted.x;eye.quaternion[1]=tilted.y;eye.quaternion[2]=tilted.z;eye.quaternion[3]=tilted.w;
        diagnostic::GameEyeTransform(native,out,center,trackingOrigin,eye,100,after,euler);
        check(diagnostic::RotationError(diagnostic::NativeCameraRotation(neutralEuler),diagnostic::NativeCameraRotation(euler))>.1f,"headset rotation remains immediate and independent of native translation stabilization");
        diagnostic::WorldAim aimBefore,aimAfter;diagnostic::ControllerWorldAim(native,out,center,trackingOrigin,hand,100,aimBefore);
        hand.position[0]+=.1f;diagnostic::ControllerWorldAim(native,out,center,trackingOrigin,hand,100,aimAfter);
        float handDistance=0;for(unsigned i=0;i<3;++i)handDistance+=std::pow(aimAfter.origin[i]-aimBefore.origin[i],2);
        check(close(std::sqrt(handDistance),10),"physical hand movement is not delayed by the native origin filter");
    }
    root[0]=100;root[1]=50;root[2]=300;camera.Reset();camera.Update(1,root,1,100,40,100);
    float terrainStep[3]{5,10,0};root[0]+=5;root[1]+=10;camera.RebaseRoomTransfer(1,terrainStep);
    camera.Update(1,root,1.01,100,40,100);camera.Apply(1,root,out);
    check(close(out[0],105)&&out[1]<51,"room transfer is immediate horizontally while native terrain Y is stabilized");
    diagnostic::arms::Mat entity;diagnostic::arms::Position(entity,{root[0],root[1],root[2]});auto beforeEntity=entity;diagnostic::arms::Mat body;
    check(diagnostic::arms::BodyWorld(entity,{},body,{out[0]-root[0],out[1]-root[1],out[2]-root[2]})&&close(diagnostic::arms::Position(body).y,out[1]),"rendered neutral torso shares the camera correction");
    bool unchanged=true;for(unsigned i=0;i<16;++i)unchanged&=entity.m[i]==beforeEntity.m[i];
    check(unchanged,"visual body stabilization does not move the native collider/entity");

    diagnostic::StereoCameraState state{true,true,false,false,false};
    check(diagnostic::ComfortFirstPerson(state,true,true,false),"a transient native third-person mode cannot expose a camera jump after calibration");
    check(!diagnostic::ComfortFirstPerson(state,true,false,false)&&!diagnostic::ComfortFirstPerson(state,false,true,false),"uncalibrated or voluntarily third-person cameras do not fabricate a first-person origin");
    state.firstPerson=true;
    check(!diagnostic::ComfortFirstPerson(state,true,false,true)&&diagnostic::ComfortFirstPerson(state,true,true,true),"first loading waits for the real camera height while later mode switches retain calibration");
    state.dialog=true;check(!diagnostic::ComfortFirstPerson(state,true,true,false),"conversations retain their native third-person camera");
    state.dialog=false;state.animal=true;check(!diagnostic::ComfortFirstPerson(state,true,true,false),"animal transformation overrides persistent first-person without leaving VR");
    check(diagnostic::StereoCameraAllowed(state,true),"animal camera remains in an existing stereo session");
    state.animal=false;check(diagnostic::ComfortFirstPerson(state,true,true,false),"human form resumes first-person policy");
    state.dialog=false;state.gameRunning=false;check(!diagnostic::ComfortFirstPerson(state,true,true,false),"main/loading menu cannot reuse a stale player camera");
    camera.Reset();check(!camera.Apply(1,root,out),"explicit recenter discards accumulated native correction");
    printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
