#pragma once
#include "RuntimeProbePolicy.h"
#include <g3sdk/Script.h>
#include "PhysicalBow.h"
#include "BowModel.h"
#include "TrackedPalette.h"
#include "RangedRenderRecovery.h"
#include "NativeRangedVisual.h"
namespace diagnostic {
// Animated bows cache a world-space skinning palette by frame ID. Moving the
// entity after that cache was populated never moves the rendered bow. Supply
// the tracked world pose to the real palette builder and invalidate its frame
// cache for each eye, then restore both palette and cache metadata.
class NativeTrackedBow {
    struct Item {eCEntity* entity=nullptr;eCVisualAnimationFactory* factory=nullptr;eCWrapper_emfx2Actor* wrapper=nullptr;EMotionFX::Actor* actor=nullptr;EMotionFX::Node* nodes=nullptr;int count=0;TrackedPalette palette;arms::Mat desired,local;GEUInt nativeFrame=0;bool cached=false,moved=false;ranged::NockedIdentity nock;ranged::VisualScope<ranged::EntityVisualAccess> visual;};
    Item items[2];bool active=false;uint64_t nextLog=0,applied=0;void (__cdecl* logger)(char const*)=nullptr;
    bow::Model model;std::string modelName;uint64_t nextModelRetry=0;ranged::Retry retry;float motionTime=0;arms::Vec nockLocal,nockWorld;arms::Mat arrowPose;bool geometry=false,modelCrossbow=false;
    static ranged::PaletteOwner Owner(Item const& item){return {reinterpret_cast<uintptr_t>(item.factory),reinterpret_cast<uintptr_t>(item.wrapper),reinterpret_cast<uintptr_t>(item.actor),reinterpret_cast<uintptr_t>(item.nodes),unsigned(item.count)};}
    static bool Text(char const* source,std::string& result){
        result.clear();for(unsigned i=0;i<160;++i){if(!Readable(source?source+i:nullptr,1))return false;char c=source[i];if(!c)return !result.empty();result.push_back(c);}return false;
    }
    static bool Bind(Item& out,eCEntity* entity,bool animatedRequired=true){
        if(!Readable(entity,sizeof(eCEntity)))return false;
        auto* animation=static_cast<eCVisualAnimation_PS*>(entity->GetPropertySet(eEPropertySetType_Animation));
        if(!animation&&!animatedRequired){out.entity=entity;out.factory=nullptr;out.wrapper=nullptr;out.actor=nullptr;out.nodes=nullptr;out.count=0;return true;}
        if(!Readable(animation,sizeof(*animation)))return false;
        auto* factory=&animation->GetAnimationFactory();if(!Readable(factory,sizeof(*factory)))return false;
        auto* wrapper=factory->GetAssembledActor();if(!Readable(wrapper,sizeof(*wrapper)))return false;
        auto* actor=wrapper->GetInstance();if(!Readable(actor,sizeof(*actor)))return false;
        int count=actor->mNodes.mLength;auto* nodes=actor->mNodes.mData;
        if(count<=0||count>int(TrackedPalette::Limit)||!Readable(nodes,count*sizeof(*nodes)))return false;
        out.entity=entity;out.factory=factory;out.wrapper=wrapper;out.actor=actor;out.nodes=nodes;out.count=count;return true;
    }
    static bool Valid(Item const& item){
        if(!Readable(item.entity,sizeof(eCEntity)))return false;
        if(item.nock.item&&!ranged::LiveNock(item.nock))return false;
        if(!item.factory)return item.nock.item&&!item.entity->HasPropertySet(eEPropertySetType_Animation);
        if(!Readable(item.factory,sizeof(*item.factory)))return false;
        auto* animation=static_cast<eCVisualAnimation_PS*>(item.entity->GetPropertySet(eEPropertySetType_Animation));
        if(!Readable(animation,sizeof(*animation))||&animation->GetAnimationFactory()!=item.factory)return false;
        auto* wrapper=item.factory->GetAssembledActor();if(!Readable(wrapper,sizeof(*wrapper)))return false;
        auto* actor=wrapper->GetInstance();if(!Readable(actor,sizeof(*actor)))return false;
        ranged::PaletteOwner current{reinterpret_cast<uintptr_t>(item.factory),reinterpret_cast<uintptr_t>(wrapper),reinterpret_cast<uintptr_t>(actor),reinterpret_cast<uintptr_t>(actor->mNodes.mData),unsigned(actor->mNodes.mLength)};
        return Owner(item).Same(current)&&Readable(actor->mNodes.mData,item.count*sizeof(EMotionFX::Node));
    }
    static arms::Mat Raw(bCMatrix const& value){arms::Mat out;memcpy(&out,&value,sizeof(out));return out;}
    static bool ReadAsset(std::string const& name,std::vector<unsigned char>& bytes){
        for(char const* root:{"Data/_compiledAnimation/","_compiledAnimation/",""}){
            eCArchiveFile file;if(!eCArchiveManager::GetInstance().GetFile(bCString((root+name).c_str()),file)||!file.Open(eCArchiveFile::bEArchiveFileAccessMode_Read,GEFalse))continue;
            auto size=file.GetFileSize();if(size<=0||size>16*1024*1024){file.Close();continue;}bytes.resize(size_t(size));bool ok=file.Read(bytes.data(),GEU32(size))==GEU32(size);file.Close();if(ok)return true;
        }return false;
    }
    void LoadModel(Item const& item,bool crossbow){
        // Factories/actors can be rebuilt or their addresses reused after a
        // save load. Cache immutable parsed assets by their name, not pointer.
        std::string name;if(!Valid(item)||!Text(item.wrapper->GetActorName().GetText(),name)){model.Clear();modelName.clear();return;}
        auto slash=name.find_last_of("/\\");if(slash!=std::string::npos)name.erase(0,slash+1);auto dot=name.find_last_of('.');if(dot!=std::string::npos)name.resize(dot);
        if(modelName==name&&modelCrossbow==crossbow&&(model.valid||GetTickCount64()<nextModelRetry))return;
        model.Clear();modelName=name;modelCrossbow=crossbow;nextModelRetry=GetTickCount64()+1000;
        std::vector<unsigned char> actor,motion;bool loaded=ReadAsset(name+".xact",actor)&&ReadAsset(name+"_Cock.xmot",motion)&&model.Load(actor,motion,crossbow);
        char text[260];sprintf_s(text,"vr_bow_native_rig model=%.100s loaded=%d refused_at=%u grip=%.1f,%.1f,%.1f string=%.1f,%.1f,%.1f draw_max=%.1f",name.c_str(),loaded,model.refusedAt,model.grip.x,model.grip.y,model.grip.z,model.nock.x,model.nock.y,model.nock.z,model.maximumDraw);if(logger)logger(text);
    }
public:
    bool fault=false;
    void Fail(char const* reason){fault=true;active=false;geometry=false;retry.Fail(GetTickCount64());nextModelRetry=0;
        char text[192];sprintf_s(text,"vr_ranged_render_retry reason=%.80s delay_ms=%llu",reason,static_cast<unsigned long long>(ranged::Retry::DelayMs));if(logger)logger(text);
    }
    // Called after a guarded restoration itself failed. Forget borrowed
    // pointers rather than writing them into actors rebuilt by a save load.
    void Abandon(){for(auto& item:items){item.palette.Restore(nullptr,0);item.cached=item.moved=false;item.visual.Abandon();item.nock={};item.entity=nullptr;item.factory=nullptr;}active=false;geometry=false;}
    void RestorePalette(){for(auto& item:items)if(item.cached){item.cached=false;
        if(!Valid(item)){item.palette.Restore(nullptr,0);continue;}
        auto& matrices=item.factory->m_arrRenderMatrices;int n=matrices.GetCount();
        if(n<=0||n>int(TrackedPalette::Limit)||!Readable(matrices.AccessArray(),n*sizeof(arms::Mat))||!item.palette.Restore(matrices.AccessArray(),n)){
            item.palette.Restore(nullptr,0);item.factory->m_bLastRenderMatricesUpdateFrame=item.nativeFrame^1u;
        }
        else item.factory->m_bLastRenderMatricesUpdateFrame=item.nativeFrame;
    }}
    void RestoreEye(){RestorePalette();for(unsigned index=2;index-- >0;){auto& item=items[index];bool moved=item.moved;item.moved=false;
        if(!moved)continue;bool valid=Valid(item);item.visual.Restore(valid);
        if(valid){bCMatrix local;memcpy(&local,&item.local,sizeof(local));item.entity->SetLocalMatrix(local);}
    }}
    void PrepareEye(){if(!active||fault)return;for(auto& item:items){
        // Static ammo must move before frustum rejection too; it cannot depend
        // on the hero palette being updated or rendered in this eye.
        if(!Valid(item)||!arms::Affine(item.desired))continue;
        if(!item.moved){
            item.local=Raw(item.entity->GetLocalMatrix());
            if(!item.visual.Capture(item.entity))continue;
            item.moved=true;
        }
        item.visual.Show();
        bCMatrix world;memcpy(&world,&item.desired,sizeof(world));item.entity->SetToWorldMatrix(world);
    }}
    void End(){RestoreEye();active=false;geometry=false;for(auto& item:items){item.factory=nullptr;item.entity=nullptr;item.nock={};}}
    void Begin(bow::Sample const& sample,float chargeFraction,void (__cdecl* log)(char const*)){End();logger=log;
        if(!sample.valid){retry.Observe({},GetTickCount64());return;}
        bool wasFault=fault;if(!retry.Observe({sample.player,sample.weapon,sample.session},GetTickCount64()))return;fault=false;
        if(wasFault){model.Clear();modelName.clear();}
        auto* player=reinterpret_cast<eCEntity*>(sample.player);auto* bow=reinterpret_cast<eCEntity*>(sample.weapon);
        // Do not dereference a retained sample from an unloaded player or a
        // weapon which the quick slot has already replaced/holstered.
        if(player!=gCSession::GetInstance().GetPlayer()||!Readable(player,sizeof(eCEntity))||!Readable(bow,sizeof(eCEntity)))return;
        if(gCEntity::GetWeapon(player,GETrue)!=bow&&gCEntity::GetWeapon(player,GEFalse)!=bow)return;
        if(gCEntity::GetUseType(bow)!=(sample.crossbow?gEUseType_CrossBow:gEUseType_Bow))return;
        auto* inventory=static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory));
        eCEntity* arrow=Readable(inventory,sizeof(*inventory))?inventory->GetStackFromSlot(gESlot_RightHand):nullptr;
        eCEntity* entity[2]{bow,arrow};arms::Vec origin,direction;float distance;if(!bow::Aim(sample,origin,direction,distance))return;
        if(!Bind(items[0],bow))return;
        for(unsigned i=0;i<2;++i){auto* item=entity[i];if(!Readable(item,sizeof(eCEntity))||item==player||(i&&gCEntity::GetUseType(item)!=(sample.crossbow?gEUseType_Bolt:gEUseType_Arrow)))continue;
            if(i){
                ranged::NockedIdentity binding{sample.player,sample.weapon,reinterpret_cast<uintptr_t>(item),sample.crossbow?7:4};
                if(!ranged::LiveNock(binding)||!Bind(items[i],item,false))continue;items[i].nock=binding;
            }
            if(!i){LoadModel(items[i],sample.crossbow);
                items[i].desired=model.valid?bow::AnchoredMesh(sample.leftAim,model.grip,sample.crossbow):bow::MeshPose(sample.leftAim,sample.crossbow);
                if(model.valid){motionTime=model.TimeForDraw(sample.crossbow?model.maximumDraw:bow::VisibleDraw(chargeFraction,model.maximumDraw));arms::Vec local,shaftDirection;if(model.ArrowAt(motionTime,local,shaftDirection)){
                    auto offset=model.nock-model.grip;nockLocal=sample.crossbow?arms::Vector(bow::MeshPose(arms::Mat{},true),offset):arms::Vec{offset.y,offset.z,offset.x};auto nock=arms::Point(items[i].desired,local);nockWorld=nock;
                    float tail=Readable(arrow,sizeof(eCEntity))?arrow->GetLocalNodeBoundary().m_Min.m_fZ:0;
                    shaftDirection=arms::Vector(items[i].desired,shaftDirection);
                    arrowPose=bow::NockedArrow(nock,shaftDirection,arms::Vector(sample.leftAim,{0,1,0}),tail);geometry=arms::Affine(arrowPose);
                }}
            }else items[i].desired=geometry?arrowPose:bow::ArrowPose(arms::Position(sample.right),direction,arms::Vector(sample.leftAim,{0,1,0}));
            active=true;
            if(probes::Enabled&&GetTickCount64()>=nextLog){char text[260];auto const& box=item->GetLocalNodeBoundary();sprintf_s(text,"vr_bow_render_binding type=%d animated=%d factory=%08lx matrices=%d bounds=%.1f,%.1f,%.1f/%.1f,%.1f,%.1f render_enabled=%d alpha=%.2f right_tracked=%d spatial_pose=1",int(gCEntity::GetUseType(item)),items[i].factory!=nullptr,DWORD(reinterpret_cast<uintptr_t>(items[i].factory)),(items[i].factory?items[i].factory->m_arrRenderMatrices.GetCount():0),box.m_Min.m_fX,box.m_Min.m_fY,box.m_Min.m_fZ,box.m_Max.m_fX,box.m_Max.m_fY,box.m_Max.m_fZ,item->IsRenderingEnabled(),item->GetRenderAlphaValue(),sample.rightTracked);if(logger)logger(text);}
        }
        if(probes::Enabled&&GetTickCount64()>=nextLog)nextLog=GetTickCount64()+1000;
    }
    bool Geometry(arms::Vec& rest,arms::Mat& arrow,arms::Vec& drawn)const{if(!geometry)return false;rest=nockLocal;arrow=arrowPose;drawn=nockWorld;return true;}
    bool SupportGeometry(arms::Mat& mesh,arms::Vec& contact)const{if(!model.valid||!modelCrossbow||!items[0].factory)return false;mesh=items[0].desired;contact=model.foregrip;return true;}
    bool CrossbowFrame()const{return active&&modelCrossbow;}
    bool Matches(eCVisualAnimationFactory const* factory)const{return active&&!fault&&((items[0].factory&&items[0].factory==factory)||(items[1].factory&&items[1].factory==factory));}
    void Apply(eCVisualAnimationFactory* factory,GEU32 frame,void* original){
        if(!Matches(factory))return;Item* item=items[0].factory==factory?items:items+1;
        if(!Valid(*item)){Fail("actor_replaced");RestoreEye();return;}
        auto& matrices=factory->m_arrRenderMatrices;int n=matrices.GetCount();auto* data=matrices.AccessArray();
        if(n!=item->count||n<=0||n>int(TrackedPalette::Limit)||!Readable(data,n*sizeof(arms::Mat))||!item->palette.Begin(data,n))return;
        item->nativeFrame=factory->m_bLastRenderMatricesUpdateFrame;item->cached=true;
        factory->m_bLastRenderMatricesUpdateFrame=frame^1u;
        bCMatrix desired;memcpy(&desired,&item->desired,sizeof(desired));using Update=GEBool(__thiscall*)(eCVisualAnimationFactory*,GEU32,bCMatrix const&);
        if(!reinterpret_cast<Update>(original)(factory,frame,desired)){Fail("palette_update_unavailable");RestoreEye();return;}
        if(item==items&&model.valid){
            if(!Valid(*item)){Fail("actor_replaced_during_update");RestoreEye();return;}
            // Scrub native bow limb/string motion in the borrowed render palette
            // only. The actor, animation time and NPC bows remain untouched.
            // Validate the whole model before changing any node. An unavailable
            // motion/model falls back to the tracked native palette, never a
            // partially modified palette or a process-wide disable latch.
            arms::Mat posed[TrackedPalette::Limit];bool complete=true;
            for(int i=0;i<n;++i){auto nativeName=item->wrapper->GetName(item->nodes+i);std::string name;
                if(!Text(nativeName.GetText(),name)||!model.Palette(name.c_str(),motionTime,item->desired,posed[i])){complete=false;break;}}
            if(complete)memcpy(data,posed,n*sizeof(arms::Mat));
            else{model.Clear();nextModelRetry=GetTickCount64()+1000;geometry=false;if(logger)logger("vr_ranged_model_palette_unavailable; tracked_native_palette_preserved");}
        }
        retry.Success();
        ++applied;
        if(probes::Enabled&&logger&&GetTickCount64()+1000>=nextLog&&applied%160==0){char text[200];auto p=arms::Position(item->desired);sprintf_s(text,"vr_bow_render_pose forced_palette=1 frame=%u nodes=%d origin=%.1f,%.1f,%.1f applied=%llu",frame,n,p.x,p.y,p.z,applied);logger(text);}
    }
};
}
