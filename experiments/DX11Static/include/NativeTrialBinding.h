#pragma once
#include "NativeSceneAdapter.h"
#include "StaticScene.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace g3vr::dx11 {
inline std::string NativeResourceKey(char const* text){
    if(!text)return {};size_t n=0;while(n<1024&&text[n])++n;if(n==1024)return {};
    std::string value(text,n);for(char& c:value){if(c=='\\')c='/';if(c>='A'&&c<='Z')c=char(c-'A'+'a');}
    auto slash=value.find_last_of('/');if(slash!=std::string::npos)value.erase(0,slash+1);
    if(value.size()>=6&&value.substr(value.size()-6)==".xcmsh")value.replace(value.size()-6,6,".g3mesh");return value;
}
inline Camera NativeCameraMeters(float const* viewCm,float const* projectionCm){
    if(!viewCm||!projectionCm)throw std::runtime_error("Missing native camera");
    DirectX::XMFLOAT4X4 view,projection;std::memcpy(&view,viewCm,64);std::memcpy(&projection,projectionCm,64);
    for(unsigned i=0;i<16;++i)if(!std::isfinite(viewCm[i])||!std::isfinite(projectionCm[i]))throw std::runtime_error("Invalid native camera");
    if(view._14!=0||view._24!=0||view._34!=0||view._44!=1||projection._34!=1||projection._44!=0||projection._43>=0||projection._33<=1)
        throw std::runtime_error("Unsupported native projection convention");
    view._41*=.01f;view._42*=.01f;view._43*=.01f;projection._43*=.01f;
    Camera camera;DirectX::XMStoreFloat4x4(&camera.viewProjection,DirectX::XMLoadFloat4x4(&view)*DirectX::XMLoadFloat4x4(&projection));return camera;
}
inline uint64_t NativeCameraSignature(Camera const& camera)noexcept{
    uint64_t hash=14695981039346656037ull;auto* bytes=reinterpret_cast<unsigned char const*>(&camera.viewProjection);
    for(unsigned i=0;i<64;++i){hash^=bytes[i];hash*=1099511628211ull;}return hash;
}
// Match the native D3D9 integer pixel centers on the D3D11 half-integer grid.
// The source projection remains unchanged for native pass identity checks.
// Verified against actual recorded 4K cameras and an independent D3D9 raster.
inline Camera NativeRasterCameraMeters(float const* viewCm,float const* projectionCm,unsigned width,unsigned height){
    if(!width||!height||width>16384||height>16384)throw std::runtime_error("Invalid native raster dimensions");
    auto camera=NativeCameraMeters(viewCm,projectionCm);auto* m=reinterpret_cast<float*>(&camera.viewProjection);
    for(unsigned row=0;row<4;++row){m[row*4]+=m[row*4+3]/float(width);m[row*4+1]-=m[row*4+3]/float(height);}
    return camera;
}
inline bool SameNativePass(float const* a,float const* b)noexcept{return a&&b&&std::memcmp(a,b,64)==0;}
inline bool ValidNativeIdentity(NativeStaticSnapshot const& s)noexcept{
    return (s.identity[0]||s.identity[1]||s.identity[2]||s.identity[3])&&s.materialSwitch==0;
}
}
