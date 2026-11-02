#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace diagnostic::temporal {
// Exact saved PID8536 skydome program. This is a rotation model, NOT permission to
// classify cleared depth as sky. Raster coverage must come from the real sky
// draw with its original read-only depth/stencil tests, after EndSolid.
struct SkyPose {
    uint64_t vertexHash=0,pixelHash=0;
    std::array<float,60> constants{}; // Actual native c0..c14.
    bool Valid()const noexcept {
        if(vertexHash!=0x9cd6b9e1f1627a6full||pixelHash!=0xe3a7e3dc1f1c8437ull)return false;
        for(float v:constants)if(!std::isfinite(v))return false;return true;
    }
};
namespace sky_detail {
using M=std::array<double,16>;
inline bool Inverse(M const& m,M& inverse)noexcept {
    double a[4][8]{};for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){if(!std::isfinite(m[r*4+c]))return false;a[r][c]=m[r*4+c];a[r][c+4]=r==c?1.:0.;}
    for(unsigned c=0;c<4;++c){unsigned pivot=c;for(unsigned r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[pivot][c]))pivot=r;
        if(std::abs(a[pivot][c])<1.e-12)return false;if(pivot!=c)for(unsigned k=0;k<8;++k)std::swap(a[pivot][k],a[c][k]);
        double scale=a[c][c];for(double& v:a[c])v/=scale;
        for(unsigned r=0;r<4;++r)if(r!=c){scale=a[r][c];for(unsigned k=0;k<8;++k)a[r][k]-=scale*a[c][k];}}
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){inverse[r*4+c]=a[r][c+4];if(!std::isfinite(inverse[r*4+c]))return false;}
    double residual=0;for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){double value=0;for(unsigned k=0;k<4;++k)value+=m[r*4+k]*inverse[k*4+c];residual=std::max(residual,std::abs(value-(r==c?1.:0.)));}return residual<1.e-6;
}
inline M Rotation(SkyPose const& pose)noexcept {M r{};r[15]=1;for(unsigned y=0;y<3;++y)for(unsigned x=0;x<3;++x)r[y*4+x]=pose.constants[(y+4)*4+x];return r;}
}
// Matches original VS dp4 -> rcp -> negative -> nrm -> three dp3 operations.
// Native inverse-view translation (.w in c4/c5/c6) is deliberately ignored.
inline bool SkyWorldRay(SkyPose const& pose,std::array<double,4> const& nativeClip,std::array<double,3>& world)noexcept {
    if(!pose.Valid())return false;for(double v:nativeClip)if(!std::isfinite(v))return false;
    double view[4]{};for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)view[r]+=pose.constants[r*4+k]*nativeClip[k];
    if(!(view[3]>1.e-12))return false;double norm=0;for(unsigned k=0;k<3;++k){view[k]=-view[k]/view[3];norm+=view[k]*view[k];}
    if(!std::isfinite(norm)||norm<1.e-20)return false;norm=std::sqrt(norm);for(unsigned k=0;k<3;++k)view[k]/=norm;
    for(unsigned r=0;r<3;++r){world[r]=0;for(unsigned k=0;k<3;++k)world[r]+=pose.constants[(r+4)*4+k]*view[k];if(!std::isfinite(world[r]))return false;}return true;
}
inline bool SkyPreviousMinusCurrentPixels(SkyPose const& now,SkyPose const& before,
    std::array<double,4> const& nativeClip,unsigned width,unsigned height,
    std::array<float,2> const& currentRasterJitter,std::array<float,2> const& previousRasterJitter,
    std::array<double,2>& pixels)noexcept {
    pixels={};if(!width||!height||width>8192||height>8192||!before.Valid()||!(nativeClip[3]>1.e-12))return false;
    for(unsigned i=0;i<2;++i)if(!std::isfinite(currentRasterJitter[i])||!std::isfinite(previousRasterJitter[i])||std::abs(currentRasterJitter[i])>.5f||std::abs(previousRasterJitter[i])>.5f)return false;
    std::array<double,3> world{};if(!SkyWorldRay(now,nativeClip,world))return false;
    sky_detail::M inverseRotation{},inverseProjection{},projection{};
    if(!sky_detail::Inverse(sky_detail::Rotation(before),inverseRotation))return false;
    std::copy_n(before.constants.begin(),16,inverseProjection.begin());if(!sky_detail::Inverse(inverseProjection,projection))return false;
    double view[4]{},previous[4]{};for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)view[r]-=inverseRotation[r*4+k]*world[k];
    for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)previous[r]+=projection[r*4+k]*view[k];
    if(!(previous[3]>1.e-12))return false;
    for(unsigned i=0;i<2;++i){double p=previous[i]/previous[3],c=nativeClip[i]/nativeClip[3];if(!std::isfinite(p)||!std::isfinite(c)||std::abs(p)>1||std::abs(c)>1)return false;
        pixels[i]=(p-c)*(i?-.5*height:.5*width)-(previousRasterJitter[i]-currentRasterJitter[i]);if(!std::isfinite(pixels[i])){pixels={};return false;}}
    return true;
}
// Two differently scaled cloud samples and independent UV scroll cannot be
// assigned one exact rigid-sky vector. This flag is input to future reactive
// classification; it NEVER grants valid cloud history on its own.
inline bool SkyCloudParametersChanged(SkyPose const& now,SkyPose const& before)noexcept {
    if(!now.Valid()||!before.Valid())return true;
    for(unsigned i=8*4;i<60;++i)if(now.constants[i]!=before.constants[i])return true;
    return false;
}
// Column-vector shader transform: previousProjection * previousRotation^-1 *
// currentRotation * currentInverseProjection.xyz. Drop the unprojected w before
// reprojection so neither camera translation nor finite scene depth enters sky.
inline bool SkyRotationTransform(SkyPose const& now,SkyPose const& before,std::array<float,16>& result)noexcept {
    result={};if(!now.Valid()||!before.Valid())return false;
    sky_detail::M inverseRotation{},inverseProjection{},projection{};
    if(!sky_detail::Inverse(sky_detail::Rotation(before),inverseRotation))return false;
    std::copy_n(before.constants.begin(),16,inverseProjection.begin());if(!sky_detail::Inverse(inverseProjection,projection))return false;
    auto rotation=sky_detail::Rotation(now);sky_detail::M ray{},world{},local{},clip{};
    std::copy_n(now.constants.begin(),12,ray.begin()); // fourth row intentionally zero.
    auto multiply=[](auto const& a,auto const& b,auto& out){for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)out[r*4+c]+=a[r*4+k]*b[k*4+c];};
    multiply(rotation,ray,world);multiply(inverseRotation,world,local);multiply(projection,local,clip);
    for(unsigned i=0;i<16;++i){result[i]=float(clip[i]);if(!std::isfinite(result[i])){result={};return false;}}return true;
}
}
