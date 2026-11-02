#include "RoomScale.h"
#include "GameStereoMath.h"
#include "TrackedBodyMath.h"
#include "HeadBodyHeading.h"
#include <cstdio>
#include <initializer_list>
#include <limits>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
float Rad(float degrees){return degrees*.01745329252f;}
Orientation Q(float degrees){float a=Rad(degrees)/2;return {std::cos(a),0,-std::sin(a),0};}
void Pose(HeadPose& head,Orientation q){head.quaternion[0]=q.w;head.quaternion[1]=q.x;head.quaternion[2]=q.y;head.quaternion[3]=q.z;}
bool Near(float a,float b){return std::fabs(a-b)<.06f;}
bool Eye(Rotation3 const& native,float const (&root)[3],float forward,Orientation center,float const (&origin)[3],HeadPose const& head,float (&eye)[3]){
    float base[3]{root[0],root[1]+170,root[2]},euler[3]{};ForwardCameraOffset(native,forward,base);
    StereoEyePose pose;for(int i=0;i<3;++i)pose.position[i]=head.position[i];
    pose.quaternion[0]=head.quaternion[1];pose.quaternion[1]=head.quaternion[2];pose.quaternion[2]=head.quaternion[3];pose.quaternion[3]=head.quaternion[0];
    return GameEyeTransform(native,base,center,origin,pose,100,eye,euler);
}
int main(){
    HeadPose head;head.qpc=1000;head.session=1;head.runtimeConnected=head.deviceConnected=head.valid=true;head.trackingResult=15;
    HeadBodyHeading facing;Rotation3 bodyHeading;float root[3]{87600,5300,-9500},origin[3]{.2f,1.6f,-.3f};
    // A rotated headset's physical eye position travels around the neck.
    // Returning the NECK offset prevents that arc from becoming a body step.
    for(float baseYaw:{0.f,91.f,179.f,-179.f,-90.f})for(float turn:{0.f,29.f,50.f,90.f,180.f,270.f,359.f})for(float forward:{-15.f,0.f,15.f,30.f}){
        float angles[3]{0,Rad(baseYaw),0};auto native=NativeCameraRotation(angles);Pose(head,Q(turn));
        head.position[0]=origin[0]+forward*.01f*std::sin(Rad(turn));head.position[1]=origin[1];
        head.position[2]=origin[2]+forward*.01f*(1-std::cos(Rad(turn)));
        float neck[3]{};CHECK(RoomNeckOffset({},native,head,origin,100,forward,neck));
        CHECK(std::hypot(neck[0],neck[2])<.001f&&neck[1]==0);
    }
    // Replay forward/backward motion, native turns and physical turns at real
    // Gothic world coordinates. Both the neutral torso and legs use one body
    // root; eye/controller transforms remain in the original XR frame.
    for(int frame=0;frame<1200;++frame){
        float turn=float(frame%360),baseYaw=float((frame*7)%360),forward=15;
        float angles[3]{0,Rad(baseYaw),0};auto native=NativeCameraRotation(angles);
        Pose(head,Q(turn));head.position[0]=origin[0]+.08f*std::sin(frame*.13f);head.position[2]=origin[2]+.08f*std::cos(frame*.13f);
        head.position[1]=origin[1]+(frame%2?-.6f:.15f);root[0]+=frame%2?2:-3;root[2]+=frame%3?1:-2;
        CHECK(facing.Step(1,native,{},head,1000,1000,bodyHeading,frame%3!=0));
        float neck[3]{},eye[3]{};CHECK(RoomNeckOffset({},native,head,origin,100,forward,neck));CHECK(Eye(native,root,forward,{},origin,head,eye));
        arms::Mat entity,body;arms::Position(entity,{root[0],root[1],root[2]});CHECK(arms::BodyWorld(entity,bodyHeading,body,{neck[0],0,neck[2]}));
        float viewYaw=Rad(baseYaw+turn);auto p=arms::Position(body);
        CHECK(Near(p.x+std::sin(viewYaw)*forward,eye[0])&&Near(p.z+std::cos(viewYaw)*forward,eye[2]));
        CHECK(p.y==root[1]); // Crouching/eye-height changes never move feet through ground.
        // A native collision-accepted room rebase changes the XR origin and
        // collider together. Neither eye nor body may jump after consuming it.
        float accepted[3]{neck[0]*.4f,0,neck[2]*.4f},rebased[3]{root[0]+accepted[0],root[1],root[2]+accepted[2]},newOrigin[3]{origin[0],origin[1],origin[2]};
        CHECK(ConsumeRoomStep({},native,accepted,100,newOrigin));float afterEye[3]{},afterNeck[3]{};
        CHECK(Eye(native,rebased,forward,{},newOrigin,head,afterEye)&&RoomNeckOffset({},native,head,newOrigin,100,forward,afterNeck));
        for(int i=0;i<3;++i)CHECK(Near(eye[i],afterEye[i]));
        CHECK(Near(root[0]+neck[0],rebased[0]+afterNeck[0])&&Near(root[2]+neck[2],rebased[2]+afterNeck[2]));
    }
    // Arbitrary recenter yaw and vertical gaze keep a finite horizontal neck.
    Pose(head,Product(Q(70),Q(45)));float neutralOrigin[3]{head.position[0],head.position[1],head.position[2]},offset[3]{};
    CHECK(RoomNeckOffset(Q(70),{},head,neutralOrigin,100,15,offset));
    CHECK(Near(offset[0],-15*std::sin(Rad(45)))&&Near(offset[2],15*(1-std::cos(Rad(45)))));
    Pose(head,{std::cos(Rad(90)/2),std::sin(Rad(90)/2),0,0});CHECK(RoomNeckOffset({}, {},head,neutralOrigin,100,15,offset)&&offset[1]==0);
    CHECK(!RoomNeckOffset({}, {},head,neutralOrigin,100,std::numeric_limits<float>::quiet_NaN(),offset));
    puts("PASS neck pivot rotation, forward/backward loops without drift, shared torso/leg anchor, grounded Y, room-transfer eye/body continuity, recenter and vertical gaze");return 0;
}
