#pragma once
#include "StereoCameraState.h"
#include <cstring>

namespace diagnostic
{
inline bool HideHeadInCamera(StereoCameraState const& state)
{
    return state.gameRunning && state.modeAvailable &&
        (state.firstPerson || state.tempFirstPerson) && !state.dialog && !state.activity && !state.animal;
}
inline bool IsHeadBodyPart(char const* name)
{
    return name && (!std::strcmp(name,"Slot_Head") || !std::strcmp(name,"Slot_Hair") ||
        !std::strcmp(name,"Slot_Beard") || !std::strcmp(name,"Slot_Helmet"));
}
using SetRenderVisibility = void(__cdecl*)(void*,bool);
// No ownership or allocation: all objects and names live for the current
// camera render only. Collect the entire hierarchy BEFORE changing any flag.
struct RenderVisibilitySnapshot
{
    struct Entry { void* object; SetRenderVisibility set; bool visible; };
    static constexpr unsigned Capacity=64;
    Entry entries[Capacity]{};
    unsigned count=0;
    bool active=false;
    bool Remember(void* object,bool visible,SetRenderVisibility set)
    {
        if(!object||!set||active)return false;
        for(unsigned i=0;i<count;++i)
            if(entries[i].object==object && entries[i].set==set)return true;
        if(count==Capacity)return false;
        entries[count++]={object,set,visible};return true;
    }
    void Hide()
    {
        active=true; // Also restore if a setter faults after changing a flag.
        for(unsigned i=0;i<count;++i)entries[i].set(entries[i].object,false);
    }
    void Restore()
    {
        if(!active)return;
        // Genome EnableRendering recursively changes children. Restore in
        // parent-before-child order so each child's own original flag wins.
        for(unsigned i=0;i<count;++i)entries[i].set(entries[i].object,entries[i].visible);
        active=false;
    }
};
}
