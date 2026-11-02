#pragma once
#include <cstdint>
namespace diagnostic {
class ControllerSneakToggle {
    uintptr_t actor=0;uint64_t time=0,firstTap=0,released=0;bool enabled=false,armed=false;
public:
    void Reset(){actor=0;time=firstTap=released=0;enabled=armed=false;}
    bool Enabled()const{return enabled;}
    bool Step(uintptr_t player,bool fresh,bool gameplay,bool button,bool learned,uint64_t stamp,bool completedGesture=false){
        if(!fresh||!player||!stamp){Reset();return false;}
        if(actor!=player||stamp<time){Reset();actor=player;}
        time=stamp;if(!learned){enabled=false;firstTap=0;}
        // Menus/radial suspend native input but retain the selected mode. Y
        // used to change radial pages cannot become a Sneak press on return.
        if(!gameplay){armed=false;firstTap=released=0;return false;}
        if(completedGesture){
            firstTap=released=0;
            if(!button)armed=true;else if(armed){armed=false;if(enabled)enabled=false;else if(learned)enabled=true;}
            return enabled;
        }
        if(firstTap&&stamp-firstTap>450)firstTap=0;
        if(!button){if(!released)released=stamp;if(stamp-released>=30)armed=true;}
        else {released=0;if(armed){armed=false;
            if(firstTap&&stamp-firstTap>=80){if(enabled)enabled=false;else if(learned)enabled=true;firstTap=0;}
            else firstTap=stamp;}}
        return enabled;
    }
};
}
