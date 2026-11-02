#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic {
inline bool ProjectReticle(arms::Vec point,float const (&view)[16],float const (&projection)[16],float& u,float& v){
    float p[4]{point.x,point.y,point.z,1},camera[4]{},clip[4]{};
    for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)camera[col]+=p[row]*view[row*4+col];
    for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)clip[col]+=camera[row]*projection[row*4+col];
    for(float x:clip)if(!std::isfinite(x))return false;
    if(clip[3]<=.001f||clip[2]<0||clip[2]>clip[3])return false;
    u=.5f+.5f*clip[0]/clip[3];v=.5f-.5f*clip[1]/clip[3];
    return u>=0&&u<=1&&v>=0&&v<=1;
}
}
