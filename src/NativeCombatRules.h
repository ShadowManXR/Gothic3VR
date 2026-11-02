#pragma once
#include <g3sdk/Script.h>
#include "CombatGestures.h"
#include "MeleeSpecialActions.h"
#include "WeaponTypes.h"
#include <cstdio>
#include "TemporaryOverride.h"
#include "NativeApplication.h"
#include "NativeRecovery.h"
namespace diagnostic {
class NativeCombatRules {
public:
    bool ready=false;
    bool Initialize(){
        auto* base=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Script_Game.dll"));
        unsigned char power[]{0x6a,0xec},normal[]{0x8d,0x5c,0x1b,0x0a};
        ready=Readable(base+0x47e4d,2)&&Readable(base+0x48536,4)&&!memcmp(base+0x47e4d,power,2)&&!memcmp(base+0x48536,normal,4);
        return ready; // parent NativeMelee::Install verifies all native module hashes
    }
    static bool Alternative(){auto const& setup=eCApplication::GetInstance().GetEngineSetup();return reinterpret_cast<unsigned char const*>(&setup)[0x1ea]!=0;}
    static int Stamina(eCEntity* player){Entity hero(player),other;return GetScriptAdmin().CallScriptFromScript("GetStaminaPoints",&hero,&other,0);}
    static bool Spend(eCEntity* player,int amount){Entity hero(player),other;return GetScriptAdmin().CallScriptFromScript("AddStaminaPoints",&hero,&other,-amount)!=0||Entity::IsCheatGodEnabled();}
    static int Tier(eCEntity* player,eCEntity* weapon){
        Entity hero(player);auto group=combat::NativeSkillGroup(gCEntity::GetUseType(weapon));if(group==combat::SkillGroup::None)return 0;
        char const* prefix=group==combat::SkillGroup::OneHand?"Perk_1H_":group==combat::SkillGroup::Staff?"Perk_Staff_":"Perk_Axe_";
        for(int tier=3;tier>=1;--tier){char name[48];sprintf_s(name,"%s%d",prefix,tier);if(hero.Inventory.IsSkillActive(Template(name)))return tier;}return 0;
    }
    struct DamageContext {Entity* victim;Entity* weapon;eCEntity* player;gCScriptRoutine_PS* routine;gEAction saved,applied;int amount;gEDamageType type;};
    static void __cdecl Damage(void* raw){auto& c=*static_cast<DamageContext*>(raw);c.victim->DoDamage(*c.weapon,GEU32(c.amount),c.type);}
    static void __cdecl Restore(void* raw){auto& c=*static_cast<DamageContext*>(raw);
        if(eCApplication::GetInstance().IsGameRunning()&&gCSession::GetInstance().GetPlayer()==c.player&&Readable(c.player,sizeof(eCDynamicEntity))&&
           c.player->GetPropertySet(eEPropertySetType_ScriptRoutine)==c.routine&&Readable(c.routine,sizeof(*c.routine))&&c.routine->GetAction().GetValue()==c.applied)
            c.routine->SetAction(bTPropertyContainer<gEAction>(c.saved));
    }
    static int Hit(Entity& target,Entity& weapon,eCEntity* player,int amount,gEDamageType type,bool power,bool thrust=false){
        auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(!Readable(routine,sizeof(*routine)))return -1;
        static_assert(gEAction_PierceAttack==11&&gEAction_PowerAttack==2&&gEDamageType_Blade==2&&gEUseType_1H==2);
        static_assert(gESpecies_Human==0&&gESpecies_Orc==5);
        auto action=static_cast<gEAction>(melee::PhysicalAttackAction(power,thrust,int(gCEntity::GetUseType(static_cast<eCEntity*>(weapon))),int(type),int(gESpecies(target.NPC.Species))));
        // The native damage reaction still checks victim routine/FOV, skill,
        // protection and consequences. Request Pierce; never kill directly.
        DamageContext context{&target,&weapon,player,routine,routine->GetAction().GetValue(),action,amount,type};
        routine->SetAction(bTPropertyContainer<gEAction>(context.applied));
        CallWithRestoration(Damage,Restore,&context);
        return int(action);
    }
};
}
