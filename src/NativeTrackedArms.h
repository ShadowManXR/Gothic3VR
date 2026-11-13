#pragma once
#include "RuntimeProbePolicy.h"
#include <g3sdk/Script.h>
#include "TrackedPalette.h"
#include "FrozenArmPalette.h"
#include "ArmTwist.h"
#include "NaturalWrist.h"
#include "MeleeHandling.h"
#include "TrackedBodyMath.h"
#include "BodyPosture.h"
#include "PhysicalBodyCrouch.h"
#include "VirtualHook.h"
#include "PhysicalMelee.h"
#include "PhysicalCrossbow.h"
#include "CrossbowHandling.h"
#include "BowAimFilter.h"
#include "BowStringGuidance.h"
#include "NativeRecovery.h"
#include "StaffMelee.h"
#include "DualWield.h"
#include "TwoHandWeight.h"
#include "HeldItemPose.h"
#include "CastingGrip.h"
#include "ShieldPose.h"
#include "LockedMeleeAttachment.h"
#include "SupportGripToggle.h"
#include "NativeRangedVisual.h"
#include "HeldWeaponVisuals.h"
#include "SummonedSwordPose.h"
#include <cstdio>

namespace diagnostic {
static_assert(gEUseType_1H==2&&gEUseType_2H==3&&gEUseType_Staff==12&&gEUseType_Halberd==51&&gEUseType_Axe==52,"Native weapon classification must match the SDK");
class NativeTrackedArms {
    using Mat=arms::Mat;using Vec=arms::Vec;
    struct RigArm {int upper=-1,lower=-1,hand=-1,index=-1,pinky=-1;Mat calibration;Vec palmLocal;bool calibrated=false;int stringTips[2]{-1,-1};Vec stringEnds[2];bool stringReady=false;};
    struct Weapon {eCEntity* entity=nullptr;Mat local,heldPose,framePose;melee::OneHandPose tracked;bool held=false,framePrepared=false,changed=false,visibilityChanged=false,visible=false,summoned=false;ranged::VisualScope<ranged::EntityVisualAccess> visual;};
    eCVisualAnimationFactory* factory=nullptr;eCWrapper_emfx2Actor* wrapper=nullptr;EMotionFX::Actor* actor=nullptr;
    EMotionFX::Node* nodes=nullptr;int count=0;RigArm rig[2];Mat landmarkBinds[2][3];bool landmarkBindsReady=false;arms::ArmTwistChain armTwist[2];arms::BoneTag tags[TrackedPalette::Limit];
    melee::MeleeHandling handling[2];Mat handlingAnchor,handlingTracking[2];bool handlingAnchorReady=false,handlingApplied[2]{};int handlingSettings[8]{20,15,30,20,25,15,30,20};
    melee::TwoHandWeight oneHandWeight;Mat weightedControllerGrip;bool weightedGripReady=false,heavyTwoHand=true;
    Mat lowerHudPose[2];bool lowerHudReady[2]{};
    Mat renderHandDelta[2];bool renderHandReady[2]{};
    Mat bind[TrackedPalette::Limit],grip[2],controllerGrip[2];bool ready[2]{};Weapon weapons[2];melee::LockedMeleeAttachment meleeAttachments[2];
    arms::ShieldContact shieldContact;int shieldSettings[6]{0,0,0,0,0,0};
    arms::CastingContact casting[2];Mat castFinger[TrackedPalette::Limit];bool castFingerReady[TrackedPalette::Limit]{};
    arms::BodyPart bodyParts[TrackedPalette::Limit]{};int pelvis=-1;bool bodyRig=false,stableBody=false;Rotation3 bodyHeading;Vec bodyRenderCorrection{};
    BodyPosture bodyPosture;Mat crouchLower[TrackedPalette::Limit];bool crouchLowerReady=false;eCEntity* boundPlayer=nullptr;
    PhysicalBodyCrouch physicalCrouch;arms::CrouchLeg crouchLegs[2];bool crouchRig=false;
    bool firstPersonBody=false,handsOnlySelected=false,paladinRig=false;
    eCEntity* player=nullptr;TrackedPalette palette;FrozenArmPalette frozen;void (__cdecl* log)(char const*)=nullptr;
    uintptr_t reportedSummonedWait[2]{},reportedSummonedReady[2]{};
    float units=100;uint64_t nextLog=0,nextWaitLog=0,applied=0,restored=0;bool scope=false,worldMismatchReported=false;eCEntity* reportedWeapon[2]{};
    melee::Sample combatSamples[2];ControllerFrame combatControllers;HeadPose combatHead;bool combatBoundsReported[2]{};eCEntity* combatReportedWeapon[2]{};
    int handCalibration[2][6]{},magicGripOffsets[3]{4,2,0};bool supportedCrossbow=false,toggleSupportGrip=false,crossbowGripHeld=false;SupportGripToggle crossbowGripToggle;crossbow::SupportedGuidance crossbowGuidance;
    int swordAngles[3]{},bowAngles[3]{},bowGripOffsets[3]{5,2,0},crossbowAngles[3]{},crossbowGripOffsets[3]{4,2,0};bool swordPose=false;bow::Sample bowSample;bool drawingBow=false;
    Vec bowNockLocal;uintptr_t bowGeometryWeapon=0;Mat bowArrow;bool bowArrowReady=false;
    bow::AimFilter bowAimFilter;bow::StringGuidance bowStringGuidance;int bowSmoothing=80,bowStringInfluence=20;crossbow::FrameSupport crossbowSupport;Mat crossbowMesh;Vec crossbowContact;bool crossbowGeometry=false;
    melee::TwoHandGrip twoHandGrip;melee::SupportAttachment supportAttachment;uintptr_t supportWeapon=0;int twoHandAngles[3]{};float supportRadius=.12f,gripLossDistance=.40f,staffLossDistance=.40f,crossbowLossDistance=.40f;bool twoHandEnabled=true,supportAllowed=false;
    melee::CoordinatedGrip sharedWorldGrip,sharedTrackingGrip;Orientation sharedCombatOrientation;bool sharedCombatValid=false;
    int staffAngles[3]{};float staffRadius=.15f;int staffGripForward=4,staffGripDepth=2;uintptr_t staffAnchorWeapon=0,offStaffAnchorWeapon=0;Vec staffAnchor{},offStaffAnchor{};
    static Mat Raw(bCMatrix const& m){Mat result;static_assert(sizeof(Mat)==sizeof(bCMatrix));memcpy(&result,&m,sizeof(result));return result;}
    static void Native(Mat const& m,bCMatrix& out){memcpy(&out,&m,sizeof(m));}
    void Report(char const* text){if(log)log(text);}
    bool BuildRig(){
        landmarkBindsReady=false;paladinRig=false;crouchLegs[0]=crouchLegs[1]={};crouchRig=false;
        rig[0]=RigArm{};rig[1]=RigArm{};armTwist[0].Reset();armTwist[1].Reset();pelvis=-1;bodyRig=true;
        casting[0].Reset();casting[1].Reset();memset(castFingerReady,0,sizeof(castFingerReady));
        int fingers[2][5][3];for(auto& side:fingers)for(auto& finger:side)for(auto& index:finger)index=-1;
        for(int i=0;i<count;++i){
            bCString name=wrapper->GetName(nodes+i);tags[i]=arms::Identify(name.GetText());
            if(!strcmp(name.GetText(),"Paladin")||!strcmp(name.GetText(),"KingGuard"))paladinRig=true;
            bodyParts[i]=arms::IdentifyBody(name.GetText());if(!strcmp(name.GetText(),"Hero_Spine_Spine_1"))pelvis=i;
            for(unsigned h=0;h<2;++h){auto& leg=crouchLegs[h];char prefix[48];sprintf_s(prefix,"Hero_%s_",h?"Right":"Left");auto n=name.GetText();auto length=strlen(prefix);
                if(!strncmp(n,prefix,length)){auto suffix=n+length;if(!strcmp(suffix,"Leg_Hip_1"))leg.root=i;else if(!strcmp(suffix,"Leg_Leg_1"))leg.upper=i;else if(!strcmp(suffix,"Leg_Leg_2"))leg.lower=i;else if(!strcmp(suffix,"Foot_Foot_1"))leg.foot=i;else if(!strcmp(suffix,"Foot_Toe_1"))leg.toe=i;}}
            char text[220];sprintf_s(text,"vr_rig_bone index=%d name=%.100s side=%d part=%d",i,name.GetText(),tags[i].side,int(tags[i].part));Report(text);
            // Read-only layout verified in the hash-guarded engine's skinning function.
            Mat inverseBind;memcpy(&inverseBind,reinterpret_cast<unsigned char*>(nodes+i)+0x84,sizeof(Mat));
            bool required=tags[i].index||tags[i].pinky||tags[i].joint||tags[i].finger>=0;
            if((required||bodyParts[i]==arms::BodyPart::Neutral)&&!arms::Inverse(inverseBind,bind[i])){
                if(required){sprintf_s(text,"vr_rig_inverse_bind_refused index=%d diagonal=%.3f,%.3f,%.3f,%.3f",i,inverseBind.m[0],inverseBind.m[5],inverseBind.m[10],inverseBind.m[15]);Report(text);return false;}
                bodyRig=false;
            }
            auto const& tag=tags[i];if(tag.side<0)continue;auto& arm=rig[tag.side];
            if(tag.joint){auto* joint=tag.part==arms::Upper?&arm.upper:tag.part==arms::Lower?&arm.lower:&arm.hand;if(*joint>=0)return false;*joint=i;}
            if(tag.index)arm.index=i;if(tag.pinky)arm.pinky=i;
            if(tag.finger>=0&&tag.segment>=0)fingers[tag.side][tag.finger][tag.segment]=i;
        }
        bodyRig=bodyRig&&pelvis>=0;
        crouchRig=bodyRig;for(auto const& leg:crouchLegs)for(int i:{leg.root,leg.upper,leg.lower,leg.foot,leg.toe})crouchRig&=i>=0;
        if(bodyRig){char text[192];auto p=arms::Position(bind[pelvis]);sprintf_s(text,"vr_body_rig pelvis=%d bind=%.1f,%.1f,%.1f; neutral_torso_animated_legs",pelvis,p.x,p.y,p.z);Report(text);}
        for(unsigned h=0;h<2;++h){auto& arm=rig[h];if(arm.upper<0||arm.lower<0||arm.hand<0)continue;
            if(arm.index<0||arm.pinky<0){Report("vr_rig_missing_finger_landmarks; hand_override_refused");continue;}
            Vec wrist=arms::Position(bind[arm.hand]);
            Vec index=arms::Position(bind[arm.index]),pinky=arms::Position(bind[arm.pinky]);char calibration[192];sprintf_s(calibration,"vr_rig_bind side=%u wrist=%.1f,%.1f,%.1f index=%.1f,%.1f,%.1f pinky=%.1f,%.1f,%.1f",h,wrist.x,wrist.y,wrist.z,index.x,index.y,index.z,pinky.x,pinky.y,pinky.z);Report(calibration);
            if(!arms::GripCalibration(bind[arm.hand],index,pinky,arm.calibration)||!bow::PalmLocal(bind[arm.hand],index,pinky,arm.palmLocal))continue;
            arm.calibrated=true;Report("vr_rig_orientation knuckle_plane; fingers=grip_minus_y index_side=genome_plus_z");
            if(h==1){
                arm.stringReady=true;
                for(unsigned f=0;f<2;++f){auto const& chain=fingers[h][f];
                    if(chain[1]<0||chain[2]<0||!bow::StringFingerEnd(bind[chain[1]],bind[chain[2]],arm.stringEnds[f])){arm.stringReady=false;break;}
                    arm.stringTips[f]=chain[2];
                }
            }
            Mat inverseHand,inverseCalibration;if(!arms::Inverse(bind[arm.hand],inverseHand)||!arms::Inverse(arm.calibration,inverseCalibration))continue;
            Vec palmDirection=arms::Vector(inverseCalibration,{h?-1.f:1.f,0,0});
            for(unsigned f=0;f<5;++f){auto const& chain=fingers[h][f];if(chain[0]<0||chain[1]<0||chain[2]<0)continue;
                Mat local[3],closed[3];for(unsigned segment=0;segment<3;++segment)local[segment]=arms::Multiply(bind[chain[segment]],inverseHand);
                if(!arms::CastingFinger(local,palmDirection,arm.palmLocal,f==0,closed))continue;
                for(unsigned segment=0;segment<3;++segment){castFinger[chain[segment]]=closed[segment];castFingerReady[chain[segment]]=true;}
            }
        }
        for(unsigned h=0;h<2;++h){int indices[3]{rig[h].hand,rig[h].index,rig[h].pinky};
            for(unsigned k=0;k<3;++k){if(indices[k]>=0)memcpy(&landmarkBinds[h][k],reinterpret_cast<unsigned char*>(nodes+indices[k])+0x84,sizeof(Mat));}}
        landmarkBindsReady=true;
        char text[220];sprintf_s(text,"vr_rig_ready bones=%d left=%d,%d,%d calibrated=%d right=%d,%d,%d calibrated=%d",count,rig[0].upper,rig[0].lower,rig[0].hand,rig[0].calibrated,rig[1].upper,rig[1].lower,rig[1].hand,rig[1].calibrated);Report(text);return true;
    }
    bool ApplyHandling(unsigned hand,eCEntity* item,melee::HandlingProfile profile,Mat& pose,Mat* mesh=nullptr,Mat* assembly=nullptr){
        unsigned offset=unsigned(profile)*2;
        if(hand>1||!handlingAnchorReady||(!handlingSettings[offset]&&!handlingSettings[offset+1])){if(hand<2)handling[hand].Reset();return false;}
        Mat tracking,filtered,weighted,delta;
        if(!melee::HandlingToTracking(pose,handlingAnchor,units,tracking)||
           !handling[hand].Step(reinterpret_cast<uintptr_t>(item),combatHead.session,combatControllers.timeMs,profile,tracking,handlingSettings[offset],handlingSettings[offset+1],filtered)||
           !melee::HandlingToWorld(filtered,handlingAnchor,units,weighted)||!melee::HandlingAssembly(pose,weighted,delta))return false;
        pose=weighted;if(mesh)*mesh=arms::Multiply(*mesh,delta);if(assembly)*assembly=delta;
        handlingTracking[hand]=filtered;handlingApplied[hand]=true;return true;
    }
    bool ValidateRig(){return Readable(factory,sizeof(*factory))&&factory->GetAssembledActor()==wrapper&&Readable(wrapper,sizeof(*wrapper))&&wrapper->GetInstance()==actor&&Readable(actor,sizeof(*actor))&&actor->mNodes.mData==nodes&&actor->mNodes.mLength==count&&count>0&&count<=int(TrackedPalette::Limit)&&Readable(nodes,count*sizeof(*nodes));}
    bool SameLandmarkBinds()const{
        if(!landmarkBindsReady)return false;
        for(unsigned h=0;h<2;++h){int indices[3]{rig[h].hand,rig[h].index,rig[h].pinky};
            for(unsigned k=0;k<3;++k)if(indices[k]>=0&&(indices[k]>=count||memcmp(&landmarkBinds[h][k],reinterpret_cast<unsigned char*>(nodes+indices[k])+0x84,sizeof(Mat))))return false;}
        return true;
    }
    bool BindRig(eCEntity* entity,void (__cdecl* logger)(char const*)){
        if(fault||!Readable(entity,sizeof(eCDynamicEntity)))return false;log=logger;
        auto* animation=static_cast<eCVisualAnimation_PS*>(entity->GetPropertySet(eEPropertySetType_Animation));
        if(!Readable(animation,sizeof(*animation))||animation->GetRagDollEnabled())return false;
        auto* newFactory=&animation->GetAnimationFactory();auto* newWrapper=newFactory->GetAssembledActor();
        if(!Readable(newWrapper,sizeof(*newWrapper)))return false;auto* newActor=newWrapper->GetInstance();if(!Readable(newActor,sizeof(*newActor)))return false;
        if(boundPlayer!=entity||newFactory!=factory||newWrapper!=wrapper||newActor!=actor||newActor->mNodes.mData!=nodes||newActor->mNodes.mLength!=count||!ValidateRig()||!SameLandmarkBinds()){
            bool samePlayer=boundPlayer==entity&&landmarkBindsReady;
            auto oldLeft=rig[0],oldRight=rig[1];auto oldPrimary=meleeAttachments[0],oldSecondary=meleeAttachments[1];
            auto oldGrip=twoHandGrip;auto oldAttachment=supportAttachment;auto oldWorld=sharedWorldGrip,oldTracking=sharedTrackingGrip;auto oldSupportWeapon=supportWeapon;
            auto oldCrossbow=crossbowSupport;auto oldToggle=crossbowGripToggle;bool oldCrossbowHeld=crossbowGripHeld;
            ResetCombatSample();bodyPosture.Reset();physicalCrouch.Reset();crouchLowerReady=false;boundPlayer=entity;
            factory=newFactory;wrapper=newWrapper;actor=newActor;nodes=actor->mNodes.mData;count=actor->mNodes.mLength;
            if(!ValidateRig()||!BuildRig()){factory=nullptr;fault=true;Report("vr_rig_validation_refused; native_arms_preserved");return false;}
            // Armor can rebuild the actor while leaving the same held item.
            // Convert its locked binding; never recapture a transient animation.
            if(samePlayer){
                if(oldRight.calibrated&&rig[1].calibrated&&oldPrimary.RebindHand(oldRight.calibration,oldRight.palmLocal,rig[1].calibration,rig[1].palmLocal))meleeAttachments[0]=oldPrimary;
                if(oldLeft.calibrated&&rig[0].calibrated&&oldSecondary.RebindHand(oldLeft.calibration,oldLeft.palmLocal,rig[0].calibration,rig[0].palmLocal))meleeAttachments[1]=oldSecondary;
                if(oldLeft.calibrated&&rig[0].calibrated){
                    if(oldGrip.holding&&oldSupportWeapon&&oldAttachment.RebindCalibration(oldLeft.calibration,rig[0].calibration)){
                        twoHandGrip=oldGrip;supportAttachment=oldAttachment;supportWeapon=oldSupportWeapon;sharedWorldGrip=oldWorld;sharedTrackingGrip=oldTracking;
                    }
                    if(oldCrossbowHeld&&oldCrossbow.RebindCalibration(oldLeft.calibration,rig[0].calibration)){
                        crossbowSupport=oldCrossbow;crossbowGripToggle=oldToggle;crossbowGripHeld=oldCrossbowHeld;
                    }
                }
            }
        }
        return ValidateRig();
    }
    void MoveWeapon(Weapon& saved,eCEntity* item,Mat const& worldPose,bool show=false){
        if(!saved.changed){saved.entity=item;saved.local=Raw(item->GetLocalMatrix());saved.changed=true;}
        if(show){if(!saved.visibilityChanged){saved.visible=item->IsRenderingEnabled()!=GEFalse;saved.visibilityChanged=true;}
            if(saved.summoned){if(!saved.visual.Active()&&!saved.visual.Capture(item))item->EnableRendering(GETrue);else saved.visual.Show();}
            else item->EnableRendering(GETrue);
        }
        bCMatrix native;Native(worldPose,native);item->SetToWorldMatrix(native);
    }
    void RestoreWeapons(bool keepPrepared=false){for(auto& weapon:weapons){
        if(keepPrepared&&(weapon.tracked.valid||weapon.held||weapon.framePrepared))continue;
        if(!Readable(weapon.entity,sizeof(eCEntity))){weapon.visual.Abandon();weapon.changed=weapon.visibilityChanged=false;continue;}
        if(weapon.changed){weapon.changed=false;bCMatrix matrix;Native(weapon.local,matrix);weapon.entity->SetLocalMatrix(matrix);}
        if(weapon.visibilityChanged){weapon.visibilityChanged=false;weapon.entity->EnableRendering(weapon.visible?GETrue:GEFalse);}
        weapon.visual.Restore(true);
    }}
public:
    void ResetBodyPosture(){bodyPosture.Reset();physicalCrouch.Reset();crouchLowerReady=false;}
    bool usePalettePose=false;
    void SetBodyVisibility(bool firstPerson,bool handsOnly){firstPersonBody=firstPerson;handsOnlySelected=handsOnly;}
    bool PhysicalHandsOnly()const{return FirstPersonHandsOnly(firstPersonBody,handsOnlySelected,physicalCrouch.HandsOnly());}
    void PreparePhysicalCrouch(eCEntity* entity,bool enabled,HeadPose const& head,float standingY,uint64_t now){
        bool tracking=head.valid&&head.runtimeConnected&&head.deviceConnected&&(head.trackingResult&8);
        bool wasHidden=physicalCrouch.HandsOnly();
        physicalCrouch.Update(reinterpret_cast<uintptr_t>(entity),head.session,now,enabled&&tracking&&entity==boundPlayer&&bodyRig&&crouchRig,
            standingY,head.position[1],units>0?std::max(0.f,-bodyPosture.NativeHeight()/units):0.f);
        if(wasHidden!=physicalCrouch.HandsOnly())Report(physicalCrouch.HandsOnly()?"vr_physical_crouch_hands_only_enter; render_scope; tracking_origin_unchanged":"vr_physical_crouch_body_visible; render_scope_restored");
    }
    void PrepareBodyPosture(eCEntity* entity,bool enabled,uint64_t now,float unitScale,void (__cdecl* logger)(char const*)){
        if(!enabled||diagnostic::NativeRecovery(entity)||!BindRig(entity,logger)||!bodyRig){bodyPosture.Reset();crouchLowerReady=false;return;}
        auto* routine=static_cast<gCScriptRoutine_PS*>(entity->GetPropertySet(eEPropertySetType_ScriptRoutine));
        if(!Readable(routine,sizeof(*routine))){bodyPosture.Reset();crouchLowerReady=false;return;}
        int action=routine->GetAction().GetValue(),animation=routine->GetAniState().GetValue();
        auto const* task=routine->GetCurrentTask().GetText();auto const* state=routine->GetCurrentState().GetText();
        bool sneak=NativeSneakState(action,animation,state),pickup=PickupLowerBody(action,task,state);
        auto model=Raw(wrapper->GetWorldMatrix(nodes+pelvis)),entityWorld=Raw(entity->GetWorldMatrix());
        if(!arms::Affine(model)||!arms::Affine(entityWorld)){ResetBodyPosture();return;}
        float drop=arms::Vector(entityWorld,{0,arms::Position(model).y-arms::Position(bind[pelvis]).y,0}).y;
        bodyPosture.Update(reinterpret_cast<uintptr_t>(entity),reinterpret_cast<uintptr_t>(factory),now,unitScale,sneak,pickup,drop);
        if(!bodyPosture.Crouched())crouchLowerReady=false;
    }
    float PostureCameraHeight()const{return bodyPosture.CameraHeight();}
    float PostureNativeHeight()const{return bodyPosture.NativeHeight();}
    void RenderHandDeltas(Mat (&out)[2],bool (&available)[2])const{for(unsigned h=0;h<2;++h){out[h]=renderHandDelta[h];available[h]=scope&&!fault&&renderHandReady[h];}}
    bool LowerArmHud(unsigned side,Mat& pose,Mat& lowerBind,Mat& handBind,Mat& calibration)const{
        if(side>1||!scope||fault||!ready[side]||!lowerHudReady[side])return false;
        pose=lowerHudPose[side];lowerBind=bind[rig[side].lower];handBind=bind[rig[side].hand];calibration=rig[side].calibration;return true;
    }
    bool LeftSupportHeld()const{return twoHandGrip.holding||supportedCrossbow;}
    bool fault=false;
    melee::Sample CombatSample(unsigned hand=1)const{return hand<2?combatSamples[hand]:melee::Sample{};}
    void DiscardFrameSamples(){combatSamples[0]=combatSamples[1]={};bowSample={};bowArrowReady=false;crossbowGeometry=false;}
    void ResetCombatSample(){handling[0].Reset();handling[1].Reset();armTwist[0].Reset();armTwist[1].Reset();for(auto& attachment:meleeAttachments)attachment.Reset();shieldContact.Reset();oneHandWeight.Reset();weightedGripReady=false;supportedCrossbow=false;crossbowGripHeld=false;crossbowGripToggle.Reset();crossbowGuidance.Reset();crossbowSupport.Reset();crossbowGeometry=false;staffAnchorWeapon=offStaffAnchorWeapon=0;combatSamples[0]=combatSamples[1]={};bowSample={};bowAimFilter.Reset();bowStringGuidance.Reset();twoHandGrip.Reset();supportAttachment.Reset();supportWeapon=0;sharedWorldGrip.Reset();sharedTrackingGrip.Reset();sharedCombatValid=false;}
    bow::Sample BowSample()const{return bowSample;}
    void SetBowGeometry(Vec rest,Mat const& arrow,Vec drawn){bowNockLocal=rest;bowGeometryWeapon=bowSample.weapon;bowSample.nockLocal=rest;bowSample.launchNock=drawn;bowSample.launchDirection=arms::Vector(arrow,{0,0,1});bowSample.nativeNock=true;bowArrow=arrow;bowArrowReady=true;}
    void SetCrossbowSettings(int const* settings,int lossDistanceCm=40){crossbowLossDistance=std::clamp(float(lossDistanceCm)*.01f,.10f,1.f);memcpy(crossbowAngles,settings,sizeof(crossbowAngles));memcpy(crossbowGripOffsets,settings+3,sizeof(crossbowGripOffsets));}
    void SetCrossbowGeometry(Mat const& mesh,Vec contact){
        crossbowMesh=mesh;crossbowContact=contact;crossbowGeometry=true;
        if(bowSample.valid&&bowSample.crossbow&&ready[0])crossbowSupport.Prepare(bowSample.weapon,supportAllowed&&crossbowGripHeld,mesh,contact,grip[0],rig[0].calibration,rig[0].palmLocal,units,crossbowLossDistance);
    }
    bool CrossbowFrame()const{return bowSample.valid&&bowSample.crossbow;}
    void SetBowState(bool drawing,int const* angles,int smoothing,int stringInfluence=20){drawingBow=drawing;memcpy(bowAngles,angles,sizeof(bowAngles));if(bowSmoothing!=smoothing)bowAimFilter.Reset();if(bowSmoothing!=smoothing||bowStringInfluence!=stringInfluence)bowStringGuidance.Reset();bowSmoothing=smoothing;bowStringInfluence=std::clamp(stringInfluence,0,40);}
    void SetBowGripOffsets(int const* offsets){memcpy(bowGripOffsets,offsets,sizeof(bowGripOffsets));}
    void SetCombatTracking(ControllerFrame const& controllers,HeadPose const& head){combatControllers=controllers;combatHead=head;}
    void SetShieldSettings(int const* values){if(memcmp(shieldSettings,values,sizeof(shieldSettings)))shieldContact.Reset();memcpy(shieldSettings,values,sizeof(shieldSettings));}
    void SetMagicGripOffsets(int const* settings){memcpy(magicGripOffsets,settings,sizeof(magicGripOffsets));}
    void SetHandCalibration(int const* left,int const* right){memcpy(handCalibration[0],left,sizeof(handCalibration[0]));memcpy(handCalibration[1],right,sizeof(handCalibration[1]));}
    void SetSwordCalibration(int const* angles){memcpy(swordAngles,angles,sizeof(swordAngles));}
    void SetMeleeHandling(int const* values){memcpy(handlingSettings,values,sizeof(handlingSettings));}
    void SetHeavyTwoHand(bool enabled){if(heavyTwoHand!=enabled)oneHandWeight.Reset();heavyTwoHand=enabled;}
    void SetTwoHandSettings(bool enabled,int const* angles,int radius,bool allowed,bool toggle=false,int lossDistanceCm=40){
        if(toggleSupportGrip!=toggle){twoHandGrip.Reset();supportAttachment.Reset();sharedWorldGrip.Reset();sharedTrackingGrip.Reset();crossbowSupport.Reset();crossbowGripToggle.Reset();crossbowGripHeld=false;}
        toggleSupportGrip=toggle;gripLossDistance=std::clamp(float(lossDistanceCm)*.01f,.10f,1.f);twoHandEnabled=enabled;memcpy(twoHandAngles,angles,sizeof(twoHandAngles));supportRadius=std::clamp(float(radius)*.01f,.05f,.25f);supportAllowed=allowed;}
    void SetStaffSettings(int const* angles,int radius,int forward,int depth,int lossDistanceCm=40){staffLossDistance=std::clamp(float(lossDistanceCm)*.01f,.10f,1.f);memcpy(staffAngles,angles,sizeof(staffAngles));staffRadius=std::clamp(float(radius)*.01f,.05f,.25f);staffGripForward=std::clamp(forward,-10,10);staffGripDepth=std::clamp(depth,-10,10);}
    void Begin(eCEntity* entity,ControllerFrame const& controllers,Rotation3 const& native,float const (&base)[3],Orientation center,float const (&origin)[3],float unitScale,float height,bool stabilizeBody,void (__cdecl* logger)(char const*),Vec const& renderCorrection={},Rotation3 const* trackedBodyHeading=nullptr){
        End();lowerHudReady[0]=lowerHudReady[1]=false;renderHandReady[0]=renderHandReady[1]=false;combatSamples[0]=combatSamples[1]={};bowSample={};bowArrowReady=false;crossbowGeometry=false;if(fault||diagnostic::NativeRecovery(entity)){combatSamples[0]=combatSamples[1]={};return;}log=logger;units=unitScale;player=entity;
        if(!BindRig(entity,logger))return;
        handlingApplied[0]=handlingApplied[1]=false;
        ControllerPose trackingOrigin;trackingOrigin.valid=trackingOrigin.tracked=true;trackingOrigin.quaternion[3]=1;
        handlingAnchorReady=arms::GripWorld(trackingOrigin,native,base,center,origin,units,height,handlingAnchor);
        for(unsigned h=0;h<2;++h){ready[h]=rig[h].calibrated&&arms::FreshGrip(controllers,h,GetTickCount64())&&arms::GripWorld(controllers.hands[h].grip,native,base,center,origin,units,height,controllerGrip[h]);
            if(ready[h]){grip[h]=controllerGrip[h];ready[h]=arms::CalibrateGrip(grip[h],h,handCalibration[h],units);}}
        weightedGripReady=false;auto* heavyWeapon=gCEntity::GetWeapon(player,GETrue);
        if(heavyTwoHand&&ready[1]&&combatHead.valid&&Readable(heavyWeapon,sizeof(eCEntity))&&WeaponType(gCEntity::GetUseType(heavyWeapon))==WeaponKind::TwoHand){
            auto weighted=controllers.hands[1].grip;
            if(oneHandWeight.Step(reinterpret_cast<uintptr_t>(heavyWeapon),combatHead.session,controllers.timeMs,weighted,twoHandGrip.holding))
                weightedGripReady=arms::GripWorld(weighted,native,base,center,origin,units,height,weightedControllerGrip);
        }else oneHandWeight.Reset();
        Mat nativeWorld=Raw(player->GetWorldMatrix());
        for(unsigned slot=0;slot<2;++slot){auto& weapon=weapons[slot];weapon={};unsigned h=slot==0?1:0;
            auto* item=gCEntity::GetWeapon(player,slot==0);
            if(item==player||!Readable(item,sizeof(eCEntity))||!PhysicalMeleeType(gCEntity::GetUseType(item))||(slot==1&&item==heavyWeapon))meleeAttachments[slot].Reset();
            if(ready[h]&&item!=player&&Readable(item,sizeof(eCEntity))&&gCEntity::GetUseType(item)==gEUseType_Cast){
                auto wrist=arms::Multiply(Raw(wrapper->GetWorldMatrix(nodes+rig[h].hand)),nativeWorld);
                auto const& bounds=item->GetLocalNodeBoundary();Mat target=arms::Multiply(rig[h].calibration,grip[h]);arms::Position(target,arms::WristAtPalm(grip[h],rig[h].calibration,rig[h].palmLocal));
                weapon.held=casting[h].Prepare(reinterpret_cast<uintptr_t>(item),Raw(item->GetWorldMatrix()),wrist,
                    {bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ})&&
                    casting[h].Pose(target,rig[h].palmLocal,weapon.heldPose,arms::Vector(grip[h],bow::GripContactOffset(magicGripOffsets,units,h)));
                if(weapon.held)weapon.entity=item;continue;
            }
            casting[h].Reset();
            // Melee calibration is captured after the forced native palette
            // update, then reused across frames instead of reading idle poses.
        }
        bowSample={};bowArrowReady=false;crossbowGeometry=false;
        if(combatHead.valid){
            for(unsigned slot=0;slot<2;++slot){auto* item=gCEntity::GetWeapon(player,slot==0);
                if(item==player||!Readable(item,sizeof(eCEntity)))continue;
                auto type=gCEntity::GetUseType(item);if(type!=gEUseType_Bow&&type!=gEUseType_CrossBow)continue;
                bowSample.crossbow=type==gEUseType_CrossBow;unsigned aimHand=bowSample.crossbow?1:0;bowSample.aimHand=aimHand;if(!ready[aimHand])continue;
                bowSample.player=reinterpret_cast<uintptr_t>(player);bowSample.weapon=reinterpret_cast<uintptr_t>(item);
                bowSample.time=controllers.timeMs;bowSample.session=combatHead.session;bowSample.units=units;bowSample.rightTracked=ready[1];if(ready[1])bowSample.right=grip[1];
                auto aimControllers=controllers;
                crossbowGripHeld=crossbowGripToggle.Step(bowSample.weapon,combatHead.session,controllers.timeMs,
                    bowSample.crossbow&&supportAllowed&&ready[0]&&ready[1],toggleSupportGrip,controllers.hands[0].squeeze);
                bool supported=bowSample.crossbow&&crossbowSupport.Holding(bowSample.weapon)&&crossbowGripHeld;
                if(!supported){crossbowSupport.Reset();crossbowGuidance.Reset();}
                if(supported!=supportedCrossbow){bowAimFilter.Reset();supportedCrossbow=supported;}
                if(supported){Mat tracking[2];
                    if(!melee::TrackingGrips(controllers,tracking)||!arms::CalibrateGrip(tracking[0],0,handCalibration[0],1)||!arms::CalibrateGrip(tracking[1],1,handCalibration[1],1)||
                       !crossbowGuidance.Step(aimControllers.hands[1].aim,tracking[0],tracking[1],bowSample.weapon,combatHead.session,controllers.timeMs)){crossbowSupport.Reset();supportedCrossbow=false;bowAimFilter.Reset();break;}
                }
                if(!bowAimFilter.Apply(aimControllers.hands[aimHand].aim,controllers.timeMs,combatHead.session,bowSample.weapon,bowSmoothing))break;
                if(bowSample.crossbow&&!supported&&!crossbow::OneHandSway(aimControllers.hands[1].aim,controllers.timeMs))break;
                int supportCalibration[6]{};
                if(!bow::TrackedAim(aimControllers,native,base,center,origin,units,height,supported?supportCalibration:handCalibration[aimHand],bowSample.crossbow?crossbowAngles:bowAngles,bowSample.left,bowSample.leftAim,aimHand))break;
                for(unsigned hand=0;hand<2;++hand){auto const& p=controllers.hands[hand].grip;auto& v=hand?bowSample.trackingRight:bowSample.trackingLeft;v={p.position[0],p.position[1],p.position[2]};}
                if(bowGeometryWeapon==bowSample.weapon)bowSample.nockLocal=bowNockLocal;
                bowSample.trigger=ready[1]&&controllers.hands[1].trigger>.65f;bowSample.valid=true;break;
            }
        }
        if(!bowSample.valid){bowAimFilter.Reset();crossbowGuidance.Reset();}
        if(!bowSample.valid||!bowSample.crossbow){crossbowGripToggle.Reset();crossbowGripHeld=false;}
        crossbowSupport.Begin(bowSample.weapon,bowSample.valid&&bowSample.crossbow&&supportAllowed&&ready[0]&&crossbowGripHeld);
        stableBody=stabilizeBody&&bodyRig&&controllers.focused;bodyHeading=trackedBodyHeading?*trackedBodyHeading:native;bodyRenderCorrection=renderCorrection;
        // Resolve the same IK wrist used by Apply before either eye or bow is
        // rendered. Weapons stay in the visible palm even at the reach limit.
        if(bowSample.valid){Mat entityWorld=Raw(player->GetWorldMatrix()),bodyWorld;bool neutral=stableBody&&arms::BodyWorld(entityWorld,bodyHeading,bodyWorld,bodyRenderCorrection);
            float r=.01745329252f,radians[3]{bowAngles[0]*r,bowAngles[1]*r,bowAngles[2]*r};Mat offset=arms::Rotation(NativeCameraRotation(radians));
            for(unsigned h=0;h<2;++h){if(!ready[h])continue;auto const& arm=rig[h];Mat upper=arms::Multiply(Raw(wrapper->GetWorldMatrix(nodes+arm.upper)),entityWorld),lower=arms::Multiply(Raw(wrapper->GetWorldMatrix(nodes+arm.lower)),entityWorld),hand=arms::Multiply(Raw(wrapper->GetWorldMatrix(nodes+arm.hand)),entityWorld),delta;
                if(neutral&&arms::Rebase(upper,arms::Multiply(bind[arm.upper],bodyWorld),delta)){upper=arms::Multiply(upper,delta);lower=arms::Multiply(lower,delta);hand=arms::Multiply(hand,delta);}
                Mat pose=h?grip[h]:arms::Multiply(offset,grip[h]);arms::Solution solution;
                Vec pole=arms::Vector(neutral?bodyWorld:entityWorld,{h?.6f:-.6f,-1,-.3f});
                if(bowSample.crossbow)pose=grip[h];
                if(!arms::SolveTracked(arms::Position(upper),arms::Position(lower),arms::Position(hand),arms::WristAtPalm(pose,arm.calibration,arm.palmLocal),pole,solution)){if(h)bowSample.rightTracked=false;if(h==bowSample.aimHand)bowSample.valid=false;continue;}
                Mat resolved=h==bowSample.aimHand?bow::ResolvedBowGrip(pose,arm.calibration,solution.wrist,arm.palmLocal,bowSample.crossbow?crossbowGripOffsets:bowGripOffsets,units,h):bow::ResolvedPalm(pose,arm.calibration,solution.wrist,arm.palmLocal);
                if(h)bowSample.right=resolved;
                if(h==bowSample.aimHand){bowSample.left=resolved;arms::Position(bowSample.leftAim,arms::Position(resolved));}
            }
        }
        if(bowSample.valid&&!bowSample.crossbow){
            bowSample.drawAim=bowSample.leftAim;bowSample.drawAimValid=true;
            if(!bowStringGuidance.Apply(bowSample.leftAim,bow::DrawRest(bowSample),arms::Position(bowSample.right),units,drawingBow,bowSample.rightTracked,
                bowSample.time,bowSample.session,bowSample.weapon,bowStringInfluence,bowSmoothing))bowSample.valid=false;
        }else bowStringGuidance.Reset();
        scope=ready[0]||ready[1]||stableBody;
        if(!scope&&GetTickCount64()>=nextWaitLog){nextWaitLog=GetTickCount64()+3000;char text[220];sprintf_s(text,"vr_arms_waiting left_tracked=%d right_tracked=%d left_calibrated=%d right_calibrated=%d age_ms=%llu",controllers.hands[0].grip.tracked,controllers.hands[1].grip.tracked,rig[0].calibrated,rig[1].calibrated,GetTickCount64()-controllers.timeMs);Report(text);}
    }
    bool Matches(eCVisualAnimationFactory const* self)const{return scope&&!fault&&self==factory;}
    bool FrozenFrame()const{return frozen.Matches(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(factory),unsigned(count));}
    void PrepareFrozenFrame(void* original,GEU32 frame,void (*preparePose)(eCVisualAnimationFactory*,bCMatrix const&)=nullptr){
        frozen.Reset();if(!scope||fault||(!ready[0]&&!ready[1]&&!stableBody)||!ValidateRig())return;
        auto* item=gCEntity::GetWeapon(player,GETrue);auto* left=gCEntity::GetWeapon(player,GEFalse);
        bool shield=ready[0]&&Readable(left,sizeof(eCEntity))&&left!=player&&gCEntity::GetUseType(left)==gEUseType_Shield;
        bool primary=ready[1]&&Readable(item,sizeof(eCEntity))&&item!=player&&PhysicalMeleeType(gCEntity::GetUseType(item));
        bool secondary=ready[0]&&Readable(left,sizeof(eCEntity))&&left!=player&&PhysicalMeleeType(gCEntity::GetUseType(left));
        // Unequip can hit the same native palette cache as armed running.
        // Freeze the neutral body even after the last weapon has disappeared.
        if(!FreezeTrackedHero(stableBody,primary,secondary,shield)&&!weapons[0].held&&!weapons[1].held)return;
        auto nativeFrame=factory->m_bLastRenderMatricesUpdateFrame;
        factory->m_bLastRenderMatricesUpdateFrame=frame^1u;
        using Update=GEBool(__thiscall*)(eCVisualAnimationFactory*,GEU32,bCMatrix const&);
        // Build the tracked rig and attached palms once BEFORE either eye's
        // frustum pass, AFTER ranged support geometry is prepared. Native
        // false/cache hits are validated by Apply.
        reinterpret_cast<Update>(original)(factory,frame,player->GetWorldMatrix());
        if(preparePose)preparePose(factory,player->GetWorldMatrix());
        Apply(factory,player->GetWorldMatrix());
        auto& matrices=factory->m_arrRenderMatrices;
        if(palette.address&&matrices.GetCount()==count&&Readable(matrices.AccessArray(),count*sizeof(Mat))&&
           frozen.Capture(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(factory),reinterpret_cast<Mat const*>(matrices.AccessArray()),unsigned(count))){
            for(auto& weapon:weapons)if(weapon.changed&&Readable(weapon.entity,sizeof(eCEntity))){weapon.framePose=Raw(weapon.entity->GetWorldMatrix());weapon.framePrepared=true;}
        }
        RestoreEye();factory->m_bLastRenderMatricesUpdateFrame=nativeFrame;
    }
    void PrepareEye(){if(scope&&!fault)for(auto& weapon:weapons)if((weapon.framePrepared||weapon.tracked.valid||weapon.held)&&Readable(weapon.entity,sizeof(eCEntity)))
        MoveWeapon(weapon,weapon.entity,weapon.framePrepared?weapon.framePose:weapon.held?weapon.heldPose:weapon.tracked.mesh,true);}

