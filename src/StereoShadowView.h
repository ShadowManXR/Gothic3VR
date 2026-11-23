#pragma once
#include "TrackedArmsMath.h"
#include "GameStereoMath.h"

namespace diagnostic {
// Fit one shadow view around both eye frusta. Include eye translation and
// canted/asymmetric lenses so a caster visible to either eye stays covered.
inline bool StereoShadowProjection(arms::Mat const& center,arms::Mat const (&eyes)[2],
    float const (&fovs)[2][4],float nearZ,float farZ,float (&projection)[16]){
    arms::Mat inverse;
    if(!arms::Inverse(center,inverse)||!std::isfinite(nearZ)||!std::isfinite(farZ)||
       nearZ<=0||farZ<=nearZ)return false;
    float horizontal=0,vertical=0;
    for(unsigned eye=0;eye<2;++eye){
        if(!arms::Affine(eyes[eye]))return false;
        const auto relative=arms::Multiply(eyes[eye],inverse);
        for(float depth:{nearZ,farZ}){
            for(unsigned side=0;side<2;++side)for(unsigned height=2;height<4;++height){
                const float x=std::tan(fovs[eye][side]),y=std::tan(fovs[eye][height]);
                auto corner=arms::Point(relative,{x*depth,y*depth,depth});
                if(!arms::Finite(corner)||corner.z<=0)return false;
                horizontal=std::max(horizontal,std::fabs(corner.x/corner.z));
                vertical=std::max(vertical,std::fabs(corner.y/corner.z));
            }
        }
    }
    if(horizontal<=0||vertical<=0)return false;
    float fov[4]{-std::atan(horizontal),std::atan(horizontal),std::atan(vertical),-std::atan(vertical)};
    StereoProjection(fov,nearZ,farZ,projection);
    return true;
}
}
