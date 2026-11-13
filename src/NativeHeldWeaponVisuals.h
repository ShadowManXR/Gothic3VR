#pragma once
#include <g3sdk/Script.h>
#include "HeldWeaponVisuals.h"
#include "NativeRangedVisual.h"
#include "TrackedArmsMath.h"
#include "CookingInteraction.h"
namespace diagnostic { namespace visual {
using MenuWeaponScope=VisibilityScope<ranged::EntityVisualAccess>;
struct StowedInventoryAccess : ranged::EntityVisualAccess {
    gCInventory_PS* inventory=nullptr;
    eCEntity* Slot(int slot)const{return inventory->GetStackFromSlot(static_cast<gESlot>(slot));}
    int Type(eCEntity* entity)const{return int(gCEntity::GetUseType(entity));}
};
inline bool CapturePlayerStowedWeapons(eCEntity* player,bool firstPerson,MenuWeaponScope& scope){
    if(!firstPerson||!Readable(player,sizeof(eCEntity)))return true;
    auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
    if(!Readable(inventory,sizeof(*inventory)))return true;
    StowedInventoryAccess access;access.inventory=inventory;
    return CaptureStowedWeapons(access,firstPerson,scope);
}
inline bool ReadName(eCEntity* entity,char (&name)[128]){
    if(!Readable(entity,sizeof(eCEntity)))return false;
    auto const& source=entity->GetName();int size=source.GetLength();auto* text=source.GetText();
    if(size<0||size>=128||!Readable(text,size_t(size)+1)||text[size])return false;
    memcpy(name,text,size_t(size)+1);return true;
}
inline bool CaptureCookingProps(eCEntity* player,MenuWeaponScope& scope){
    if(!Readable(player,sizeof(eCEntity)))return true;
    for(unsigned slot=0;slot<2;++slot){
        auto* item=gCEntity::GetWeapon(player,slot==0);
        if(!item||item==player||!Readable(item,sizeof(eCEntity)))continue;
        char name[128]{};
        if(ReadName(item,name)&&CookingProp(name)&&!scope.Capture(item))return false;
    }
    return true;
}
struct FlameTreeAccess {
    using Entity=eCEntity;
    bool Valid(Entity* entity)const{return Readable(entity,sizeof(Entity));}
    Entity* Parent(Entity* entity)const{return entity->GetParent();}
    int Type(Entity* entity)const{return int(gCEntity::GetUseType(entity));}
    bool IsFlameSword(Entity* entity)const{char name[128]{};return ReadName(entity,name)&&FlameSword(name);}
    int Children(Entity* entity)const{return entity->GetChildrenCount();}
    Entity* Child(Entity* entity,int i)const{return entity->GetChildAt(i);}
    uintptr_t WorldTail(Entity* entity)const{
        auto* particle=static_cast<eCParticle_PS*>(entity->GetPropertySet(eEPropertySetType_Particle));
        if(!Readable(particle,sizeof(*particle))||particle->GetInstanced()||particle->GetCoordinateSystem().GetValue()!=eECoordinateSystem_Independent)return 0;
        char name[128]{};
        return ReadName(entity,name)&&FlameWorldTail(name,0,false)&&particle->GetEntity()==entity?reinterpret_cast<uintptr_t>(particle):0;
    }
};
// Cold path: only descendants of the actual equipped1H Flame Sword(s).
// Cleared on every outer camera render, VR stop and before world teardown.
inline bool CaptureFlameTargets(eCEntity* player,ControllerFrame const& frame,uint64_t now,FlameTailTargets& targets){
    targets.Reset();if(!Readable(player,sizeof(eCEntity)))return true;
    for(unsigned slot=0;slot<2;++slot){unsigned hand=slot?0:1;if(!arms::FreshGrip(frame,hand,now))continue;
        auto* weapon=gCEntity::GetWeapon(player,slot==0);if(!weapon||weapon==player)continue;
        if(!CollectFlameWeapon(FlameTreeAccess{},weapon,targets)){targets.Reset();return false;}
    }return true;
}
inline bool CaptureMenuStaff(eCEntity* player,bool firstPerson,bool menu,bool radial,MenuWeaponScope& scope){
    if(!firstPerson||(!menu&&!radial)||!Readable(player,sizeof(eCEntity)))return true;
    for(unsigned slot=0;slot<2;++slot){auto* weapon=gCEntity::GetWeapon(player,slot==0);
        if(!weapon||weapon==player||!Readable(weapon,sizeof(eCEntity)))continue;
        if(HideMenuStaff(firstPerson,menu,radial,int(gCEntity::GetUseType(weapon)))&&!scope.Capture(weapon))return false;
    }return true;
}
}}
