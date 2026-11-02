#pragma once
#include "ControllerData.h"
#include <cmath>
namespace diagnostic {
class GuardPose {
    bool raised=false;
public:
    void Reset(){raised=false;}
    bool Step(ControllerFrame const& frame,unsigned hand,float headY,uint64_t now,bool allowed){
        if(!allowed||hand>1||!frame.focused||now<frame.timeMs||now-frame.timeMs>100||
           !frame.hands[hand].active||!frame.hands[hand].grip.valid||!std::isfinite(headY)){
            Reset();return false;}
        float height=frame.hands[hand].grip.position[1]-headY;
        if(!std::isfinite(height)){Reset();return false;}
        // Relative to the headset: works seated and with different heights.
        // Hysteresis avoids chattering while the hand rests at chest level.
        raised=height>=(raised?-.55f:-.45f);return raised;
    }
};
}
