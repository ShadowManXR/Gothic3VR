#pragma once
#include "HeadControlMath.h"
#include "CameraComfort.h"
namespace diagnostic {
inline bool RoomWalkingMode(int mode){return mode>=1&&mode<=5;}
inline bool RoomPositionTracked(HeadPose const& head,int64_t now,int64_t frequency){
    if((head.trackingResult&0xa)!=0xa||!FreshOrientation(head,now,frequency,100))return false;
    for(float v:head.position)if(!std::isfinite(v))return false;return true;
}
// The same transform as GameEyeTransform: inverse recenter rotation, reverse Z,
// then the upright native camera heading. Never put headset Y into actor motion.
inline bool RoomWorldOffset(Orientation center,Rotation3 native,float const (&head)[3],float const (&origin)[3],float units,float (&worldOffset)[3]){
    if(!Normalize(center)||!std::isfinite(units)||units<=0)return false;
    auto c=Matrix(center);native=UprightCamera(native);float local[3]{};
    for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k){if(!std::isfinite(head[k])||!std::isfinite(origin[k]))return false;local[i]+=c.m[k*3+i]*(head[k]-origin[k]);}
    local[2]=-local[2];local[1]=0;
    for(unsigned i=0;i<3;++i){worldOffset[i]=0;for(unsigned k=0;k<3;++k)worldOffset[i]+=native.m[i*3+k]*local[k]*units;if(!std::isfinite(worldOffset[i]))return false;}
    worldOffset[1]=0;return true;
}
// The tracking device sits in front of the neck/root. Camera forward offset
// must rotate around that neck, rather than making a head turn look like a
// lateral body step. Use this SAME horizontal target for physics and the rig.
inline bool RoomNeckOffset(Orientation center,Rotation3 const& native,HeadPose const& head,float const (&origin)[3],float units,float forward,float (&offset)[3]){
    if(!std::isfinite(forward)||!RoomWorldOffset(center,native,head.position,origin,units,offset))return false;
    Rotation3 relative;if(!RelativeHeadRotation(center,{head.quaternion[0],head.quaternion[1],head.quaternion[2],head.quaternion[3]},relative))return false;
    auto world=Product(UprightCamera(native),relative);float viewYaw=0,baseYaw=0;
    if(!HorizontalHeading(world,viewYaw)){
        // The headset's right axis remains defined at vertical gaze. Never
        // reset the horizontal neck pivot to world-forward when looking up.
        if(!std::isfinite(world.m[0])||!std::isfinite(world.m[6])||std::hypot(world.m[0],world.m[6])<.05f)return false;
        viewYaw=std::atan2(-world.m[6],world.m[0]);
    }
    if(!HorizontalHeading(native,baseYaw))return false;
    offset[0]+=forward*(std::sin(baseYaw)-std::sin(viewYaw));
    offset[2]+=forward*(std::cos(baseYaw)-std::cos(viewYaw));
    return std::isfinite(offset[0])&&std::isfinite(offset[2]);
}
inline bool RoomStep(float const (&offset)[3],float units,float (&step)[3],float elapsedSeconds=1.f/90.f){
    for(float v:offset)if(!std::isfinite(v))return false;
    if(!std::isfinite(units)||units<=0||!std::isfinite(elapsedSeconds)||elapsedSeconds<=0)return false;
    float distance=std::hypot(offset[0],offset[2]);
    // Allow small natural leaning; catch up in bounded steps. Origin jumps are
    // handled by recentering, never by teleporting the player across the world.
    if(distance<=.08f*units||distance>1.5f*units)return false;
    // Bound root catch-up by elapsed time rather than a 12 cm teleport per
    // frame. Head and controller poses themselves remain unfiltered optical
    // 6DoF. Cap long pauses so they cannot produce a sudden large transfer.
    float length=std::min({distance-.08f*units,.12f*units,3.f*units*std::min(elapsedSeconds,.04f)});
    step[0]=offset[0]*length/distance;step[1]=0;step[2]=offset[2]*length/distance;return true;
}
inline bool RoomAccepted(float const (&request)[3],float const (&accepted)[3],float units,float stepHeight){
    for(float v:accepted)if(!std::isfinite(v))return false;
    float length=std::hypot(request[0],request[2]),moved=std::hypot(accepted[0],accepted[2]);
    return std::isfinite(stepHeight)&&length>0&&moved<=length+.01f*units&&
        accepted[0]*request[0]+accepted[2]*request[2]>=-.001f*units*length&&
        std::fabs(accepted[1])<=std::min(std::max(stepHeight,0.f)+.02f*units,.4f*units);
}
inline bool ConsumeRoomStep(Orientation center,Rotation3 native,float const (&accepted)[3],float units,float (&origin)[3]){
    if(!Normalize(center)||!std::isfinite(units)||units<=0)return false;
    for(float v:accepted)if(!std::isfinite(v))return false;
    native=UprightCamera(native);auto c=Matrix(center);float local[3]{};
    for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k)local[i]+=native.m[k*3+i]*(k==1?0:accepted[k])/units;
    local[2]=-local[2];
    float delta[3]{};for(unsigned i=0;i<3;++i)for(unsigned k=0;k<3;++k)delta[i]+=c.m[i*3+k]*local[k];
    // Upright recenter is required for body following. Crouching is entirely
    // optical and must survive both terrain movement and horizontal transfers.
    origin[0]+=delta[0];origin[2]+=delta[2];return true;
}
}

