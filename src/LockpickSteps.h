#pragma once
#include <cmath>
#include <cstdint>
namespace diagnostic {
// Each guess consumes the latch. Crossing the horizontal centre, direction
// changes and joystick chatter cannot re-arm it: both axes must rest neutral.
class LockpickSteps {
    uint64_t previous=0,neutralSince=0,pulseSince=0;int pulse=0;bool armed=false;
public:
    void Reset(){previous=neutralSince=pulseSince=0;pulse=0;armed=false;}
    int Step(bool valid,float x,float y,uint64_t time){
        if(!valid||!time||!std::isfinite(x)||!std::isfinite(y)){Reset();return 0;}
        if(previous&&(time<previous||time-previous>100))Reset();
        previous=time;
        if(std::hypot(x,y)<.25f){if(!neutralSince)neutralSince=time;if(time-neutralSince>=100)armed=true;}
        else {neutralSince=0;if(armed&&std::fabs(x)>.65f&&std::fabs(x)>std::fabs(y)){
            armed=false;pulse=x<0?-1:1;pulseSince=time;
        }}
        return pulseSince&&time>=pulseSince&&time-pulseSince<80?pulse:0;
    }
};
}
