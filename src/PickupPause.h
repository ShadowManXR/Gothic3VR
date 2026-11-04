#pragma once
#include <cstdint>
namespace diagnostic {
enum class PickupPause { Wait, Animation };
// Return addresses in the supported Script_Game.dll. The inventory insertion
// between the two animation blocks remains entirely in the original script.
inline bool PickupPauseSite(uintptr_t callerRva,PickupPause kind){
    return kind==PickupPause::Wait?callerRva==0x82348:
        callerRva==0x4aef3||callerRva==0x4bb93;
}
inline bool SkipPickupPause(PickupPause kind,uintptr_t callerRva,bool vr,bool begin,bool fullStop,
                            uintptr_t actor,uintptr_t player,uintptr_t spuActor,unsigned waitMs=500){
    return vr&&begin&&!fullStop&&player&&actor==player&&spuActor==player&&
        PickupPauseSite(callerRva,kind)&&(kind!=PickupPause::Wait||waitMs==500);
}
}
