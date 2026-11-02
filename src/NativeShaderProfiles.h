#pragma once
#include "TemporalNativeGeometry.h"
#include <utility>
namespace diagnostic::temporal {
// Exact programs saved by earlier game captures and independently recovered
// from Shader.Cache. Register locations describe their native POSITION path;
// cache membership alone grants neither draw eligibility nor object history.
struct NativeShaderProfile {
    uint64_t hash;NativeGeometryKind kind;unsigned matrix,projection;bool canonical;
    unsigned windPlayer=0,windAmplitude=0,windWavelength=0,windPhase=0;
};
inline constexpr NativeShaderProfile NativeShaderProfiles[]={
    {0x5bb23accc4adfbb1ull,NativeGeometryKind::RigidWorld,4,0,true},
    {0xda20ec985ed6afb2ull,NativeGeometryKind::InstancesA,0,68,true},
    {0x859f0a9037a61421ull,NativeGeometryKind::InstancesB,0,68,true},
    {0xbce087d5f40bd09eull,NativeGeometryKind::Wind,0,7,true},
    {0x22191ed1ad6d4dc5ull,NativeGeometryKind::RigidWorld,19,7,false},
    {0xffc55c7183137bbeull,NativeGeometryKind::RigidWorld,19,7,false},
    {0x73c43dc1099dd65dull,NativeGeometryKind::RigidWorld,12,0,false},
    {0x86a624c9eec5d087ull,NativeGeometryKind::RigidWorld,12,0,false},
    {0x29fb2f11532b54c0ull,NativeGeometryKind::InstancesA,119,187,false},
    {0x955d9400295120fcull,NativeGeometryKind::InstancesB,119,187,false},
    {0xa038d65f138b7200ull,NativeGeometryKind::InstancesA,119,187,false},
#include "NativeShaderProfiles.generated.h"
    // Exact branch/dependency and symbolic POSITION proof from original
    // bytecode: work/prove-native-wind-profiles.py. Alpha coverage is separate.
    {0x41f6fe97b5a7f43cull,NativeGeometryKind::Wind,0,7,false,26,27,28,29},
    {0x15e5561db503fe47ull,NativeGeometryKind::Wind,0,0,false,19,20,21,22},
    {0xae9dd751093f445eull,NativeGeometryKind::Wind,0,7,false,25,26,27,28},
    {0xdefc0ffc9e74fd88ull,NativeGeometryKind::Wind,0,0,false,18,19,20,21},
};
inline NativeShaderProfile const* FindNativeShaderProfile(uint64_t hash)noexcept{
    for(auto const& p:NativeShaderProfiles)if(p.hash==hash)return &p;return nullptr;
}
inline bool NormalizeNativePose(NativeShaderProfile const& profile,float const* values,unsigned registers,NativeGeometryPose& output)noexcept{
    if(!values)return false;NativeGeometryPose result;
    if(profile.canonical){if(registers<83)return false;std::copy_n(values,83*4,result.constants.begin());}
    else if(profile.kind==NativeGeometryKind::Wind){
        if(profile.projection+4>registers||profile.windPlayer>=registers||profile.windAmplitude>=registers||profile.windWavelength>=registers||profile.windPhase>=registers)return false;
        std::copy_n(values+profile.projection*4,16,result.constants.begin()+7*4);
        for(auto pair:{std::pair<unsigned,unsigned>{profile.windPlayer,22},{profile.windAmplitude,23},{profile.windWavelength,24},{profile.windPhase,25}})
            std::copy_n(values+pair.first*4,4,result.constants.begin()+pair.second*4);
        if(profile.hash==0x15e5561db503fe47ull){
            if(registers<19)return false;
            // Exact native fade inputs for the separately validated alpha PS.
            std::copy_n(values+15*4,4,result.constants.begin()+18*4);
            std::copy_n(values+17*4,4,result.constants.begin()+20*4);
            std::copy_n(values+18*4,4,result.constants.begin()+21*4);
        }
    }else{
        bool rigid=profile.kind==NativeGeometryKind::RigidWorld;
        if(!rigid&&profile.kind!=NativeGeometryKind::InstancesA&&profile.kind!=NativeGeometryKind::InstancesB)return false;
        unsigned rows=rigid?3:51,targetMatrix=rigid?4:0,targetProjection=rigid?0:68;
        if(profile.matrix+rows>registers||profile.projection+4>registers)return false;
        std::copy_n(values+profile.matrix*4,rows*4,result.constants.begin()+targetMatrix*4);
        std::copy_n(values+profile.projection*4,16,result.constants.begin()+targetProjection*4);
    }
    if(!result.Valid())return false;output=result;return true;
}
}
