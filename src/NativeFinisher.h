#pragma once
#include <g3sdk/Script.h>
#include "CombatGestures.h"
#include "CombatRefinement.h"
#include "VirtualHook.h"
#include "RankedVisibility.h"
#include "FinisherRoiHint.h"
namespace diagnostic {
class NativeFinisher {
    combat::ExecutionHold hold;uintptr_t activePlayer=0;uint64_t started=0;
    combat::FinisherRoiHint hint;
    static bool Belongs(eCEntity* entity,eCEntity* owner){
        for(int i=0;i<8&&Readable(entity,sizeof(eCEntity));++i){
            if(entity==owner)return true;
            auto* interaction=static_cast<gCInteraction_PS*>(entity->GetPropertySet(eEPropertySetType_Interaction));
            if(Readable(interaction,sizeof(*interaction))&&interaction->GetOwner().GetEntity()==owner)return true;
            entity=entity->GetParent();
        }return false;
    }
    static bool ClearSight(eCEntity* player,eCEntity* target,WorldAim const& aim,float const (&lo)[3],float const (&hi)[3],float units,float rating){
        float point[3];if(!combat::FinisherSightPoint(aim,lo,hi,units,rating,point))return false;
        bCVector begin(aim.origin[0],aim.origin[1],aim.origin[2]),delta(point[0]-aim.origin[0],point[1]-aim.origin[1],point[2]-aim.origin[2]);
        float length=delta.GetMagnitude();if(length<.01f*units)return true;
        auto& physics=eCPhysicsScene::GetInstance();constexpr unsigned hints=eETraceRayHints_TestStaticShapes|eETraceRayHints_TestDynamicShapes|eETraceRayHints_Distance|eETraceRayHints_UserData;
        if(!physics.TraceRayAllHits(begin,delta,player,hints))return true;
        auto const& query=physics.GetQueryReport();int n=query.m_arrHitEntity.GetCount();if(n<0||n>4096||query.m_arrDistance.GetCount()!=n)return false;
        for(int i=0;i<n;++i){float distance=query.m_arrDistance[i];if(!std::isfinite(distance)||distance<.02f*units||distance>=length-.04f*units)continue;
            auto* hit=query.m_arrHitEntity[i];if(Belongs(hit,player)||Belongs(hit,target))continue;return false;}
        return true;
    }
public:
    int candidates=0,down=0,rays=0,clear=0;
    void Reset(){hold.Reset();activePlayer=0;started=0;hint.Reset();}
    void ForgetHint(){hint.Clear();}
    void SearchContext(eCEntity* player,eCEntity* weapon,uint64_t session){hint.Context(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(weapon),session);}
    bool HasHint()const{return hint.HasSelection();}
    uint64_t Reuses()const{return hint.reuses;}
    bool Pending()const{return hold.Holding();}
    bool Busy(eCEntity* player){
        if(!player||reinterpret_cast<uintptr_t>(player)!=activePlayer)return false;
        if(!Readable(player,sizeof(eCDynamicEntity))){Reset();return false;}
        auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(!Readable(routine,sizeof(*routine))){Reset();return false;}
        Entity hero(player);auto state=hero.Routine.GetCurrentState();
        bool native=routine->GetAction().GetValue()==gEAction_FinishingAttack||strstr(state.GetText(),"FinishingAttack")!=nullptr;
        if(!native&&GetTickCount64()-started>250){Reset();return false;}return native||GetTickCount64()-started<=250;
    }
    bool Eligible(eCEntity* player,eCEntity* target,float units){
        if(!std::isfinite(units)||units<=0||!Readable(player,sizeof(eCDynamicEntity))||!Readable(target,sizeof(eCDynamicEntity))||target==player||!target->HasPropertySet(eEPropertySetType_NPC))return false;
        return EligibleAt(player->GetWorldPosition(),target,units);
    }
    bool EligibleAt(bCVector const& a,eCEntity* target,float units){
        auto const& b=target->GetWorldPosition();
        if(!combat::FinisherWithinReach(a.m_fX,a.m_fY,a.m_fZ,b.m_fX,b.m_fY,b.m_fZ,units))return false;
        if(!gCEntity::IsDown(target)||gCEntity::IsDead(target))return false;
        Entity victim(target);auto species=gESpecies(victim.NPC.Species);
        return species==gESpecies_Human||species==gESpecies_Orc;
    }
    eCEntity* Reuse(eCEntity* player,WorldAim const& aim,float units){
        if(!hint.HasSelection())return nullptr;
        if(!Readable(player,sizeof(eCDynamicEntity))||!std::isfinite(units)||units<=0){hint.Clear();return nullptr;}
        auto const& interactions=gCInteractionAdmin::GetInstance().GetInteractionsInROI();int n=interactions.GetCount();
        if(n<0||n>65536){hint.Clear();return nullptr;}
        auto read=[&](size_t i){combat::RoiIdentity current{};
            combat::ReadOwnedRoiEntry([&](){auto* interaction=interactions[int(i)];if(!interaction)return false;
                current={reinterpret_cast<uintptr_t>(interaction),reinterpret_cast<uintptr_t>(interaction->GetEntity())};return true;});return current;};
        auto valid=[&](uintptr_t pointer){return combat::ReadOwnedRoiEntry([&](){auto* entity=reinterpret_cast<eCEntity*>(pointer);
            if(!Eligible(player,entity,units))return false;auto const& box=entity->GetWorldTreeBoundary();
            float lo[3]{box.m_Min.m_fX,box.m_Min.m_fY,box.m_Min.m_fZ},hi[3]{box.m_Max.m_fX,box.m_Max.m_fY,box.m_Max.m_fZ},rating;
            return combat::FinisherBounds(aim,lo,hi,units,rating)&&ClearSight(player,entity,aim,lo,hi,units,rating);});};
        return reinterpret_cast<eCEntity*>(hint.Reuse(size_t(n),read,valid));
    }
    eCEntity* Find(eCEntity* player,WorldAim const& aim,float units){
        candidates=down=rays=clear=0;hint.Clear();
        if(!Readable(player,sizeof(eCDynamicEntity))||!std::isfinite(units)||units<=0)return nullptr;
        // Native interaction ROI remains authoritative after knock-out even
        // when a living NPC's upright dynamic collider no longer participates.
        auto const& interactions=gCInteractionAdmin::GetInstance().GetInteractionsInROI();
        int n=interactions.GetCount();if(n<0||n>65536)return nullptr;float best=2;auto origin=player->GetWorldPosition();
        struct Candidate {eCEntity* entity=nullptr;float rating=0,lo[3]{},hi[3]{};int index=0;uintptr_t interaction=0;};
        RankedVisibility<Candidate> ranked;Candidate selected;
        auto visible=[&](Candidate const& c){++rays;bool hit=ClearSight(player,c.entity,aim,c.lo,c.hi,units,c.rating);if(hit)++clear;return hit;};
        for(int i=0;i<n;++i){Candidate candidate;
            bool accepted=combat::ReadOwnedRoiEntry([&](){auto* interaction=interactions[i];if(!interaction)return false;
                auto* entity=interaction->GetEntity();if(!entity||entity==player)return false;auto const& position=entity->GetWorldPosition();
                // Reject distant clutter before native NPC/property searches.
                if(!combat::FinisherWithinReach(origin.m_fX,origin.m_fY,origin.m_fZ,position.m_fX,position.m_fY,position.m_fZ,units)||!entity->HasPropertySet(eEPropertySetType_NPC))return false;++candidates;
                if(!EligibleAt(origin,entity,units))return false;++down;auto const& box=entity->GetWorldTreeBoundary();
                candidate.entity=entity;candidate.index=i;candidate.interaction=reinterpret_cast<uintptr_t>(interaction);
                float lo[3]{box.m_Min.m_fX,box.m_Min.m_fY,box.m_Min.m_fZ},hi[3]{box.m_Max.m_fX,box.m_Max.m_fY,box.m_Max.m_fZ};
                for(unsigned j=0;j<3;++j){candidate.lo[j]=lo[j];candidate.hi[j]=hi[j];}
                return combat::FinisherBounds(aim,candidate.lo,candidate.hi,units,candidate.rating)&&candidate.rating<best;});
            if(!accepted)continue;
            if(ranked.Full())ranked.Drain(best,selected,visible);
            ranked.Add(candidate);}
        ranked.Drain(best,selected,visible);
        if(selected.entity)hint.Remember(size_t(selected.index),{selected.interaction,reinterpret_cast<uintptr_t>(selected.entity)});
        return selected.entity;
    }
    bool Update(eCEntity* player,eCEntity* target,bool held,bool allowed,uint64_t time,double seconds,float units){
        if(Busy(player))return false;
        bool eligible=allowed&&Eligible(player,target,units);
        if(!hold.Step(reinterpret_cast<uintptr_t>(target),held,eligible,time,seconds))return false;
        // Let the existing player state and native finishing animation own
        // death, attack reason, witnesses, quest credit and victim protection.
        Entity hero(player),victim(target);hero.NPC.SetCurrentTarget(victim);hero.Focus.SetFocusEntity(victim);
        hero.Routine.SetState("PS_Melee_FinishingAttack");activePlayer=reinterpret_cast<uintptr_t>(player);started=GetTickCount64();return true;
    }
};
}
