#include "RoomScale.h"
#include "GameStereoMath.h"
#include "RoomNativeState.h"
#include <cstdio>
#include <limits>
int main(){
    unsigned failures=0,checks=0;auto check=[&](bool ok,char const* what){++checks;if(!ok){++failures;printf("FAIL %s\n",what);}};
    auto close=[](float a,float b){return std::fabs(a-b)<.002f;};
    diagnostic::HeadPose pose;pose.qpc=1000;pose.session=2;pose.valid=pose.runtimeConnected=pose.deviceConnected=true;pose.trackingResult=15;pose.quaternion[0]=1;
    check(diagnostic::RoomPositionTracked(pose,1050,1000),"fresh full positional tracking permits physical walking");
    pose.trackingResult=7;check(!diagnostic::RoomPositionTracked(pose,1050,1000),"orientation-only tracking cannot move the character");pose.trackingResult=15;
    check(!diagnostic::RoomPositionTracked(pose,1101,1000),"stale headset pose cannot continue walking");
    pose.position[0]=std::numeric_limits<float>::quiet_NaN();check(!diagnostic::RoomPositionTracked(pose,1050,1000),"nonfinite physical position is rejected");
    check(diagnostic::RoomWalkingMode(1)&&diagnostic::RoomWalkingMode(4)&&!diagnostic::RoomWalkingMode(6)&&!diagnostic::RoomWalkingMode(14),"standing and ground walking supported, jump and ragdoll remain native");
    for(float yaw:{0.f,.7f,2.2f})for(float recenterYaw:{0.f,-.4f,1.3f}){
        float angles[3]{.3f,yaw,.2f};auto native=diagnostic::UprightCamera(diagnostic::NativeCameraRotation(angles));
        diagnostic::Orientation center{std::cos(recenterYaw*.5f),0,std::sin(recenterYaw*.5f),0};
        float origin[3]{3,1.6f,-4},head[3]{3.6f,1.2f,-4.3f},world[3]{},step[3]{};
        check(diagnostic::RoomWorldOffset(center,native,head,origin,100,world)&&close(world[1],0),"physical crouch and slope pitch never move the root vertically");
        check(diagnostic::RoomStep(world,100,step)&&std::hypot(step[0],step[2])<=12.001f,"large normal steps transfer through bounded collision requests");
        float accepted[3]{step[0]*.4f,2,step[2]*.4f};check(diagnostic::RoomAccepted(step,accepted,100,30),"native collision-clipped movement and terrain height accepted");
        float base[3]{87600,5300,-9500},oldEye[3]{},newEye[3]{},euler[3]{};
        diagnostic::StereoEyePose eye{{head[0]-.032f,head[1],head[2]},{0,center.y,0,center.w},{}};
        diagnostic::GameEyeTransform(native,base,center,origin,eye,100,oldEye,euler);
        float originalY=origin[1];check(diagnostic::ConsumeRoomStep(center,native,accepted,100,origin)&&origin[1]==originalY,"only accepted horizontal displacement rebases the shared head / hand tracking origin");
        base[0]+=accepted[0];base[1]+=accepted[1];base[2]+=accepted[2];diagnostic::GameEyeTransform(native,base,center,origin,eye,100,newEye,euler);
        // Large world positions have an approximately 0.008cm float ULP.
        check(std::fabs(oldEye[0]-newEye[0])<.03f&&std::fabs(oldEye[2]-newEye[2])<.03f&&close(newEye[1]-oldEye[1],2),"transfer preserves rendered head location without double walking and keeps native terrain height");
        float blocked[3]{};auto copy=origin[0];check(diagnostic::ConsumeRoomStep(center,native,blocked,100,origin)&&close(copy,origin[0]),"blocked body does not consume physical displacement");
    }
    float offset[3]{4,0,4},step[3]{};check(!diagnostic::RoomStep(offset,100,step),"small leaning stays optical without root jitter");
    offset[0]=200;check(!diagnostic::RoomStep(offset,100,step),"runtime origin jumps do not teleport the character");
    float backlog[3]{80,0,0};
    for(float dt:{1.f/45.f,1.f/90.f,1.f/120.f}){
        check(diagnostic::RoomStep(backlog,100,step,dt)&&close(step[0],300.f*dt),"root catch-up is time based at 45 / 90 / 120 Hz rather than twelve centimetres per frame");
    }
    check(diagnostic::RoomStep(backlog,100,step,2.f)&&step[0]<=12.001f,"a tracking or loading pause cannot make the root teleport across the world");
    check(!diagnostic::RoomStep(backlog,100,step,0)&&!diagnostic::RoomStep(backlog,100,step,-1)&&
        !diagnostic::RoomStep(backlog,100,step,std::numeric_limits<float>::quiet_NaN()),"first frame and invalid frame times do not transfer the root");
    float request[3]{10,0,0},teleport[3]{100,0,0},wrong[3]{-10,0,0},drop[3]{10,-100,0};
    check(!diagnostic::RoomAccepted(request,teleport,100,30)&&!diagnostic::RoomAccepted(request,wrong,100,30)&&!diagnostic::RoomAccepted(request,drop,100,30),"invalid collision result cannot teleport, reverse, or drop the actor");
    // Replay a joystick run while a second sensor query resolves a physical
    // step. The old implementation overwrote the native goal and triggered
    // ResetMotion during MoveWorld. Preserve all native sensor frame flags
    // and velocities; translate absolute goals by just the accepted step.
    for(unsigned processing:{0u,1u}){
        unsigned char sensor[0x60],movement[0x3dc];memset(sensor,0x5a,sizeof(sensor));memset(movement,0xa5,sizeof(movement));movement[0x264]=static_cast<unsigned char>(processing);
        float root[3]{1000,2000,-3000},joystickGoal[3]{1000,2000,-2950};
        for(unsigned offset:{0x14u,0x20u,0x2cu})memcpy(sensor+offset,root,sizeof(root));memcpy(sensor+0x44,joystickGoal,sizeof(root));
        memcpy(movement+0x2c0,joystickGoal,sizeof(root));memcpy(movement+0x360,joystickGoal,sizeof(root));
        float framePosition[3]{root[0],root[1],root[2]-5};memcpy(movement+0x11c,framePosition,sizeof(framePosition));
        unsigned char originalSensor[0x60],originalMovement[0x3dc];memcpy(originalSensor,sensor,sizeof(sensor));memcpy(originalMovement,movement,sizeof(movement));
        diagnostic::RoomNativeState nativeState;nativeState.Save(sensor,movement);
        float roomGoal[3]{1000,2000,-3010};diagnostic::RoomNativeState::Request(sensor,roomGoal);
        memset(sensor+0x14,0xcc,0x4c);nativeState.RestoreSensor(sensor);
        check(!memcmp(sensor,originalSensor,sizeof(sensor))&&!memcmp(movement,originalMovement,sizeof(movement)),"collision query and abort restore joystick goal, flags, sensor smoothing and movement state exactly");
        diagnostic::RoomNativeState::BeginTransfer(movement);check(movement[0x264]==1,"room body transfer uses the native guard against ResetMotion");
        float acceptedRoom[3]{0,2,-6};nativeState.Commit(sensor,movement,acceptedRoom);
        float translated[3];memcpy(translated,movement+0x2c0,sizeof(translated));
        check(close(translated[2]-(root[2]+acceptedRoom[2]),50)&&close(translated[1],2002),"forward joystick goal stays fifty centimetres ahead while a backward physical step and terrain height add independently");
        float history[3];memcpy(history,movement+0x11c,sizeof(history));
        check(close(root[2]+acceptedRoom[2]-history[2],5)&&close(root[1]+acceptedRoom[1]-history[1],0),"a backward physical step does not reverse the native forward frame displacement or inject terrain motion into animation velocity");
        for(unsigned offset:{0x14u,0x20u,0x2cu,0x44u}){float before[3];memcpy(before,originalSensor+offset,sizeof(before));diagnostic::RoomNativeState::Translate(originalSensor+offset,before,acceptedRoom);}
        for(unsigned offset:{0x11cu,0x2c0u,0x360u}){float before[3];memcpy(before,originalMovement+offset,sizeof(before));diagnostic::RoomNativeState::Translate(originalMovement+offset,before,acceptedRoom);}
        check(!memcmp(sensor,originalSensor,sizeof(sensor))&&!memcmp(movement,originalMovement,sizeof(movement)),"successful room step changes only absolute positions; velocity, run mode, input and guard are preserved");
    }
    // With stale frame history, a 6 cm backward room step turns a 5 cm
    // forward joystick frame into -1 cm of apparent native animation motion.
    for(float physical:{-6.f,6.f}){
        unsigned char sensor[0x60]{},movement[0x3dc]{};
        float previous[3]{1000,2000,-3000},root[3]{1000,2000,-2995};
        for(unsigned frame=0;frame<120;++frame){
            memcpy(movement+0x11c,previous,sizeof(previous));
            for(unsigned offset:{0x14u,0x20u,0x2cu,0x44u})memcpy(sensor+offset,root,sizeof(root));
            memcpy(movement+0x2c0,root,sizeof(root));memcpy(movement+0x360,root,sizeof(root));
            diagnostic::RoomNativeState state;state.Save(sensor,movement);float delta[3]{0,0,physical};state.Commit(sensor,movement,delta);
            root[2]+=physical;float history[3];memcpy(history,movement+0x11c,sizeof(history));
            check(close(root[2]-history[2],5),"repeated forward/backward room rebases leave joystick animation and correction displacement unchanged");
            memcpy(previous,root,sizeof(root));root[2]+=5;
        }
    }
    printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}

