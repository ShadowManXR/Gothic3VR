#pragma once
#include <g3sdk/Script.h>
#include "MagicAim.h"
#include "NativeRecovery.h"
#include "WeaponTypes.h"
namespace diagnostic {
static_assert(gEUseType_Cast==15&&gESpellType_Projectile==0,"Native magic classification must match the verified script");
class NativeMagic {
    void (__cdecl* logger)(char const*)=nullptr;uintptr_t reported=0;
public:
    magic::Aim aim;bool ready=false,fault=false;uint64_t redirected=0;
    void Initialize(bool verifiedRangedImport,void (__cdecl* log)(char const*)){ready=verifiedRangedImport;logger=log;}
    // PS_Magic_Cast / PowerCast / QuickCast resolve Item.Spell from the actual
    // right-held inventory template. Staff magic uses a left staff + right Cast
    // item, and the native Item.Spell/ActivateSkill dependencies supply its spell.
    eCEntity* Selected(eCEntity* player){
        if(!ready||fault||!Readable(player,sizeof(eCEntity))||NativeRecovery(player))return nullptr;
        auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
        if(!Readable(inventory,sizeof(*inventory)))return nullptr;
        auto* held=inventory->GetStackFromSlot(gESlot_RightHand);
        if(!Readable(held,sizeof(eCEntity))||gCEntity::GetUseType(held)!=gEUseType_Cast)return nullptr;
        Entity hero(player);int index=hero.Inventory.FindStackIndex(gESlot_RightHand);if(index<0)return nullptr;
        auto item=hero.Inventory.GetTemplateItem(index);auto spell=item.Item.GetSpell();eCEntity* entity=static_cast<eCEntity*>(spell);
        if(!Readable(entity,sizeof(eCEntity))||!entity->HasPropertySet(eEPropertySetType_Magic))return nullptr;
        if(reported!=reinterpret_cast<uintptr_t>(entity)){
            reported=reinterpret_cast<uintptr_t>(entity);auto* left=inventory->GetStackFromSlot(gESlot_LeftHand);
            char text[240];sprintf_s(text,"vr_magic_selected spell=%.120s staff_equipped=%d native_cast_input=1 native_mana_skills_charge_reload=1",entity->GetName().GetText(),Readable(left,sizeof(eCEntity))&&WeaponType(gCEntity::GetUseType(left))==WeaponKind::Staff);
            if(logger)logger(text);
        }return entity;
    }
    void Cancel(){aim.Reset();reported=0;}
    bool PrepareProjectile(PSProjectile* wrapper,uint64_t now){
        auto const& s=aim.sample;if(!ready||fault||!s.valid||now<s.time||now-s.time>100||!Readable(wrapper,sizeof(*wrapper)))return false;
        auto* projectile=static_cast<gCProjectile_PS*>(wrapper->m_pEngineEntityPropertySet);
        if(!Readable(projectile,sizeof(*projectile)))return false;auto* entity=projectile->GetEntity();
        auto* player=reinterpret_cast<eCEntity*>(s.player);auto* spell=reinterpret_cast<eCEntity*>(s.weapon);
        if(!Readable(entity,sizeof(eCEntity))||!Readable(spell,sizeof(eCEntity))||Selected(player)!=spell)return false;
        auto* interaction=static_cast<gCInteraction_PS*>(entity->GetPropertySet(eEPropertySetType_Interaction));
        if(!Readable(interaction,sizeof(*interaction))||interaction->GetOwner().GetEntity()!=player)return false;
        // Bolts/arrows and unrelated owned projectiles are not magic. The
        // native OnCast records the associated spell on Interaction.Spell.
        Entity launched(entity);Entity source=launched.Interaction.GetSpell();eCEntity* sourceEntity=static_cast<eCEntity*>(source);
        if(sourceEntity!=spell)return false;
        arms::Vec origin,direction;float separation;if(!bow::Aim(s,origin,direction,separation))return false;
        auto transform=bow::ArrowPose(origin,direction,arms::Vector(s.leftAim,{0,1,0}));bCMatrix native;memcpy(&native,&transform,sizeof(native));entity->SetToWorldMatrix(native);
        projectile->SetTargetDirection(bCVector(direction.x,direction.y,direction.z));
        // Preserve native velocity, path style (including seeking spells),
        // target, damage/effects, mana cost and cooldown. Only launch pose moves.
        ++redirected;if(logger){char text[200];sprintf_s(text,"vr_magic_projectile_controller_aim launches=%llu spell=%.100s native_stats_preserved=1",redirected,spell->GetName().GetText());logger(text);}return true;
    }
};
}
