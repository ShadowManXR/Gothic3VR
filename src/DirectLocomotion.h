#pragma once
#include <cmath>
namespace diagnostic
{
inline bool DirectWalkingMode(int mode) { return (mode>=2 && mode<=5) || mode==10; }
// Orient the horizontal speed selected by the game. Preserve Y exactly;
// zero native speed never creates movement, and invalid input passes through.
inline bool RedirectHorizontalVelocity(float const (&native)[3],float const (&direction)[3],float (&result)[3])
{
    for(unsigned i=0;i<3;++i)if(!std::isfinite(native[i]) || !std::isfinite(direction[i]))return false;
    float speed=std::hypot(native[0],native[2]);
    float length=std::hypot(direction[0],direction[2]);
    if(!std::isfinite(speed) || !std::isfinite(length) || speed<.0001f || length<.0001f)return false;
    result[0]=direction[0]/length*speed;result[1]=native[1];result[2]=direction[2]/length*speed;
    return std::isfinite(result[0]) && std::isfinite(result[2]);
}
}
