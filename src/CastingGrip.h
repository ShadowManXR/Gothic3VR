#pragma once
#include "TrackedArmsMath.h"
namespace diagnostic { namespace arms {
// Render-only closed grip from the native bind finger chains. Native idle,
// walking and cast actions continue underneath and are restored after each eye.
// Work in wrist space, retaining all native segment lengths and knuckles.
inline bool CastingFinger(Mat const (&local)[3],Vec palmDirection,Vec palmCentre,bool thumb,Mat (&closed)[3]){
    for(auto const& m:local)if(!Affine(m))return false;
    Vec along=Position(local[1])-Position(local[0]),bend=palmDirection;
    if(thumb)bend=palmCentre-Position(local[0]);
    if(!Unit(along)||!Unit(bend))return false;
    bend=bend-along*Dot(along,bend);if(!Unit(bend))return false;
    Vec axis=Cross(along,bend);if(!Unit(axis))return false;
    float angles[3]{thumb?25.f:40.f,thumb?55.f:100.f,thumb?75.f:130.f};Mat rotations[3];
    for(unsigned i=0;i<3;++i){float half=angles[i]*.00872664626f;
        rotations[i]=Rotation(Matrix({std::cos(half),axis.x*std::sin(half),axis.y*std::sin(half),axis.z*std::sin(half)}));
        closed[i]=Multiply(local[i],rotations[i]);
        Vec position=i?Position(closed[i-1])+Vector(rotations[i-1],Position(local[i])-Position(local[i-1])):Position(local[0]);
        Position(closed[i],position);if(!Affine(closed[i]))return false;
    }return true;
}
} }
