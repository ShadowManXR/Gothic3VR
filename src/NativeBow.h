#pragma once
#include <g3sdk/Script.h>
#include "PhysicalCrossbow.h"
#include "RangedRenderRecovery.h"
#include "NativeRangedVisual.h"
#include "RangedFaultRecovery.h"
#include "NockLease.h"
#include "NativePhasePerformance.h"
#include "NativeMotionDuration.h"
#include <string>
#include "NativeApplication.h"
#include "NativeRecovery.h"

namespace diagnostic {
static_assert(gEUseType_Bow==5&&gEUseType_CrossBow==6&&gEUseType_Arrow==4&&gEUseType_Bolt==7,"Native ranged weapon/ammo classification must match the SDK");
class NativeBow {
    void (__cdecl* logger)(char const*)=nullptr;
    using ShootScript=GEInt(GE_STDCALL*)(gCScriptProcessingUnit*,GELPVoid,GELPVoid,GEInt);
    ShootScript shoot=nullptr;DWORD pendingThread=0;eCEntity* pendingPlayer=nullptr;bow::Shot pending;gEUseType pendingAmmo=gEUseType_Arrow;
    ranged::NockLease nock;gEUseType ammoType=gEUseType_Arrow;
    char const* operation="idle";
    void (__cdecl* timing)(performance::NativePhase,uint64_t)=nullptr;
    uint64_t nextLog=0,nextNock=0,nextMotionRetry=0;unsigned launched=0;
    ranged::FaultRecovery recovery;
    crossbow::Reload reload;std::string reloadMotion;double reloadDuration=0;
    bow::ChargeProfile bowCharge;int reportedBowTier=0;
    void Report(char const* text){if(logger)logger(text);}
    static bool Profile(wchar_t const* name,char const* hex){
        auto module=GetModuleHandleW(name);wchar_t path[MAX_PATH];unsigned char actual[32]{},expected[32]{};
        for(unsigned i=0;i<32;++i){unsigned n=0;if(sscanf_s(hex+2*i,"%2x",&n)!=1)return false;expected[i]=static_cast<unsigned char>(n);}
        return module&&GetModuleFileNameW(module,path,MAX_PATH)&&FileSha256(path,actual)&&!memcmp(actual,expected,32);
    }
    static gCInventory_PS* Inventory(eCEntity* player){return Readable(player,sizeof(eCEntity))?static_cast<gCInventory_PS*>(player->GetPropertySet(eEPropertySetType_Inventory)):nullptr;}
    static bool Ammo(eCEntity* item,gEUseType type){return Readable(item,sizeof(eCEntity))&&gCEntity::GetUseType(item)==type&&item->HasPropertySet(eEPropertySetType_Projectile);}
    static gEUseType Ammunition(eCEntity* weapon){return gCEntity::GetUseType(weapon)==gEUseType_CrossBow?gEUseType_Bolt:gEUseType_Arrow;}
    bool Nock(eCEntity* player,gEUseType type){
        operation="read_nock_slot";ammoType=type;
        auto* inventory=Inventory(player);if(!Readable(inventory,sizeof(*inventory)))return false;
        auto* right=inventory->GetStackFromSlot(gESlot_RightHand);
        if(Ammo(right,type)){nock.Borrow(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(right),inventory->FindStackIndex(gESlot_RightHand));return true;}
        if(right&&right!=player)return false;
        Entity hero(player);int index=inventory->FindStackIndex(gESlot_Ammo);
        if(index<0||inventory->GetStackUseType(index)!=type||hero.Inventory.GetAmount(index)<=0){
            index=-1;for(int i=0;i<inventory->GetStackCount();++i)if(inventory->GetStackUseType(i)==type&&hero.Inventory.GetAmount(i)>0){index=i;break;}
        }
        if(index<0)return false;
        // The installed HoldStackRightHand unconditionally returns false after
        // linking the stack (Game RVA B84AD). Its actual slot is authoritative.
        operation="hold_right_ammo_stack";hero.Inventory.HoldRightStack(index);
        right=inventory->GetStackFromSlot(gESlot_RightHand);
        if(!Ammo(right,type))return false;
        nock.Borrow(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(right),inventory->FindStackIndex(gESlot_RightHand));
        Report("vr_ranged_nocked; native_ammo_stack_right_hand; no_ammo_consumed");return true;
    }
    bool Fire(eCEntity* player,bow::Shot const& shot){
        operation="validate_live_shot_slot";
        auto* inventory=Inventory(player);
        if(!Readable(inventory,sizeof(*inventory))||!nock.WithCurrent(reinterpret_cast<uintptr_t>(player),
            [&]{return ranged::NockSlot{reinterpret_cast<uintptr_t>(inventory->GetStackFromSlot(gESlot_RightHand)),inventory->FindStackIndex(gESlot_RightHand)};},
            [&](uintptr_t item){return Ammo(reinterpret_cast<eCEntity*>(item),ammoType);}))return false;
        Entity hero(player),other;int index=inventory->FindStackIndex(gESlot_RightHand),before=hero.Inventory.GetItemAmount(ammoType);
        if(index<0||hero.Inventory.GetAmount(index)<=0)return false;
        pendingPlayer=player;pendingThread=GetCurrentThreadId();pending=shot;pendingAmmo=ammoType;launched=0;
        // Verified native Shoot spawns the actual right-held ammo template,
        // assigns owner and weapon/ammo damage, consumes one and restores ammo.
        // Shoot owns/deletes the previous held entity. Nothing may retain it
        // across native projectile/inventory callbacks or an equipment change.
        nock.Clear();operation="native_shoot_script";
        uint64_t shootStarted=timing?performance::Now():0;
        int remaining=shoot(nullptr,&hero,&other,0);operation="native_shoot_result";
        ClearPending();
        if(timing)timing(performance::NativePhase::RangedScript,shootStarted);
        int after=hero.Inventory.GetItemAmount(ammoType);
        char text[192];sprintf_s(text,"vr_ranged_release type=%d power=%.2f projectile_calls=%u ammo=%d->%d remaining=%d",int(ammoType),shot.power,launched,before,after,remaining);Report(text);
        if(launched!=1){Fail(GetTickCount64());Report("vr_ranged_launch_count_refused; fresh_context_retry_required");}
        // The native script owns the stack after shooting. Never replenish here.
        nock.Clear();return launched==1&&!fault;
    }
    static bool ReadMotion(std::string name,std::vector<unsigned char>& bytes){
        if(name.empty())return false;
        if(name.size()<5||_stricmp(name.c_str()+name.size()-5,".xmot"))name+=".xmot";
        for(char const* root:{"", "Data/_compiledAnimation/", "_compiledAnimation/"}){
            eCArchiveFile file;if(!eCArchiveManager::GetInstance().GetFile(bCString((root+name).c_str()),file)||!file.Open(eCArchiveFile::bEArchiveFileAccessMode_Read,GEFalse))continue;
            auto size=file.GetFileSize();if(size<=0||size>16*1024*1024){file.Close();continue;}
            bytes.resize(size_t(size));bool ok=file.Read(bytes.data(),GEU32(size))==GEU32(size);file.Close();if(ok)return true;
        }return false;
    }
    static bool ResolveMotion(std::string const& alias,std::vector<unsigned char>& bytes,std::string& resolved,double& seconds){
        if(ReadMotion(alias,bytes)&&crossbow::MotionDuration(bytes,seconds)){resolved=alias;return true;}
        // Native compiled descriptor seen in the installed hero resource. VFS
        // still supplies any loose/patched override of this exact selected alias.
        if(ReadMotion(alias+"%_00_P0_0",bytes)&&crossbow::MotionDuration(bytes,seconds)){resolved=alias+"%_00_P0_0.xmot";return true;}
        // GetOverlayAni returns a prefix; compiled resources append the native
        // blend/pose descriptor (for example %_00_P0_0), before .xmot.
        for(char const* root:{"Data/_compiledAnimation/","_compiledAnimation/",""}){
            bTObjArray<bCString> files;eCArchiveManager::GetInstance().FindFiles(bCString((std::string(root)+alias+"*.xmot").c_str()),files);
            int count=files.GetCount();if(count<0||count>256)continue;std::vector<std::string> candidates;
            for(int i=0;i<count;++i){std::string file=files[i].GetText();if(crossbow::MotionAliasMatch(alias,file))candidates.push_back(file);}
            std::sort(candidates.begin(),candidates.end());
            for(auto const& file:candidates)if(ReadMotion(file,bytes)&&crossbow::MotionDuration(bytes,seconds)){resolved=file;return true;}
        }return false;
    }
    bool ReloadProfile(eCEntity* player,uint64_t now,double& speed,int& tier){
        Entity hero(player);auto name=hero.GetOverlayAni(gEAction_Cock,gEPhase_Begin);std::string selected=name.GetText();
        if(selected.empty())return false;
        if(selected!=reloadMotion||(reloadDuration<=0&&now>=nextMotionRetry)){
            reloadMotion=selected;reloadDuration=0;nextMotionRetry=now+5000;std::vector<unsigned char> bytes;
            std::string resolved;double duration=0;bool found=ResolveMotion(selected,bytes,resolved,duration);
            if(!found){auto* admin=static_cast<eCAnimationAdmin*>(eCModuleAdmin::GetInstance().FindModule("eCAnimationAdmin"));
                if(Readable(admin,sizeof(*admin))){auto* resource=admin->QueryMotionDataEntity(name,eEResourceCacheBehavior_Lazy);
                    if(Readable(resource,sizeof(*resource)))found=ResolveMotion(resource->GetFilePath().GetText(),bytes,resolved,duration)||ResolveMotion(resource->GetFileName().GetText(),bytes,resolved,duration);}}
            if(found)reloadDuration=duration;
            char text[192];sprintf_s(text,"vr_crossbow_native_reload_motion duration_s=%.6f parsed=%d name=%.90s",reloadDuration,reloadDuration>0,selected.c_str());Report(text);
            if(found){sprintf_s(text,"vr_crossbow_reload_resource=%.150s",resolved.c_str());Report(text);}
        }
        // Match installed PS_Ranged_Reload action Cock (43) speed helper:
        // active Crossbow III takes precedence over II, and reads native constants.
        auto* base=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Script_Game.dll"));float native=1; tier=1;
        if(hero.Inventory.IsSkillActive(Template("Perk_Crossbow_3"))){tier=3;memcpy(&native,base+0x107518,4);}
        else if(hero.Inventory.IsSkillActive(Template("Perk_Crossbow_2"))){tier=2;memcpy(&native,base+0x107638,4);}
        speed=native;return reloadDuration>0&&std::isfinite(speed)&&speed>0;
    }
public:
    char const* Operation()const{return operation;}
    void SetTiming(void (__cdecl* record)(performance::NativePhase,uint64_t)){timing=record;}
    void BeginContext(){operation="ranged_context";}
    VirtualHook projectileHook;bow::Draw draw;crossbow::Trigger crossbowTrigger;bool ready=false,fault=false;
    bool Install(void* replacement,void (__cdecl* log)(char const*)){
        logger=log;
        if(!Profile(L"Script_Game.dll","4d29189281ec26eac7c704fa67ed323df5c090d50b068626cf600aecf4262a24")||
           !Profile(L"Game.dll","4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00")||
           !Profile(L"Script.dll","03c483148d0ea88e007d2f18eab30400a6012439ebbe4fea6ebf837aa2abd981"))return false;
        auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Script_Game.dll"));
        constexpr unsigned char prefix[]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
        if(!Readable(base+0x86450,sizeof(prefix))||memcmp(base+0x86450,prefix,sizeof(prefix)))return false;
        auto** cell=reinterpret_cast<void**>(base+0xe4834);
        auto expected=GetProcAddress(GetModuleHandleW(L"Script.dll"),"?Shoot@PSProjectile@@QAEXXZ");
        if(!expected||!Readable(cell,sizeof(void*))||*cell!=expected||!projectileHook.Prepare(cell,0,replacement)||!projectileHook.Install())return false;
        memcpy(&bowCharge.seconds,base+0x1076d0,8);memcpy(&bowCharge.base,base+0x1076c8,8);
        memcpy(&bowCharge.gain,base+0x107508,8);memcpy(&bowCharge.velocity,base+0x107418,8);
        if(!bowCharge.Valid()){projectileHook.Restore();return false;}
        shoot=reinterpret_cast<ShootScript>(base+0x86450);ready=true;
        Report("vr_physical_bow_ready; native_Shoot_callback_and_projectile_import_verified");
        Report("vr_physical_crossbow_ready right_hand=1 right_trigger=1 shared_stabilized_reticle=1 native_bolt_ammo=1");return true;
    }
    eCEntity* Weapon(eCEntity* player){
        if(!ready||!Readable(player,sizeof(eCEntity))||NativeRecovery(player))return nullptr;
        for(unsigned hand=0;hand<2;++hand){auto* item=gCEntity::GetWeapon(player,hand==0);
            if(item!=player&&Readable(item,sizeof(eCEntity))&&(gCEntity::GetUseType(item)==gEUseType_Bow||gCEntity::GetUseType(item)==gEUseType_CrossBow))return item;}
        return nullptr;
    }
    void Fail(uint64_t now){Abandon();fault=true;recovery.Fail(now);Report("vr_ranged_fault_retry_scheduled; ownership_forgotten; delay_ms=500; trigger_release_required");}
    bool Recover(eCEntity* player,eCEntity* weapon,uint64_t session,uint64_t now){
        if(!ready||!player||!weapon)return false;
        if(!recovery.Observe({reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(weapon),session},now))return false;
        if(fault){Abandon();fault=false;nextMotionRetry=0;Report("vr_ranged_fault_recovered; native_current_weapon_validated; fresh_trigger_required");}
        return true;
    }
    void ClearPending(){pendingPlayer=nullptr;pendingThread=0;pending={};}
    // Menu/equipment/recovery code owns native inventory transitions. A VR
    // cancel only releases its borrow; it must not re-equip an old stack while
    // the game is already replacing/removing it. The next live Nock reacquires.
    void Cancel(){operation="cancel_borrow";draw.Cancel();crossbowTrigger.Cancel();ClearPending();nock.Clear();nextNock=0;}
    // After native world teardown a readable pointer is not a live entity.
    // Forget ownership without invoking any inventory/entity/SDK operation.
    void Abandon(){draw.Cancel();crossbowTrigger.Cancel();ClearPending();nock.Clear();nextNock=0;reload={};reportedBowTier=0;}

