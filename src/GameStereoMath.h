#pragma once
#include "HeadControlMath.h"
#include "StereoRuntime.h"
namespace diagnostic
{
inline bool StereoProjectionDepth(float nearZ,float farZ,float (&out)[16]) noexcept
{
    if(!std::isfinite(nearZ)||!std::isfinite(farZ)||nearZ<=0||farZ<=nearZ)return false;
    // Native RenderLowPoly temporarily moves BOTH clip planes. Keep the XR
    // lens geometry, but use the depth range of the current native render pass.
    float z=farZ/(farZ-nearZ),translation=-nearZ*z;
    if(!std::isfinite(z)||!std::isfinite(translation))return false;
    out[10]=z;out[11]=1;out[14]=translation;out[15]=0;
    return true;
}
inline void StereoProjection(float const (&fov)[4],float nearZ,float farZ,float (&out)[16])
{
    float left=std::tan(fov[0]),right=std::tan(fov[1]),up=std::tan(fov[2]),down=std::tan(fov[3]);
    for(float& v:out)v=0;
    // Genome/D3D9: row vectors, +Z forward, depth [0,1].
    out[0]=2/(right-left);out[5]=2/(up-down);out[8]=-(right+left)/(right-left);out[9]=-(up+down)/(up-down);
    out[10]=farZ/(farZ-nearZ);out[11]=1;out[14]=-nearZ*farZ/(farZ-nearZ);
}
inline bool GameEyeTransform(Rotation3 const& native,float const (&position)[3],Orientation center,float const (&centerPosition)[3],
    StereoEyePose const& eye,float unitsPerMeter,float (&eyePosition)[3],float (&eyeEuler)[3])
{
    Rotation3 relative;Orientation q{eye.quaternion[3],eye.quaternion[0],eye.quaternion[1],eye.quaternion[2]};
    if(!RelativeHeadRotation(center,q,relative)||!Normalize(center))return false;NativeCameraEuler(Product(native,relative),eyeEuler);
    auto centerRotation=Matrix(center);float local[3]{};
    for(int i=0;i<3;++i)for(int k=0;k<3;++k)local[i]+=centerRotation.m[k*3+i]*(eye.position[k]-centerPosition[k]);
    local[2]=-local[2];
    for(int i=0;i<3;++i){eyePosition[i]=position[i];for(int k=0;k<3;++k)eyePosition[i]+=native.m[i*3+k]*local[k]*unitsPerMeter;
        if(!std::isfinite(eyePosition[i])||!std::isfinite(eyeEuler[i]))return false;}
    return true;
}
}
