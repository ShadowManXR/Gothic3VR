#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11_1.h>
#include <openxr/openxr_platform.h>
#include "Client.h"
#include "../OpenXrRuntimeSelection.h"
#include <bcrypt.h>
#include <cwchar>
#include <algorithm>
namespace diagnostic::meta_bridge {
namespace {
template<class...T>Writer Args(T const&...v){Writer w;Put(w,v...);return w;}
void Hr(HRESULT hr,char const* message){if(FAILED(hr))throw std::runtime_error(message);}
std::wstring Quote(std::filesystem::path const& p){auto s=p.wstring();if(s.find(L'\"')!=s.npos)throw std::runtime_error("Invalid Meta host path");return L"\""+s+L"\"";}
std::vector<wchar_t> Environment(){
 auto* source=GetEnvironmentStringsW();if(!source)throw std::runtime_error("Cannot read Meta host environment");std::vector<wchar_t> out;
 for(auto p=source;*p;p+=wcslen(p)+1)if(_wcsnicmp(p,L"__COMPAT_LAYER=",15)!=0){auto n=wcslen(p)+1;out.insert(out.end(),p,p+n);}
 FreeEnvironmentStringsW(source);out.push_back(0);return out;
}
}
Client::Client(std::filesystem::path const& loader32,std::filesystem::path const& logFile,std::filesystem::path const& manifest32){
  try{if(current)throw std::runtime_error("A Meta bridge is already connected");auto folder=loader32.parent_path()/L"meta64";auto host=folder/L"Gothic3VRMetaHost64.exe",loader=folder/L"openxr_loader64.dll";
  if(!std::filesystem::is_regular_file(host)||!std::filesystem::is_regular_file(loader))throw std::runtime_error("Meta 64-bit host is not installed");
  auto source=manifest32.empty()?xr_runtime::Select(OpenXrRuntimeChoice::MetaLink):manifest32;auto manifest=source.parent_path()/L"oculus_openxr_64.json";xr_runtime::ValidateManifest(manifest,IMAGE_FILE_MACHINE_AMD64);
  uint8_t random[16];if(BCryptGenRandom(nullptr,random,sizeof(random),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Cannot create unique Meta bridge name");wchar_t token[33]{};for(unsigned i=0;i<16;++i)swprintf_s(token+i*2,3,L"%02x",random[i]);std::wstring name=L"\\\\.\\pipe\\G3VR_Meta64_"+std::wstring(token);
  auto hostLog=logFile.parent_path()/(logFile.stem().wstring()+L"-meta64.jsonl");
  auto command=Quote(host)+L" --pipe \""+name+L"\" --loader "+Quote(loader)+L" --manifest "+Quote(manifest)+L" --log "+Quote(hostLog);
  auto env=Environment();STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
  job=CreateJobObjectW(nullptr,nullptr);if(!job)throw std::runtime_error("Cannot create Meta host job");JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit{};limit.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
  if(!SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limit,sizeof(limit)))throw std::runtime_error("Cannot protect Meta host lifetime");
  if(!CreateProcessW(host.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_UNICODE_ENVIRONMENT|CREATE_NO_WINDOW|CREATE_SUSPENDED,env.data(),folder.c_str(),&startup,&child))throw std::runtime_error("Cannot start Meta 64-bit host");process=child.hProcess;
  if(!AssignProcessToJobObject(job,process)){TerminateProcess(process,1);CloseHandle(child.hThread);throw std::runtime_error("Cannot bind Meta host lifetime to the game");}
  ResumeThread(child.hThread);CloseHandle(child.hThread);pipe.Connect(name,process);
  auto result=Call(Command::Hello,{},[&](Reader& r,XrResult){instance=FromHandle<XrInstance>(r.Pod<uint64_t>());});if(XR_FAILED(result))throw std::runtime_error(lastError.empty()?"Meta 64-bit runtime initialization failed":lastError);current=this;
 }catch(...){Cleanup();throw;}
}
void Client::Cleanup()noexcept{pipe.Reset();if(process)WaitForSingleObject(process,2000);if(job){CloseHandle(job);job=nullptr;}if(process){CloseHandle(process);process=nullptr;}if(current==this)current=nullptr;}
Client::~Client(){if(!destroyed)Call(Command::DestroyInstance);Cleanup();}
XrResult XRAPI_CALL Client::DestroyInstance(XrInstance){auto result=current->Call(Command::DestroyInstance);current->destroyed=true;return result;}
XrResult XRAPI_CALL Client::GetInstanceProperties(XrInstance,XrInstanceProperties* out){return current->Call(Command::InstanceProperties,{},[&](Reader& r,XrResult){out->runtimeVersion=r.Pod<XrVersion>();TakeText(r,out->runtimeName);});}
XrResult XRAPI_CALL Client::GetSystem(XrInstance,XrSystemGetInfo const* in,XrSystemId* out){return current->Call(Command::GetSystem,Args(in->formFactor),[&](Reader& r,XrResult){*out=r.Pod<XrSystemId>();});}
XrResult XRAPI_CALL Client::GetSystemProperties(XrInstance,XrSystemId id,XrSystemProperties* out){return current->Call(Command::SystemProperties,Args(id),[&](Reader& r,XrResult){Take(r,out->systemId,out->vendorId);TakeText(r,out->systemName);Take(r,out->graphicsProperties.maxSwapchainImageHeight,out->graphicsProperties.maxSwapchainImageWidth,out->graphicsProperties.maxLayerCount,out->trackingProperties.orientationTracking,out->trackingProperties.positionTracking);});}
XrResult XRAPI_CALL Client::GetD3D11GraphicsRequirementsKHR(XrInstance,XrSystemId id,XrGraphicsRequirementsD3D11KHR* out){return current->Call(Command::GraphicsRequirements,Args(id),[&](Reader& r,XrResult){Take(r,out->adapterLuid.LowPart,out->adapterLuid.HighPart);out->minFeatureLevel=D3D_FEATURE_LEVEL(r.Pod<uint32_t>());});}
XrResult XRAPI_CALL Client::CreateSession(XrInstance,XrSessionCreateInfo const* in,XrSession* out){
 auto* binding=static_cast<XrGraphicsBindingD3D11KHR const*>(in->next);if(!binding||binding->type!=XR_TYPE_GRAPHICS_BINDING_D3D11_KHR||!binding->device)return XR_ERROR_GRAPHICS_DEVICE_INVALID;
 Hr(binding->device->QueryInterface(IID_PPV_ARGS(&current->device)),"Meta client needs D3D11.1 shared resource access");binding->device->GetImmediateContext(&current->context);
 return current->Call(Command::CreateSession,Args(in->createFlags,in->systemId),[&](Reader& r,XrResult){*out=FromHandle<XrSession>(r.Pod<uint64_t>());});
}
XrResult XRAPI_CALL Client::DestroySession(XrSession){return current->Call(Command::DestroySession);}
XrResult XRAPI_CALL Client::BeginSession(XrSession,XrSessionBeginInfo const* in){return current->Call(Command::BeginSession,Args(in->primaryViewConfigurationType));}
XrResult XRAPI_CALL Client::EndSession(XrSession){return current->Call(Command::EndSession);}
XrResult XRAPI_CALL Client::RequestExitSession(XrSession){return current->Call(Command::RequestExitSession);}
XrResult XRAPI_CALL Client::PollEvent(XrInstance,XrEventDataBuffer* out){return current->Call(Command::PollEvent,{},[&](Reader& r,XrResult result){if(result==XR_SUCCESS)ReadEvent(r,*out);});}
XrResult XRAPI_CALL Client::CreateReferenceSpace(XrSession,XrReferenceSpaceCreateInfo const* in,XrSpace* out){auto w=Args(in->referenceSpaceType);Write(w,in->poseInReferenceSpace);return current->Call(Command::CreateReferenceSpace,w,[&](Reader& r,XrResult){*out=FromHandle<XrSpace>(r.Pod<uint64_t>());});}
XrResult XRAPI_CALL Client::DestroySpace(XrSpace space){return current->Call(Command::DestroySpace,Args(HandleBits(space)));}
XrResult XRAPI_CALL Client::EnumerateViewConfigurationViews(XrInstance,XrSystemId id,XrViewConfigurationType type,uint32_t cap,uint32_t* count,XrViewConfigurationView* data){return current->Call(Command::ViewConfigurationViews,Args(id,type,cap),[&](Reader& r,XrResult result){*count=r.Pod<uint32_t>();if(cap&&XR_SUCCEEDED(result)){if(*count>cap)throw std::runtime_error("Meta view count overflow");for(uint32_t n=0;n<*count;++n)Read(r,data[n]);}});}
XrResult XRAPI_CALL Client::EnumerateEnvironmentBlendModes(XrInstance,XrSystemId id,XrViewConfigurationType type,uint32_t cap,uint32_t* count,XrEnvironmentBlendMode* data){return current->Call(Command::BlendModes,Args(id,type,cap),[&](Reader& r,XrResult result){*count=r.Pod<uint32_t>();if(cap&&XR_SUCCEEDED(result)){if(*count>cap)throw std::runtime_error("Meta blend count overflow");for(uint32_t n=0;n<*count;++n)data[n]=r.Pod<XrEnvironmentBlendMode>();}});}
XrResult XRAPI_CALL Client::EnumerateSwapchainFormats(XrSession,uint32_t cap,uint32_t* count,int64_t* data){return current->Call(Command::SwapchainFormats,Args(cap),[&](Reader& r,XrResult result){*count=r.Pod<uint32_t>();if(cap&&XR_SUCCEEDED(result)){if(*count>cap)throw std::runtime_error("Meta format count overflow");for(uint32_t n=0;n<*count;++n)data[n]=r.Pod<int64_t>();}});}
XrResult XRAPI_CALL Client::CreateSwapchain(XrSession,XrSwapchainCreateInfo const* in,XrSwapchain* out){Writer w;Write(w,*in);return current->Call(Command::CreateSwapchain,w,[&](Reader& r,XrResult){auto id=r.Pod<uint64_t>();*out=FromHandle<XrSwapchain>(id);current->chains.emplace(id,Chain{});});}
XrResult XRAPI_CALL Client::DestroySwapchain(XrSwapchain chain){auto flushed=current->FlushReleased();if(XR_FAILED(flushed))return flushed;auto id=HandleBits(chain);current->chains.erase(id);return current->Call(Command::DestroySwapchain,Args(id));}
XrResult XRAPI_CALL Client::EnumerateSwapchainImages(XrSwapchain chain,uint32_t cap,uint32_t* count,XrSwapchainImageBaseHeader* output){
 auto id=HandleBits(chain);return current->Call(Command::SwapchainImages,Args(id,cap),[&](Reader& r,XrResult result){*count=r.Pod<uint32_t>();if(!cap||XR_FAILED(result))return;if(*count>cap||*count>32)throw std::runtime_error("Meta image count overflow");auto& c=current->chains.at(id);
  for(uint32_t n=0;n<*count;++n){auto name=r.Wide();if(c.textures.size()<=n){Texture image;Hr(current->device->OpenSharedResourceByName(name.c_str(),DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,IID_PPV_ARGS(&image)),"Cannot open Meta NT shared image");Mutex mutex;Hr(image.As(&mutex),"Meta shared keyed mutex unavailable");c.textures.push_back(image);c.mutexes.push_back(mutex);}auto* images=reinterpret_cast<XrSwapchainImageD3D11KHR*>(output);if(images[n].type!=XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR)throw std::runtime_error("Meta image type mismatch");images[n].texture=c.textures[n].Get();}
 });
}
XrResult XRAPI_CALL Client::AcquireSwapchainImage(XrSwapchain chain,XrSwapchainImageAcquireInfo const*,uint32_t* index){auto id=HandleBits(chain);auto found=current->chains.find(id);if(found==current->chains.end())return XR_ERROR_HANDLE_INVALID;if(found->second.acquired&&found->second.released){auto flushed=current->FlushReleased();if(XR_FAILED(flushed))return flushed;}return current->Call(Command::AcquireImage,Args(id),[&](Reader& r,XrResult){auto& c=current->chains.at(id);c.index=r.Pod<uint32_t>();if(c.index>=c.textures.size()||c.acquired&&!c.released)throw std::runtime_error("Invalid Meta acquired image");if(c.mutexes[c.index]->AcquireSync(0,5000)!=S_OK)throw std::runtime_error("Meta shared image ownership timed out");c.acquired=true;c.released=false;*index=c.index;});}
XrResult XRAPI_CALL Client::WaitSwapchainImage(XrSwapchain chain,XrSwapchainImageWaitInfo const*){auto found=current->chains.find(HandleBits(chain));return found!=current->chains.end()&&found->second.acquired&&!found->second.released?XR_SUCCESS:XR_ERROR_CALL_ORDER_INVALID;}
XrResult XRAPI_CALL Client::ReleaseSwapchainImage(XrSwapchain chain,XrSwapchainImageReleaseInfo const*){try{auto id=HandleBits(chain);auto& c=current->chains.at(id);if(!c.acquired||c.released)return XR_ERROR_CALL_ORDER_INVALID;current->context->Flush();Hr(c.mutexes[c.index]->ReleaseSync(1),"Meta shared image release failed");c.released=true;current->releases.push_back(id);return XR_SUCCESS;}catch(std::exception const& e){current->lastError=e.what();return XR_ERROR_RUNTIME_FAILURE;}}
XrResult Client::FlushReleased()noexcept{try{if(releases.empty())return XR_SUCCESS;Writer w;w.Pod(uint32_t(releases.size()));for(auto id:releases)w.Pod(id);auto result=Call(Command::ReleaseImage,w);if(XR_SUCCEEDED(result)){for(auto id:releases)chains.at(id).acquired=false;releases.clear();}return result;}catch(std::exception const& e){lastError=e.what();return XR_ERROR_RUNTIME_FAILURE;}}
XrResult XRAPI_CALL Client::WaitFrame(XrSession,XrFrameWaitInfo const*,XrFrameState* out){current->actionSnapshot.clear();current->poseSnapshot.clear();return current->Call(Command::WaitFrame,{},[&](Reader& r,XrResult){Read(r,*out);});}
XrResult XRAPI_CALL Client::BeginFrame(XrSession,XrFrameBeginInfo const*){return current->Call(Command::BeginFrame);}
XrResult XRAPI_CALL Client::EndFrame(XrSession,XrFrameEndInfo const* in){auto w=Args(uint32_t(current->releases.size()));for(auto id:current->releases)w.Pod(id);WriteFrame(w,*in);auto result=current->Call(Command::EndFrame,w);for(auto id:current->releases)current->chains.at(id).acquired=false;current->releases.clear();return result;}
XrResult XRAPI_CALL Client::LocateViews(XrSession,XrViewLocateInfo const* in,XrViewState* state,uint32_t cap,uint32_t* count,XrView* data){return current->Call(Command::LocateViews,Args(in->viewConfigurationType,in->displayTime,HandleBits(in->space),cap),[&](Reader& r,XrResult result){Take(r,state->viewStateFlags,*count);if(cap&&XR_SUCCEEDED(result)){if(*count>cap)throw std::runtime_error("Meta located view overflow");for(uint32_t n=0;n<*count;++n)Read(r,data[n]);}});}
XrResult XRAPI_CALL Client::ResultToString(XrInstance,XrResult result,char text[XR_MAX_RESULT_STRING_SIZE]){if(result==XR_ERROR_RUNTIME_FAILURE&&!current->lastError.empty()){strncpy_s(text,XR_MAX_RESULT_STRING_SIZE,current->lastError.c_str(),_TRUNCATE);return XR_SUCCESS;}return current->Call(Command::ResultString,Args(result),[&](Reader& r,XrResult){auto s=r.Text();strncpy_s(text,XR_MAX_RESULT_STRING_SIZE,s.c_str(),_TRUNCATE);});}
XrResult XRAPI_CALL Client::StringToPath(XrInstance,char const* name,XrPath* out){Writer w;w.Text(name);return current->Call(Command::StringToPath,w,[&](Reader& r,XrResult){*out=r.Pod<XrPath>();});}
XrResult XRAPI_CALL Client::CreateActionSet(XrInstance,XrActionSetCreateInfo const* in,XrActionSet* out){Writer w;w.Text(in->actionSetName);w.Text(in->localizedActionSetName);w.Pod(in->priority);return current->Call(Command::CreateActionSet,w,[&](Reader& r,XrResult){*out=FromHandle<XrActionSet>(r.Pod<uint64_t>());});}
XrResult XRAPI_CALL Client::DestroyActionSet(XrActionSet set){return current->Call(Command::DestroyActionSet,Args(HandleBits(set)));}
XrResult XRAPI_CALL Client::CreateAction(XrActionSet set,XrActionCreateInfo const* in,XrAction* out){auto w=Args(HandleBits(set));w.Text(in->actionName);w.Text(in->localizedActionName);Put(w,in->actionType,BoundedCount(in->countSubactionPaths,8));for(uint32_t n=0;n<in->countSubactionPaths;++n)w.Pod(in->subactionPaths[n]);return current->Call(Command::CreateAction,w,[&](Reader& r,XrResult){*out=FromHandle<XrAction>(r.Pod<uint64_t>());});}
XrResult XRAPI_CALL Client::SuggestInteractionProfileBindings(XrInstance,XrInteractionProfileSuggestedBinding const* in){auto w=Args(in->interactionProfile,BoundedCount(in->countSuggestedBindings));for(uint32_t n=0;n<in->countSuggestedBindings;++n)Put(w,HandleBits(in->suggestedBindings[n].action),in->suggestedBindings[n].binding);return current->Call(Command::SuggestBindings,w);}
XrResult XRAPI_CALL Client::AttachSessionActionSets(XrSession,XrSessionActionSetsAttachInfo const* in){auto w=Args(BoundedCount(in->countActionSets,16));for(uint32_t n=0;n<in->countActionSets;++n)w.Pod(HandleBits(in->actionSets[n]));return current->Call(Command::AttachActionSets,w);}
XrResult XRAPI_CALL Client::CreateActionSpace(XrSession,XrActionSpaceCreateInfo const* in,XrSpace* out){auto w=Args(HandleBits(in->action),in->subactionPath);Write(w,in->poseInActionSpace);return current->Call(Command::CreateActionSpace,w,[&](Reader& r,XrResult){*out=FromHandle<XrSpace>(r.Pod<uint64_t>());});}
XrResult XRAPI_CALL Client::SyncActions(XrSession,XrActionsSyncInfo const* in){auto w=Args(BoundedCount(in->countActiveActionSets,16));for(uint32_t n=0;n<in->countActiveActionSets;++n)Put(w,HandleBits(in->activeActionSets[n].actionSet),in->activeActionSets[n].subactionPath);current->actionSnapshot.clear();current->poseSnapshot.clear();
 return current->Call(Command::SyncActions,w,[&](Reader& r,XrResult result){if(result!=XR_SUCCESS)return;current->poseSnapshotTime=r.Pod<XrTime>();auto actions=BoundedCount(r.Pod<uint32_t>());for(uint32_t i=0;i<actions;++i){auto s=r.Pod<ActionState>();current->actionSnapshot[{s.action,s.path}]=s;}auto poses=BoundedCount(r.Pod<uint32_t>());for(uint32_t i=0;i<poses;++i){auto s=r.Pod<LocatedSpace>();current->poseSnapshot[{s.space,s.base}]=s;}});
}
XrResult XRAPI_CALL Client::GetActionStatePose(XrSession,XrActionStateGetInfo const* info,XrActionStatePose* out){return State(Command::PoseState,info,out);}
XrResult XRAPI_CALL Client::GetActionStateBoolean(XrSession,XrActionStateGetInfo const* info,XrActionStateBoolean* out){return State(Command::BooleanState,info,out);}
XrResult XRAPI_CALL Client::GetActionStateFloat(XrSession,XrActionStateGetInfo const* info,XrActionStateFloat* out){return State(Command::FloatState,info,out);}
XrResult XRAPI_CALL Client::GetActionStateVector2f(XrSession,XrActionStateGetInfo const* info,XrActionStateVector2f* out){return State(Command::VectorState,info,out);}
XrResult XRAPI_CALL Client::LocateSpace(XrSpace space,XrSpace base,XrTime time,XrSpaceLocation* out){auto found=current->poseSnapshot.find({HandleBits(space),HandleBits(base)});if(time==current->poseSnapshotTime&&found!=current->poseSnapshot.end()){auto const& s=found->second;out->locationFlags=s.flags;out->pose.orientation={s.orientation[0],s.orientation[1],s.orientation[2],s.orientation[3]};out->pose.position={s.position[0],s.position[1],s.position[2]};return XR_SUCCESS;}return current->Call(Command::LocateSpace,Args(HandleBits(space),HandleBits(base),time),[&](Reader& r,XrResult){Read(r,*out);});}
XrResult XRAPI_CALL Client::ApplyHapticFeedback(XrSession,XrHapticActionInfo const* info,XrHapticBaseHeader const* haptic){if(haptic->type!=XR_TYPE_HAPTIC_VIBRATION)return XR_ERROR_VALIDATION_FAILURE;auto& v=*reinterpret_cast<XrHapticVibration const*>(haptic);return current->Call(Command::Haptic,Args(HandleBits(info->action),info->subactionPath,v.duration,v.frequency,v.amplitude));}
}
