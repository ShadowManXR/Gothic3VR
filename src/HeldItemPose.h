#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic { namespace arms {
// Keep the native item-to-wrist attachment while moving it before culling.
// Casting items do not use melee blade axes or weapon angle overrides.
inline bool HeldItemPose(Mat const& item,Mat const& wrist,Mat const& calibration,Mat const& grip,float units,Mat& moved){
    Mat inverse;if(!Affine(item)||!Affine(calibration)||!Affine(grip)||!std::isfinite(units)||units<=0||!Inverse(wrist,inverse))return false;
    Mat target=Multiply(calibration,grip);Position(target,Point(grip,{0,0,-.08f*units}));
    moved=Multiply(item,Multiply(inverse,target));return Affine(moved);
}
// Cast meshes have animated idle/carry translations which cannot be used as
// contact points. Cache only attachment rotation; anchor the mesh centre in
// the calibrated palm on every frame, before either eye is culled.
class CastingContact {
    uintptr_t identity=0;Mat orientation;Vec centre;
public:
    void Reset(){identity=0;}
    bool Prepare(uintptr_t key,Mat const& item,Mat const& nativeWrist,Vec minimum,Vec maximum){
        if(!key||!Affine(item)||!Affine(nativeWrist)||!Finite(minimum)||!Finite(maximum)||minimum.x>maximum.x||minimum.y>maximum.y||minimum.z>maximum.z||Length(maximum-minimum)<.001f){Reset();return false;}
        if(identity==key)return true;
        Mat inverse;if(!Inverse(nativeWrist,inverse)){Reset();return false;}
        orientation=Multiply(item,inverse);Position(orientation,{});centre=(minimum+maximum)*.5f;identity=key;return true;
    }
    bool Pose(Mat const& hand,Vec palm,Mat& moved,Vec worldOffset={})const{
        if(!identity||!Affine(hand)||!Finite(palm)||!Finite(worldOffset))return false;
        moved=Multiply(orientation,hand);Position(moved,Point(hand,palm)+worldOffset-Vector(moved,centre));return Affine(moved);
    }
};
} }
