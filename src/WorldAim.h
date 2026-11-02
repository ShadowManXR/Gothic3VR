#pragma once
#include "GameStereoMath.h"
#include "ControllerData.h"
#include "VrSettings.h"
#include <cstring>
#include <algorithm>
namespace diagnostic {
struct WorldAim {
    float origin[3]{},rotation[3]{},direction[3]{};
};
// Ray selection uses the object's surface, not the player-to-object-centre
// distance. A small contact zone keeps a target when the hand reaches it.
inline bool WorldAimBox(WorldAim const& aim,float const (&minimum)[3],float const (&maximum)[3],float reach,float units,float& rating,bool pickup=false){
    if(!std::isfinite(reach)||reach<=0||!std::isfinite(units)||units<=0)return false;
    float distance2=0,norm=0;
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(minimum[i])||!std::isfinite(maximum[i])||minimum[i]>maximum[i]||!std::isfinite(aim.origin[i])||!std::isfinite(aim.direction[i]))return false;
        float delta=std::max(minimum[i]-aim.origin[i],std::max(0.f,aim.origin[i]-maximum[i]));distance2+=delta*delta;norm+=aim.direction[i]*aim.direction[i];
    }
    if(std::fabs(norm-1)>.02f||distance2>reach*reach)return false;
    float distance=std::sqrt(distance2);
    if(distance<=(pickup?.15f:.10f)*units){rating=.05f*distance/reach;return true;}
    // Thin ground items need a forgiving hit area and downward reach from a
    // standing hand. NPCs retain their existing ray/contact tolerances.
    float entry=0,exit=reach,padding=(pickup?.06f:.025f)*units+distance*(pickup?.105f:.04f);
    for(unsigned i=0;i<3;++i){
        float lo=minimum[i]-padding-aim.origin[i],hi=maximum[i]+padding-aim.origin[i],direction=aim.direction[i];
        if(std::fabs(direction)<.00001f){if(lo>0||hi<0)return false;continue;}
        float a=lo/direction,b=hi/direction;if(a>b)std::swap(a,b);
        entry=std::max(entry,a);exit=std::min(exit,b);if(entry>exit)return false;
    }
    rating=.05f+.95f*entry/reach;return exit>=0&&entry<=reach;
}
inline bool WorldAimReady(ControllerFrame const& frame,uint64_t now,bool allowed,bool menu) {
    return allowed&&!menu&&frame.focused&&now>=frame.timeMs&&now-frame.timeMs<=100&&frame.hands[1].active&&frame.hands[1].aim.valid;
}
inline bool ControllerWorldAim(Rotation3 const& native,float const (&position)[3],Orientation center,float const (&centerPosition)[3],ControllerPose const& pose,float units,WorldAim& aim) {
    if(!pose.valid||!std::isfinite(units)||units<=0)return false;
    float norm=0;for(float v:pose.quaternion){if(!std::isfinite(v))return false;norm+=v*v;}
    if(std::fabs(norm-1)>.02f)return false;
    StereoEyePose eye{};memcpy(eye.position,pose.position,sizeof(eye.position));memcpy(eye.quaternion,pose.quaternion,sizeof(eye.quaternion));
    if(!GameEyeTransform(native,position,center,centerPosition,eye,units,aim.origin,aim.rotation))return false;
    auto r=NativeCameraRotation(aim.rotation);for(int i=0;i<3;++i)aim.direction[i]=r.m[i*3+2];return true;
}
inline bool WorldAimElevationBias(WorldAim& aim,int upPercent) {
    if(!upPercent)return true;
    for(float value:aim.direction)if(!std::isfinite(value))return false;
    float horizontal=std::hypot(aim.direction[0],aim.direction[2]);
    if(horizontal*horizontal+aim.direction[1]*aim.direction[1]<.0001f)return false;
    // 15% means a 0.15 rise/run bias (atan(0.15) = 8.53 degrees).
    // Use world elevation, not the rolled controller's local pitch axis.
    // Only the interaction ray changes; the physical grip/origin stays put.
    float elevation=std::atan2(aim.direction[1],horizontal)+std::atan(float(std::clamp(upPercent,InteractionAimMinimum,InteractionAimMaximum))*.01f);
    aim.rotation[0]=std::clamp(elevation,-1.5697963f,1.5697963f);
    aim.rotation[1]=std::atan2(aim.direction[0],aim.direction[2]);aim.rotation[2]=0;
    auto rotation=NativeCameraRotation(aim.rotation);
    for(unsigned i=0;i<3;++i)aim.direction[i]=rotation.m[i*3+2];
    return true;
}
inline bool WorldAimAngles(WorldAim const& aim,float const (&point)[3],float& azi,float& elev) {
    float delta[3];for(int i=0;i<3;++i){delta[i]=point[i]-aim.origin[i];if(!std::isfinite(delta[i]))return false;}
    if(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]<.0001f)return false;
    constexpr float degrees=57.2957795131f;
    // Match Genome GetAziElevUnit: positive elevation is up, in degrees.
    azi=std::remainder(std::atan2(delta[0],delta[2])-std::atan2(aim.direction[0],aim.direction[2]),6.28318530718f)*degrees;
    elev=(std::atan2(delta[1],std::hypot(delta[0],delta[2]))-std::atan2(aim.direction[1],std::hypot(aim.direction[0],aim.direction[2])))*degrees;
    return std::isfinite(azi)&&std::isfinite(elev);
}
}
