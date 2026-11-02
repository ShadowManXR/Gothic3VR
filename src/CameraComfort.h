#pragma once
#include "HeadControlMath.h"
#include <cstdint>
namespace diagnostic {
inline Orientation UprightCenter(Orientation pose){if(!Normalize(pose))return {};auto rotation=Matrix(pose);float yaw=std::atan2(rotation.m[2],rotation.m[8]);return {std::cos(yaw*.5f),0,std::sin(yaw*.5f),0};}
enum class RecenterPhase {LegacyCamera,StereoFrame};
inline bool RecenterTracking(Orientation head,float const (&headPosition)[3],bool upright,Orientation& center,float (&centerPosition)[3],RecenterPhase phase=RecenterPhase::StereoFrame,bool geometricStereo=true) {
    // Movement can run before the stereo renderer in the same application
    // update. It must not consume a stereo reset or replace its yaw-only
    // center with the full pitch/roll pose while leaving translation stale.
    if(geometricStereo&&phase!=RecenterPhase::StereoFrame)return false;
    if(!Normalize(head))return false;for(float value:headPosition)if(!std::isfinite(value))return false;
    center=upright?UprightCenter(head):head;for(unsigned i=0;i<3;++i)centerPosition[i]=headPosition[i];return true;
}
inline Rotation3 UprightCamera(Rotation3 const& native){float yaw=0;HorizontalHeading(native,yaw);float euler[3]{0,yaw,0};return NativeCameraRotation(euler);}
inline void ForwardCameraOffset(Rotation3 const& native,float units,float (&position)[3]){float yaw=0;if(HorizontalHeading(native,yaw)){position[0]+=std::sin(yaw)*units;position[2]+=std::cos(yaw)*units;}}
class CameraHeightAnchor {
    uintptr_t actor=0;float height=0;
public:
    void Reset(){actor=0;}
    bool Snapshot(float& out)const{if(!actor||!std::isfinite(height))return false;out=height;return true;}
    bool Restore(uintptr_t player,float value){if(!player||!std::isfinite(value))return false;actor=player;height=value;return true;}
    bool Calibrated(uintptr_t player) const {return player&&actor==player;}
    // Dialogue and temporary camera modes belong to the same player/world.
    // Keep the eye-height calibration until the world or comfort mode changes.
    void Context(bool gameRunning,bool stabilized){if(!gameRunning||!stabilized)Reset();}
    float Height(uintptr_t player,float playerY,float nativeY,float postureHeight=0){if(!player||!std::isfinite(playerY)||!std::isfinite(nativeY)||!std::isfinite(postureHeight)){Reset();return nativeY;}if(actor!=player){actor=player;height=nativeY-playerY-postureHeight;}return playerY+height;}
    void Position(uintptr_t player,float const (&root)[3],float (&native)[3],float postureHeight=0){
        if(!player)return;for(float v:root)if(!std::isfinite(v))return;
        // The head bone moves sideways/backwards when drawing a weapon. The
        // root is the stable horizontal origin; tracked headset translation
        // and the user's forward offset are applied afterwards.
        native[0]=root[0];native[2]=root[2];native[1]=Height(player,root[1],native[1],postureHeight);
    }
};
}
