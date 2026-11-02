#pragma once
#include "TemporalObjectHistory.h"
namespace diagnostic::temporal {
enum class NativeGeometryKind:unsigned {RigidWorld=0,InstancesA=1,InstancesB=2,Wind=3};
inline uint64_t NativeGeometryHash(NativeGeometryKind kind){
    switch(kind){case NativeGeometryKind::RigidWorld:return 0x5bb23accc4adfbb1ull;
    case NativeGeometryKind::InstancesA:return 0xda20ec985ed6afb2ull;
    case NativeGeometryKind::InstancesB:return 0x859f0a9037a61421ull;
    case NativeGeometryKind::Wind:return 0xbce087d5f40bd09eull;default:return 0;}
}
struct NativeGeometryPose {
    std::array<float,83*4> constants{};
    bool Valid()const noexcept {for(float f:constants)if(!std::isfinite(f))return false;return true;}
};
struct NativeGeometryVertex {float position[3]{};float auxiliary=0;};
// Independent reference for the native position programs, not a declaration
// that mutable geometry has valid history. Caller must establish identity,
// vertex-content version and instance mapping before granting history.
inline bool ProjectNative(NativeGeometryKind kind,NativeGeometryPose const& pose,NativeGeometryVertex const& v,float (&clip)[4])noexcept {
    if(!pose.Valid()||!std::isfinite(v.auxiliary))return false;
    auto const& c=pose.constants;float p[4]{v.position[0],v.position[1],v.position[2],1},world[4]{0,0,0,1};
    unsigned matrix=0,projection=0;
    if(kind==NativeGeometryKind::RigidWorld){matrix=4;projection=0;}
    else if(kind==NativeGeometryKind::InstancesA||kind==NativeGeometryKind::InstancesB){
        float index=std::floor(v.auxiliary);if(index<0||index>16)return false;matrix=unsigned(index)*3;projection=68;
    }else if(kind==NativeGeometryKind::Wind){
        world[0]=p[0];world[1]=p[1];world[2]=p[2];projection=7;
        if(v.auxiliary>0){
            // Native shader wraps four phases before sincos. Preserve the
            // native order, including two waves on each horizontal axis.
            auto wave=[](float position,float wavelength,float phase,bool cosine){float a=std::fma(position,wavelength,phase);a=std::fma(a,.159154937f,.5f);a-=std::floor(a);a=std::fma(a,6.28318548f,-3.14159274f);return cosine?std::cos(a):std::sin(a);};
            // SM3 sincos destination.x is cosine, destination.y is sine.
            float z=std::fma(v.auxiliary,wave(p[2],c[24*4],c[25*4],true)*c[23*4],p[2]);
            z=std::fma(v.auxiliary,wave(p[2],c[24*4+1],c[25*4+1],false)*c[23*4+1],z);
            float x=std::fma(v.auxiliary,wave(p[0],c[24*4],c[25*4],false)*c[23*4],p[0]);
            x=std::fma(v.auxiliary,wave(p[0],c[24*4+1],c[25*4+1],true)*c[23*4+1],x);
            float dx=x-c[22*4],dy=p[1]-(130.f+c[22*4+1]),dz=z-c[22*4+2];
            float distance=std::sqrt(dx*dx+dy*dy+dz*dz);if(!(distance>1.e-6f))return false;
            float factor=(130.f-std::min(distance,130.f))*.0076923077f;
            world[0]=std::fma(dx/distance*factor,50.f,x);world[2]=std::fma(dz/distance*factor,50.f,z);
        }
    }else return false;
    if(kind!=NativeGeometryKind::Wind)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)world[i]+=p[j]*c[(matrix+i)*4+j];
    for(unsigned i=0;i<4;++i){clip[i]=0;for(unsigned j=0;j<4;++j)clip[i]+=world[j]*c[(projection+i)*4+j];if(!std::isfinite(clip[i]))return false;}
    return clip[3]>1.e-5f;
}
}
