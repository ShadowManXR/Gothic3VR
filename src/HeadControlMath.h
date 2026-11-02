#pragma once
#include "TrackingData.h"
#include <algorithm>
#include <cmath>

namespace diagnostic
{
struct Orientation { float w=1, x=0, y=0, z=0; };
struct Rotation3 { float m[9] = {1,0,0,0,1,0,0,0,1}; };
inline bool Normalize(Orientation& q)
{
    float n=std::sqrt(q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z);
    if (!std::isfinite(n) || n<.001f) return false;
    q.w/=n; q.x/=n; q.y/=n; q.z/=n; return true;
}
inline Orientation Product(Orientation a, Orientation b)
{
    return {a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z,
            a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w};
}
inline Rotation3 Matrix(Orientation q)
{
    float x=q.x,y=q.y,z=q.z,w=q.w;
    return {{1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),
             2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),
             2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)}};
}
inline Rotation3 Product(Rotation3 const& a, Rotation3 const& b)
{
    Rotation3 c;
    for (int r=0;r<3;++r) for (int col=0;col<3;++col)
    {
        c.m[r*3+col]=0;
        for (int k=0;k<3;++k) c.m[r*3+col]+=a.m[r*3+k]*b.m[k*3+col];
    }
    return c;
}
inline bool RelativeHeadRotation(Orientation center, Orientation pose, Rotation3& result)
{
    if (!Normalize(center) || !Normalize(pose)) return false;
    Orientation delta=Product({center.w,-center.x,-center.y,-center.z},pose);
    if (!Normalize(delta)) return false;
    result=Matrix(delta);
    // OpenXR: right-handed, -Z forward. Genome camera: +Z forward.
    // Change basis S*R*S with S=diag(1,1,-1), preserving pitch/yaw/roll.
    result.m[2]=-result.m[2]; result.m[5]=-result.m[5];
    result.m[6]=-result.m[6]; result.m[7]=-result.m[7];
    return true;
}
inline bool FreshOrientation(HeadPose const& pose, int64_t now, int64_t frequency, unsigned maxAgeMs)
{
    // XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT=4; validity alone is insufficient.
    if (!pose.runtimeConnected || !pose.deviceConnected || !pose.valid || !(pose.trackingResult&4) ||
        frequency<=0 || pose.qpc<=0 || now<pose.qpc ||
        1000.0*static_cast<double>(now-pose.qpc)/frequency>maxAgeMs) return false;
    Orientation q{pose.quaternion[0],pose.quaternion[1],pose.quaternion[2],pose.quaternion[3]};
    return Normalize(q);
}
// Verified against CalcViewMatrix in Engine 1.75.20108.14 and recorded game views:
// numeric view 3x3 = column rotations Rz(roll)*Ry(yaw)*Rx(-pitch).
// SharedBase's matrix *= reverses the ordinary numeric matrix product.
inline Rotation3 NativeCameraRotation(float const (&e)[3])
{
    float cp=std::cos(e[0]),sp=std::sin(e[0]);
    float cy=std::cos(e[1]),sy=std::sin(e[1]);
    float cr=std::cos(e[2]),sr=std::sin(e[2]);
    return {{cr*cy,-cr*sy*sp-sr*cp,cr*sy*cp-sr*sp,
             sr*cy,-sr*sy*sp+cr*cp,sr*sy*cp+cr*sp,
             -sy,-cy*sp,cy*cp}};
}
inline void NativeCameraEuler(Rotation3 const& a, float (&e)[3])
{
    float c=std::hypot(a.m[0],a.m[3]);
    e[1]=std::atan2(-a.m[6],c);
    if (c>1e-5f) {e[0]=-std::atan2(a.m[7],a.m[8]); e[2]=std::atan2(a.m[3],a.m[0]);}
    else { e[0]=0; e[2]=std::atan2(-a.m[1],a.m[4]); }
}
inline float RotationError(Rotation3 const& a, Rotation3 const& b)
{
    float error=0;
    for (int i=0;i<9;++i)
    {
        if (!std::isfinite(a.m[i]) || !std::isfinite(b.m[i])) return 1e9f;
        error=std::max(error,std::fabs(a.m[i]-b.m[i]));
    }
    return error;
}
inline bool HorizontalHeading(Rotation3 const& orientation, float& yaw)
{
    float x=orientation.m[2],z=orientation.m[8];
    if(!std::isfinite(x) || !std::isfinite(z) || x*x+z*z<.0025f)return false;
    yaw=std::atan2(x,z);return true;
}
}
