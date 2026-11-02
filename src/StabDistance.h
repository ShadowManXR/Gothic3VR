#pragma once
#include "MeleeSpecialActions.h"
namespace diagnostic { namespace combat {
// Short thrusts qualify by fast handle translation along the blade, never by
// a tip-only wrist arc, previous sideways travel or slow accumulated motion.
class StabDistance {
    melee::Blade previous{};arms::Vec start{},axis{};uint64_t time=0;
    unsigned stroke=0;int minimumSpeed=0;float travel=0,speed=0;bool forward=false;
    void Seed(melee::Blade const& blade){start=blade.root;axis=blade.tip-blade.root;arms::Unit(axis);travel=0;forward=false;}
public:
    void Reset(){previous={};start=axis={};time=0;stroke=0;minimumSpeed=0;travel=speed=0;forward=false;}
    float Speed()const{return speed;}
    bool Ready(int minimumCm)const{
        float minimum=std::clamp(minimumCm,5,100)*.01f;
        return forward&&travel+.00001f>=minimum&&arms::Dot(previous.root-start,axis)+.00001f>=minimum*.8f;
    }
    void Step(melee::Blade const& blade,uint64_t stamp,unsigned generation,bool moving,int minimumCmPerSecond){
        int speedSetting=std::clamp(minimumCmPerSecond,40,800);auto direction=blade.tip-blade.root;
        if(!stamp||!generation||!arms::Finite(blade.root)||!arms::Finite(blade.tip)||!arms::Unit(direction)){Reset();return;}
        if(!time||generation!=stroke||stamp<time||stamp-time>100||minimumSpeed!=speedSetting){
            Reset();time=stamp;stroke=generation;minimumSpeed=speedSetting;previous=blade;Seed(blade);return;
        }
        if(stamp==time)return;
        auto before=previous;float seconds=(stamp-time)*.001f;previous=blade;time=stamp;
        float distance=arms::Length(blade.root-before.root);
        speed=distance/seconds;
        bool valid=moving&&distance<=.6f&&arms::Length(blade.tip-before.tip)<=1.5f&&
            speed+.00001f>=speedSetting*.01f&&melee::ForwardThrust(before,blade)&&arms::Dot(axis,direction)>=.8660254f;
        if(!valid){Seed(blade);return;}
        travel+=distance;forward=true;
    }
};
}}
