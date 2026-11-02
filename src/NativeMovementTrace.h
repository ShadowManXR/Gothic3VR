#pragma once
#include <cstdint>
namespace diagnostic {
// Explicit one-shot observer: no background thread, allocation, file polling,
// state mutation or renderer work. Four phase snapshots per selected update.
class NativeMovementTraceWindow {
    bool armed=false,started=false,selected=false;
    uint64_t began=0,last=0,selectedFrame=0;
    unsigned samples=0;
public:
    void Reset(){armed=started=selected=false;began=last=selectedFrame=0;samples=0;}
    void Arm(){Reset();armed=true;}
    bool Armed()const{return armed;}
    bool Selected(uint64_t frame)const{return armed&&selected&&frame==selectedFrame;}
    bool Select(uint64_t frame,uint64_t now){
        if(!armed)return false;
        if(started&&(now<last||now-began>=120000)){Reset();return false;}
        if(Selected(frame))return true;
        if(started&&now-last<250)return false;
        if(samples>=480){Reset();return false;}
        if(!started){began=now;started=true;}
        last=now;selectedFrame=frame;selected=true;++samples;return true;
    }
};
}
