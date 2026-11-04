#pragma once
#include <g3sdk/Script.h>
#include "VirtualHook.h"
#include "ChestSkinPose.h"
#include "ChestInteraction.h"
namespace diagnostic {
inline bool NativeChestInteraction(eCEntity* player,bool wasChest=false){
    if(!Readable(player,sizeof(eCDynamicEntity)))return false;
    auto* routine=static_cast<gCScriptRoutine_PS*>(player->GetPropertySet(eEPropertySetType_ScriptRoutine));
    if(!Readable(routine,sizeof(*routine)))return false;
    auto* target=routine->GetSPU().GetTargetEntity();
    bool chest=Readable(target,sizeof(eCDynamicEntity))&&int(gCEntity::GetUseType(target))==106;
    return ChestInteraction(chest,routine->GetAction().GetValue(),routine->GetCurrentTask().GetText(),routine->GetCurrentState().GetText(),wasChest);
}
// Freeze only the displayed hero skin. Animation events, interaction alignment,
// chest lid motion, lock checks and script timing still run in the native game.
class NativeChestVisual {
    ChestSkinPose pose;
    uintptr_t owner=0; eCVisualAnimationFactory* factory=nullptr;
    eCWrapper_emfx2Actor* wrapper=nullptr;EMotionFX::Actor* actor=nullptr;void* nodes=nullptr;
    bool freeze=false,observe=false;float root[3]{};
    bool Valid()const{
        return Readable(factory,sizeof(*factory))&&factory->GetAssembledActor()==wrapper&&
            Readable(wrapper,sizeof(*wrapper))&&wrapper->GetInstance()==actor&&Readable(actor,sizeof(*actor))&&actor->mNodes.mData==nodes;
    }
public:
    void Abandon(){pose.Restore(nullptr,0);owner=0;factory=nullptr;wrapper=nullptr;actor=nullptr;nodes=nullptr;freeze=observe=false;pose.Forget();}
    bool Ready()const{return Valid()&&pose.Matches(owner,reinterpret_cast<uintptr_t>(factory),unsigned(factory->m_arrRenderMatrices.GetCount()));}
    void Restore(){
        if(!pose.Pending())return;
        if(Valid()){auto& matrices=factory->m_arrRenderMatrices;auto* data=matrices.AccessArray();int count=matrices.GetCount();
            if(count>0&&count<=int(TrackedPalette::Limit)&&Readable(data,count*sizeof(arms::Mat))){pose.Restore(data,unsigned(count));return;}}
        pose.Restore(nullptr,0);pose.Forget();
    }
    void End(){Restore();freeze=observe=false;}
    void Reset(){End();owner=0;factory=nullptr;wrapper=nullptr;actor=nullptr;nodes=nullptr;pose.Forget();}
    void Begin(eCEntity* player,bool chest,bool gameplay){
        End();if(!Readable(player,sizeof(eCDynamicEntity))){Reset();return;}
        auto* animation=static_cast<eCVisualAnimation_PS*>(player->GetPropertySet(eEPropertySetType_Animation));
        if(!Readable(animation,sizeof(*animation))||animation->GetRagDollEnabled()){Reset();return;}
        auto* next=&animation->GetAnimationFactory();auto* nextWrapper=next->GetAssembledActor();
        if(!Readable(nextWrapper,sizeof(*nextWrapper))){Reset();return;}auto* nextActor=nextWrapper->GetInstance();
        if(!Readable(nextActor,sizeof(*nextActor))){Reset();return;}
        if(owner!=reinterpret_cast<uintptr_t>(player)||factory!=next||wrapper!=nextWrapper||actor!=nextActor||nodes!=nextActor->mNodes.mData){
            Reset();owner=reinterpret_cast<uintptr_t>(player);factory=next;wrapper=nextWrapper;actor=nextActor;nodes=actor->mNodes.mData;}
        auto p=player->GetWorldPosition();root[0]=p.m_fX;root[1]=p.m_fY;root[2]=p.m_fZ;
        freeze=chest;observe=gameplay&&!chest;
        if(!freeze&&!observe)pose.Forget();
    }
    bool Matches(eCVisualAnimationFactory* self)const{return self==factory&&(freeze||observe);}
    void Updated(eCVisualAnimationFactory* self,bCMatrix const& world){
        if(!Matches(self)||!Valid())return;
        auto* player=reinterpret_cast<eCEntity*>(owner);if(!Readable(player,sizeof(eCDynamicEntity)))return;
        auto const& actual=player->GetWorldMatrix();for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
            if(!std::isfinite(world(r,c))||std::fabs(world(r,c)-actual(r,c))>.05f)return;
        auto& matrices=self->m_arrRenderMatrices;int count=matrices.GetCount();auto* data=matrices.AccessArray();
        if(count<=0||count>int(TrackedPalette::Limit)||!Readable(data,count*sizeof(arms::Mat)))return;
        auto* skin=reinterpret_cast<arms::Mat*>(data);auto key=reinterpret_cast<uintptr_t>(self);
        if(observe){pose.Remember(owner,key,skin,unsigned(count),root);return;}
        pose.Apply(owner,key,skin,unsigned(count),root);
    }
};
}
