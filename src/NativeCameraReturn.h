#pragma once
#include "HeadControlMath.h"
#include <cstddef>
#include <cstring>
#include <initializer_list>
namespace diagnostic {
// Observe the native exceptions, not the renderer's temporary camera or a GUI
// panel. Dialogue may finish with the native FP flag already restored.
class CameraReturnTransition {
    uintptr_t actor=0,camera=0;bool third=false,pending=false;
public:
    void Reset(){actor=camera=0;third=pending=false;}
    bool Blocks(uintptr_t player)const{return player&&player==actor&&(third||pending);}
    bool Observe(uintptr_t player,uintptr_t owner,bool nativeThird){
        if(!player||!owner){Reset();return false;}
        if(actor!=player||camera!=owner){Reset();actor=player;camera=owner;}
        if(nativeThird){third=true;pending=false;return false;}
        if(third){third=false;pending=true;}
        return pending;
    }
    void Complete(){pending=false;}
};
// CP1.75.14: ProcessCurrentMode stores inverse(view) at22C. WishedRotation
// uses22C as the FP target and2AC as its interpolation history. SetTarget
// rebuilds position/actor26C but does NOT reset either rotation cache.
// The caller verifies the full Game.dll profile and native slots before use.
struct NativeCameraReturn {
    static constexpr size_t Current=0x22c,History=0x2ac,Size=History+64;
    static bool Facing(float x,float z,Rotation3& view){
        if(!std::isfinite(x)||!std::isfinite(z)||std::hypot(x,z)<.05f)return false;
        float angles[3]{0,std::atan2(x,z),0};view=NativeCameraRotation(angles);return true;
    }
    static bool Seed(void* object,size_t bytes,Rotation3 const& view){
        float yaw=0;if(!object||bytes<Size||!HorizontalHeading(view,yaw))return false;
        for(float value:view.m)if(!std::isfinite(value))return false;
        auto* data=static_cast<unsigned char*>(object);
        // Row-vector world rotation is the transpose of the numeric view.
        // Keep translations, homogeneous elements, target and actor caches.
        for(size_t offset:{Current,History})for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
            std::memcpy(data+offset+(r*4+c)*4,&view.m[c*3+r],4);
        float zero=0;
        std::memcpy(data+0x218,&zero,4); // consumed filtered mouse yaw
        std::memcpy(data+0x21c,&zero,4); // consumed filtered mouse pitch
        return true;
    }
};
}
