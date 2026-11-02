#pragma once
#include <cmath>
namespace diagnostic {
struct LocatorGestureResult {bool pulse=false,consumeX=false,suppressHeal=false;};
class ControllerLocatorGesture {
    bool armed=false,captured=false,triggerHeld=false;
public:
    void Reset(){armed=false;captured=false;triggerHeld=false;}
    LocatorGestureResult Step(bool allowed,bool x,float trigger){
        LocatorGestureResult out;
        if(!allowed||!std::isfinite(trigger)){armed=false;triggerHeld=false;if(!x)captured=false;out.consumeX=captured;out.suppressHeal=captured;return out;}
        if(trigger<.4f)triggerHeld=false;else if(trigger>.65f)triggerHeld=true;
        if(!x){armed=true;captured=false;return out;}
        if(armed){armed=false;if(triggerHeld){captured=true;out.pulse=true;}}
        out.consumeX=captured;out.suppressHeal=captured;return out;
    }
};
}
