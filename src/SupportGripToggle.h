#pragma once
#include <cmath>
#include <cstdint>
namespace diagnostic {
// Local to supported weapons: never changes the raw shield/menu grip action.
class SupportGripToggle {
    uintptr_t item=0;uint64_t session=0,time=0;bool mode=false,armed=false,down=false,latched=false;
public:
    void Reset(){item=0;session=time=0;armed=down=latched=false;}
    bool Step(uintptr_t weapon,uint64_t origin,uint64_t stamp,bool allowed,bool toggle,float squeeze){
        if(!allowed||!weapon||!stamp||!std::isfinite(squeeze)){Reset();return false;}
        bool gap=time&&stamp>=time&&stamp-time>100;
        if(item!=weapon||session!=origin||mode!=toggle||stamp<time||(gap&&!latched)){
            Reset();item=weapon;session=origin;mode=toggle;
        }else if(gap&&latched){
            time=stamp;down=squeeze>.65f;armed=squeeze<.35f;return true;
        }
        if(stamp==time)return toggle?latched:squeeze>.65f;
        time=stamp;if(!toggle)return squeeze>.65f;
        if(squeeze<.35f){down=false;armed=true;}
        else if(squeeze>.65f&&!down){down=true;if(armed){latched=!latched;armed=false;}}
        return latched;
    }
    // A click far from the grip cannot attach later just by approaching it.
    void Confirm(bool attached){if(mode&&latched&&!attached)latched=false;}
};
}
