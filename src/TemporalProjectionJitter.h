#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
#include <cstdint>
namespace diagnostic::temporal {
// SDK row-vector projection. The parameter is the ACTUAL displacement of the
// rasterized image in pixels (+right,+down), also used for NGX jitter and motion
// de-jittering. It is not a head/camera translation or an XR compositor FOV.
inline std::array<float,16> JitterProjection(std::array<float,16> const& projection,
    unsigned width,unsigned height,float rasterX,float rasterY){
    if(!width||!height||width>8192||height>8192||uint64_t(width)*height>33554432||!std::isfinite(rasterX)||!std::isfinite(rasterY)||std::abs(rasterX)>.5f||std::abs(rasterY)>.5f)
        throw std::runtime_error("Invalid temporal projection jitter");
    auto result=projection;double dx=2.*rasterX/width,dy=-2.*rasterY/height;
    for(unsigned r=0;r<4;++r){for(unsigned c=0;c<4;++c)if(!std::isfinite(projection[r*4+c]))throw std::runtime_error("Nonfinite temporal projection");
        result[r*4]=float(double(projection[r*4])+dx*projection[r*4+3]);
        result[r*4+1]=float(double(projection[r*4+1])+dy*projection[r*4+3]);
        if(!std::isfinite(result[r*4])||!std::isfinite(result[r*4+1]))throw std::runtime_error("Temporal projection overflow");}
    return result;
}
}
