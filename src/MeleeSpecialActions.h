#pragma once
#include "PhysicalMelee.h"
namespace diagnostic { namespace melee {
// Tracking-space handle travel distinguishes a thrust from wrist rotation,
// sideways cuts, real-world walking and animation movement.
inline bool ForwardThrust(Blade const& from,Blade const& to){
    Vec axis=to.tip-to.root,motion=to.root-from.root,tip=to.tip-from.tip;
    if(!arms::Finite(axis)||!arms::Finite(motion)||!arms::Finite(tip)||!arms::Unit(axis))return false;
    float length=arms::Length(motion);if(length<.0015f||length>.6f)return false;
    return arms::Dot(motion,axis)>=.75f*length&&arms::Dot(tip,axis)>=.5f*length;
}
inline bool NativePierceTarget(int species){
    // CP 1.75.14 IsHumanoid: Human (0) or Orc (5). Other creatures must
    // retain physical VR thrust damage instead of entering the Pierce path.
    return species==0||species==5;
}
inline int PhysicalAttackAction(bool power,bool thrust,int useType,int damageType,int targetSpecies){
    // Classify each contacted victim, not the whole stroke. Pierce retains
    // native Assassin/FOV/awareness rules; all other targets receive the same
    // swept VR contact through normal/power damage. Never dispatch both.
    return thrust&&useType==2&&damageType==2&&NativePierceTarget(targetSpecies)?11:power?2:1;
}
}}
