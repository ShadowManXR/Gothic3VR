#pragma once
namespace diagnostic {
struct ControllerFocusQuery {
    bool rangedAim=false;
    bool nativeCrossHair=false;
    bool weaponAim=false;
    bool candidateQuery=false;
};
// Native AutoCollectFocus calls FindFocusEntity(false), even with a ranged
// weapon drawn. Its result is the persistent focus read by the native HUD.
// Keep that native candidate query: the native crosshair path reads a distance
// report without ensuring it exists. Never promote an automatic query to it.
// Only remove VR pickup reach for ranged weapons; native ranges/LOS remain.
inline ControllerFocusQuery PlanControllerFocus(bool crossHair,bool rangedWeapon,bool magic){
    return {crossHair||rangedWeapon||magic,crossHair,rangedWeapon||magic,rangedWeapon&&!crossHair};
}
}