    void BeforePalette(){RestorePalette();RestoreWeapons(true);}
    void RestoreEye(){RestorePalette();RestoreWeapons();}
    void RestorePalette(){
        if(palette.address){
            if(!Readable(factory,sizeof(*factory))){palette.Restore(nullptr,0);fault=true;return;}
            auto& current=factory->m_arrRenderMatrices;auto* data=current.AccessArray();auto n=current.GetCount();
            if(n<=0||n>int(TrackedPalette::Limit)||!Readable(data,size_t(n)*sizeof(Mat))||!palette.Restore(data,unsigned(n))){palette.Restore(nullptr,0);fault=true;Report("vr_arms_palette_changed; override_disabled");return;}
            ++restored;
        }
    }
    void Apply(eCVisualAnimationFactory* self,bCMatrix const& worldMatrix){
        if(!Matches(self)||!ValidateRig())return;
        auto& array=factory->m_arrRenderMatrices;auto* data=array.AccessArray();if(array.GetCount()!=count||!Readable(data,count*sizeof(Mat)))return;
        if(FrozenFrame()){
            if(!palette.Begin(data,unsigned(count)))return;
            frozen.Apply(reinterpret_cast<uintptr_t>(player),reinterpret_cast<uintptr_t>(factory),reinterpret_cast<Mat*>(data),unsigned(count));
            for(auto& weapon:weapons)if(weapon.framePrepared&&Readable(weapon.entity,sizeof(eCEntity)))MoveWeapon(weapon,weapon.entity,weapon.framePose,true);
            return;
        }
        Mat entityWorld=Raw(worldMatrix),bone[TrackedPalette::Limit];if(!arms::Affine(entityWorld))return;
        // The palette's final argument must be the player's world matrix, not a
        // camera-relative matrix. Reject a renderer variant with another convention.
        Mat actual=Raw(player->GetWorldMatrix());for(int i=0;i<16;++i)if(std::fabs(actual.m[i]-entityWorld.m[i])>.05f){if(!worldMismatchReported){worldMismatchReported=true;Report("vr_arms_world_argument_mismatch; native_arms_preserved");}return;}
        for(int i=0;i<count;++i){
            // A scoped chest freeze replaces the skin, not the animation wrapper.
            // Derive deltas from the skin pose that will actually be rendered.
            if(usePalettePose)bone[i]=arms::Multiply(bind[i],Raw(data[i]));
            else {bCMatrix model=wrapper->GetWorldMatrix(nodes+i);bone[i]=arms::Multiply(Raw(model),entityWorld);}
            if(!arms::Affine(bone[i]))return;
        }
        for(unsigned slot=0;slot<2;++slot){unsigned h=slot==0?1:0;auto* item=gCEntity::GetWeapon(player,slot==0);
            if(!ready[h]||item==player||!Readable(item,sizeof(eCEntity))||(slot==1&&weapons[0].entity==item))continue;
            int type=gCEntity::GetUseType(item);if(!PhysicalMeleeType(type))continue;
            weapons[slot].summoned=visual::SummonedFlameSword(item->GetName().GetText(),type);
            float initialLimit=weapons[slot].summoned?.75f*units:0.f;
            auto key=reinterpret_cast<uintptr_t>(item);
            if(!meleeAttachments[slot].Capture(key,type,Raw(item->GetWorldMatrix()),bone[rig[h].hand],initialLimit)){
                if(weapons[slot].summoned&&reportedSummonedWait[slot]!=key){reportedSummonedWait[slot]=key;char text[220];
                    sprintf_s(text,"vr_summoned_sword_attachment_wait offset_cm=%.1f; native_spawn_pose_not_locked",arms::Length(arms::Position(Raw(item->GetWorldMatrix()))-arms::Position(bone[rig[h].hand]))/units*100);Report(text);}
                continue;
            }
            if(weapons[slot].summoned&&reportedSummonedReady[slot]!=key){reportedSummonedReady[slot]=key;char text[220];
                sprintf_s(text,"vr_summoned_sword_attachment_ready native_visible=%d alpha=%.3f offset_cm=%.1f; scoped_stereo_visibility",item->IsRenderingEnabled(),item->GetRenderAlphaValue(),arms::Length(arms::Position(meleeAttachments[slot].Attachment()))/units*100);Report(text);}
            weapons[slot].entity=item;
            // Staff has its accepted weapon-only axis/palm profile; it still
            // caches native attachment geometry and shares its support frame.
            if(WeaponType(type)==WeaponKind::Staff)continue;
            auto const& bounds=item->GetLocalNodeBoundary();melee::Blade blade;
            if(melee::LocalContact({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,type,blade)&&
               weapons[slot].tracked.PrepareAttachment(meleeAttachments[slot].Attachment(),rig[h].calibration,blade,grip[h],TwoHandType(type)?twoHandAngles:swordAngles,units,rig[h].palmLocal)&&
               melee::AnchorSummonedSword(item->GetName().GetText(),type,h,weapons[slot].tracked))weapons[slot].entity=item;
        }
        Mat bodyWorld,bodyDelta[TrackedPalette::Limit],armBase[2];bool bodyApplied=false;Vec bodyShift{};
        // The chest skin already supplies a fixed body pose. Only the arms may change it.
        if(stableBody&&!usePalettePose&&arms::BodyWorld(entityWorld,bodyHeading,bodyWorld,bodyRenderCorrection)){
            Mat ground=bodyWorld;
            bodyApplied=arms::BodyPoseDeltas(entityWorld,ground,bind,bone,bodyParts,unsigned(count),pelvis,
                bodyPosture.Crouched(),bodyPosture.Pickup(),crouchLowerReady?crouchLower:nullptr,bodyWorld,bodyDelta);
            if(bodyApplied&&bodyPosture.Crouched()&&!bodyPosture.Pickup()){
                // Native model-space lower-body reference, never a previous
                // world/eye pose: it follows ground translation and turning.
                for(int i=0;i<count;++i)if(i==pelvis||bodyParts[i]==arms::BodyPart::Leg)crouchLower[i]=Raw(wrapper->GetWorldMatrix(nodes+i));
                crouchLowerReady=true;
            }
            if(bodyApplied&&crouchRig&&physicalCrouch.DropMeters()>.0005f)
                arms::LowerRenderBody(bone,bodyParts,unsigned(count),crouchLegs,physicalCrouch.DropMeters()*units,bodyWorld,bodyDelta);
            for(unsigned h=0;h<2&&bodyApplied;++h)if(rig[h].upper>=0)
                bodyApplied=arms::Rebase(bone[rig[h].upper],arms::Multiply(bind[rig[h].upper],bodyWorld),armBase[h]);
            if(bodyApplied)bodyShift=arms::Position(arms::Multiply(bone[pelvis],bodyDelta[pelvis]))-arms::Position(bone[pelvis]);
        }
        // Prepare all deltas before writing. A refused torso pose leaves the
        // previous tracked-arm path available, with no partial body override.
        if(!bodyApplied){armBase[0]=Mat{};armBase[1]=Mat{};}
        Mat poseGrip[2]{grip[0],grip[1]};for(unsigned slot=0;slot<2;++slot)if(weapons[slot].tracked.valid)poseGrip[slot==0?1:0]=weapons[slot].tracked.grip;swordPose=false;sharedCombatValid=false;bool twoHandWeapon=false,wasSupported=twoHandGrip.holding,supportAttached=false;Vec supportWrist{},supportRendered{},staffPalm{},supportPalmLocal=rig[0].palmLocal;int meleeType=0;bool staffWeapon=false,staffMeshReady=false,weightBeforeSupport=false;Mat staffMesh;
        auto applyWeight=[&](Mat& target){
            auto prepared=weapons[0].tracked;prepared.grip=target;
            auto before=prepared.AtWrist(arms::WristAtPalm(target,rig[1].calibration,rig[1].palmLocal));
            Mat hand=arms::Multiply(rig[1].calibration,target);arms::Position(hand,arms::WristAtPalm(target,rig[1].calibration,rig[1].palmLocal));
            Mat weighted,resolved;
            if(!melee::WeightedMesh(before,controllerGrip[1],weightedControllerGrip,arms::Point(hand,rig[1].palmLocal),weighted)||
               !melee::GripFromMesh(target,before,weighted,resolved))return false;
            target=resolved;return true;
        };
        if(ready[1]){auto* sword=gCEntity::GetWeapon(player,GETrue);
            if(sword!=player&&Readable(sword,sizeof(eCEntity))&&PhysicalMeleeType(gCEntity::GetUseType(sword))){
                meleeType=int(gCEntity::GetUseType(sword));twoHandWeapon=TwoHandType(meleeType);staffWeapon=WeaponType(meleeType)==WeaponKind::Staff;
                auto const& bounds=sword->GetLocalNodeBoundary();melee::Blade blade;Mat inverse;
                if(melee::LocalContact({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,meleeType,blade)&&arms::Inverse(bone[rig[1].hand],inverse)){
                    auto key=reinterpret_cast<uintptr_t>(sword);
                    if(!meleeAttachments[0].Matches(key,meleeType))return;
                    Mat attachment=staffWeapon?meleeAttachments[0].Attachment():weapons[0].tracked.attachment;
                    if(staffWeapon){
                        if(staffAnchorWeapon!=key||!meleeAttachments[0].ProfileReady()){staffAnchorWeapon=0;melee::Blade shaft;
                            if(melee::HandleSegment({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,meleeType,shaft)&&melee::StaffAnchor(attachment,shaft,rig[1].palmLocal,staffAnchor))staffAnchorWeapon=key;}
                        bool contactReady=melee::StaffPalmContact(grip[1],rig[1].calibration,arms::WristAtPalm(grip[1],rig[1].calibration,rig[1].palmLocal),rig[1].palmLocal,units,staffGripForward,staffPalm,staffGripDepth,1);
                        Mat hand=arms::Multiply(rig[1].calibration,grip[1]);arms::Position(hand,arms::WristAtPalm(grip[1],rig[1].calibration,rig[1].palmLocal));
                        if(contactReady&&staffAnchorWeapon==key&&!meleeAttachments[0].ProfileReady()&&
                           melee::StaffWeaponPose(blade,staffAnchor,grip[1],staffPalm,staffAngles,staffMesh))meleeAttachments[0].LockProfile(staffMesh,hand);
                        staffMeshReady=contactReady&&staffAnchorWeapon==key&&meleeAttachments[0].ProfileAtHand(hand,staffMesh);
                        swordPose=staffMeshReady; // One-hand palm stays untwisted; support moves it with the mesh below.
                        if(staffMeshReady&&!wasSupported){Mat delta;
                            if(ApplyHandling(1,sword,melee::HandlingProfile::StaffOneHand,poseGrip[1],&staffMesh,&delta))staffPalm=arms::Point(delta,staffPalm);
                        }
                    }else{staffAnchorWeapon=0;swordPose=weapons[0].tracked.valid;}
                    // Acquisition must use the pose already shown with one
                    // hand, including its weight lag; attaching support does
                    // not discard that pose or snap the primary wrist.
                    if(swordPose&&twoHandWeapon&&!staffWeapon&&!wasSupported&&heavyTwoHand&&weightedGripReady&&weapons[0].tracked.valid)
                        weightBeforeSupport=applyWeight(poseGrip[1]);
                    if(swordPose&&twoHandWeapon){
                        auto* offhand=gCEntity::GetWeapon(player,GEFalse);
                        bool freeLeft=offhand==sword||offhand==player||!offhand||(Readable(offhand,sizeof(eCEntity))&&(gCEntity::GetUseType(offhand)==gEUseType_None||gCEntity::GetUseType(offhand)==gEUseType_Fist));
                        // Predict the actual rendered mesh using the same native
                        // attachment and exact wrist target as the palette below.
                        auto meshForGrip=[&](Mat const& pose){if(staffWeapon){Mat mesh=pose;melee::AnchorStaffWeapon(mesh,staffAnchor,staffPalm);return mesh;}Mat hand=arms::Multiply(rig[1].calibration,pose);arms::Position(hand,arms::WristAtPalm(pose,rig[1].calibration,rig[1].palmLocal));return arms::Multiply(attachment,hand);};
                        Mat mesh=meshForGrip(staffWeapon?staffMesh:poseGrip[1]),initialStaffMesh=staffMesh;melee::Blade handle;
                        bool handleValid=melee::HandleSegment({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,meleeType,handle);
                        if(handleValid){auto worldHandle=melee::WorldSegment(handle,mesh);
                            if(!staffWeapon&&!melee::SupportFistContact(rig[0].calibration,rig[0].palmLocal,0,units,supportPalmLocal)){twoHandGrip.Reset();return;}
                            if(staffWeapon&&!melee::StaffContactLocal(rig[0].calibration,rig[0].palmLocal,0,staffGripForward,staffGripDepth,units,supportPalmLocal)){twoHandGrip.Reset();return;}
                            Vec leftPalm=arms::Position(bow::ResolvedPalm(grip[0],rig[0].calibration,arms::WristAtPalm(grip[0],rig[0].calibration,rig[0].palmLocal),supportPalmLocal));
                            Vec rightPalm=arms::Position(bow::ResolvedPalm(poseGrip[1],rig[1].calibration,arms::WristAtPalm(poseGrip[1],rig[1].calibration,rig[1].palmLocal),rig[1].palmLocal));
                            Vec primary=staffWeapon?staffPalm:melee::ClosestPoint(worldHandle,rightPalm),axis=arms::Vector(mesh,blade.tip-blade.root);
                            bool continuing=wasSupported&&supportWeapon==key;
                            if(twoHandGrip.Update(key,meleeType,combatControllers,combatHead.session,
                                twoHandEnabled&&supportAllowed&&ready[0]&&combatHead.valid,freeLeft,
                                continuing?arms::Position(controllerGrip[1]):primary,continuing?arms::Position(controllerGrip[0]):leftPalm,axis,units,staffWeapon?staffRadius:supportRadius,0,&worldHandle,toggleSupportGrip)){
                                bool existing=wasSupported&&supportWeapon==key;
                                bool captured=existing||supportAttachment.Capture(mesh,handle,leftPalm,grip[0]);
                                Mat supported;Vec contact;
                                bool oriented=false;
                                if(captured){Mat raw[2],rawSupported;
                                    bool inputs=melee::TrackingGrips(combatControllers,raw);
                                    if(inputs&&(!existing||!sharedWorldGrip.Valid()||!sharedTrackingGrip.Valid())){
                                        auto held=staffWeapon?staffMesh:poseGrip[1];
                                        inputs=melee::CaptureHeldGrip(sharedWorldGrip,sharedTrackingGrip,held,controllerGrip[0],controllerGrip[1],raw[0],raw[1],combatControllers.timeMs,twoHandGrip.sign,staffWeapon?nullptr:&axis);
                                        if(inputs){char text[180];sprintf_s(text,"vr_two_hand_acquire type=%d current_rendered_pose=1 weight_retained=%d axis_realign=0",meleeType,weightBeforeSupport);Report(text);}
                                    }
                                    oriented=inputs&&sharedWorldGrip.Step(controllerGrip[0],controllerGrip[1],combatControllers.timeMs,supported)&&
                                        sharedTrackingGrip.Step(raw[0],raw[1],combatControllers.timeMs,rawSupported)&&melee::PoseOrientation(rawSupported,sharedCombatOrientation);
                                    sharedCombatValid=oriented;
                                }
                                if(oriented&&
                                   supportAttachment.Resolve(meshForGrip(supported),rig[0].calibration,supportPalmLocal,poseGrip[0],supportWrist,contact)&&
                                   melee::SupportContactInReach(leftPalm,contact,units,staffWeapon?staffLossDistance:gripLossDistance)){
                                    if(staffWeapon){staffMesh=meshForGrip(supported);staffMeshReady=melee::GripFromMesh(grip[1],initialStaffMesh,staffMesh,poseGrip[1]);
                                        if(!staffMeshReady){twoHandGrip.Reset();return;}}
                                    else poseGrip[1]=supported;supportWeapon=key;supportAttached=true;
                                }else twoHandGrip.Reset();
                            }
                        }else twoHandGrip.Reset();
                    }
                }
            }
        }
        if(!staffWeapon)staffAnchorWeapon=0;
        if(!twoHandWeapon||!swordPose)twoHandGrip.Reset();
        if(!twoHandGrip.holding){supportAttachment.Reset();supportWeapon=0;sharedWorldGrip.Reset();sharedTrackingGrip.Reset();sharedCombatValid=false;}
        // Weight changes the whole visual hand/weapon assembly, never the
        // mesh alone. Preserve the same palm contact through wide rotations.
        if(twoHandWeapon&&!staffWeapon&&!supportAttached&&!weightBeforeSupport&&heavyTwoHand&&weightedGripReady&&weapons[0].tracked.valid)
            applyWeight(poseGrip[1]);
        // Supported weapons have one filtered frame; transport the mesh and
        // both attached hands together after the coordinated grip solver.
        if(supportAttached){auto* item=reinterpret_cast<eCEntity*>(supportWeapon);Mat delta;
            auto profile=staffWeapon?melee::HandlingProfile::StaffTwoHand:melee::HandlingProfile::SupportedTwoHand;
            if(ApplyHandling(1,item,profile,poseGrip[1],staffWeapon?&staffMesh:nullptr,&delta)){
                poseGrip[0]=arms::Multiply(poseGrip[0],delta);supportWrist=arms::Point(delta,supportWrist);
                if(staffWeapon)staffPalm=arms::Point(delta,staffPalm);
            }
        }else if(staffWeapon&&!handlingApplied[1]){auto* item=gCEntity::GetWeapon(player,GETrue);Mat delta;
            if(ApplyHandling(1,item,melee::HandlingProfile::StaffOneHand,poseGrip[1],&staffMesh,&delta))staffPalm=arms::Point(delta,staffPalm);
        }else if(twoHandWeapon&&!staffWeapon){handling[1].Reset();} // retain the native-compatible heavy one-hand profile
        for(unsigned slot=0;slot<2;++slot){unsigned h=slot==0?1:0;auto* item=gCEntity::GetWeapon(player,slot==0);
            if(ready[h]&&Readable(item,sizeof(eCEntity))&&item!=player&&gCEntity::GetUseType(item)==gEUseType_1H)
                ApplyHandling(h,item,melee::HandlingProfile::OneHand,poseGrip[h]);
            else if(!(h==1&&(staffWeapon||supportAttached)))handling[h].Reset();
        }
        for(unsigned slot=0;slot<2;++slot)if(weapons[slot].tracked.valid){unsigned h=slot==0?1:0;
            weapons[slot].tracked.grip=poseGrip[h];weapons[slot].tracked.mesh=weapons[slot].tracked.AtWrist(arms::WristAtPalm(poseGrip[h],rig[h].calibration,rig[h].palmLocal));}
        if(wasSupported!=twoHandGrip.holding){char text[180];sprintf_s(text,"vr_two_hand_grip type=%d held=%d direction_sign=%.0f",meleeType,twoHandGrip.holding,twoHandGrip.sign);Report(text);}
        // Bow angles already orient the bow/aim sample. Keep the rendered
        // left hand at its calibrated controller pose, as with the right hand;
        // applying BowPitch again here bent only the avatar's wrist.
        bool crossbowAttached=false;Vec crossbowWrist{};
        if(bowSample.valid&&bowSample.crossbow&&ready[0])crossbowAttached=crossbowSupport.Read(poseGrip[0],crossbowWrist);
        Mat stringGrip;Vec stringWrist{},stringContact{};auto const& stringArm=rig[1];
        bool stringAttached=ready[1]&&drawingBow&&bowArrowReady&&bowSample.valid&&!bowSample.crossbow&&stringArm.stringReady&&
            bow::StringFingerContact(bone[stringArm.hand],bone[stringArm.stringTips[0]],stringArm.stringEnds[0],
                bone[stringArm.stringTips[1]],stringArm.stringEnds[1],stringContact)&&
            bow::StringHandGrip(bowSample,drawingBow,bowArrowReady,poseGrip[1],stringArm.calibration,stringContact,stringGrip,stringWrist);
        if(stringAttached)poseGrip[1]=stringGrip;
        Mat deltas[2][4];bool solved[2]{};arms::Solution solutions[2];arms::TwistAngles twistAngles[2];bool twistApplied[2]{};
        for(unsigned h=0;h<2;++h){if(!ready[h]){armTwist[h].Reset();continue;}auto const& arm=rig[h];
            Mat upper=arms::Multiply(bone[arm.upper],armBase[h]),lower=arms::Multiply(bone[arm.lower],armBase[h]),hand=arms::Multiply(bone[arm.hand],armBase[h]);
            Vec pole=arms::Vector(bodyApplied?bodyWorld:entityWorld,{h? .6f:-.6f,-1,-.3f});
            Vec target=h==1&&stringAttached?stringWrist:h==0&&crossbowAttached?crossbowWrist:h==0&&supportAttached?supportWrist:arms::WristAtPalm(poseGrip[h],rig[h].calibration,rig[h].palmLocal);
            // Controller reach is authoritative in every equipment state, including
            // empty hands and casting; stretch only the rendered bone axes.
            bool exactTracking=true;
            if(!(exactTracking?arms::SolveTracked(arms::Position(upper),arms::Position(lower),arms::Position(hand),target,pole,solutions[h]):arms::Solve(arms::Position(upper),arms::Position(lower),arms::Position(hand),target,pole,solutions[h])))continue;
            arms::LowerTrackedForearm(units,solutions[h]);
            Mat upperDelta,lowerDelta;
            auto jointDelta=exactTracking?arms::TrackedJointDelta:arms::JointDelta;
            if(!jointDelta(upper,arms::Position(lower),arms::Position(upper),solutions[h].elbow,upperDelta)||
               !jointDelta(lower,arms::Position(hand),solutions[h].elbow,solutions[h].wrist,lowerDelta))continue;
            Mat newHand=arms::Multiply(arm.calibration,poseGrip[h]),inv;arms::Position(newHand,solutions[h].wrist);
            auto const& referenceWorld=bodyApplied?bodyWorld:entityWorld;
            twistApplied[h]=arms::ProgressiveArmTwist(upper,lower,
                arms::Multiply(bind[arm.upper],referenceWorld),arms::Multiply(bind[arm.lower],referenceWorld),arms::Multiply(bind[arm.hand],referenceWorld),
                newHand,solutions[h].elbow,solutions[h].wrist,upperDelta,lowerDelta,armTwist[h],
                reinterpret_cast<uintptr_t>(player),combatHead.session,combatControllers.timeMs,twistAngles[h]);
            deltas[h][arms::Upper]=arms::Multiply(armBase[h],upperDelta);deltas[h][arms::Lower]=arms::Multiply(armBase[h],lowerDelta);
            // These armored bodies can retain a different native skin pose
            // during assembly. Anchor the actual rendered hand palette, rather
            // than assuming its pose equals the animation wrapper's hand.
            Mat nativeSkinHand=paladinRig?arms::Multiply(bind[arm.hand],Raw(data[arm.hand])):bone[arm.hand];
            if(!arms::Inverse(nativeSkinHand,inv))continue;deltas[h][arms::Hand]=arms::Multiply(inv,newHand);renderHandDelta[h]=deltas[h][arms::Hand];renderHandReady[h]=true;solved[h]=true;
        }
        if(!solved[0]&&!solved[1]&&!bodyApplied)return;
        if(!palette.Begin(data,unsigned(count)))return;
        for(int i=0;i<count;++i){auto tag=tags[i];Mat change;
            if(tag.side>=0){if(solved[tag.side])change=deltas[tag.side][tag.part];else if(bodyApplied)change=armBase[tag.side];else continue;}
            else if(bodyApplied&&bodyParts[i]!=arms::BodyPart::Other)change=bodyDelta[i];else continue;
            if(tag.side>=0&&tag.finger>=0&&solved[tag.side]&&castFingerReady[i]&&weapons[tag.side?0:1].held){
                auto const& arm=rig[tag.side];Mat newHand=arms::Multiply(bone[arm.hand],deltas[tag.side][arms::Hand]),inverse;
                if(!arms::Inverse(bone[i],inverse)){fault=true;RestoreEye();return;}
                change=arms::Multiply(inverse,arms::Multiply(castFinger[i],newHand));
            }
            Mat posed=arms::Multiply(palette.saved[i],change);if(!arms::Affine(posed)){fault=true;RestoreEye();return;}memcpy(data+i,&posed,sizeof(Mat));}
        for(unsigned h=0;h<2;++h){lowerHudReady[h]=solved[h];if(solved[h])
            lowerHudPose[h]=arms::Multiply(bind[rig[h].lower],Raw(data[rig[h].lower]));}
        if(PhysicalHandsOnly()){
            Vec wrists[2]{};for(unsigned h=0;h<2;++h)wrists[h]=solved[h]?solutions[h].wrist:arms::Position(bodyWorld);
            arms::HandsOnlySkin(reinterpret_cast<Mat*>(data),tags,unsigned(count),wrists[0],wrists[1],solved);
        }
        // Keep existing attachment/pivot offsets. These entities are restored
        // after each eye; inventory, ownership and NPC poses are never changed.
        for(unsigned slot=0;slot<2;++slot){auto* item=gCEntity::GetWeapon(player,slot==0);auto& saved=weapons[slot];if(item==player||!Readable(item,sizeof(eCEntity))||(slot==1&&weapons[0].changed&&weapons[0].entity==item))continue;
            unsigned hand=slot==0?1:0;auto type=gCEntity::GetUseType(item);
            // Ranged weapons and nocked ammo have one pose/visibility owner in
            // NativeTrackedBow, independent of this hero palette/cache.
            if(type==gEUseType_Bow||type==gEUseType_CrossBow||type==gEUseType_Arrow||type==gEUseType_Bolt)continue;
            if(reportedWeapon[slot]!=item){reportedWeapon[slot]=item;char text[220];sprintf_s(text,"vr_tracked_weapon slot=%u type=%d pose_type=%d entity=%08lx hand=%u name=%.80s",slot,int(type),MeleePoseType(int(type)),DWORD(reinterpret_cast<uintptr_t>(item)),hand,item->GetName().GetText());Report(text);}
            if(!solved[hand])continue;
            unsigned nativeHand=slot==0?1:0;if(rig[nativeHand].hand<0)continue;
            Mat desired=arms::Multiply(bone[rig[hand].hand],deltas[hand][arms::Hand]),inv;if(!arms::Inverse(bone[rig[nativeHand].hand],inv))continue;
            if(saved.changed&&saved.entity!=item)continue;
            Mat moved=saved.held&&saved.entity==item?saved.heldPose:saved.tracked.valid&&saved.entity==item?saved.tracked.AtWrist(solutions[hand].wrist):arms::Multiply(Raw(item->GetWorldMatrix()),arms::Multiply(inv,desired));if(!arms::Affine(moved))continue;
            if(slot==1&&type==gEUseType_Shield){
                auto const& bounds=item->GetLocalNodeBoundary();
                if(!shieldContact.Prepare(reinterpret_cast<uintptr_t>(item),Raw(item->GetWorldMatrix()),bone[rig[0].hand],rig[0].palmLocal,
                    {bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ})||
                   !shieldContact.Pose(desired,grip[0],rig[0].palmLocal,shieldSettings,units,moved))continue;
            }
            bool unsupported=hand==1&&heavyTwoHand&&WeaponType(int(type))==WeaponKind::TwoHand&&!supportAttached;
            if(slot==0&&staffWeapon&&staffMeshReady)moved=staffMesh;
            if(slot==1&&WeaponType(int(type))==WeaponKind::Staff){
                // Native magic equipment holds a staff in the left hand and
                // the selected Cast item in the right. Keep the same accepted
                // shaft/palm profile, mirrored contact, without rotating the hand.
                if(!meleeAttachments[slot].Matches(reinterpret_cast<uintptr_t>(item),type))continue;
                auto const& bounds=item->GetLocalNodeBoundary();melee::Blade shaft;Mat nativeAttachment=meleeAttachments[slot].Attachment();Vec contact;
                auto key=reinterpret_cast<uintptr_t>(item);
                bool shaftReady=melee::HandleSegment({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,int(type),shaft);
                if(key!=offStaffAnchorWeapon||!meleeAttachments[slot].ProfileReady()){offStaffAnchorWeapon=0;if(shaftReady&&melee::StaffAnchor(nativeAttachment,shaft,rig[0].palmLocal,offStaffAnchor))offStaffAnchorWeapon=key;}
                bool contactReady=shaftReady&&offStaffAnchorWeapon==key&&melee::StaffPalmContact(grip[0],rig[0].calibration,solutions[0].wrist,rig[0].palmLocal,units,staffGripForward,contact,staffGripDepth,0);
                if(contactReady&&!meleeAttachments[slot].ProfileReady()&&melee::StaffWeaponPose(shaft,offStaffAnchor,grip[0],contact,staffAngles,moved))meleeAttachments[slot].LockProfile(moved,desired);
                if(!contactReady||!meleeAttachments[slot].ProfileAtHand(desired,moved))continue;
            }
            MoveWeapon(saved,item,moved,PhysicalMeleeType(type)||saved.tracked.valid||saved.held||type==gEUseType_Cast||type==gEUseType_Shield||(slot==1&&WeaponType(int(type))==WeaponKind::Staff));
            if(hand==1&&supportAttached&&reinterpret_cast<uintptr_t>(item)==supportWeapon)supportRendered=arms::Point(moved,supportAttachment.LocalPoint());
            if((hand==1?PhysicalMeleeType(type):type==gEUseType_1H)&&combatHead.valid&&arms::FreshGrip(combatControllers,hand,GetTickCount64())){
                auto& combatSample=combatSamples[hand];
                if(combatReportedWeapon[hand]!=item){combatReportedWeapon[hand]=item;combatBoundsReported[hand]=false;}
                auto const& bounds=item->GetLocalNodeBoundary();melee::Blade local;
                if(melee::LocalContact({bounds.m_Min.m_fX,bounds.m_Min.m_fY,bounds.m_Min.m_fZ},{bounds.m_Max.m_fX,bounds.m_Max.m_fY,bounds.m_Max.m_fZ},units,int(type),local)){
                    auto const& pose=combatControllers.hands[hand].grip;
                    combatSample.player=reinterpret_cast<uintptr_t>(player);combatSample.weapon=reinterpret_cast<uintptr_t>(item);
                    combatSample.trigger=combatControllers.hands[hand].trigger>.65f;combatSample.time=combatControllers.timeMs;combatSample.session=combatHead.session;combatSample.units=units;
                    combatSample.blade={arms::Point(moved,local.root),arms::Point(moved,local.tip)};combatSample.hand=arms::Position(grip[hand]);
                    combatSample.trackedBlade=melee::TrackingWeaponBlade(combatControllers,hand,controllerGrip[hand],combatSample.blade,units,combatSample.trackingBlade);
                    combatSample.relativeHand={pose.position[0]-combatHead.position[0],pose.position[1]-combatHead.position[1],pose.position[2]-combatHead.position[2]};
                    combatSample.trackingHand={pose.position[0],pose.position[1],pose.position[2]};
                    combatSample.orientation={pose.quaternion[3],pose.quaternion[0],pose.quaternion[1],pose.quaternion[2]};combatSample.valid=true;
                    combatSample.twoHand=hand==1&&supportAttached&&twoHandGrip.holding;combatSample.heavySingleHand=unsupported;
                    if(unsupported&&weightedGripReady)combatSample.orientation=oneHandWeight.Rotation();combatSample.doubleEnded=WeaponType(int(type))==WeaponKind::Staff;
                    if(combatSample.doubleEnded&&!melee::TrackingShaftAxis(combatControllers,controllerGrip[1],combatSample.blade.tip-combatSample.blade.root,combatSample.trackingShaftAxis))combatSample.valid=false;
                    if(combatSample.twoHand){if(TwoHandType(int(type))){combatSample.orientation=sharedCombatOrientation;combatSample.valid=sharedCombatValid;}
                        else if(!melee::SupportOrientation(combatControllers,twoHandGrip.sign,combatSample.orientation))combatSample.valid=false;}
                    if(handlingApplied[hand]){
                        auto p=arms::Position(handlingTracking[hand]);combatSample.hand=arms::Position(poseGrip[hand]);
                        combatSample.trackingHand=p;combatSample.relativeHand={p.x-combatHead.position[0],p.y-combatHead.position[1],p.z-combatHead.position[2]};
                        if(!melee::PoseOrientation(handlingTracking[hand],combatSample.orientation))combatSample.valid=false;
                    }
                    if(!combatBoundsReported[hand]){combatBoundsReported[hand]=true;char text[220];sprintf_s(text,"vr_melee_blade hand=%u local=%.1f,%.1f,%.1f/%.1f,%.1f,%.1f length_cm=%.1f forward_pose=%d",hand,local.root.x,local.root.y,local.root.z,local.tip.x,local.tip.y,local.tip.z,arms::Length(local.tip-local.root)/units*100,swordPose);Report(text);}
                }else {combatSample={};if(!combatBoundsReported[hand]){combatBoundsReported[hand]=true;Report("vr_melee_mesh_bounds_refused; no_guessed_blade");}}
            }
        }
        ++applied;
        if(probes::Enabled&&GetTickCount64()>=nextLog){nextLog=GetTickCount64()+1000;char text[220];sprintf_s(text,"vr_arms_pose left=%d limited=%d right=%d limited=%d applied=%llu restored=%llu; physical_melee_sample=%d left_sample=%d",solved[0],solutions[0].limited,solved[1],solutions[1].limited,applied,restored,combatSamples[1].valid,combatSamples[0].valid);Report(text);sprintf_s(text,"vr_body_pose stable=%d pelvis_shift_cm=%.1f,%.1f,%.1f",bodyApplied,bodyShift.x,bodyShift.y,bodyShift.z);Report(text);
            for(unsigned h=0;h<2;++h)if(handlingApplied[h]){char handlingText[160];sprintf_s(handlingText,"vr_melee_handling hand=%u applied=1 supported=%d staff=%d tracking_space=1 rigid_palms=1",h,supportAttached,staffWeapon);Report(handlingText);}
            for(unsigned h=0;h<2;++h)if(solved[h]&&!(h==0&&(supportAttached||crossbowAttached))&&!(h==1&&stringAttached)){auto const& arm=rig[h];
                Vec palm=arms::Point(Raw(data[arm.hand]),arms::Point(bind[arm.hand],arm.palmLocal));
                sprintf_s(text,"vr_hand_pivot hand=%u palm_error_cm=%.3f rig_palm_pivot=1",h,arms::Length(palm-arms::Position(poseGrip[h]))/units*100);Report(text);}
            for(unsigned h=0;h<2;++h)if(solved[h]){char twistText[180];sprintf_s(twistText,"vr_arm_twist hand=%u active=%d wrist_deg=%.1f forearm_deg=%.1f upper_deg=%.1f; exact_hand=1 progressive=1",h,twistApplied[h],twistAngles[h].wrist/arms::ArmDegree,twistAngles[h].forearm/arms::ArmDegree,twistAngles[h].upper/arms::ArmDegree);Report(twistText);}
            for(unsigned slot=0;slot<2;++slot)if(weapons[slot].tracked.valid){unsigned h=slot==0?1:0;auto const& w=weapons[slot];
                sprintf_s(text,"vr_1h_spatial_pose hand=%u pre_cull=1 render_enabled=%d native_visible=%d wrist_error_cm=%.3f",h,w.entity->IsRenderingEnabled(),w.visible,arms::Length(solutions[h].wrist-arms::WristAtPalm(w.tracked.grip,rig[h].calibration,rig[h].palmLocal))/units*100);Report(text);}
            if(bowSample.valid&&solved[bowSample.aimHand]){unsigned h=bowSample.aimHand;auto const& arm=rig[h];Vec rendered=arms::Point(Raw(data[arm.hand]),arms::Point(bind[arm.hand],arm.palmLocal))+arms::Vector(poseGrip[h],bow::GripContactOffset(bowSample.crossbow?crossbowGripOffsets:bowGripOffsets,units,h)),wanted=arms::Position(bowSample.left);
                sprintf_s(text,"vr_bow_palm_anchor rendered=%.1f,%.1f,%.1f planned=%.1f,%.1f,%.1f error_cm=%.3f wrist_error_cm=%.3f exact_tracking=1",rendered.x,rendered.y,rendered.z,wanted.x,wanted.y,wanted.z,arms::Length(rendered-wanted)/units*100,arms::Length(solutions[h].wrist-arms::WristAtPalm(poseGrip[h],rig[h].calibration,rig[h].palmLocal))/units*100);Report(text);}
            if(staffWeapon&&staffMeshReady&&solved[1]){auto const& arm=rig[1];
                Vec contact=arms::Point(staffMesh,staffAnchor),rendered=arms::Point(Raw(data[arm.hand]),arms::Point(bind[arm.hand],arm.palmLocal))+arms::Vector(poseGrip[1],melee::StaffContactOffset(1,staffGripForward,staffGripDepth,units));
                sprintf_s(text,"vr_staff_weapon_only type=%d palm_error_cm=%.3f right_hand_untwisted=%d grip_forward_cm=%d depth_cm=%d fixed_direction=1 palm_locked=1 supported=%d",meleeType,arms::Length(rendered-contact)/units*100,!supportAttached,staffGripForward,staffGripDepth,supportAttached);Report(text);}
            if(supportAttached&&solved[0]&&solved[1]){auto const& arm=rig[0];Vec rendered=arms::Point(Raw(data[arm.hand]),arms::Point(bind[arm.hand],supportPalmLocal)),local=supportAttachment.LocalPoint();
                sprintf_s(text,"vr_two_hand_palm_anchor type=%d local=%.2f,%.2f,%.2f error_cm=%.3f coordinated=%d reused_tracking_world_updates=%llu",meleeType,local.x,local.y,local.z,arms::Length(rendered-supportRendered)/units*100,sharedCombatValid,sharedWorldGrip.ReusedTrackingWorldUpdates());Report(text);}
        }
    }
    void End(){if(bowSample.valid&&bowSample.crossbow)crossbowGripToggle.Confirm(crossbowSupport.Holding(bowSample.weapon));RestoreEye();renderHandReady[0]=renderHandReady[1]=false;frozen.Reset();for(auto& weapon:weapons){weapon.tracked.valid=false;weapon.framePrepared=false;}scope=false;ready[0]=ready[1]=false;player=nullptr;}
};
}
