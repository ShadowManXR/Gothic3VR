#pragma once
#include <g3sdk/Script.h>
#include "PhysicalMelee.h"
#include "NativeApplication.h"
#include "MeleeHitPointsHook.h"
#include "WeaponTypes.h"
#include "DualWield.h"
#include "TwoHandWeight.h"
#include "NativeCombatRules.h"
#include "CombatRefinement.h"
#include "SwingVelocity.h"
#include "StabDistance.h"

namespace diagnostic {
class NativeMelee {
    using Vec=arms::Vec;
    void (__cdecl* logger)(char const*)=nullptr;
    melee::CombatChannel channels[2];uint64_t nextLog[2]{},nextVelocityLog[2]{};unsigned contactHand=1;bool unsupportedContact=false;uint64_t contactTime=0,singleNextHit[2]{};
    combat::SwingVelocity velocities[2];int speedMinimums[3]{100,100,100};
    combat::StabDistance stabs[2];int stabMinimums[3]{15,15,15},stabSpeedMinimums[3]{50,50,50};
    combat::StrokeDistance strokeDistances[2];int strokeMinimums[6]{50,50,50,50,50,50};
    NativeCombatRules rules;combat::PowerCharge charges[2];combat::StrokeBudget budgets[2];bool chargedStroke[2]{},thrustStroke[2]{};
    eCEntity* combatPlayer=nullptr;uintptr_t equipment[2]{},chargePlayers[2]{};uint64_t chargeSessions[2]{},nextTierRead[2]{};int skillTiers[2]{};
    void (__cdecl* feedback)(unsigned,float)=nullptr;
    eCEntity* pendingTarget=nullptr; eCEntity* pendingWeapon=nullptr;DWORD pendingThread=0;
    int maximum=0,calculated=0,delivered=0,hpDelivered=0,nativeBudget=0;float fraction=0;bool damageObserved=false,hpObserved=false;
    void Report(char const* text){if(logger)logger(text);}
    static bool Profile(wchar_t const* module,char const* hex){
        auto image=GetModuleHandleW(module);wchar_t path[MAX_PATH];unsigned char actual[32]{},expected[32]{};
        for(unsigned i=0;i<32;++i){unsigned value=0;if(sscanf_s(hex+2*i,"%2x",&value)!=1)return false;expected[i]=static_cast<unsigned char>(value);}
        return image&&GetModuleFileNameW(image,path,MAX_PATH)&&FileSha256(path,actual)&&!memcmp(actual,expected,32);
    }
    bool Owned(eCEntity* entity,eCEntity* player,eCEntity* weapon){
        for(unsigned i=0;i<8&&Readable(entity,sizeof(eCEntity));++i){if(entity==player||entity==weapon)return true;entity=entity->GetParent();}return false;
    }
    eCEntity* Receiver(eCEntity* entity){
        for(unsigned i=0;i<8&&Readable(entity,sizeof(eCEntity));++i){
            if(entity->HasPropertySet(eEPropertySetType_NPC)&&entity->HasPropertySet(eEPropertySetType_DamageReceiver))return entity;
            entity=entity->GetParent();
        }return nullptr;
    }
    struct Contact {eCEntity* entity=nullptr;Vec point;bool hit=false;};
    Contact Trace(Vec a,Vec b,eCEntity* player,eCEntity* weapon){
        Contact result;float length=arms::Length(b-a);if(length<.001f)return result;
        auto& physics=eCPhysicsScene::GetInstance();
        constexpr unsigned hints=eETraceRayHints_TestStaticShapes|eETraceRayHints_TestDynamicShapes|eETraceRayHints_Impact|eETraceRayHints_Distance|eETraceRayHints_UserData;
        if(!physics.TraceRayAllHits(bCVector(a.x,a.y,a.z),bCVector(b.x-a.x,b.y-a.y,b.z-a.z),player,hints))return result;
        auto const& query=physics.GetQueryReport();
        int n=query.m_arrHitEntity.GetCount();if(n<0||n>4096||query.m_arrDistance.GetCount()!=n||query.m_arrWorldImpact.GetCount()!=n)return result;
        float nearest=length+.01f;
        for(int i=0;i<n;++i){auto* entity=query.m_arrHitEntity[i];float distance=query.m_arrDistance[i];
            if(!std::isfinite(distance)||distance<0||distance>nearest||Owned(entity,player,weapon))continue;
            auto const& point=query.m_arrWorldImpact[i];nearest=distance;result={entity,{point.m_fX,point.m_fY,point.m_fZ},true};
        }return result; // Consume the shared report before any other physics query.
    }
    void Apply(eCEntity* target,eCEntity* weapon,int weaponMaximum,float intensity,char const* source){
        auto* damage=static_cast<gCDamage_PS*>(weapon->GetPropertySet(eEPropertySetType_Damage));
        auto* receiver=static_cast<gCDamageReceiver_PS*>(target->GetPropertySet(eEPropertySetType_DamageReceiver));
        if(!Readable(damage,sizeof(*damage))||!Readable(receiver,sizeof(*receiver)))return;
        // The native callback must be intercepted; otherwise do not risk an unscaled hit.
        auto** table=*reinterpret_cast<void***>(target);
        if(!Readable(table,97*sizeof(void*))||table[96]!=damageHook.replacement)return;
        Entity victim(target),inflictor(weapon);int before=receiver->GetHitPoints();
        pendingTarget=target;pendingWeapon=weapon;pendingThread=GetCurrentThreadId();maximum=weaponMaximum;fraction=intensity;
        damageObserved=hpObserved=false;calculated=delivered=hpDelivered=nativeBudget=0;
        // This is a weapon contact, not an attack animation or synthetic click.
        // The inflictor is the actual owned weapon, so native scripts resolve the hero.
        int action=NativeCombatRules::Hit(victim,inflictor,combatPlayer,weaponMaximum,damage->GetDamageType().GetValue(),chargedStroke[contactHand],thrustStroke[contactHand]);
        int after=receiver->GetHitPoints();ClearPending();
        char text[240];_snprintf_s(text,_TRUNCATE,"vr_melee_hit hand=%u target=%08lx action=%d thrust=%d weapon_base=%d intensity=%.2f native=%d applied=%d hp=%d->%d callback=%d hp_callback=%d source=%s",contactHand,DWORD(reinterpret_cast<uintptr_t>(target)),action,thrustStroke[contactHand],weaponMaximum,intensity,calculated,hpDelivered,before,after,damageObserved,hpObserved,source);Report(text);
    }
    void Touch(melee::Swing& swing,eCEntity* target,Vec point,melee::Sample const& sample,eCEntity* player,eCEntity* weapon,int limit,char const* source){
        if(!melee::UnsupportedHitReady(contactTime,singleNextHit[contactHand],unsupportedContact))return;
        if(!target||target==player||swing.AlreadyHit(reinterpret_cast<uintptr_t>(target))||gCEntity::IsDead(target)||gCEntity::IsDown(target))return;
        // Check both directions: an origin inside a collider can miss its entry.
        auto visible=Trace(sample.hand,point,player,weapon);
        if(visible.hit&&Receiver(visible.entity)!=target){++blocked;return;}
        auto reverse=Trace(point,sample.hand,player,weapon);
        if(reverse.hit&&Receiver(reverse.entity)!=target){++blocked;return;}
        if(!swing.Hit(reinterpret_cast<uintptr_t>(target)))return;
        ++hits;Apply(target,weapon,limit,swing.intensity,source);if(unsupportedContact)singleNextHit[contactHand]=melee::UnsupportedNextHit(contactTime);
    }
    void OverlapSweep(melee::Swing& swing,melee::Sample const& sample,eCEntity* player,eCEntity* weapon,int limit){
        unsigned steps=melee::CapsuleSteps(swing.from.blade,swing.to.blade,sample.units);if(!steps)return;
        bTPtrArray<eCEntity*> entities;bCCapsule capsule;
        for(unsigned step=0;step<=steps;++step){
            auto blade=melee::Interpolate(swing.from.blade,swing.to.blade,float(step)/steps);melee::BladeCapsule shape;
            if(!melee::Capsule(blade,sample.units,shape))continue;
            float rows[9];for(int row=0;row<3;++row)for(int col=0;col<3;++col)rows[row*3+col]=shape.orientation.m[row*4+col];
            bCMatrix3 orientation;orientation.SetMatrix(rows);capsule.SetOrientation(orientation);
            capsule.SetCenter(bCVector(shape.center.x,shape.center.y,shape.center.z));capsule.SetHeight(melee::NativeCapsuleHeight(shape));capsule.SetRadius(shape.radius);
            entities.RemoveAll();++overlapQueries;
            // Engine hash verified at Install: false selects accurate PhysX overlap;
            // dynamic shapes only, exclude player, collect all, no property/group filter.
            if(!eCPhysicsScene::GetInstance().CollectEntitiesInCapsule(capsule,GEFalse,player,&entities,eETraceRayHints_TestDynamicShapes,GEFalse,eEPropertySetType_Unknown,0))continue;
            int count=entities.GetCount();if(count<0||count>1024)return;
            for(int i=0;i<count;++i){auto* entity=entities[i];if(Owned(entity,player,weapon))continue;auto* target=Receiver(entity);
                if(!target||swing.AlreadyHit(reinterpret_cast<uintptr_t>(target)))continue;
                auto const& bounds=target->GetWorldNodeBoundary();Vec center{(bounds.m_Min.m_fX+bounds.m_Max.m_fX)*.5f,(bounds.m_Min.m_fY+bounds.m_Max.m_fY)*.5f,(bounds.m_Min.m_fZ+bounds.m_Max.m_fZ)*.5f};
                if(!arms::Finite(center))continue;++overlapContacts;
                Touch(swing,target,melee::ClosestPoint(blade,center),sample,player,weapon,limit,"capsule");
            }
        }
    }
public:
    VirtualHook damageHook;MeleeHitPointsHook hitPointsHook;bool ready=false,fault=false;uint64_t hits=0,blocked=0,overlapQueries=0,overlapContacts=0;
    bool Install(void* replacement,void* hpReplacement,void (__cdecl* log)(char const*)){
        logger=log;
        if(!Profile(L"Game.dll","4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00")||
           !Profile(L"Script.dll","03c483148d0ea88e007d2f18eab30400a6012439ebbe4fea6ebf837aa2abd981")||
           !Profile(L"Engine.dll","991cf674a9cf1bc60cddc204be0993e8251336f4d3471543a37846cc6840bf5a")||
           !Profile(L"Script_Game.dll","4d29189281ec26eac7c704fa67ed323df5c090d50b068626cf600aecf4262a24"))return false;
        auto game=GetModuleHandleW(L"Game.dll");size_t slot=0;
        auto** table=reinterpret_cast<void**>(GetProcAddress(game,"??_7gCEntity@@6B@"));
        auto expected=GetProcAddress(game,"?OnDamage@gCEntity@@UAEXPAVeCEntity@@0HHAAVeCContactIterator@@@Z");
        if(!VirtualSlot(&gCEntity::OnDamage,slot)||slot!=96||!Readable(table,97*sizeof(void*))||table[slot]!=expected||!damageHook.Prepare(table,slot,replacement)||!damageHook.Install())return false;
        auto hpEntry=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Script_Game.dll"))+0x35b50;
        if(!hitPointsHook.Install(hpEntry,hpReplacement)){damageHook.Restore();return false;}
        if(!rules.Initialize()){damageHook.Restore();return false;}
        ready=true;Report("vr_physical_melee_ready; swept_mesh_blade native_damage_post_armor_scaling; click_attacks_disabled_in_VR_melee");return true;
    }
    void ClearPending(){pendingTarget=pendingWeapon=nullptr;pendingThread=0;}
    bool Pending(eCEntity* target,eCEntity* inflictor)const{return pendingTarget&&target==pendingTarget&&inflictor==pendingWeapon&&GetCurrentThreadId()==pendingThread;}
    bool PendingHealth()const{return pendingTarget&&GetCurrentThreadId()==pendingThread;}
    int ScaleHealth(eCEntity* target,int delta){
        if(!PendingHealth()||target!=pendingTarget||delta>=0)return delta;
        // OnDamage calls AssessHit again. Scale its final signed HP delta,
        // before native ChangeHitPoints writes HP and the script tests for death.
        int native=int(std::min<int64_t>(-int64_t(delta),INT_MAX));
        if(!hpObserved)nativeBudget=native;
        calculated=native;delivered=std::min(melee::ScaledDamage(native,maximum,fraction),std::max(0,melee::ScaledDamage(nativeBudget,maximum,fraction)-hpDelivered));
        hpDelivered+=delivered;hpObserved=true;
        return -delivered;
    }
    void SetStabSettings(int const* values){for(unsigned i=0;i<3;++i)stabMinimums[i]=std::clamp(values[i],5,100);}
    void SetStabSpeedSettings(int const* values){for(unsigned i=0;i<3;++i)stabSpeedMinimums[i]=std::clamp(values[i],40,800);}
    void SetVelocitySettings(int const* values){for(unsigned i=0;i<3;++i)speedMinimums[i]=std::clamp(values[i],50,800);}
    void SetStrokeSettings(int const* values){for(unsigned i=0;i<6;++i)strokeMinimums[i]=std::clamp(values[i],10,200);}
    void SetFeedback(void (__cdecl* emit)(unsigned,float)){feedback=emit;}
    void Reset(){for(auto& stab:stabs)stab.Reset();for(auto& velocity:velocities)velocity.Reset();for(auto& distance:strokeDistances)distance.Reset();for(auto& channel:channels)channel.Reset();for(auto& charge:charges)charge.Reset();for(auto& budget:budgets)budget.Reset();memset(equipment,0,sizeof(equipment));memset(chargePlayers,0,sizeof(chargePlayers));memset(chargeSessions,0,sizeof(chargeSessions));combatPlayer=nullptr;ClearPending();}
    eCEntity* Weapon(eCEntity* player,unsigned hand=1){
        if(hand>1||!ready||!Readable(player,sizeof(eCDynamicEntity)))return nullptr;
        auto* weapon=gCEntity::GetWeapon(player,hand==1);if(!Readable(weapon,sizeof(eCEntity))||weapon==player)return nullptr;
        auto type=gCEntity::GetUseType(weapon);if(!PhysicalMeleeType(type))return nullptr;
        if(hand==0){auto* right=gCEntity::GetWeapon(player,GETrue);
            if(!Readable(right,sizeof(eCEntity))||!melee::DualWieldTypes(int(gCEntity::GetUseType(right)),int(type),reinterpret_cast<uintptr_t>(right),reinterpret_cast<uintptr_t>(weapon)))return nullptr;}
        auto* damage=static_cast<gCDamage_PS*>(weapon->GetPropertySet(eEPropertySetType_Damage));
        return Readable(damage,sizeof(*damage))&&damage->GetDamageAmount()>0?weapon:nullptr;
    }
    int Scale(eCEntity* target,eCEntity* inflictor,int native){
        if(!pendingTarget||target!=pendingTarget||inflictor!=pendingWeapon||GetCurrentThreadId()!=pendingThread)return native;
        auto* receiver=static_cast<gCDamageReceiver_PS*>(target->GetPropertySet(eEPropertySetType_DamageReceiver));
        calculated=native;delivered=melee::ScaledDamage(native,maximum,fraction);damageObserved=true;
        if(Readable(receiver,sizeof(*receiver)))receiver->SetDamageAmount(delivered);
        return delivered;
    }
    void Update(melee::Sample const& sample,eCEntity* player,eCEntity* weapon,bool allowed,uint64_t now,unsigned hand=1){
        if(hand>1)return;
        auto& channel=channels[hand];auto& swing=channel.swing;
        bool moving=channel.Step(sample,reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(weapon),allowed&&ready&&!fault,now);
        if(!allowed||!ready||fault||!sample.valid||!weapon){strokeDistances[hand].Reset();velocities[hand].Reset();stabs[hand].Reset();charges[hand].Reset();budgets[hand].Reset();equipment[hand]=chargePlayers[hand]=0;return;}
        if(equipment[hand]!=sample.weapon||chargePlayers[hand]!=sample.player||chargeSessions[hand]!=sample.session){strokeDistances[hand].Reset();velocities[hand].Reset();stabs[hand].Reset();charges[hand].Reset();budgets[hand].Reset();equipment[hand]=sample.weapon;chargePlayers[hand]=sample.player;chargeSessions[hand]=sample.session;nextTierRead[hand]=0;}
        if(now>=nextTierRead[hand]){skillTiers[hand]=NativeCombatRules::Tier(player,weapon);nextTierRead[hand]=now+1000;}
        int tier=skillTiers[hand];double dt=eCApplication::GetInstance().GetScaledFrameTimeInSeconds();
        if(charges[hand].Step(sample.weapon,sample.session,sample.time,sample.trigger,tier,!sample.heavySingleHand,dt)&&feedback)feedback(hand,.45f);
        combatPlayer=player;
        contactHand=hand;unsupportedContact=sample.heavySingleHand;contactTime=now;
        if(now>=nextLog[hand]){nextLog[hand]=now+1000;char text[240];sprintf_s(text,"vr_melee_swing hand=%u active=%d speed_mps=%.2f intensity=%.2f stroke=%u hits=%llu blocked=%llu overlaps=%llu contacts=%llu",hand,moving,swing.speed,swing.intensity,swing.generation,hits,blocked,overlapQueries,overlapContacts);Report(text);}
        if(sample.trackedBlade)strokeDistances[hand].Step(sample.trackingBlade,sample.time,swing.generation,moving);
        auto kind=WeaponType(int(gCEntity::GetUseType(weapon)));unsigned group=kind==WeaponKind::Staff?4:kind==WeaponKind::OneHand?0:2;
        if(sample.trackedBlade)velocities[hand].Step(sample.trackingBlade,sample.time,swing.generation,moving,speedMinimums[group/2]);else velocities[hand].Reset();
        if(sample.trackedBlade)stabs[hand].Step(sample.trackingBlade,sample.time,swing.generation,moving,stabSpeedMinimums[group/2]);else stabs[hand].Reset();
        bool stabReady=sample.trackedBlade&&stabs[hand].Ready(stabMinimums[group/2]);
        bool damageReady=stabReady||(sample.trackedBlade&&strokeDistances[hand].Ready(strokeMinimums[group])&&velocities[hand].Ready(strokeMinimums[group]));
        bool staminaReady=stabReady||(sample.trackedBlade&&strokeDistances[hand].Ready(strokeMinimums[group+1])&&velocities[hand].Ready(strokeMinimums[group+1]));
        if(now>=nextVelocityLog[hand]){nextVelocityLog[hand]=now+1000;char text[240];sprintf_s(text,"vr_melee_velocity hand=%u speed_mps=%.2f minimum_mps=%.2f damage_ready=%d stamina_ready=%d stab_ready=%d stab_cm=%d; tracking_space_fast_path=1",hand,velocities[hand].Speed(),speedMinimums[group/2]*.01f,damageReady,staminaReady,stabReady,stabMinimums[group/2]);Report(text);
            sprintf_s(text,"vr_melee_stab_speed hand=%u speed_mps=%.2f minimum_mps=%.2f distance_cm=%d ready=%d; independent_of_swing=1",hand,stabs[hand].Speed(),stabSpeedMinimums[group/2]*.01f,stabMinimums[group/2],stabReady);Report(text);}
        if((!damageReady&&!staminaReady)||!moving||!melee::UnsupportedHitReady(now,singleNextHit[hand],unsupportedContact))return;
        if(charges[hand].Holding()&&!charges[hand].Ready())return;
        bool charged=charges[hand].Ready()&&!sample.heavySingleHand;
        bool newStroke=!budgets[hand].ForStroke(swing.generation),newDebit=staminaReady&&(newStroke||!budgets[hand].Paid());
        int stamina=newStroke||newDebit?NativeCombatRules::Stamina(player):0;int normalCost=combat::QuickAttackCost(NativeCombatRules::Alternative());
        bool permitted=budgets[hand].Decide(swing.generation,charged,damageReady,staminaReady,normalCost,combat::PowerAttackCost,stamina,Entity::IsCheatGodEnabled(),[&](int cost){return NativeCombatRules::Spend(player,cost);});
        if(newStroke&&budgets[hand].Power())charges[hand].Consume();
        if(newDebit){char text[240];sprintf_s(text,"vr_melee_stamina hand=%u stroke=%u power=%d tier=%d cost=%d stamina=%d->%d allowed=%d; native_attribute_script=1",hand,swing.generation,budgets[hand].Power(),tier,budgets[hand].Power()?combat::PowerAttackCost:normalCost,stamina,NativeCombatRules::Stamina(player),permitted);Report(text);}
        if(!permitted)return;
        chargedStroke[hand]=budgets[hand].Power();if(chargedStroke[hand])charges[hand].Consume();
        thrustStroke[hand]=stabReady&&swing.from.trackedBlade&&swing.to.trackedBlade&&melee::ForwardThrust(swing.from.trackingBlade,swing.to.trackingBlade);
        if(chargedStroke[hand])swing.intensity=1;
        swing.intensity=melee::UnsupportedDamageFraction(swing.intensity,unsupportedContact);
        auto sweep=melee::BladeSweep(swing.from.blade,swing.to.blade,sample.units);
        auto* damage=static_cast<gCDamage_PS*>(weapon->GetPropertySet(eEPropertySetType_Damage));int limit=damage->GetDamageAmount();
        for(unsigned i=0;i<sweep.count;++i){
            auto contact=Trace(sweep.segments[i].a,sweep.segments[i].b,player,weapon);if(!contact.hit)continue;
            Touch(swing,Receiver(contact.entity),contact.point,sample,player,weapon,limit,"ray");
        }
        OverlapSweep(swing,sample,player,weapon,limit);
    }
};
}
