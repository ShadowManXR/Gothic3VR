#include "StereoOpenXrApi.h"
#include "meta_bridge/Pipe.h"
#include "meta_bridge/Layers.h"
#include "meta_bridge/FrameTiming.h"
#include "meta_bridge/FrameWaiter.h"
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <map>
#include <memory>
using namespace diagnostic::meta_bridge;
using stereo::ComPtr;
struct SharedImage {
 ComPtr<ID3D11Texture2D> texture;ComPtr<IDXGIKeyedMutex> mutex;HANDLE share=nullptr;std::wstring name;
 ~SharedImage(){if(share)CloseHandle(share);}
};
struct Chain {
 XrSwapchain handle=XR_NULL_HANDLE;std::vector<XrSwapchainImageD3D11KHR> xrImages;std::vector<std::shared_ptr<SharedImage>> images;bool acquired=false;uint32_t index=0,ordinal=0;
};
struct Server {
 g3vr::Log log;g3vr::Api api;Pipe pipe;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;ComPtr<ID3D11Query> fence;
 std::map<uint64_t,Chain> chains;XrSession session=XR_NULL_HANDLE;std::wstring token;bool quit=false,begun=false;XrTime predicted=0;uint64_t submitted=0,copiedImages=0;uint32_t nextChain=0;
 struct Action {XrAction handle=XR_NULL_HANDLE;XrActionType type=XR_ACTION_TYPE_BOOLEAN_INPUT;std::vector<XrPath> paths;};
 std::vector<Action> actions;std::vector<XrSpace> actionSpaces;std::map<XrReferenceSpaceType,XrSpace> referenceSpaces;
 FrameTiming timing;PFN_xrConvertTimeToWin32PerformanceCounterKHR displayClock=nullptr;
 FrameWaiter frameWaiter;bool prefetchAllowed=false;uint64_t staleWaits=0;
 static double NowMs(){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return double(t.QuadPart)*1000./double(f.QuadPart);}
 void TimingReport(){if(timing.Count()){auto report=timing.Report();if(!report.empty())log.Line(report);}}
 WaitedFrame NativeWait(){WaitedFrame f;f.startedMs=NowMs();XrFrameWaitInfo i{XR_TYPE_FRAME_WAIT_INFO};f.result=api.WaitFrame(session,&i,&f.state);f.finishedMs=NowMs();return f;}
 bool DisplayMs(XrTime time,double& ms){LARGE_INTEGER display{},frequency{};if(!displayClock||XR_FAILED(displayClock(api.instance,time,&display)))return false;QueryPerformanceFrequency(&frequency);ms=double(display.QuadPart)*1000./double(frequency.QuadPart);return true;}
 void StopWaiter(){prefetchAllowed=false;if(!frameWaiter.Stop(1500)){try{log.Line("{\"type\":\"meta_waiter_shutdown_timeout\",\"owned_host_exit\":true}");}catch(...){}ExitProcess(1);}}
 Server(std::wstring const& name,std::wstring const& file):log(file),token(name.substr(name.find_last_of(L'\\')+1)){pipe.Serve(name);}
 ~Server(){StopWaiter();try{if(begun&&session){XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=predicted;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;api.EndFrame(session,&end);}for(auto& pair:chains){auto& c=pair.second;if(c.acquired){XrSwapchainImageReleaseInfo r{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};api.ReleaseSwapchainImage(c.handle,&r);}c.images.clear();api.DestroySwapchain(c.handle);}chains.clear();if(session)api.DestroySession(session);}catch(...){} }
 void Device(XrGraphicsRequirementsD3D11KHR const& req){if(device)return;ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"Meta DXGI factory");ComPtr<IDXGIAdapter1> adapter;
  for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> a;auto hr=factory->EnumAdapters1(i,&a);if(hr==DXGI_ERROR_NOT_FOUND)break;stereo::Hr(hr,"Meta adapter enumeration");DXGI_ADAPTER_DESC1 d{};a->GetDesc1(&d);if(!memcmp(&d.AdapterLuid,&req.adapterLuid,sizeof(LUID))){adapter=a;break;}}
  if(!adapter)throw std::runtime_error("Meta GPU not found");stereo::Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,&req.minFeatureLevel,1,D3D11_SDK_VERSION,&device,nullptr,&context),"Meta D3D11 device");D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};stereo::Hr(device->CreateQuery(&q,&fence),"Meta GPU fence");
 }
 void CreateShared(Chain& chain,XrSwapchainCreateInfo const& info){
  uint32_t count=0;api.Check(api.EnumerateSwapchainImages(chain.handle,0,&count,nullptr),"Meta image count");BoundedCount(count,32);chain.xrImages.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});api.Check(api.EnumerateSwapchainImages(chain.handle,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(chain.xrImages.data())),"Meta images");
  if(info.format!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||info.arraySize!=1||info.sampleCount!=1)throw std::runtime_error("Unsupported Meta bridge swapchain format");
  // A synchronous bridge has one image in flight per swapchain. Share one GPU
  // allocation across its XR slots instead of multiplying 4K staging memory.
  for(uint32_t i=0;i<count;++i){if(i){chain.images.push_back(chain.images.front());continue;}auto image=std::make_shared<SharedImage>();image->name=L"Local\\"+token+L"_"+std::to_wstring(HandleBits(chain.handle))+L"_"+std::to_wstring(i);
   D3D11_TEXTURE2D_DESC d{};d.Width=info.width;d.Height=info.height;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_B8G8R8A8_TYPELESS;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
   stereo::Hr(device->CreateTexture2D(&d,nullptr,&image->texture),"Meta NT shared texture");stereo::Hr(image->texture.As(&image->mutex),"Meta keyed mutex");ComPtr<IDXGIResource1> resource;stereo::Hr(image->texture.As(&resource),"Meta shared resource");stereo::Hr(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,image->name.c_str(),&image->share),"Meta NT texture handle");chain.images.push_back(std::move(image));
  }
 }
 void CompleteImages(std::vector<uint64_t> const& released){
  std::vector<Chain*> locked;locked.reserve(released.size());
  try{for(auto id:released){auto& c=chains.at(id);if(!c.acquired||c.index>=c.images.size())throw std::runtime_error("Meta image was not acquired");auto& image=*c.images[c.index];if(image.mutex->AcquireSync(1,5000)!=S_OK)throw std::runtime_error("Meta shared image ownership timed out");locked.push_back(&c);context->CopyResource(c.xrImages[c.index].texture,image.texture.Get());}
   if(!locked.empty()){context->End(fence.Get());context->Flush();auto deadline=GetTickCount64()+5000;for(;;){auto hr=context->GetData(fence.Get(),nullptr,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);if(hr==S_OK)break;stereo::Hr(hr,"Meta GPU copy completion");if(GetTickCount64()>=deadline)throw std::runtime_error("Meta GPU copy timed out");Sleep(0);}}
  }catch(...){for(auto* c:locked)c->images[c->index]->mutex->ReleaseSync(0);throw;}
  for(auto* c:locked){
#ifdef G3VR_META_GPU_FIXTURE
   D3D11_TEXTURE2D_DESC d{};c->xrImages[c->index].texture->GetDesc(&d);if(d.Width!=256||d.Height!=256)throw std::runtime_error("GPU fixture requires 256-pixel test textures");d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d.BindFlags=d.MiscFlags=0;ComPtr<ID3D11Texture2D> readback;stereo::Hr(device->CreateTexture2D(&d,nullptr,&readback),"fixture staging");context->CopyResource(readback.Get(),c->xrImages[c->index].texture);D3D11_MAPPED_SUBRESOURCE mapped{};stereo::Hr(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"fixture map");bool valid=true;for(unsigned y:{0u,127u,255u})for(unsigned x:{0u,127u,255u}){auto pixel=static_cast<uint8_t const*>(mapped.pData)+y*mapped.RowPitch+x*4;uint8_t expected[4]={uint8_t(c->ordinal<2?255:0),0,0,uint8_t(c->ordinal<2?255:0)};valid=valid&&!memcmp(pixel,expected,4);}context->Unmap(readback.Get(),0);if(!valid)throw std::runtime_error("Meta fixture pixel mismatch after cross-process GPU copy");log.Line("{\"type\":\"meta_fixture_pixels_verified\",\"ordinal\":"+std::to_string(c->ordinal)+"}");
#endif
   ++copiedImages;stereo::Hr(c->images[c->index]->mutex->ReleaseSync(0),"Meta release shared image");XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};api.Check(api.ReleaseSwapchainImage(c->handle,&release),"Meta release XR image");c->acquired=false;}
 }
 void ActionSnapshot(Writer& out){
  std::vector<ActionState> states;std::vector<LocatedSpace> poses;
  for(auto const& action:actions)for(auto path:action.paths){ActionState s{};s.action=HandleBits(action.handle);s.path=path;XrActionStateGetInfo info{XR_TYPE_ACTION_STATE_GET_INFO};info.action=action.handle;info.subactionPath=path;
   if(action.type==XR_ACTION_TYPE_POSE_INPUT){XrActionStatePose v{XR_TYPE_ACTION_STATE_POSE};api.Check(api.GetActionStatePose(session,&info,&v),"Meta pose snapshot");s.active=v.isActive;}
   else if(action.type==XR_ACTION_TYPE_BOOLEAN_INPUT){XrActionStateBoolean v{XR_TYPE_ACTION_STATE_BOOLEAN};api.Check(api.GetActionStateBoolean(session,&info,&v),"Meta button snapshot");s.active=v.isActive;s.changed=v.changedSinceLastSync;s.time=v.lastChangeTime;s.x=float(v.currentState);}
   else if(action.type==XR_ACTION_TYPE_FLOAT_INPUT){XrActionStateFloat v{XR_TYPE_ACTION_STATE_FLOAT};api.Check(api.GetActionStateFloat(session,&info,&v),"Meta float snapshot");s.active=v.isActive;s.changed=v.changedSinceLastSync;s.time=v.lastChangeTime;s.x=v.currentState;}
   else if(action.type==XR_ACTION_TYPE_VECTOR2F_INPUT){XrActionStateVector2f v{XR_TYPE_ACTION_STATE_VECTOR2F};api.Check(api.GetActionStateVector2f(session,&info,&v),"Meta stick snapshot");s.active=v.isActive;s.changed=v.changedSinceLastSync;s.time=v.lastChangeTime;s.x=v.currentState.x;s.y=v.currentState.y;}
   else continue;states.push_back(s);
  }
  for(auto space:actionSpaces)for(auto base:referenceSpaces){if(base.first!=XR_REFERENCE_SPACE_TYPE_LOCAL&&base.first!=XR_REFERENCE_SPACE_TYPE_VIEW)continue;XrSpaceLocation v{XR_TYPE_SPACE_LOCATION};api.Check(api.LocateSpace(space,base.second,predicted,&v),"Meta controller snapshot");LocatedSpace s{};s.space=HandleBits(space);s.base=HandleBits(base.second);s.flags=v.locationFlags;s.orientation[0]=v.pose.orientation.x;s.orientation[1]=v.pose.orientation.y;s.orientation[2]=v.pose.orientation.z;s.orientation[3]=v.pose.orientation.w;s.position[0]=v.pose.position.x;s.position[1]=v.pose.position.y;s.position[2]=v.pose.position.z;poses.push_back(s);}
  Put(out,predicted,uint32_t(states.size()));for(auto const& s:states)out.Pod(s);out.Pod(uint32_t(poses.size()));for(auto const& s:poses)out.Pod(s);
 }
 XrResult Execute(Command command,Reader& in,Writer& out,std::wstring const& loader,std::wstring const& manifest){
  auto instance=api.instance;
  switch(command){
  case Command::Hello:{auto request=std::filesystem::path(loader).parent_path()/L"meta-frame-timing.request";if(DeleteFileW(request.c_str())){timing.Enable(NowMs());log.Line("{\"type\":\"meta_frame_timing_armed\",\"max_seconds\":120,\"one_shot\":true,\"pixel_readback\":false}");}api.Initialize(loader,log,diagnostic::OpenXrRuntimeChoice::MetaLink,manifest,true);PFN_xrVoidFunction function=nullptr;if(XR_SUCCEEDED(api.get(api.instance,"xrConvertTimeToWin32PerformanceCounterKHR",&function)))displayClock=reinterpret_cast<PFN_xrConvertTimeToWin32PerformanceCounterKHR>(function);log.Line("{\"type\":\"meta_frame_pipeline\",\"prefetch\":\"wait_only\",\"cached_poses\":false,\"queued_images\":false}");out.Pod(HandleBits(api.instance));return XR_SUCCESS;}
  case Command::DestroyInstance:quit=true;return XR_SUCCESS;
  case Command::InstanceProperties:{XrInstanceProperties p{XR_TYPE_INSTANCE_PROPERTIES};auto r=api.GetInstanceProperties(instance,&p);if(XR_SUCCEEDED(r)){out.Pod(p.runtimeVersion);out.Text(p.runtimeName);}return r;}
  case Command::GetSystem:{XrSystemGetInfo i{XR_TYPE_SYSTEM_GET_INFO};i.formFactor=in.Pod<XrFormFactor>();XrSystemId id=0;auto r=api.GetSystem(instance,&i,&id);if(XR_SUCCEEDED(r))out.Pod(id);return r;}
  case Command::SystemProperties:{auto id=in.Pod<XrSystemId>();XrSystemProperties p{XR_TYPE_SYSTEM_PROPERTIES};auto r=api.GetSystemProperties(instance,id,&p);if(XR_SUCCEEDED(r)){Put(out,p.systemId,p.vendorId);out.Text(p.systemName);Put(out,p.graphicsProperties.maxSwapchainImageHeight,p.graphicsProperties.maxSwapchainImageWidth,p.graphicsProperties.maxLayerCount,p.trackingProperties.orientationTracking,p.trackingProperties.positionTracking);}return r;}
  case Command::GraphicsRequirements:{auto id=in.Pod<XrSystemId>();XrGraphicsRequirementsD3D11KHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};auto r=api.GetD3D11GraphicsRequirementsKHR(instance,id,&req);if(XR_SUCCEEDED(r)){Device(req);Put(out,req.adapterLuid.LowPart,req.adapterLuid.HighPart,uint32_t(req.minFeatureLevel));}return r;}
  case Command::CreateSession:{XrSessionCreateInfo i{XR_TYPE_SESSION_CREATE_INFO};Take(in,i.createFlags,i.systemId);XrGraphicsBindingD3D11KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};binding.device=device.Get();i.next=&binding;auto r=api.CreateSession(instance,&i,&session);if(XR_SUCCEEDED(r))out.Pod(HandleBits(session));return r;}
  case Command::DestroySession:{StopWaiter();auto r=api.DestroySession(session);if(XR_SUCCEEDED(r))session=XR_NULL_HANDLE;return r;}
  case Command::BeginSession:{XrSessionBeginInfo i{XR_TYPE_SESSION_BEGIN_INFO};i.primaryViewConfigurationType=in.Pod<XrViewConfigurationType>();auto r=api.BeginSession(session,&i);if(r==XR_SUCCESS){frameWaiter.Start([this]{return NativeWait();});prefetchAllowed=true;}return r;}
  case Command::EndSession:StopWaiter();return api.EndSession(session);
  case Command::RequestExitSession:{auto r=api.RequestExitSession(session);StopWaiter();return r;}
  case Command::PollEvent:{XrEventDataBuffer e{XR_TYPE_EVENT_DATA_BUFFER};auto r=api.PollEvent(instance,&e);if(r==XR_SUCCESS)WriteEvent(out,e);return r;}
  case Command::CreateReferenceSpace:{XrReferenceSpaceCreateInfo i{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};i.referenceSpaceType=in.Pod<XrReferenceSpaceType>();Read(in,i.poseInReferenceSpace);XrSpace space=0;auto r=api.CreateReferenceSpace(session,&i,&space);if(XR_SUCCEEDED(r)){out.Pod(HandleBits(space));referenceSpaces[i.referenceSpaceType]=space;}return r;}
  case Command::DestroySpace:{auto space=FromHandle<XrSpace>(in.Pod<uint64_t>());actionSpaces.erase(std::remove(actionSpaces.begin(),actionSpaces.end(),space),actionSpaces.end());for(auto it=referenceSpaces.begin();it!=referenceSpaces.end();)if(it->second==space)it=referenceSpaces.erase(it);else ++it;return api.DestroySpace(space);}
  case Command::ViewConfigurationViews:{auto id=in.Pod<XrSystemId>();auto type=in.Pod<XrViewConfigurationType>();auto cap=BoundedCount(in.Pod<uint32_t>());uint32_t count=0;std::vector<XrViewConfigurationView> data(cap,{XR_TYPE_VIEW_CONFIGURATION_VIEW});auto r=api.EnumerateViewConfigurationViews(instance,id,type,cap,&count,data.data());out.Pod(count);if(XR_SUCCEEDED(r)&&cap)for(uint32_t n=0;n<count;++n)Write(out,data.at(n));return r;}
  case Command::BlendModes:{auto id=in.Pod<XrSystemId>();auto type=in.Pod<XrViewConfigurationType>();auto cap=BoundedCount(in.Pod<uint32_t>());uint32_t count=0;std::vector<XrEnvironmentBlendMode> data(cap);auto r=api.EnumerateEnvironmentBlendModes(instance,id,type,cap,&count,data.data());out.Pod(count);if(XR_SUCCEEDED(r)&&cap)for(uint32_t n=0;n<count;++n)out.Pod(data.at(n));return r;}
  case Command::SwapchainFormats:{auto cap=BoundedCount(in.Pod<uint32_t>());uint32_t count=0;std::vector<int64_t> data(cap);auto r=api.EnumerateSwapchainFormats(session,cap,&count,data.data());out.Pod(count);if(XR_SUCCEEDED(r)&&cap)for(uint32_t n=0;n<count;++n)out.Pod(data.at(n));return r;}
  case Command::CreateSwapchain:{XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};Read(in,info);info.usageFlags|=XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;Chain chain;chain.ordinal=nextChain++;auto r=api.CreateSwapchain(session,&info,&chain.handle);if(XR_SUCCEEDED(r)){auto id=HandleBits(chain.handle);try{CreateShared(chain,info);}catch(...){api.DestroySwapchain(chain.handle);throw;}out.Pod(id);chains.emplace(id,std::move(chain));}return r;}
  case Command::DestroySwapchain:{auto id=in.Pod<uint64_t>();auto& c=chains.at(id);c.images.clear();auto r=api.DestroySwapchain(c.handle);chains.erase(id);return r;}
  case Command::SwapchainImages:{auto& c=chains.at(in.Pod<uint64_t>());auto cap=BoundedCount(in.Pod<uint32_t>());out.Pod(uint32_t(c.images.size()));if(cap&&cap<c.images.size())return XR_ERROR_SIZE_INSUFFICIENT;if(cap)for(auto& image:c.images)out.Wide(image->name);return XR_SUCCESS;}
  case Command::AcquireImage:{auto& c=chains.at(in.Pod<uint64_t>());if(c.acquired)return XR_ERROR_CALL_ORDER_INVALID;auto started=timing.Enabled()?NowMs():0;XrSwapchainImageAcquireInfo a{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};auto r=api.AcquireSwapchainImage(c.handle,&a,&c.index);if(XR_SUCCEEDED(r)){c.acquired=true;XrSwapchainImageWaitInfo w{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};w.timeout=XR_INFINITE_DURATION;r=api.WaitSwapchainImage(c.handle,&w);if(XR_SUCCEEDED(r))out.Pod(c.index);}if(timing.Enabled())timing.Acquired(NowMs()-started);return r;}
  case Command::ReleaseImage:{auto n=BoundedCount(in.Pod<uint32_t>(),32);std::vector<uint64_t> released(n);for(auto& id:released)id=in.Pod<uint64_t>();in.Complete();CompleteImages(released);return XR_SUCCESS;}
  case Command::WaitFrame:{auto started=NowMs();auto f=frameWaiter.Started()?frameWaiter.Take():NativeWait();double display=0;bool converted=XR_SUCCEEDED(f.result)&&DisplayMs(f.state.predictedDisplayTime,display);
   // A loading screen or suspended game can leave a timing result unused.
   // Complete that empty frame before waiting again; never reuse stale poses.
   bool stale=ExpiredWait(f,NowMs(),converted,display);
   if(stale){XrFrameBeginInfo b{XR_TYPE_FRAME_BEGIN_INFO};api.Check(api.BeginFrame(session,&b),"Meta discard stale wait begin");XrFrameEndInfo e{XR_TYPE_FRAME_END_INFO};e.displayTime=f.state.predictedDisplayTime;e.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;api.Check(api.EndFrame(session,&e),"Meta discard stale wait end");++staleWaits;f=frameWaiter.Started()?frameWaiter.Take():NativeWait();converted=XR_SUCCEEDED(f.result)&&DisplayMs(f.state.predictedDisplayTime,display);}
   if(XR_SUCCEEDED(f.result)){predicted=f.state.predictedDisplayTime;Write(out,f.state);if(timing.Enabled()){auto ready=NowMs();timing.Waited(started,ready,predicted,f.state.predictedDisplayPeriod,converted,display);timing.Prefetched(f.startedMs,f.finishedMs,started);}}return f.result;}
  case Command::BeginFrame:{XrFrameBeginInfo i{XR_TYPE_FRAME_BEGIN_INFO};auto r=api.BeginFrame(session,&i);if(XR_SUCCEEDED(r))begun=true;return r;}
  case Command::EndFrame:{auto received=timing.Enabled()?NowMs():0;auto n=BoundedCount(in.Pod<uint32_t>(),32);std::vector<uint64_t> released(n);for(auto& id:released)id=in.Pod<uint64_t>();FrameLayers layers(in);in.Complete();CompleteImages(released);auto copied=timing.Enabled()?NowMs():0;begun=false;auto r=api.EndFrame(session,&layers.info);if(XR_SUCCEEDED(r)&&layers.info.layerCount)++submitted;if(timing.Enabled())timing.Submitted(received,copied,NowMs(),!layers.projections.empty(),XR_SUCCEEDED(r));if(r==XR_SUCCESS&&prefetchAllowed&&frameWaiter.Started())frameWaiter.Schedule();return r;}
  case Command::LocateViews:{XrViewLocateInfo i{XR_TYPE_VIEW_LOCATE_INFO};Take(in,i.viewConfigurationType,i.displayTime);i.space=FromHandle<XrSpace>(in.Pod<uint64_t>());auto cap=BoundedCount(in.Pod<uint32_t>(),2);XrViewState state{XR_TYPE_VIEW_STATE};uint32_t count=0;std::vector<XrView> views(cap,{XR_TYPE_VIEW});auto r=api.LocateViews(session,&i,&state,cap,&count,views.data());Put(out,state.viewStateFlags,count);if(XR_SUCCEEDED(r)&&cap)for(uint32_t n=0;n<count;++n)Write(out,views.at(n));return r;}
  case Command::ResultString:{char text[XR_MAX_RESULT_STRING_SIZE]{};auto r=api.ResultToString(instance,in.Pod<XrResult>(),text);if(XR_SUCCEEDED(r))out.Text(text);return r;}
  case Command::StringToPath:{auto name=in.Text();XrPath path=0;auto r=api.StringToPath(instance,name.c_str(),&path);if(XR_SUCCEEDED(r))out.Pod(path);return r;}
  case Command::CreateActionSet:{XrActionSetCreateInfo i{XR_TYPE_ACTION_SET_CREATE_INFO};TakeText(in,i.actionSetName);TakeText(in,i.localizedActionSetName);i.priority=in.Pod<uint32_t>();XrActionSet set=0;auto r=api.CreateActionSet(instance,&i,&set);if(XR_SUCCEEDED(r))out.Pod(HandleBits(set));return r;}
  case Command::DestroyActionSet:actions.clear();return api.DestroyActionSet(FromHandle<XrActionSet>(in.Pod<uint64_t>()));
  case Command::CreateAction:{auto set=FromHandle<XrActionSet>(in.Pod<uint64_t>());XrActionCreateInfo i{XR_TYPE_ACTION_CREATE_INFO};TakeText(in,i.actionName);TakeText(in,i.localizedActionName);i.actionType=in.Pod<XrActionType>();i.countSubactionPaths=BoundedCount(in.Pod<uint32_t>(),8);std::vector<XrPath> paths(i.countSubactionPaths);for(auto& path:paths)path=in.Pod<XrPath>();i.subactionPaths=paths.data();XrAction action=0;auto r=api.CreateAction(set,&i,&action);if(XR_SUCCEEDED(r)){out.Pod(HandleBits(action));actions.push_back({action,i.actionType,std::move(paths)});}return r;}
  case Command::SuggestBindings:{XrInteractionProfileSuggestedBinding i{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};i.interactionProfile=in.Pod<XrPath>();i.countSuggestedBindings=BoundedCount(in.Pod<uint32_t>());std::vector<XrActionSuggestedBinding> bindings(i.countSuggestedBindings);for(auto& b:bindings){b.action=FromHandle<XrAction>(in.Pod<uint64_t>());b.binding=in.Pod<XrPath>();}i.suggestedBindings=bindings.data();return api.SuggestInteractionProfileBindings(instance,&i);}
  case Command::AttachActionSets:{XrSessionActionSetsAttachInfo i{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};i.countActionSets=BoundedCount(in.Pod<uint32_t>(),16);std::vector<XrActionSet> sets(i.countActionSets);for(auto& set:sets)set=FromHandle<XrActionSet>(in.Pod<uint64_t>());i.actionSets=sets.data();return api.AttachSessionActionSets(session,&i);}
  case Command::CreateActionSpace:{XrActionSpaceCreateInfo i{XR_TYPE_ACTION_SPACE_CREATE_INFO};i.action=FromHandle<XrAction>(in.Pod<uint64_t>());i.subactionPath=in.Pod<XrPath>();Read(in,i.poseInActionSpace);XrSpace space=0;auto r=api.CreateActionSpace(session,&i,&space);if(XR_SUCCEEDED(r)){out.Pod(HandleBits(space));actionSpaces.push_back(space);}return r;}
  case Command::SyncActions:{XrActionsSyncInfo i{XR_TYPE_ACTIONS_SYNC_INFO};i.countActiveActionSets=BoundedCount(in.Pod<uint32_t>(),16);std::vector<XrActiveActionSet> sets(i.countActiveActionSets);for(auto& set:sets){set.actionSet=FromHandle<XrActionSet>(in.Pod<uint64_t>());set.subactionPath=in.Pod<XrPath>();}i.activeActionSets=sets.data();auto r=api.SyncActions(session,&i);if(r==XR_SUCCESS)ActionSnapshot(out);return r;}
  case Command::PoseState:case Command::BooleanState:case Command::FloatState:case Command::VectorState:{XrActionStateGetInfo i{XR_TYPE_ACTION_STATE_GET_INFO};i.action=FromHandle<XrAction>(in.Pod<uint64_t>());i.subactionPath=in.Pod<XrPath>();
   if(command==Command::PoseState){XrActionStatePose s{XR_TYPE_ACTION_STATE_POSE};auto r=api.GetActionStatePose(session,&i,&s);if(XR_SUCCEEDED(r))Write(out,s);return r;}
   if(command==Command::BooleanState){XrActionStateBoolean s{XR_TYPE_ACTION_STATE_BOOLEAN};auto r=api.GetActionStateBoolean(session,&i,&s);if(XR_SUCCEEDED(r))Write(out,s);return r;}
   if(command==Command::FloatState){XrActionStateFloat s{XR_TYPE_ACTION_STATE_FLOAT};auto r=api.GetActionStateFloat(session,&i,&s);if(XR_SUCCEEDED(r))Write(out,s);return r;}
   XrActionStateVector2f s{XR_TYPE_ACTION_STATE_VECTOR2F};auto r=api.GetActionStateVector2f(session,&i,&s);if(XR_SUCCEEDED(r))Write(out,s);return r;}
  case Command::LocateSpace:{auto space=FromHandle<XrSpace>(in.Pod<uint64_t>()),base=FromHandle<XrSpace>(in.Pod<uint64_t>());auto time=in.Pod<XrTime>();XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};auto r=api.LocateSpace(space,base,time,&location);if(XR_SUCCEEDED(r))Write(out,location);return r;}
  case Command::Haptic:{XrHapticActionInfo i{XR_TYPE_HAPTIC_ACTION_INFO};i.action=FromHandle<XrAction>(in.Pod<uint64_t>());i.subactionPath=in.Pod<XrPath>();XrHapticVibration v{XR_TYPE_HAPTIC_VIBRATION};Take(in,v.duration,v.frequency,v.amplitude);return api.ApplyHapticFeedback(session,&i,reinterpret_cast<XrHapticBaseHeader const*>(&v));}
  default:return XR_ERROR_FUNCTION_UNSUPPORTED;
  }
 }
 void Run(std::wstring const& loader,std::wstring const& manifest){
  while(!quit){auto packet=pipe.Receive();Writer data;XrResult result=XR_ERROR_RUNTIME_FAILURE;std::string error;
   try{result=Execute(Command(packet.first.command),packet.second,data,loader,manifest);packet.second.Complete();}catch(std::exception const& e){error=e.what();log.Line("{\"type\":\"meta_host_command_error\",\"command\":"+std::to_string(packet.first.command)+",\"error\":"+g3vr::Quote(error)+"}");data.bytes.clear();}
   Writer response;response.Pod(result);if(XR_FAILED(result))response.Text(error);response.Data(data.bytes.data(),data.bytes.size());pipe.Send(packet.first,response);
   if(timing.Count()&&timing.ReportDue(NowMs()))TimingReport();
  }
 TimingReport();
#ifdef G3VR_META_GPU_FIXTURE
  constexpr auto readback="true";
#else
  constexpr auto readback="false";
#endif
  log.Line("{\"type\":\"meta_host_finished\",\"submissions\":"+std::to_string(submitted)+",\"gpu_images_copied\":"+std::to_string(copiedImages)+",\"stale_waits_discarded\":"+std::to_string(staleWaits)+",\"cpu_pixel_readback\":"+readback+"}");
 }
};
int wmain(int argc,wchar_t** argv){try{static_assert(sizeof(void*)==8);if(argc!=9||wcscmp(argv[1],L"--pipe")||wcscmp(argv[3],L"--loader")||wcscmp(argv[5],L"--manifest")||wcscmp(argv[7],L"--log"))return 2;Server server(argv[2],argv[8]);server.Run(argv[4],argv[6]);return 0;}catch(std::exception const& e){OutputDebugStringA(e.what());return 1;}}
