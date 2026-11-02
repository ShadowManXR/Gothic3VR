#pragma once
#include "TrackedArmsMath.h"
#include "WeaponTypes.h"
#include <cstdint>
namespace diagnostic { namespace melee {
// A drawn mesh has one item-to-hand transform. Native animation updates must
// not recalibrate that transform while the tracked player is holding it.
class LockedMeleeAttachment {
    uintptr_t weapon=0;int type=0;arms::Mat attachment,profile;bool profileReady=false;
public:
    void Reset(){weapon=0;type=0;profileReady=false;}
    bool Capture(uintptr_t item,int kind,arms::Mat const& nativeMesh,arms::Mat const& nativeHand,float maximumInitialOffset=0){
        if(!item||!PhysicalMeleeType(kind)||!arms::Affine(nativeMesh)||!arms::Affine(nativeHand))return false;
        if(weapon==item&&type==kind)return true;
        // A conjured item can occupy RightHand before the native attachment
        // processes its initial world pose. Never lock that spawn/origin offset.
        // Existing held bindings retain their fixed transform during animations.
        if(!std::isfinite(maximumInitialOffset)||maximumInitialOffset<0||
           (maximumInitialOffset>0&&arms::Length(arms::Position(nativeMesh)-arms::Position(nativeHand))>maximumInitialOffset))return false;
        arms::Mat inverse;if(!arms::Inverse(nativeHand,inverse))return false;
        auto candidate=arms::Multiply(nativeMesh,inverse);if(!arms::Affine(candidate))return false;
        attachment=candidate;weapon=item;type=kind;profileReady=false;return true;
    }
    bool Matches(uintptr_t item,int kind)const{return weapon==item&&type==kind&&item!=0;}
    bool RebindHand(arms::Mat const& before,arms::Vec beforePalm,arms::Mat const& after,arms::Vec afterPalm){
        if(!weapon)return false;
        arms::Mat inverse;
        if(!arms::Inverse(arms::HandAtPalm(arms::Mat{},after,afterPalm),inverse))return false;
        auto delta=arms::Multiply(arms::HandAtPalm(arms::Mat{},before,beforePalm),inverse);
        auto next=arms::Multiply(attachment,delta),nextProfile=arms::Multiply(profile,delta);
        if(!arms::Affine(next)||(profileReady&&!arms::Affine(nextProfile)))return false;
        attachment=next;if(profileReady)profile=nextProfile;return true;
    }
    arms::Mat const& Attachment()const{return attachment;}
    bool ProfileReady()const{return weapon&&profileReady;}
    bool LockProfile(arms::Mat const& mesh,arms::Mat const& hand){
        if(!weapon||!arms::Affine(mesh))return false;if(profileReady)return true;
        arms::Mat inverse;if(!arms::Inverse(hand,inverse))return false;
        profile=arms::Multiply(mesh,inverse);profileReady=arms::Affine(profile);return profileReady;
    }
    bool ProfileAtHand(arms::Mat const& hand,arms::Mat& mesh)const{
        if(!ProfileReady()||!arms::Affine(hand))return false;mesh=arms::Multiply(profile,hand);return arms::Affine(mesh);
    }
};
// When support or weight changes the weapon orientation, move the visual grip
// by the same rigid transform. Its palm contact and orientation cannot slip.
inline bool GripFromMesh(arms::Mat const& referenceGrip,arms::Mat const& referenceMesh,
                         arms::Mat const& mesh,arms::Mat& grip){
    if(!arms::Affine(referenceGrip)||!arms::Affine(referenceMesh)||!arms::Affine(mesh))return false;
    auto before=referenceMesh,hand=referenceGrip,after=mesh;arms::Mat inverse;
    arms::Position(before,{});arms::Position(hand,{});arms::Position(after,{});
    if(!arms::Inverse(before,inverse))return false;
    auto localPosition=arms::Vector(inverse,arms::Position(referenceGrip)-arms::Position(referenceMesh));
    grip=arms::Multiply(arms::Multiply(hand,inverse),after);
    arms::Position(grip,arms::Position(mesh)+arms::Vector(after,localPosition));return arms::Affine(grip);
}
} }