    bool Pending()const{return pendingPlayer&&pendingThread==GetCurrentThreadId();}
    bool OwnProjectile(PSProjectile* wrapper){
        if(!Pending()||!Readable(wrapper,sizeof(*wrapper)))return false;
        auto* projectile=static_cast<gCProjectile_PS*>(wrapper->m_pEngineEntityPropertySet);
        if(!Readable(projectile,sizeof(*projectile)))return false;auto* arrow=projectile->GetEntity();if(!Ammo(arrow,pendingAmmo))return false;
        auto* interaction=static_cast<gCInteraction_PS*>(arrow->GetPropertySet(eEPropertySetType_Interaction));
        return Readable(interaction,sizeof(*interaction))&&interaction->GetOwner().GetEntity()==pendingPlayer;
    }
    bool Prepare(PSProjectile* wrapper){
        if(!Pending()||!Readable(wrapper,sizeof(*wrapper)))return false;
        auto* projectile=static_cast<gCProjectile_PS*>(wrapper->m_pEngineEntityPropertySet);
        if(!Readable(projectile,sizeof(*projectile)))return false;auto* arrow=projectile->GetEntity();
        if(!Ammo(arrow,pendingAmmo)||projectile->IsFlying())return false;
        auto* interaction=static_cast<gCInteraction_PS*>(arrow->GetPropertySet(eEPropertySetType_Interaction));
        if(!Readable(interaction,sizeof(*interaction))||interaction->GetOwner().GetEntity()!=pendingPlayer)return false;
        if(launched)return false;auto up=arms::Vec{0,1,0};if(std::fabs(arms::Dot(pending.direction,up))>.98f)up={1,0,0};
        auto pose=bow::NockedArrow(pending.origin,pending.direction,up,arrow->GetLocalNodeBoundary().m_Min.m_fZ);bCMatrix native;memcpy(&native,&pose,sizeof(native));arrow->SetToWorldMatrix(native);
        projectile->AccessTargetEntity().SetEntity(static_cast<eCEntity const*>(nullptr));projectile->SetTargetDirection(bCVector(pending.direction.x,pending.direction.y,pending.direction.z));
        projectile->SetPathStyle(bTPropertyContainer<gEProjectilePath>(gEProjectilePath_Physics));
        // A zero/stale native bow charge is precisely the first-shot bug. Only
        // crossbows depend on the native velocity already assigned by Shoot.
        float speed=projectile->GetShootVelocity();if(pendingAmmo==gEUseType_Bolt&&(!std::isfinite(speed)||speed<=0))return false;
        // Native Shoot copies charge from the previously held ammo into the new arrow.
        // Initialize EVERY bow shot, including the first, from this release's
        // physical charge. Never multiply stale charge or compress the curve
        // with sqrt. Crossbow velocity/damage stay completely native.
        if(pendingAmmo==gEUseType_Arrow){
            auto* damage=static_cast<gCDamage_PS*>(arrow->GetPropertySet(eEPropertySetType_Damage));
            if(!Readable(damage,sizeof(*damage))||!bow::ApplyArrowCharge(*projectile,*damage,bowCharge,pending.power))return false;
        }
        // Held ammo may be hidden/transparent during native combat poses.
        // Launched ammo is a different entity. Normalize its initial visibility
        // once, without retaining it or overriding native impacts/decay.
        bool visible=arrow->IsRenderingEnabled()!=GEFalse;float alpha=arrow->GetRenderAlphaValue();
        if(!ranged::RevealLaunch<ranged::EntityVisualAccess>(arrow,true,false))return false;
        char text[192];sprintf_s(text,"vr_ranged_projectile_visible type=%d native_visible=%d native_alpha=%.3f visible=1 alpha=1 native_lifetime=1",int(pendingAmmo),visible,alpha);Report(text);
        ++launched;return true;
    }
    void Update(bow::Sample const& sample,eCEntity* player,eCEntity* weapon,bool allowed,uint64_t now,int const* settings,double scaledSeconds,bool paused){
        operation="physical_ranged_update";
        if(!player){Abandon();return;}
        reload.Advance(scaledSeconds,paused);
        allowed=allowed&&ready&&!fault&&weapon&&sample.weapon==reinterpret_cast<uintptr_t>(weapon)&&sample.player==reinterpret_cast<uintptr_t>(player);
        if(!allowed){Cancel();return;}
        auto expected=Ammunition(weapon);
        if(sample.crossbow!=(expected==gEUseType_Bolt)){Cancel();return;}
        if(sample.crossbow){
            draw.Cancel();
            if(now>=nextNock){nextNock=now+250;if(!Nock(player,expected)&&now>=nextLog){nextLog=now+1000;Report("vr_crossbow_no_ammunition_or_nock_refused");}}
            operation="crossbow_reload_profile";double speed=0;int tier=0;bool profile=ReloadProfile(player,now,speed,tier);
            auto shot=crossbowTrigger.Step(sample,profile&&allowed&&sample.nativeNock,now,reload.Ready());
            if(shot.fire&&Nock(player,expected)&&Fire(player,shot)){
                reload.Commit(reloadDuration,speed);char text[180];sprintf_s(text,"vr_crossbow_reload tier=%d native_speed=%.3f duration_s=%.6f ready_in_s=%.6f",tier,speed,reloadDuration,reload.Remaining());Report(text);
            }
            return;
        }
        crossbowTrigger.Cancel();
        // Prepare the next native held arrow without drawing/consuming it.
        // Reacquire after the native Shoot script destroys the previous entity.
        if(!draw.pulling&&now>=nextNock){nextNock=now+250;if(!Nock(player,expected)&&now>=nextLog){nextLog=now+1000;Report("vr_bow_no_ammunition_or_nock_refused");}}
        operation="bow_skill_profile";Entity hero(player);int tier=hero.Inventory.IsSkillActive(Template("Perk_Bow_3"))?3:hero.Inventory.IsSkillActive(Template("Perk_Bow_2"))?2:1;
        double fullCharge=bow::ChargeSeconds(tier,bowCharge.seconds);
        if(reportedBowTier!=tier){reportedBowTier=tier;char text[192];sprintf_s(text,"vr_bow_progression tier=%d full_charge_s=%.3f VR_timed_draw_speed=%.2f physical_only=%d native_charge_curve=1",tier,fullCharge,bow::DrawSpeed(tier),tier>=3);Report(text);}
        bool was=draw.pulling;auto shot=draw.Step(sample,allowed&&!paused,now,float(settings[3])*.01f,float(settings[4])*.01f,fullCharge,scaledSeconds,tier>=3);
        if(!was&&draw.pulling&&!Nock(player,expected)){draw.Cancel();Report("vr_bow_no_ammunition_or_nock_refused");}
        if(shot.fire)Fire(player,shot);
        else if(was&&!draw.pulling)nock.Clear();
        if(draw.pulling&&now>=nextLog){nextLog=now+400;char text[192];sprintf_s(text,"vr_bow_draw cm=%.1f power=%.2f velocity=%.2f tier=%d",draw.drawMeters*100,draw.power,bowCharge.Velocity(draw.power),tier);Report(text);}
    }
};
}

