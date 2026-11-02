#pragma once
#include "HeadControlMath.h"
#include <algorithm>
#include <cstdint>
namespace diagnostic {
// Work in world space at the entry head pose. Subtract that pose's translation
// again when storing the base, so room-scale displacement cannot put the entry
// eyes back inside the hero. The global XR origin remains untouched.
template<class Clearance>
bool FrameDialogCamera(float const (&origin)[3],float const (&player)[3],
    float const (&headOffset)[3],Rotation3 const& view,float units,
    Clearance&& clearance,float (&base)[3]){
    if(!std::isfinite(units)||units<=0)return false;
    float eye[3];for(unsigned i=0;i<3;++i){
        if(!std::isfinite(origin[i])||!std::isfinite(player[i])||!std::isfinite(headOffset[i]))return false;
        eye[i]=origin[i]+headOffset[i];if(!std::isfinite(eye[i]))return false;
        base[i]=origin[i];
    }
    float distance=std::hypot(eye[0]-player[0],eye[2]-player[2]),height=eye[1]-player[1];
    // A native third-person shot that is already outside the hero stays exact.
    if(distance>=2.f*units&&height>=.8f*units&&height<=3.f*units)return true;
    float fx=view.m[2],fz=view.m[8],length=std::hypot(fx,fz);
    if(!std::isfinite(length)||length<.001f)return false;
    fx/=length;fz/=length;
    float pivot[3]{player[0],player[1]+1.65f*units,player[2]};
    float dx=(-2.f*fx+.55f*fz)*units,dz=(-2.f*fz-.55f*fx)*units;
    float reach=std::hypot(dx,dz),best=0,chosen[3]{};
    // Prefer behind/over the shoulder. In confined rooms choose a clear side
    // once, rather than locking inside a wall or orbiting on subsequent frames.
    // Stay behind the subject so the unchanged viewing direction still frames
    // the hero; never escape a wall by placing the camera facing away from him.
    constexpr float turns[]{0,.45f,-.45f};
    for(float angle:turns){
        float c=std::cos(angle),s=std::sin(angle);
        float candidate[3]{pivot[0]+c*dx+s*dz,pivot[1],pivot[2]-s*dx+c*dz};
        float fraction=clearance(pivot,candidate);
        if(!std::isfinite(fraction))continue;
        fraction=std::clamp(fraction,0.f,1.f);float available=fraction*reach;
        if(available<.8f*units||available<=best)continue;
        best=available;for(unsigned i=0;i<3;++i)chosen[i]=pivot[i]+fraction*(candidate[i]-pivot[i]);
        if(fraction>=.999f)break;
    }
    if(best==0)return false;
    for(unsigned i=0;i<3;++i)base[i]=chosen[i]-headOffset[i];
    return true;
}
// Freeze the native dialogue framing, not the tracked eyes. This owns only a
// copied render base; it never disables the camera AI or modifies NPC scripts.
class DialogCameraLock {
    uintptr_t actor=0;
    Rotation3 rotation;
    float position[3]{};
public:
    void Reset(){actor=0;}
    bool Active()const{return actor!=0;}
    bool Active(uintptr_t player)const{return actor!=0&&actor==player;}
    bool Capture(bool dialog,uintptr_t player,Rotation3 const& native,float const (&origin)[3]){
        if(!dialog||!player){Reset();return false;}
        if(actor==player)return true; // Native cuts, orbiting and lerps cannot re-anchor it.
        Reset();
        for(float value:origin)if(!std::isfinite(value))return false;
        for(float value:native.m)if(!std::isfinite(value))return false;
        for(unsigned i=0;i<3;++i){
            float length=0;for(unsigned k=0;k<3;++k)length+=native.m[i*3+k]*native.m[i*3+k];
            if(std::fabs(length-1.f)>.01f)return false;
            for(unsigned j=0;j<i;++j){float dot=0;for(unsigned k=0;k<3;++k)dot+=native.m[i*3+k]*native.m[j*3+k];if(std::fabs(dot)>.01f)return false;}
        }
        actor=player;rotation=native;for(unsigned i=0;i<3;++i)position[i]=origin[i];return true;
    }
    bool Apply(uintptr_t player,Rotation3& outRotation,float (&outPosition)[3])const{
        if(!actor||actor!=player)return false;
        outRotation=rotation;for(unsigned i=0;i<3;++i)outPosition[i]=position[i];return true;
    }
};
}
