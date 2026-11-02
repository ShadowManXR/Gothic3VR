#pragma once
#include "Pipe.h"
#include "Layers.h"
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <filesystem>
#include <map>
#include <mutex>
namespace diagnostic::meta_bridge {
class Client {
 using Texture=Microsoft::WRL::ComPtr<ID3D11Texture2D>;using Mutex=Microsoft::WRL::ComPtr<IDXGIKeyedMutex>;
 struct Chain {std::vector<Texture> textures;std::vector<Mutex> mutexes;uint32_t index=0;bool acquired=false,released=false;};
 Pipe pipe;HANDLE process=nullptr,job=nullptr;std::mutex callMutex;std::map<uint64_t,Chain> chains;std::vector<uint64_t> releases;
 Microsoft::WRL::ComPtr<ID3D11Device1> device;Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
 XrInstance instance=XR_NULL_HANDLE;bool destroyed=false;std::string lastError;
 std::map<std::pair<uint64_t,uint64_t>,ActionState> actionSnapshot;
 std::map<std::pair<uint64_t,uint64_t>,LocatedSpace> poseSnapshot;XrTime poseSnapshotTime=0;
 inline static Client* current=nullptr;
 void Cleanup()noexcept;
 XrResult FlushReleased()noexcept;
 template<class F>XrResult Call(Command command,Writer const& input,F decode)noexcept{
  try{std::lock_guard<std::mutex> lock(callMutex);auto response=pipe.Call(command,input);auto result=response.Pod<XrResult>();if(XR_FAILED(result)){lastError=response.Text();if(result!=XR_ERROR_SIZE_INSUFFICIENT)return result;}decode(response,result);response.Complete();return result;}
  catch(std::exception const& e){lastError=e.what();return XR_ERROR_RUNTIME_FAILURE;}
 }
 XrResult Call(Command command,Writer const& input={})noexcept{return Call(command,input,[](Reader&,XrResult){});}
 template<class T>static XrResult State(Command command,XrActionStateGetInfo const* info,T* state){
  auto found=current->actionSnapshot.find({HandleBits(info->action),info->subactionPath});
  if(found!=current->actionSnapshot.end()){auto const& s=found->second;state->isActive=s.active;
   if constexpr(!std::is_same_v<T,XrActionStatePose>){state->changedSinceLastSync=s.changed;state->lastChangeTime=s.time;
    if constexpr(std::is_same_v<T,XrActionStateVector2f>)state->currentState={s.x,s.y};else state->currentState=static_cast<decltype(state->currentState)>(s.x);
   }return XR_SUCCESS;
  }
  Writer w;Put(w,HandleBits(info->action),info->subactionPath);return current->Call(command,w,[&](Reader& r,XrResult){Read(r,*state);});
 }
public:
 Client(std::filesystem::path const& loader32,std::filesystem::path const& logFile,std::filesystem::path const& manifest32={});
 ~Client();Client(Client const&)=delete;Client& operator=(Client const&)=delete;
 XrInstance Instance()const{return instance;}
 uint32_t RequestCount()const{return pipe.Calls();}
 template<class Api>void Install(Api& api){
#define META_BIND(name) api.name=&name;
  META_BIND(DestroyInstance) META_BIND(GetInstanceProperties) META_BIND(GetSystem) META_BIND(GetSystemProperties)
  META_BIND(GetD3D11GraphicsRequirementsKHR) META_BIND(CreateSession) META_BIND(DestroySession) META_BIND(BeginSession) META_BIND(EndSession) META_BIND(RequestExitSession) META_BIND(PollEvent)
  META_BIND(CreateReferenceSpace) META_BIND(DestroySpace) META_BIND(EnumerateViewConfigurationViews) META_BIND(EnumerateEnvironmentBlendModes) META_BIND(EnumerateSwapchainFormats)
  META_BIND(CreateSwapchain) META_BIND(DestroySwapchain) META_BIND(EnumerateSwapchainImages) META_BIND(AcquireSwapchainImage) META_BIND(WaitSwapchainImage) META_BIND(ReleaseSwapchainImage)
  META_BIND(WaitFrame) META_BIND(BeginFrame) META_BIND(EndFrame) META_BIND(LocateViews) META_BIND(ResultToString) META_BIND(ApplyHapticFeedback)
  META_BIND(StringToPath) META_BIND(CreateActionSet) META_BIND(DestroyActionSet) META_BIND(CreateAction) META_BIND(SuggestInteractionProfileBindings) META_BIND(AttachSessionActionSets)
  META_BIND(CreateActionSpace) META_BIND(SyncActions) META_BIND(GetActionStatePose) META_BIND(GetActionStateBoolean) META_BIND(GetActionStateFloat) META_BIND(GetActionStateVector2f) META_BIND(LocateSpace)
#undef META_BIND
 }
 static XrResult XRAPI_CALL DestroyInstance(XrInstance);
 static XrResult XRAPI_CALL GetInstanceProperties(XrInstance,XrInstanceProperties*);
 static XrResult XRAPI_CALL GetSystem(XrInstance,XrSystemGetInfo const*,XrSystemId*);
 static XrResult XRAPI_CALL GetSystemProperties(XrInstance,XrSystemId,XrSystemProperties*);
 static XrResult XRAPI_CALL GetD3D11GraphicsRequirementsKHR(XrInstance,XrSystemId,XrGraphicsRequirementsD3D11KHR*);
 static XrResult XRAPI_CALL CreateSession(XrInstance,XrSessionCreateInfo const*,XrSession*);
 static XrResult XRAPI_CALL DestroySession(XrSession);
 static XrResult XRAPI_CALL BeginSession(XrSession,XrSessionBeginInfo const*);
 static XrResult XRAPI_CALL EndSession(XrSession);
 static XrResult XRAPI_CALL RequestExitSession(XrSession);
 static XrResult XRAPI_CALL PollEvent(XrInstance,XrEventDataBuffer*);
 static XrResult XRAPI_CALL CreateReferenceSpace(XrSession,XrReferenceSpaceCreateInfo const*,XrSpace*);
 static XrResult XRAPI_CALL DestroySpace(XrSpace);
 static XrResult XRAPI_CALL EnumerateViewConfigurationViews(XrInstance,XrSystemId,XrViewConfigurationType,uint32_t,uint32_t*,XrViewConfigurationView*);
 static XrResult XRAPI_CALL EnumerateEnvironmentBlendModes(XrInstance,XrSystemId,XrViewConfigurationType,uint32_t,uint32_t*,XrEnvironmentBlendMode*);
 static XrResult XRAPI_CALL EnumerateSwapchainFormats(XrSession,uint32_t,uint32_t*,int64_t*);
 static XrResult XRAPI_CALL CreateSwapchain(XrSession,XrSwapchainCreateInfo const*,XrSwapchain*);
 static XrResult XRAPI_CALL DestroySwapchain(XrSwapchain);
 static XrResult XRAPI_CALL EnumerateSwapchainImages(XrSwapchain,uint32_t,uint32_t*,XrSwapchainImageBaseHeader*);
 static XrResult XRAPI_CALL AcquireSwapchainImage(XrSwapchain,XrSwapchainImageAcquireInfo const*,uint32_t*);
 static XrResult XRAPI_CALL WaitSwapchainImage(XrSwapchain,XrSwapchainImageWaitInfo const*);
 static XrResult XRAPI_CALL ReleaseSwapchainImage(XrSwapchain,XrSwapchainImageReleaseInfo const*);
 static XrResult XRAPI_CALL WaitFrame(XrSession,XrFrameWaitInfo const*,XrFrameState*);
 static XrResult XRAPI_CALL BeginFrame(XrSession,XrFrameBeginInfo const*);
 static XrResult XRAPI_CALL EndFrame(XrSession,XrFrameEndInfo const*);
 static XrResult XRAPI_CALL LocateViews(XrSession,XrViewLocateInfo const*,XrViewState*,uint32_t,uint32_t*,XrView*);
 static XrResult XRAPI_CALL ResultToString(XrInstance,XrResult,char[XR_MAX_RESULT_STRING_SIZE]);
 static XrResult XRAPI_CALL ApplyHapticFeedback(XrSession,XrHapticActionInfo const*,XrHapticBaseHeader const*);
 static XrResult XRAPI_CALL StringToPath(XrInstance,char const*,XrPath*);
 static XrResult XRAPI_CALL CreateActionSet(XrInstance,XrActionSetCreateInfo const*,XrActionSet*);
 static XrResult XRAPI_CALL DestroyActionSet(XrActionSet);
 static XrResult XRAPI_CALL CreateAction(XrActionSet,XrActionCreateInfo const*,XrAction*);
 static XrResult XRAPI_CALL SuggestInteractionProfileBindings(XrInstance,XrInteractionProfileSuggestedBinding const*);
 static XrResult XRAPI_CALL AttachSessionActionSets(XrSession,XrSessionActionSetsAttachInfo const*);
 static XrResult XRAPI_CALL CreateActionSpace(XrSession,XrActionSpaceCreateInfo const*,XrSpace*);
 static XrResult XRAPI_CALL SyncActions(XrSession,XrActionsSyncInfo const*);
 static XrResult XRAPI_CALL GetActionStatePose(XrSession,XrActionStateGetInfo const*,XrActionStatePose*);
 static XrResult XRAPI_CALL GetActionStateBoolean(XrSession,XrActionStateGetInfo const*,XrActionStateBoolean*);
 static XrResult XRAPI_CALL GetActionStateFloat(XrSession,XrActionStateGetInfo const*,XrActionStateFloat*);
 static XrResult XRAPI_CALL GetActionStateVector2f(XrSession,XrActionStateGetInfo const*,XrActionStateVector2f*);
 static XrResult XRAPI_CALL LocateSpace(XrSpace,XrSpace,XrTime,XrSpaceLocation*);
};
}
