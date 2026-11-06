#pragma once
#include <cstdint>
namespace diagnostic {
struct HudVisibility {bool compass=false,slotBar=false,vitals=false,targetHealthBar=true;};
inline HudVisibility EffectiveHudVisibility(HudVisibility saved,bool inventoryOpen){
    if(inventoryOpen){saved.slotBar=true;saved.vitals=true;}return saved;
}
class HudInventoryFrameCache {
    uint64_t epoch=0;bool initialized=false,inventory=false;
public:
    template<class Query> bool Get(uint64_t frame,Query query){
        if(!initialized||epoch!=frame){epoch=frame;initialized=true;inventory=query();}return inventory;
    }
};
// Native HUD view roots have Window.ID=-1, separate from their layout anchors.
// These are the dedicated Game.dll view vtables, verified against its profile.
inline unsigned HudViewControlId(unsigned windowId,uintptr_t tableRva){
    if(tableRva==0x2d1b44)return 20379;
    if(tableRva==0x2d16fc)return 20386; // gCHUDViewBarFocus2
    if(tableRva==0x2d180c)return 20387;
    if(tableRva==0x2d1924)return 20389;
    if(tableRva==0x2d15d4)return 20388;
    return windowId;
}
inline bool HudPaintMessage(unsigned message){return message==15||message==20||message==0x85;} // WM_PAINT / WM_ERASEBKGND / WM_NCPAINT
inline bool HiddenHudControl(unsigned id,HudVisibility const& show){
    if(id==20545)return !show.compass; // IDC_STATIC_COMPASS_2
    if(id==20379||id==20380)return !show.slotBar; // quick-slot view and list
    if(id==20387||id==20388||id==20389||id==20382||id==20383||id==20384)return !show.vitals;
    if(id==20386||id==20381)return !show.targetHealthBar; // focus view and progress bar
    return false;
}
template<class Window,class Id,class Parent>
bool HiddenHudWindow(Window window,HudVisibility const& show,Id id,Parent parent){
    // Children can have their own paint callbacks. Walk ancestors so decorative
    // frames and labels disappear with the selected HUD panel as well.
    for(unsigned depth=0;window&&depth<32;++depth){
        if(HiddenHudControl(id(window),show))return true;
        auto next=parent(window);if(next==window)break;window=next;
    }
    return false;
}
}
