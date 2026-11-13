#pragma once
#include <g3sdk/Script.h>
#include "PhysicalBow.h"
#include "RangedVisualScope.h"
namespace diagnostic { namespace ranged {
inline float AmmoTail(eCEntity* item,float fallback){
    bCBox meshBounds;
    meshBounds.Invalidate();
    auto* mesh=item->GetPropertySet(eEPropertySetType_MeshVisual);
    if(Readable(mesh,sizeof(*mesh)))mesh->GetBoundary(meshBounds,GEFalse);
    auto const& entityBounds=item->GetLocalNodeBoundary();
    return bow::AmmoTail(meshBounds.m_Min.m_fZ,meshBounds.m_Max.m_fZ,
        entityBounds.m_Min.m_fZ,entityBounds.m_Max.m_fZ,fallback);
}
struct EntityVisualAccess {
    using Entity=eCEntity;
    bool Valid(Entity* item)const{return Readable(item,sizeof(Entity));}
    bool Visible(Entity* item)const{return item->IsRenderingEnabled()!=GEFalse;}
    float Alpha(Entity* item)const{return item->GetRenderAlphaValue();}
    int Children(Entity* item)const{return item->GetChildrenCount();}
    Entity* Child(Entity* item,int i)const{return item->GetChildAt(i);}
    void SetVisible(Entity* item,bool value)const{item->EnableRendering(value?GETrue:GEFalse);}
    void SetAlpha(Entity* item,float value)const{item->SetRenderAlphaValue(value,GEFalse);}
    void SetAlphaTree(Entity* item,float value)const{item->SetRenderAlphaValue(value,GETrue);}
};
inline bool LiveNock(NockedIdentity const& bound){
    auto* player=gCSession::GetInstance().GetPlayer();
    if(reinterpret_cast<uintptr_t>(player)!=bound.player||!Readable(player,sizeof(eCEntity)))return false;
    auto* weapon=reinterpret_cast<eCEntity*>(bound.weapon);
    if(!Readable(weapon,sizeof(eCEntity))||(gCEntity::GetWeapon(player,GETrue)!=weapon&&gCEntity::GetWeapon(player,GEFalse)!=weapon))return false;
    if(gCEntity::GetUseType(weapon)!=(bound.ammo==7?gEUseType_CrossBow:gEUseType_Bow))return false;
    auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
    if(!Readable(inventory,sizeof(*inventory)))return false;
    auto* item=inventory->GetStackFromSlot(gESlot_RightHand);
    if(reinterpret_cast<uintptr_t>(item)!=bound.item||!Readable(item,sizeof(eCEntity)))return false;
    auto* projectile=static_cast<gCProjectile_PS*>(item->GetPropertySet(eEPropertySetType_Projectile));
    return Readable(projectile,sizeof(*projectile))&&projectile->GetEntity()==item&&
        bound.Same({reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(weapon),reinterpret_cast<uintptr_t>(item),int(gCEntity::GetUseType(item))},projectile->IsFlying()!=GEFalse);
}
}}
