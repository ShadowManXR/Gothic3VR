#pragma once
#include "PhysicalMelee.h"
#include "WeaponTypes.h"

namespace diagnostic { namespace melee {
// Each drawn 1H entity retains its own native attachment. Borrow its tracked
// spatial pose before frustum collection, then use the same pose in skinning.
struct OneHandPose {
    arms::Mat attachment,grip,mesh,calibration;Vec palmLocal;float units=100;bool valid=false;
    arms::Mat AtWrist(Vec wrist)const{
        auto hand=arms::Multiply(calibration,grip);arms::Position(hand,wrist);
        return arms::Multiply(attachment,hand);
    }
    bool AnchorContact(Vec meshContact,Vec handContact){
        if(!valid||!arms::Finite(meshContact)||!arms::Finite(handContact))return false;
        // A render-only contact constraint: keep the established orientation,
        // hand calibration and blade angles, replacing only the mesh offset.
        arms::Position(attachment,handContact-arms::Vector(attachment,meshContact));
        mesh=AtWrist(arms::WristAtPalm(grip,calibration,palmLocal));
        valid=arms::Affine(mesh);return valid;
    }
    bool Prepare(arms::Mat const& nativeMesh,arms::Mat const& nativeHand,
                 arms::Mat const& handCalibration,Blade const& blade,
                 arms::Mat const& trackedGrip,int const* angles,float scale,Vec palm){
        valid=false;arms::Mat inverse;
        if(!std::isfinite(scale)||scale<=0||!arms::Affine(nativeMesh)||!arms::Inverse(nativeHand,inverse))return false;
        return PrepareAttachment(arms::Multiply(nativeMesh,inverse),handCalibration,blade,trackedGrip,angles,scale,palm);
    }
    bool PrepareAttachment(arms::Mat const& fixedAttachment,arms::Mat const& handCalibration,Blade const& blade,
                           arms::Mat const& trackedGrip,int const* angles,float scale,Vec palm){
        valid=false;if(!std::isfinite(scale)||scale<=0||!arms::Affine(fixedAttachment))return false;
        calibration=handCalibration;units=scale;palmLocal=palm;
        // Blade-axis and weapon-angle calibration belong to the rigid mesh
        // attachment. Applying them to the tracked hand forced the wrist into
        // an artificial bend, even with all hand-calibration settings at zero.
        // Build the equivalent attachment in palm-centred local space so the
        // established mesh pose is retained without world-coordinate cancellation.
        arms::Mat weaponGrip,inverseHand;
        if(!SwordGrip(fixedAttachment,calibration,blade,arms::Mat{},angles,weaponGrip)||
           !arms::Inverse(arms::HandAtPalm(arms::Mat{},calibration,palmLocal),inverseHand))return false;
        attachment=arms::Multiply(arms::Multiply(fixedAttachment,arms::HandAtPalm(weaponGrip,calibration,palmLocal)),inverseHand);
        grip=trackedGrip;
        mesh=AtWrist(arms::WristAtPalm(grip,calibration,palmLocal));valid=arms::Affine(mesh);return valid;
    }
};
inline bool DualWieldTypes(int right,int left,uintptr_t rightEntity,uintptr_t leftEntity){
    return rightEntity&&leftEntity&&rightEntity!=leftEntity&&
        WeaponType(right)==WeaponKind::OneHand&&WeaponType(left)==WeaponKind::OneHand;
}
// Separate stroke histories, sample consumption and hit sets for each hand.
// Losing or changing one weapon must not reset the other hand's active stroke.
struct CombatChannel {
    Swing swing;uint64_t consumed=0,session=0;uintptr_t player=0,weapon=0;
    void Reset(){swing.Reset();consumed=session=0;player=weapon=0;}
    bool Step(Sample const& sample,uintptr_t expectedPlayer,uintptr_t expectedWeapon,bool allowed,uint64_t now){
        if(!allowed||!expectedPlayer||!expectedWeapon||!sample.valid||sample.player!=expectedPlayer||sample.weapon!=expectedWeapon||now<sample.time||now-sample.time>100){Reset();return false;}
        if(player!=sample.player||weapon!=sample.weapon||session!=sample.session){Reset();player=sample.player;weapon=sample.weapon;session=sample.session;}
        if(sample.time==consumed)return false;consumed=sample.time;return swing.Step(sample);
    }
};
} }
