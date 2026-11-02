#pragma once
#include "HeldWeaponVisuals.h"
#include "DualWield.h"
#include "TwoHandMelee.h"
namespace diagnostic::melee {
// The native conjured sword uses G3_Weapon_1H_Sword_Flame_01.xcmsh.
// Its guard/pivot is near Z=0; the handle lies between about -26 and -4 cm.
// Bind its handle centre, not its guard or the transient spawn world offset.
inline arms::Vec SummonedSwordHandle(float units){return {0,0,-.16f*units};}
inline bool AnchorSummonedSword(char const* name,int type,unsigned hand,OneHandPose& pose){
    if(!visual::SummonedFlameSword(name,type))return true;
    arms::Vec contact;
    return pose.valid&&hand<2&&SupportFistContact(pose.calibration,pose.palmLocal,hand,pose.units,contact)&&
        pose.AnchorContact(SummonedSwordHandle(pose.units),contact);
}
}
