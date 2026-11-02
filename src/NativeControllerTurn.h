#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace diagnostic {
// Integrate once per application update. Consume at the camera ROTATION
// evaluation, never in a translation preview which may run more than once.
class NativeControllerTurn {
    double previous=0;uint64_t frame=0,stamp=0;float pending=0;bool consumed=true;
public:
    void Reset(){previous=0;frame=stamp=0;pending=0;consumed=true;}
    void Sample(bool allowed,float axis,int speed,uint64_t tick,uint64_t update,double seconds){
        double dt=previous?seconds-previous:0;previous=seconds;frame=update;stamp=tick;pending=0;consumed=false;
        if(!allowed||!std::isfinite(seconds)||!std::isfinite(axis)||dt<0){Reset();return;}
        if(std::fabs(axis)<=.4f)return;
        float magnitude=(std::min(std::fabs(axis),1.f)-.4f)/.6f;
        pending=std::copysign(magnitude,axis)*600.f*float(std::clamp(speed,0,2000))*.01f*float(std::min(dt,.05));
    }
    float Consume(uint64_t update,uint64_t now){
        // Freshness is checked when the update samples the controller. Work
        // earlier in this same update must not expire an accepted command.
        // A later application update can never consume this turn.
        if(consumed||update!=frame||now<stamp){pending=0;consumed=true;return 0;}
        consumed=true;float out=pending;pending=0;return out;
    }
};
// Translation previews also run inside GetCurrentWishedRotation. Every such
// preview must see the SAME angle until that one rotation evaluation returns.
// Afterwards preserve the latest physical mouse input, without a VR residual.
class NativeTurnEvaluation {
    uintptr_t camera=0;float turn=0,native=0;
public:
    bool Owns(uintptr_t value)const{return camera&&camera==value;}
    bool Begin(uintptr_t value,float input,float angle){
        if(camera||!value||!std::isfinite(input)||!std::isfinite(angle))return false;
        camera=value;native=input;turn=angle;return true;
    }
    float Filter(uintptr_t value,float input){if(!Owns(value))return input;native=input;return input+turn;}
    float End(uintptr_t value){if(!Owns(value))return 0;float out=native;camera=0;native=turn=0;return out;}
};
// Native camera rotation can be evaluated again during translation/pre-render
// in the same update. Its zero-mouse branch uses a different native basis.
// Keep the accepted absolute orientation for those reevaluations, rather than
// returning the old basis after the controller delta has already been consumed.
// Translation/homogeneous elements and native function side effects are preserved.
class NativeTurnOutput {
    uintptr_t camera=0;uint64_t frame=0;float rotation[9]{};
public:
    void Reset(){camera=0;frame=0;}
    bool Matches(uintptr_t value,uint64_t update)const{return camera&&camera==value&&frame==update;}
    void Store(uintptr_t value,uint64_t update,float const* matrix){
        Reset();if(!value||!update||!matrix)return;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)if(!std::isfinite(matrix[r*4+c]))return;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)rotation[r*3+c]=matrix[r*4+c];camera=value;frame=update;
    }
    bool Apply(uintptr_t value,uint64_t update,float* matrix)const{
        if(!matrix||!Matches(value,update))return false;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)matrix[r*4+c]=rotation[r*3+c];return true;
    }
};
// Controller X has the opposite sign to the native mouse input. Preserve the
// native sensitivity chain while mapping left/right to the controller axis.
inline float NativeTurnAngle(float pixels,bool inverted,float preset,float mode,float sensitivity){
    if(!std::isfinite(pixels)||!std::isfinite(preset)||!std::isfinite(mode)||!std::isfinite(sensitivity)||
       preset<0||mode<0||sensitivity<0)return 0;
    float angle=pixels*(inverted?-1.f:1.f)*.05f*preset*mode*sensitivity;
    return std::isfinite(angle)?angle:0;
}
}
