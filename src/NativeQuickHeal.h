#pragma once
#include <g3sdk/Script.h>
#include "PressOnce.h"
namespace diagnostic {
class NativeQuickHeal {
    PressOnce press;
public:
    bool Update(eCEntity* player,bool allowed,bool held,uint64_t now){
        if(!press.Step(reinterpret_cast<uintptr_t>(player),allowed,held,now))return false;
        Entity hero(player),other;
        // Native H action, argument0 selects Health. The game's script finds
        // It_Potion_Health in the inventory and runs PS_QuickUse; it owns item
        // consumption, restrictions and animation, independent of quick slots.
        return GetScriptAdmin().CallScriptFromScript("OnPlayerPotionQuickUse",&hero,&other,0)!=0;
    }
    void Reset(){press.Reset();}
};
}
