#pragma once
#include "DirectLocomotion.h"
#include <algorithm>
#include <cstdint>
namespace diagnostic {
// Generate the controller's ground contribution before the native pipeline
// adds moving-floor motion and resolves physics. Never filter camera positions.
class GroundMotor {
    uintptr_t actor=0;uint64_t process=0;float current=0;
public:
    void Reset(){actor=0;process=0;current=0;}
    float Speed()const{return current;}
    // NPC movement callbacks share the virtual hook; never reset the player's
    // accumulated starting response just because a different actor updates.
    void ResetFor(uintptr_t player){if(player==actor)Reset();}
    bool StepNative(uintptr_t player,uint64_t frame,bool moving,float const (&direction)[3],float resolvedMaximum,double seconds,int startMs,float (&out)[3]){
        // GetCurrentMaxSpeedOfDir already includes mode, current modifier and
        // ground slope. Multiplying them again compounds sprint and terrain.
        return Step(player,frame,moving,direction,resolvedMaximum,1,1,1,seconds,startMs,out);
    }
    bool Step(uintptr_t player,uint64_t frame,bool moving,float const (&direction)[3],float nativeMaximum,
              float modeModifier,float speedModifier,float terrainModifier,double seconds,int startMs,float (&out)[3]){
        if(!player||!frame||!std::isfinite(seconds)||seconds<0||!std::isfinite(nativeMaximum)||nativeMaximum<0||
           !std::isfinite(modeModifier)||modeModifier<0||!std::isfinite(speedModifier)||speedModifier<0||!std::isfinite(terrainModifier)||terrainModifier<0)return false;
        for(float v:direction)if(!std::isfinite(v))return false;
        if(player!=actor){Reset();actor=player;}
        if(!moving){current=0;process=frame;out[0]=out[1]=out[2]=0;return true;}
        float length=std::hypot(direction[0],direction[2]);if(length<.0001f)return false;
        float target=nativeMaximum*modeModifier;if(!std::isfinite(target))return false;
        if(frame!=process){
            if(startMs<=0||target<current)current=target;
            else current=std::min(target,current+target*float(std::min(seconds,.05))/float(startMs*.001));
            process=frame;
        }
        float speed=current*speedModifier*terrainModifier;if(!std::isfinite(speed))return false;
        out[0]=direction[0]/length*speed;out[1]=0;out[2]=direction[2]/length*speed;return true;
    }
};
}
