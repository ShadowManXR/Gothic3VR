#pragma once
#include <cstdint>
namespace diagnostic {
class ControllerSprintToggle {
    uint64_t previous=0,began=0;bool enabled=false,armed=false,pressed=false,traveled=false,pressMoved=false;
public:
    void Reset(){previous=began=0;enabled=armed=pressed=traveled=pressMoved=false;}
    bool Enabled()const{return enabled;}
    bool Completed(bool fresh,bool gameplay,bool pulse,uint64_t time,bool moving){
        if(!fresh||!gameplay||!time||(previous&&time<previous)){Reset();return false;}
        previous=time;
        if(pulse){enabled=!enabled;traveled=enabled&&moving;}
        if(enabled&&moving)traveled=true;
        if(enabled&&traveled&&!moving){enabled=false;traveled=false;}
        return enabled;
    }

    bool Step(bool fresh,bool gameplay,bool button,uint64_t time,bool moving=true){
        if(!fresh||!time||(previous&&time<previous)){Reset();return false;}
        previous=time;
        if(!gameplay){enabled=traveled=armed=pressed=pressMoved=false;began=0;return false;}
        // Commit on release: the L3+R3 recenter chord can cancel a pending
        // tap before it becomes Sprint. A held startup button cannot toggle.
        if(!button){if(pressed&&time>=began&&time-began<=600){enabled=!enabled;traveled=enabled&&pressMoved;}
            pressed=false;pressMoved=false;began=0;armed=true;}
        else if(armed&&!pressed){pressed=true;armed=false;began=time;pressMoved=moving;}
        if(pressed)pressMoved|=moving;
        if(enabled&&moving)traveled=true;
        if(enabled&&traveled&&!moving){enabled=false;traveled=false;}
        return enabled;
    }
};
}
