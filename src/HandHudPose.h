#pragma once
#include "TrackedArmsMath.h"

namespace diagnostic {
// All transforms use the renderer's row-vector convention. The bind anchor is
// constant in lower-arm space; controller poses never position these panels.
inline bool HandHudAnchor(arms::Mat const& lowerBind,arms::Mat const& handBind,
                          arms::Mat const& calibration,unsigned side,arms::Mat& local){
    using namespace arms;
    Mat inverseLower,inverseCalibration;
    if(!Inverse(lowerBind,inverseLower)||!Inverse(calibration,inverseCalibration))return false;
    Mat grip=Multiply(inverseCalibration,handBind),panel;
    float sign=side?1.f:-1.f;
    // Up follows the fingers; the face points away from the palm.
    Vec x=Vector(grip,{0,0,sign}),y=Vector(grip,{0,-1,0}),z=Cross(x,y);
    if(!Unit(x)||!Unit(y)||!Unit(z))return false;
    panel.m[0]=x.x;panel.m[1]=x.y;panel.m[2]=x.z;
    panel.m[4]=y.x;panel.m[5]=y.y;panel.m[6]=y.z;
    panel.m[8]=z.x;panel.m[9]=z.y;panel.m[10]=z.z;
    Position(panel,Position(handBind)*.75f+Position(lowerBind)*.25f);
    local=Multiply(panel,inverseLower);return Affine(local);
}
inline bool HandHudPose(arms::Mat const& anchor,arms::Mat const& lower,
                        int const* offsets,float units,arms::Mat& out){
    using namespace arms;
    if(!offsets||!std::isfinite(units)||units<=0||!Affine(anchor)||!Affine(lower))return false;
    Mat base=Multiply(anchor,lower);
    // IK can stretch the arm; the physical display retains its own dimensions.
    Vec x=Vector(base,{1,0,0}),y=Vector(base,{0,1,0});
    if(!Unit(x))return false;y=y-x*Dot(x,y);if(!Unit(y))return false;Vec z=Cross(x,y);
    base.m[0]=x.x;base.m[1]=x.y;base.m[2]=x.z;
    base.m[4]=y.x;base.m[5]=y.y;base.m[6]=y.z;
    base.m[8]=z.x;base.m[9]=z.y;base.m[10]=z.z;
    constexpr float radians=3.14159265358979323846f/180;
    Mat rotation;
    for(unsigned axis=0;axis<3;++axis){float half=offsets[3+axis]*radians*.5f;Orientation q{std::cos(half),0,0,0};
        if(axis==0)q.x=std::sin(half);if(axis==1)q.y=std::sin(half);if(axis==2)q.z=std::sin(half);
        rotation=Multiply(rotation,Rotation(Matrix(q)));}
    Position(rotation,{offsets[0]*units*.01f,offsets[1]*units*.01f,offsets[2]*units*.01f});
    out=Multiply(rotation,base);return Affine(out);
}
}
