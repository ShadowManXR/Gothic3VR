#pragma once
#include <windows.h>
#include <openxr/openxr.h>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>
#include "ControllerData.h"
#include "ControllerProfiles.h"
namespace diagnostic {
class QuestActions {
    XrActionSet set=XR_NULL_HANDLE;
    XrAction grip=0,aim=0,stick=0,trigger=0,squeeze=0,primary=0,secondary=0,stickClick=0,haptic=0,menu=0;
    XrPath hands[2]{};XrSpace grips[2]{},aims[2]{};
public:
    ControllerFrame frame;XrResult lastHapticResult=XR_SUCCESS;
    struct ProfileResult {char const* path;bool enabled;XrResult result;};
    std::vector<ProfileResult> profileResults;
    template<class Api> void Initialize(Api& api,XrSession session,std::vector<std::string> const& extensions={}) {
        profileResults.clear();
        auto path=[&](char const* name){XrPath p=0;api.Check(api.StringToPath(api.instance,name,&p),name);return p;};
        hands[0]=path("/user/hand/left");hands[1]=path("/user/hand/right");
        XrActionSetCreateInfo si{XR_TYPE_ACTION_SET_CREATE_INFO};strcpy_s(si.actionSetName,"gothic_controls");strcpy_s(si.localizedActionSetName,"Gothic 3 VR controls");
        api.Check(api.CreateActionSet(api.instance,&si,&set),"create controller action set");
        auto action=[&](char const* name,XrActionType type){XrActionCreateInfo ci{XR_TYPE_ACTION_CREATE_INFO};ci.actionType=type;strcpy_s(ci.actionName,name);strcpy_s(ci.localizedActionName,name);ci.countSubactionPaths=2;ci.subactionPaths=hands;XrAction a=0;api.Check(api.CreateAction(set,&ci,&a),name);return a;};
        grip=action("grip_pose",XR_ACTION_TYPE_POSE_INPUT);aim=action("aim_pose",XR_ACTION_TYPE_POSE_INPUT);
        stick=action("joystick",XR_ACTION_TYPE_VECTOR2F_INPUT);trigger=action("trigger",XR_ACTION_TYPE_FLOAT_INPUT);squeeze=action("squeeze",XR_ACTION_TYPE_FLOAT_INPUT);
        primary=action("primary",XR_ACTION_TYPE_BOOLEAN_INPUT);secondary=action("secondary",XR_ACTION_TYPE_BOOLEAN_INPUT);stickClick=action("joystick_click",XR_ACTION_TYPE_BOOLEAN_INPUT);
        haptic=action("weapon_feedback",XR_ACTION_TYPE_VIBRATION_OUTPUT);
        menu=action("game_menu",XR_ACTION_TYPE_BOOLEAN_INPUT);
        unsigned accepted=0;
        for(auto const& profile:ControllerProfiles){
            if(!ControllerProfileEnabled(profile,extensions)){profileResults.push_back({profile.path,false,XR_ERROR_EXTENSION_NOT_PRESENT});continue;}
            std::vector<XrActionSuggestedBinding> bindings;
            for(unsigned h=0;h<2;++h){std::string root=h?"/user/hand/right/input/":"/user/hand/left/input/";
                auto bind=[&](XrAction a,char const* component){bindings.push_back({a,path((root+component).c_str())});};
                bind(grip,"grip/pose");bind(aim,"aim/pose");bind(stick,"thumbstick");bind(trigger,"trigger/value");bind(squeeze,"squeeze/value");bind(stickClick,"thumbstick/click");
                if(profile.layout==ControllerLayout::Generic){bind(primary,"primary/click");bind(secondary,"secondary/click");}
                else if(profile.layout==ControllerLayout::Index){bind(primary,"a/click");bind(secondary,"b/click");}
                else if(profile.layout==ControllerLayout::Frame){
                    // Match Valve's Touch emulation: bottom = X/A, top three = Y/B.
                    bind(primary,h?"a/click":"dpad_down/click");bind(secondary,h?"b/click":"dpad_up/click");
                    bind(secondary,h?"x/click":"dpad_left/click");bind(secondary,h?"y/click":"dpad_right/click");
                }else{bind(primary,h?"a/click":"x/click");bind(secondary,h?"b/click":"y/click");}
                bindings.push_back({haptic,path((std::string(h?"/user/hand/right/output/":"/user/hand/left/output/")+"haptic").c_str())});
            }
            if(profile.layout==ControllerLayout::Touch)bindings.push_back({menu,path("/user/hand/left/input/menu/click")});
            else if(profile.layout==ControllerLayout::Frame)bindings.push_back({menu,path("/user/hand/left/input/view/click")});
            // Index and Generic expose no application menu button. Main menu is
            // already available in the radial menu; never consume System.
            XrInteractionProfileSuggestedBinding suggest{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};suggest.interactionProfile=path(profile.path);suggest.countSuggestedBindings=uint32_t(bindings.size());suggest.suggestedBindings=bindings.data();
            auto result=api.SuggestInteractionProfileBindings(api.instance,&suggest);profileResults.push_back({profile.path,true,result});
            if(XR_SUCCEEDED(result)){++accepted;continue;}
            if(result!=XR_ERROR_PATH_UNSUPPORTED&&result!=XR_ERROR_FEATURE_UNSUPPORTED&&result!=XR_ERROR_EXTENSION_NOT_PRESENT)api.Check(result,"suggest controller profile bindings");
        }
        if(!accepted)api.Check(XR_ERROR_PATH_UNSUPPORTED,"no supported controller binding profile");
        XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};attach.countActionSets=1;attach.actionSets=&set;api.Check(api.AttachSessionActionSets(session,&attach),"attach controller actions");
        for(unsigned h=0;h<2;++h){XrActionSpaceCreateInfo ci{XR_TYPE_ACTION_SPACE_CREATE_INFO};ci.subactionPath=hands[h];ci.poseInActionSpace.orientation.w=1;ci.action=grip;api.Check(api.CreateActionSpace(session,&ci,&grips[h]),"create controller grip space");ci.action=aim;api.Check(api.CreateActionSpace(session,&ci,&aims[h]),"create controller aim space");}
    }
    template<class Api> bool Pulse(Api& api,XrSession session,unsigned hand,float amplitude,int milliseconds=45){
        if(hand>1||!std::isfinite(amplitude)||!frame.focused||!frame.hands[hand].active||!haptic){lastHapticResult=XR_SESSION_NOT_FOCUSED;return false;}
        XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};info.action=haptic;info.subactionPath=hands[hand];
        XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};vibration.duration=XrDuration(std::clamp(milliseconds,40,250))*1000000;vibration.frequency=XR_FREQUENCY_UNSPECIFIED;vibration.amplitude=std::clamp(amplitude,0.f,1.f);
        lastHapticResult=api.ApplyHapticFeedback(session,&info,reinterpret_cast<XrHapticBaseHeader const*>(&vibration));return lastHapticResult==XR_SUCCESS;
    }
    template<class Api> void Sync(Api& api,XrSession session,XrSpace local,XrSpace view,XrTime time,bool focused) {
        frame={};frame.timeMs=GetTickCount64();if(!set||!focused)return;
        XrActiveActionSet active{set,XR_NULL_PATH};XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};sync.countActiveActionSets=1;sync.activeActionSets=&active;
        XrResult result=api.SyncActions(session,&sync);if(result==XR_SESSION_NOT_FOCUSED)return;api.Check(result,"sync Quest actions");frame.focused=true;
        for(unsigned h=0;h<2;++h){auto& out=frame.hands[h];XrActionStateGetInfo get{XR_TYPE_ACTION_STATE_GET_INFO};get.subactionPath=hands[h];
            auto pose=[&](XrAction a,XrSpace actionSpace,XrSpace base){ControllerPose p;get.action=a;XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};api.Check(api.GetActionStatePose(session,&get,&state),"controller pose state");if(!state.isActive)return p;
                XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};api.Check(api.LocateSpace(actionSpace,base,time,&location),"locate controller");
                auto flags=XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;if((location.locationFlags&flags)!=flags)return p;
                p.valid=true;auto tracked=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;p.tracked=(location.locationFlags&tracked)==tracked;p.position[0]=location.pose.position.x;p.position[1]=location.pose.position.y;p.position[2]=location.pose.position.z;p.quaternion[0]=location.pose.orientation.x;p.quaternion[1]=location.pose.orientation.y;p.quaternion[2]=location.pose.orientation.z;p.quaternion[3]=location.pose.orientation.w;return p;};
            out.grip=pose(grip,grips[h],local);out.aim=pose(aim,aims[h],local);out.aimInView=pose(aim,aims[h],view);
            get.action=stick;XrActionStateVector2f vector{XR_TYPE_ACTION_STATE_VECTOR2F};api.Check(api.GetActionStateVector2f(session,&get,&vector),"controller joystick");if(vector.isActive){out.stick[0]=vector.currentState.x;out.stick[1]=vector.currentState.y;}
            auto number=[&](XrAction a){get.action=a;XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};api.Check(api.GetActionStateFloat(session,&get,&state),"controller analog input");if(state.isActive)frame.availableButtons|=1u<<(h*6+(a==squeeze?1:0));return state.isActive?state.currentState:0.f;};
            auto button=[&](XrAction a){get.action=a;XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};api.Check(api.GetActionStateBoolean(session,&get,&state),"controller button");if(state.isActive)frame.availableButtons|=1u<<(h*6+(a==primary?2:a==secondary?3:a==stickClick?4:5));return state.isActive&&state.currentState;};
            out.trigger=number(trigger);out.squeeze=number(squeeze);out.primary=button(primary);out.secondary=button(secondary);out.stickClick=button(stickClick);out.menu=h==0&&button(menu);
            out.active=out.grip.valid||vector.isActive;
        }
    }
    template<class Api> void Destroy(Api& api) noexcept {frame={};for(unsigned h=0;h<2;++h){if(grips[h]&&api.DestroySpace)api.DestroySpace(grips[h]);if(aims[h]&&api.DestroySpace)api.DestroySpace(aims[h]);grips[h]=aims[h]=0;}if(set&&api.DestroyActionSet)api.DestroyActionSet(set);set=0;}
};
}
