#pragma once
#include <cmath>
#include <cstddef>
#include <cstring>
namespace diagnostic {
// CP1.75.14 CameraAI::GetCurrentWishedRotation, verified native body:
// alpha = clamp(dt * rate * ramp(elapsed / duration), 0, 1).
// The full Game.dll profile and the two native field operands gate production.
inline constexpr size_t TurnElapsedOffset=0x224,TurnRampOffset=0x3b0,TurnRateOffset=0x3b4;
inline constexpr unsigned TurnRampLoadRva=0x24d03,TurnRateMultiplyRva=0x24d65;
inline constexpr unsigned char TurnRampLoad[]{0xd9,0x87,0xb0,0x03,0,0};
inline constexpr unsigned char TurnRateMultiply[]{0xd8,0x8f,0xb4,0x03,0,0};

// Only used inside the already selected first-person instant-VR camera call.
// Give native Slerp its target (alpha1), retaining native matrix construction,
// mouse evaluation, side effects and return-buffer ABI. No output smoother.
class NativeTurnInterpolation {
    unsigned char* camera=nullptr;
    unsigned char elapsed[4]{},ramp[4]{},rate[4]{};
public:
    bool Begin(void* object,size_t bytes,float dt){
        if(camera||!object||bytes<TurnRateOffset+4||!std::isfinite(dt)||dt<.000001f||dt>1.f)return false;
        auto* data=static_cast<unsigned char*>(object);float a=0,b=0,c=0;
        std::memcpy(&a,data+TurnElapsedOffset,4);std::memcpy(&b,data+TurnRampOffset,4);std::memcpy(&c,data+TurnRateOffset,4);
        if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||b<0||c<0)return false;
        std::memcpy(elapsed,data+TurnElapsedOffset,4);std::memcpy(ramp,data+TurnRampOffset,4);std::memcpy(rate,data+TurnRateOffset,4);
        camera=data;float immediate=0,full=2.f/dt;
        std::memcpy(data+TurnRampOffset,&immediate,4);std::memcpy(data+TurnRateOffset,&full,4);return true;
    }
    void End(){
        if(!camera)return;
        std::memcpy(camera+TurnElapsedOffset,elapsed,4);std::memcpy(camera+TurnRampOffset,ramp,4);std::memcpy(camera+TurnRateOffset,rate,4);camera=nullptr;
    }
};
}
