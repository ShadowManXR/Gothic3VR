#pragma once
#include "PhysicalBow.h"
#include "BowAimFilter.h"
namespace diagnostic { namespace magic {
// Reuse the ranged sight and tracking-space tremor filter. Casting itself is
// owned by Gothic: charged/quick/self/target spells keep their native semantics.
class Aim {
    bow::AimFilter filter;
public:
    bow::Sample sample;
    void Reset(){sample={};filter.Reset();}
    bool Prepare(uintptr_t player,uintptr_t spell,ControllerFrame const& frame,HeadPose const& head,
                 Rotation3 const& native,float const (&base)[3],Orientation center,float const (&origin)[3],
                 float units,float height,int const* calibration,int smoothing,uint64_t now){
        sample={};
        if(!player||!spell||!head.valid||!arms::FreshGrip(frame,1,now)){filter.Reset();return false;}
        auto stabilized=frame;int angles[3]{};
        if(!filter.Apply(stabilized.hands[1].aim,frame.timeMs,head.session,spell,smoothing)||
           !bow::TrackedAim(stabilized,native,base,center,origin,units,height,calibration,angles,sample.left,sample.leftAim,1))return false;
        sample.player=player;sample.weapon=spell;sample.time=frame.timeMs;sample.session=head.session;sample.units=units;
        sample.aimHand=1;sample.rightTracked=true;sample.right=sample.left;sample.valid=true;
        sample.launchNock=arms::Position(sample.left);sample.launchDirection=arms::Vector(sample.leftAim,{0,0,1});
        sample.nativeNock=arms::Finite(sample.launchNock)&&arms::Unit(sample.launchDirection);return sample.nativeNock;
    }
};
} }
