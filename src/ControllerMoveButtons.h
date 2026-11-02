#pragma once
#include <cmath>
#include <cstdint>
#include "ControllerSprintToggle.h"
#include "DoubleTapPress.h"
namespace diagnostic {
struct MoveButtonActions {bool sprint=false,heal=false;};
class ControllerMoveButtons {
    ControllerSprintToggle sprint;
    DoubleTapPress heal;
    bool healHeld=false;
public:
    void CancelHeal(){heal.Reset();healHeld=false;}
    void CancelSprint(){sprint.Reset();}
    void Reset(){sprint.Reset();heal.Reset();healHeld=false;}
    MoveButtonActions Step(bool active,bool gameplay,float trigger,bool click,uint64_t now,bool moving,bool mapped=false,bool mappedHeal=false,bool mappedSprint=false){
        MoveButtonActions out;
        if(!active||!gameplay||!now||!std::isfinite(trigger)){Reset();return out;}
        out.sprint=mapped?sprint.Completed(active,gameplay,mappedSprint,now,moving):sprint.Step(active,gameplay,click,now,moving);
        // Double left trigger is independent of the support squeeze: the
        // player can keep both hands attached to a 2H weapon throughout.
        if(trigger<.4f)healHeld=false;
        else if(trigger>.65f)healHeld=true;
        out.heal=mapped?mappedHeal:heal.Step(true,healHeld,now);
        return out;
    }
};
}
