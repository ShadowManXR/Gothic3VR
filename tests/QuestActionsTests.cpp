#include "QuestActions.h"
#include <map>
#include <stdexcept>
#include <cstdio>
#include <set>
unsigned destroyedSpaces=0,destroyedSets=0;
XrResult XRAPI_CALL DestroySpaceFixture(XrSpace){++destroyedSpaces;return XR_SUCCESS;}
XrResult XRAPI_CALL DestroySetFixture(XrActionSet){++destroyedSets;return XR_SUCCESS;}
struct ApiFixture {
    XrInstance instance=1;unsigned actionCount=0,spaceCount=0,bindingCount=0,syncCount=0,hapticCount=0;bool attached=false,active=true,valid=true,focused=true,tracked=true;
    XrResult ApplyHapticFeedback(XrSession,XrHapticActionInfo const* info,XrHapticBaseHeader const* pulse){auto v=reinterpret_cast<XrHapticVibration const*>(pulse);if(info->action!=9||!info->subactionPath||(v->duration<40000000||v->duration>250000000)||v->amplitude<0||v->amplitude>1)return XR_ERROR_VALIDATION_FAILURE;++hapticCount;return XR_SUCCESS;}
    std::map<std::string,XrPath> paths;
    std::map<std::string,std::vector<std::pair<XrAction,std::string>>> suggestions;
    std::map<std::string,XrResult> profileErrors;
    std::string Name(XrPath path)const{for(auto const& p:paths)if(p.second==path)return p.first;return "";}
    PFN_xrDestroySpace DestroySpace=DestroySpaceFixture;PFN_xrDestroyActionSet DestroyActionSet=DestroySetFixture;
    void Check(XrResult r,char const* operation){if(XR_FAILED(r))throw std::runtime_error(operation);}
    XrResult StringToPath(XrInstance,char const* text,XrPath* path){auto inserted=paths.emplace(text,XrPath(paths.size()+1));*path=inserted.first->second;return XR_SUCCESS;}
    XrResult CreateActionSet(XrInstance,XrActionSetCreateInfo const*,XrActionSet* out){*out=9;return XR_SUCCESS;}
    XrResult CreateAction(XrActionSet,XrActionCreateInfo const* info,XrAction* out){if(info->countSubactionPaths!=2||info->subactionPaths[0]==info->subactionPaths[1])return XR_ERROR_VALIDATION_FAILURE;*out=++actionCount;return XR_SUCCESS;}
    XrResult SuggestInteractionProfileBindings(XrInstance,XrInteractionProfileSuggestedBinding const* info){
        bindingCount=info->countSuggestedBindings;auto name=Name(info->interactionProfile);auto& list=suggestions[name];
        for(unsigned i=0;i<bindingCount;++i)list.emplace_back(info->suggestedBindings[i].action,Name(info->suggestedBindings[i].binding));
        auto error=profileErrors.find(name);return error==profileErrors.end()?XR_SUCCESS:error->second;}
    XrResult AttachSessionActionSets(XrSession,XrSessionActionSetsAttachInfo const* info){attached=info->countActionSets==1;return XR_SUCCESS;}
    XrResult CreateActionSpace(XrSession,XrActionSpaceCreateInfo const* info,XrSpace* out){if(!attached||info->poseInActionSpace.orientation.w!=1)return XR_ERROR_VALIDATION_FAILURE;*out=++spaceCount;return XR_SUCCESS;}
    XrResult SyncActions(XrSession,XrActionsSyncInfo const*){++syncCount;return focused?XR_SUCCESS:XR_SESSION_NOT_FOCUSED;}
    XrResult GetActionStatePose(XrSession,XrActionStateGetInfo const*,XrActionStatePose* out){out->isActive=active;return XR_SUCCESS;}
    XrResult GetActionStateFloat(XrSession,XrActionStateGetInfo const*,XrActionStateFloat* out){out->isActive=active;out->currentState=1;return XR_SUCCESS;}
    XrResult GetActionStateBoolean(XrSession,XrActionStateGetInfo const*,XrActionStateBoolean* out){out->isActive=active;out->currentState=XR_TRUE;return XR_SUCCESS;}
    XrResult GetActionStateVector2f(XrSession,XrActionStateGetInfo const*,XrActionStateVector2f* out){out->isActive=active;out->currentState={.4f,.8f};return XR_SUCCESS;}
    XrResult LocateSpace(XrSpace source,XrSpace,XrTime,XrSpaceLocation* out){out->locationFlags=valid?(XR_SPACE_LOCATION_POSITION_VALID_BIT|XR_SPACE_LOCATION_ORIENTATION_VALID_BIT):XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;if(tracked)out->locationFlags|=XR_SPACE_LOCATION_POSITION_TRACKED_BIT|XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;out->pose.orientation.w=1;out->pose.position.x=source<=2?-.25f:.25f;return XR_SUCCESS;}
};
int main(){
    unsigned failures=0;auto check=[&](bool ok,char const* message){printf("%s %s\n",ok?"PASS":"FAIL",message);failures+=!ok;};
    ApiFixture api;diagnostic::QuestActions actions;actions.Initialize(api,1);
    check(api.actionCount==10&&api.suggestions["/interaction_profiles/oculus/touch_controller"].size()==19&&api.spaceCount==4&&api.attached,"both hands retain 19 original Touch bindings and attached grip/aim action spaces");
    check(api.suggestions.size()==2&&api.suggestions["/interaction_profiles/valve/index_controller"].size()==18,"core Index profile is available without enabling optional extensions");
    check(actions.profileResults.size()==6&&!actions.profileResults[2].enabled&&!actions.profileResults[4].enabled,"vendor profiles are skipped before path creation when their extension is disabled");
    check(api.paths.count("/interaction_profiles/oculus/touch_controller")&&api.paths.count("/user/hand/left/input/x/click")&&api.paths.count("/user/hand/right/input/a/click")&&api.paths.count("/user/hand/left/input/menu/click")&&!api.paths.count("/user/hand/right/input/menu/click"),"left X and right A use their valid per-hand Touch paths");
    actions.Sync(api,1,2,3,100,true);auto const& frame=actions.frame;
    check(frame.focused&&frame.hands[0].active&&frame.hands[1].active&&frame.hands[0].grip.position[0]<0&&frame.hands[1].grip.position[0]>0&&frame.hands[1].aimInView.valid,"independent local poses and view-relative UI aim");
    check(frame.hands[0].stick[1]==.8f&&frame.hands[1].trigger==1&&frame.hands[1].primary&&frame.hands[0].menu&&!frame.hands[1].menu,"analog and boolean action values reach controller frame");
    check(actions.Pulse(api,1,1,.45f)&&api.hapticCount==1&&!actions.Pulse(api,1,2,.45f),"ready feedback uses the correct subaction and a bounded vibration");
    check(actions.Pulse(api,1,0,.9f,160)&&api.hapticCount==2&&actions.lastHapticResult==XR_SUCCESS,"sneak feedback sends a stronger 160 ms pulse through the left output action");
    check(frame.hands[0].grip.tracked&&frame.hands[1].grip.tracked,"full positional and rotational tracking is retained for physical arms");
    api.tracked=false;actions.Sync(api,1,2,3,100,true);check(frame.hands[0].grip.valid&&!frame.hands[0].grip.tracked,"runtime inferred poses remain distinct from fully tracked hands");api.tracked=true;
    api.active=false;actions.Sync(api,1,2,3,101,true);check(!frame.hands[0].active&&!frame.hands[0].grip.valid&&frame.hands[1].trigger==0&&!frame.hands[1].primary,"inactive actions clear previous held input and poses");
    api.active=true;api.valid=false;actions.Sync(api,1,2,3,102,true);check(!frame.hands[0].grip.valid&&!frame.hands[1].aimInView.valid,"orientation-only location is not accepted as 6DoF pointer");
    api.focused=false;actions.Sync(api,1,2,3,103,true);check(!frame.focused&&!frame.hands[1].active&&frame.hands[1].trigger==0,"XR_SESSION_NOT_FOCUSED clears all input without fatal error");
    auto count=api.syncCount;actions.Sync(api,1,2,3,104,false);check(api.syncCount==count&&!frame.focused,"unfocused session is not synchronized");check(!actions.Pulse(api,1,0,.9f,160)&&actions.lastHapticResult==XR_SESSION_NOT_FOCUSED,"failed feedback records its explicit runtime/context result");
    actions.Destroy(api);actions.Destroy(api);check(destroyedSpaces==4&&destroyedSets==1,"action spaces and set are cleaned once");
    {
        auto none=diagnostic::SupportedControllerExtensions({"unrelated_extension"});
        auto supported=diagnostic::SupportedControllerExtensions({"XR_BD_controller_interaction","XR_VALVE_frame_controller_interaction","XR_KHR_generic_controller","XR_BD_controller_interaction"});
        check(none.empty()&&supported.size()==3&&std::set<std::string>(supported.begin(),supported.end()).size()==3,"only advertised controller extensions are requested, without duplicate Pico entries");
        ApiFixture multi;diagnostic::QuestActions controller;controller.Initialize(multi,1,supported);
        auto has=[&](char const* profile,XrAction action,char const* path){auto const& list=multi.suggestions[profile];return std::find(list.begin(),list.end(),std::make_pair(action,std::string(path)))!=list.end();};
        check(multi.suggestions.size()==6&&multi.spaceCount==4&&multi.actionCount==10,"all hardware profiles share the original actions and four pose spaces");
        for(auto p:{"/interaction_profiles/bytedance/pico4_controller","/interaction_profiles/bytedance/pico_neo3_controller"})
            check(multi.suggestions[p]==multi.suggestions["/interaction_profiles/oculus/touch_controller"],"Pico four-face-button bindings retain Touch action semantics and both haptic outputs");
        check(has("/interaction_profiles/valve/index_controller",6,"/user/hand/left/input/a/click")&&has("/interaction_profiles/valve/index_controller",7,"/user/hand/left/input/b/click"),"Index left A/B map to logical X/Y without unsupported X/Y paths");
        check(has("/interaction_profiles/valve/frame_controller_valve",6,"/user/hand/left/input/dpad_down/click")&&has("/interaction_profiles/valve/frame_controller_valve",7,"/user/hand/left/input/dpad_right/click")&&has("/interaction_profiles/valve/frame_controller_valve",10,"/user/hand/left/input/view/click")&&multi.suggestions["/interaction_profiles/valve/frame_controller_valve"].size()==23,"Frame uses bottom/upper face mapping and View menu from Valve compatibility conventions");
        check(has("/interaction_profiles/khr/generic_controller",6,"/user/hand/left/input/primary/click")&&has("/interaction_profiles/khr/generic_controller",7,"/user/hand/right/input/secondary/click")&&multi.suggestions["/interaction_profiles/khr/generic_controller"].size()==18,"Generic fallback uses semantic buttons and does not invent a menu input");
        bool safe=true;for(auto const& p:multi.suggestions)for(auto const& b:p.second){safe&=b.second.find("/system/")==std::string::npos;if(p.first.find("index_controller")!=std::string::npos||p.first.find("generic_controller")!=std::string::npos)safe&=b.first!=10;}
        check(safe,"system buttons remain reserved and radial main menu remains the fallback");
        controller.Destroy(multi);
    }
    for(auto rejected:{"/interaction_profiles/valve/index_controller","/interaction_profiles/bytedance/pico4_controller","/interaction_profiles/valve/frame_controller_valve"}){
        ApiFixture limited;limited.profileErrors[rejected]=XR_ERROR_PATH_UNSUPPORTED;diagnostic::QuestActions controller;
        controller.Initialize(limited,1,{"XR_BD_controller_interaction","XR_VALVE_frame_controller_interaction"});controller.Sync(limited,1,2,3,100,true);
        check(limited.attached&&controller.frame.focused&&controller.frame.hands[0].trigger==1,"unsupported candidate profile does not interrupt existing Touch input or VR initialization");controller.Destroy(limited);
    }
    {ApiFixture onlyIndex;onlyIndex.profileErrors["/interaction_profiles/oculus/touch_controller"]=XR_ERROR_PATH_UNSUPPORTED;diagnostic::QuestActions controller;controller.Initialize(onlyIndex,1);check(onlyIndex.attached,"a runtime rejecting Touch can still initialize through Index");controller.Destroy(onlyIndex);}
    for(auto result:{XR_ERROR_PATH_UNSUPPORTED,XR_ERROR_RUNTIME_FAILURE}){ApiFixture broken;broken.profileErrors["/interaction_profiles/oculus/touch_controller"]=result;broken.profileErrors["/interaction_profiles/valve/index_controller"]=result;diagnostic::QuestActions controller;bool failed=false;try{controller.Initialize(broken,1);}catch(...){failed=true;}check(failed&&!broken.attached,"no usable bindings or an unexpected runtime failure is reported before attaching input");controller.Destroy(broken);}
    return failures?1:0;
}
