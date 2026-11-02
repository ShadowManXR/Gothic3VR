#include "StereoOpenXrApi.h"
#include "D3D9Bridge.h"
#include "D3D9UiCapture.h"
#include "StereoRuntime.h"
#include "ProfilingOptions.h"
#include "RuntimeProbePolicy.h"
#include "GpuUpscaler.h"
#include "DxvkFoveation.h"
#include <algorithm>
#include <memory>
#include "QuestActions.h"
#include "GpuFfrFeather.h"
#include "RadialImage.h"
#include "WorldStatusImage.h"
#include "FrameCopyBatch.h"
#include "AsyncGpuTiming.h"
#include "DeferredProfileLines.h"
#include "TemporalRenderPhase.h"
#include "TemporalProjectionJitter.h"
#include "dlaa/Protocol.h"
#include "RendererDlaaClient.h"
#include "NativeStaticTrial.h"
#include "NativeTrialMode.h"
#include "RendererFeasibility.h"
#include <fstream>
#ifdef G3VR_STEREO_FRAME_CAPTURE
#include "StereoFrameCapture.h"
#endif
#ifdef G3VR_NATIVE_MOTION
#include "GameTemporalInputs.h"
#endif
namespace diagnostic
{
namespace
{
struct Runtime
{
#ifdef G3VR_STEREO_FRAME_CAPTURE
    StereoFrameCapture video;
    uint64_t nextVideoPoll=0;
    std::filesystem::path videoOutput;
    void PollVideo()noexcept {
        if(video.Active()||GetTickCount64()<nextVideoPoll)return;nextVideoPoll=GetTickCount64()+250;
        try{auto request=log.path.parent_path().parent_path()/L"stereo-capture.request";
            if(GetFileAttributesW(request.c_str())==INVALID_FILE_ATTRIBUTES)return;
            std::ifstream input(request,std::ios::binary);std::string path;std::getline(input,path);input.close();
            if(!DeleteFileW(request.c_str()))return;
            if(!path.empty()&&path.back()=='\r')path.pop_back();auto base=std::filesystem::u8path(path);
            if(!base.is_absolute())throw std::runtime_error("Capture output must be an absolute path");
            videoOutput=base/(L"sequence-"+std::to_wstring(GetTickCount64()));
            video.Start(renderer.device.Get(),StereoFrameCapture::LongLimit,true);
            ui.writeTrace.Start();
            log.Line("{\"type\":\"stereo_video_started\",\"width\":192,\"height\":108,\"encoding\":\"L8\",\"maximum_pairs\":4096,\"maximum_seconds\":60,\"readback_during_recording\":false}");
        }catch(std::exception const& e){video.Reset();ui.writeTrace.Stop();log.Line("{\"type\":\"stereo_video_failed\",\"error\":"+g3vr::Quote(e.what())+"}");}
    }
    void FinishVideo(bool submittedPair)noexcept {
        try{if(!video.End(submittedPair))return;ui.writeTrace.Stop();auto count=video.Count();video.Save(renderer.context.Get(),videoOutput);ui.writeTrace.Save(videoOutput);video.Reset();
            log.Line("{\"type\":\"stereo_video_saved\",\"pairs\":"+std::to_string(count)+",\"output\":"+g3vr::Quote(videoOutput.u8string())+"}");
        }catch(std::exception const& e){video.Reset();ui.writeTrace.Stop();log.Line("{\"type\":\"stereo_video_failed\",\"error\":"+g3vr::Quote(e.what())+"}");}
    }
#endif
    g3vr::Log log;g3vr::Api api;stereo::Renderer renderer;stereo::ComPtr<IDirect3DDevice9Ex> device9;
    stereo::ComPtr<IDirect3DQuery9> fence9;stereo::ComPtr<ID3D11Query> fence11;
    g3vr::DxvkGpuBridge dxvk;
    using WrapperUpdate=HRESULT(WINAPI*)(ID3D11Texture2D*);
    WrapperUpdate wrapperUpdate=nullptr;
    std::unique_ptr<g3vr::dx11::NativeStaticTrial> staticTrial;
#ifdef G3VR_RENDERER_FEASIBILITY
    std::filesystem::path feasibilityOutput;
    void FlushFeasibility(bool exiting=false)noexcept{
        try{auto& trial=renderer_trial::experiment;
            if(feasibilityOutput.empty()||trial.written||!trial.Armed())return;
            if(exiting)trial.Abort("session_ended_before_screening_completed");
            if(!trial.Finished())return;
            std::ofstream csv(feasibilityOutput/L"frames.csv"),meta(feasibilityOutput/L"screening.json");
            if(!csv||!meta)return;trial.Csv(csv);trial.Metadata(meta);csv.flush();meta.flush();
            if(csv&&meta){trial.written=true;log.Line("{\"type\":\"renderer_feasibility_finished\",\"performance_gain_proven\":false,\"image_equivalent\":false}");}
        }catch(...){}
    }
#endif
    g3vr::DxvkFoveation foveation;int ffrMode=0,ffrDiameter=75,ffrTransition=10;bool ffrConfigured=false,ffrDirty=false;
    GpuFfrFeather ffrFeather;bool ffrFeatherFault=false;unsigned ffrEyeMask=0;float ffrCenters[4]{.5f,.5f,.5f,.5f};
    GpuUpscaler upscaler;uint32_t outputWidth=0,outputHeight=0;
    g3vr::FrameCopyBatch<XrSwapchain> pendingCopies;
    g3vr::DeferredFrameCopies<ID3D11Texture2D> pendingTextures;
    probes::FrameCollector metrics;
    performance::UpdateCollector updateMetrics;
    performance::NativePhaseCollector nativeMetrics;
    uint64_t performanceFrequency=performance::Frequency();
    int timingContext=0;
    int timingEye=-1;bool drawProfile=false;
    performance::DeferredProfileLines deferredProfile;
    uint64_t nextDrawSample[2]{},nextDrawSpike[2]{};
    std::array<uint64_t,performance::StageCount+performance::UpdateStageCount+performance::NativePhaseCount> spikeAfter{};
    std::array<uint64_t,2> sceneStallAfter{};
    performance::AsyncGpuTiming gpuMetrics;
    bool batchCopies=true;
    uint64_t uiStarted=0,uiDrawsAtBegin=0;
    XrSystemId system=XR_NULL_SYSTEM_ID;XrSession session=XR_NULL_HANDLE;XrSpace space=XR_NULL_HANDLE,viewSpace=XR_NULL_HANDLE;
    XrSessionState state=XR_SESSION_STATE_UNKNOWN;bool running=false,begun=false;uint64_t focusedAt=0,frames=0,submitted=0,lastSample=0,lastControllerSample=0;
    XrFrameState frame{XR_TYPE_FRAME_STATE};XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};uint32_t width=0,height=0;unsigned captured=0;
    struct Eye {XrSwapchain swapchain=XR_NULL_HANDLE;std::vector<XrSwapchainImageD3D11KHR> images;g3vr::SharedEye shared;uint64_t copies=0;} eyes[2];
    g3vr::UiCapture ui;
    std::filesystem::path temporalRequest;unsigned temporalRemaining=0;
    temporal::WorldGate temporalGate;
    bool temporalJitterOptIn=false;
    bool dlaaEnabled=false;
    g3vr::RendererDlaaClient dlaaClient;
    stereo::ComPtr<ID3D11Texture2D> dlaaColor[2];
    ID3D11Texture2D* dlaaTargets[2]{};
    std::array<std::array<uint32_t,4>,2> dlaaOverlay{};
    uint64_t dlaaSerial=0,dlaaLastTime=0,dlaaReported=0;
    uint64_t temporalReplayTicks[2]{},temporalTransferTicks[2]{},temporalFinalizeTicks[2]{};
    std::array<uint64_t,2> temporalPreparedFrames{};
    std::array<bool,2> temporalPreparedProbes{};
    std::array<std::array<float,2>,2> temporalRasterJitter{};
    g3vr::DepthSnapshot temporalDepth[2];uint64_t temporalDepthFrame[2]{};
    std::string temporalDepthError[2];bool temporalDepthDone=false;
    g3vr::DepthSnapshot temporalOpaqueDepth[2];uint64_t temporalOpaqueFrame[2]{};
    std::string temporalOpaqueError[2];
    stereo::ComPtr<ID3D11Texture2D> temporalColor[2];
#ifdef G3VR_NATIVE_MOTION
    std::unique_ptr<g3vr::GameTemporalInputs> temporalGpu;
    bool temporalGpuFailed=false;uint64_t temporalGpuFrames[2]{};
    std::string temporalGpuError;
#endif
    XrSwapchain uiSwapchain=XR_NULL_HANDLE;
    std::vector<XrSwapchainImageD3D11KHR> uiImages;
    bool uiCaptured=false,cinemaCaptured=false;
    uint64_t uiFrames=0,uiOnlyFrames=0;
    int diagnosticState=-100;uint64_t diagnosticChanged=0;unsigned diagnosticDumps=0;
    UiLayout uiLayout;
    RadialState radial,radialUploaded;bool radialValid=false;int worldStatus=0,uploadedStatus=0;float radialScale=.8f;
    XrSwapchain radialSwapchain=XR_NULL_HANDLE;std::vector<XrSwapchainImageD3D11KHR> radialImages;
    QuestActions controllers;
    std::vector<XrTime> referenceChanges;
    std::wstring uiDumpFile;
    explicit Runtime(wchar_t const* file):log(file){
#ifdef G3VR_UI_COMPOSITION_CAPTURE
        wchar_t dump[32768]{};auto n=GetEnvironmentVariableW(L"G3VR_DUMP_UI_FILE",dump,32768);if(n&&n<32768)uiDumpFile=dump;
#endif
    }
    void ProfileLine(char const* line){if(timingEye>=0)deferredProfile.Push(line);else log.Line(line);}
    void FlushProfileLines()noexcept{
        if constexpr(!probes::Enabled)return;
        try{deferredProfile.Drain([&](char const* line){log.Line(line);});
            auto dropped=deferredProfile.TakeDropped();if(dropped){char line[160];std::snprintf(line,sizeof(line),"{\"type\":\"vr_profile_lines_dropped\",\"count\":%llu}",static_cast<unsigned long long>(dropped));log.Line(line);}
        }catch(...){}
    }
    void CpuSpike(unsigned key,char const* name,uint64_t started,uint64_t elapsed,int context) noexcept {
        uint64_t frequency=performanceFrequency;
        if(!frequency||key>=spikeAfter.size()||elapsed<frequency/50)return;
        uint64_t now=started+elapsed;if(now<spikeAfter[key])return;spikeAfter[key]=now+frequency/4;
        try{char line[640];std::snprintf(line,sizeof(line),"{\"type\":\"vr_cpu_spike\",\"stage\":\"%s\",\"context\":%d,\"eye\":%d,\"wall_ms\":%.4f,\"xr_frame_serial\":%llu,\"qpc_started\":%llu,\"qpc_frequency\":%llu,\"thread\":%lu}",
            name,context,timingEye,double(elapsed)*1000/double(frequency),static_cast<unsigned long long>(frames),static_cast<unsigned long long>(started),static_cast<unsigned long long>(frequency),GetCurrentThreadId());ProfileLine(line);}catch(...){}
    }
    void ReportEyeDraw(performance::EyeDrawSample const& sample,unsigned eye)noexcept{
        if(eye>1||!performanceFrequency)return;auto now=sample.started+sample.elapsed;bool spike=sample.elapsed>=performanceFrequency/50;
        if(spike?now<nextDrawSpike[eye]:now<nextDrawSample[eye])return;
        nextDrawSample[eye]=now+2*performanceFrequency;if(spike)nextDrawSpike[eye]=now+performanceFrequency/4;
        try{char line[1024];if(sample.Json(line,sizeof(line),performanceFrequency,eye,timingContext,frames,spike))log.Line(line);}catch(...){}
    }
    void Initialize(IDirect3DDevice9* native,wchar_t const* loader,VrSettings const& settings)
    {
        wchar_t mode[8]{};if(GetEnvironmentVariableW(L"G3VR_COPY_BATCH",mode,8)&&wcscmp(mode,L"0")==0)batchCopies=false;
        drawProfile=probes::Enabled&&performance::ProfilingOptIn(L"G3VR_DRAW_PROFILE");
        temporalJitterOptIn=probes::Enabled&&performance::ProfilingOptIn(L"G3VR_TEMPORAL_JITTER_CAPTURE");
        log.Line(drawProfile?"{\"type\":\"vr_draw_profile_mode\",\"enabled\":true,\"gpu_waits_added\":0}":"{\"type\":\"vr_draw_profile_mode\",\"enabled\":false}");
        RadialGeometry();
        stereo::Hr(native->QueryInterface(IID_PPV_ARGS(&device9)),"Gothic 3 D3D9Ex required");
        wchar_t gpuMode[8]{};bool gpuEnabled=probes::Enabled&&!(GetEnvironmentVariableW(L"G3VR_GPU_PROFILE",gpuMode,8)&&wcscmp(gpuMode,L"0")==0);
        bool gpuReady=gpuEnabled&&gpuMetrics.Initialize(device9.Get());
        log.Line(gpuReady?"{\"type\":\"vr_gpu_timer_ready\",\"sample_period_frames\":16,\"slots\":4,\"blocking\":false}":"{\"type\":\"vr_gpu_timer_unavailable\",\"rendering_unchanged\":true}");
        api.Initialize(loader,log,settings.openXrRuntime);XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        api.Check(api.GetSystem(api.instance,&si,&system),"xrGetSystem");XrGraphicsRequirementsD3D11KHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        api.Check(api.GetD3D11GraphicsRequirementsKHR(api.instance,system,&req),"D3D11 requirements");
        D3DDEVICE_CREATION_PARAMETERS creation{};stereo::Hr(device9->GetCreationParameters(&creation),"native adapter");
        stereo::ComPtr<IDirect3D9> parent;stereo::Hr(device9->GetDirect3D(&parent),"D3D9 parent");stereo::ComPtr<IDirect3D9Ex> parentEx;
        stereo::Hr(parent.As(&parentEx),"D3D9Ex parent");LUID luid{};stereo::Hr(parentEx->GetAdapterLUID(creation.AdapterOrdinal,&luid),"native adapter LUID");
        if(memcmp(&luid,&req.adapterLuid,sizeof(luid)))throw std::runtime_error("Gothic 3 and the selected OpenXR runtime must use the same GPU");
        stereo::ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");stereo::ComPtr<IDXGIAdapter1> adapter;
        for(UINT i=0;;++i){stereo::ComPtr<IDXGIAdapter1> next;HRESULT hr=factory->EnumAdapters1(i,&next);if(hr==DXGI_ERROR_NOT_FOUND)break;stereo::Hr(hr,"GPU enumeration");
            DXGI_ADAPTER_DESC1 d{};next->GetDesc1(&d);if(!memcmp(&d.AdapterLuid,&luid,sizeof(luid))){adapter=next;break;}}
        if(!adapter)throw std::runtime_error("Cannot find native GPU in DXGI");renderer.Initialize(adapter.Get(),req.minFeatureLevel);
        dxvk.Initialize(device9.Get());
#ifdef G3VR_RENDERER_FEASIBILITY
        wchar_t trialMode[4]{},trialOutput[32768]{};
        auto trialLength=GetEnvironmentVariableW(L"G3VR_RENDERER_FEASIBILITY_OUTPUT",trialOutput,32768);
        if(GetEnvironmentVariableW(L"G3VR_RENDERER_FEASIBILITY",trialMode,4)==1&&trialMode[0]==L'1'&&trialLength&&trialLength<32768){
            feasibilityOutput=trialOutput;renderer_trial::experiment.Arm(performance::Frequency(),true);
            log.Line("{\"type\":\"renderer_feasibility_ready\",\"warmup_seconds\":12,\"phase_seconds\":[8,4,8,4,8],\"diagnostic_static_color_omission\":true,\"replacement_renderer\":false}");
        }
#endif
        wrapperUpdate=reinterpret_cast<WrapperUpdate>(GetProcAddress(GetModuleHandleW(L"d3d9.dll"),"G3VR_UpdateWrapperEye"));
        ffrMode=FfrMode(settings.ffrMode);ffrDiameter=std::clamp(settings.ffrDiameterPercent,40,95);ffrTransition=FfrTransition(settings.ffrTransitionPercent);ffrDirty=ffrMode!=0;

        stereo::ComPtr<IDirect3DSurface9> backbuffer;stereo::Hr(device9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&backbuffer),"native backbuffer");
        D3DSURFACE_DESC desc{};stereo::Hr(backbuffer->GetDesc(&desc),"native image size");width=desc.Width;height=desc.Height;
        auto requested=MakeUpscalePlan(settings.renderWidth?settings.renderWidth:width,settings.renderHeight?settings.renderHeight:height,settings.upscaleMethod,settings.upscaleScalePercent,settings.upscaleSharpness,settings.renderWidth!=0,settings.fxaaEnabled,settings.smaaEnabled,settings.smaaQuality);
        auto plan=MatchUpscaleInput(requested,width,height);outputWidth=plan.outputWidth;outputHeight=plan.outputHeight;
        wchar_t staticMode[2]{};
        unsigned staticTrialMode=0;
        if constexpr(probes::StaticTrial)staticTrialMode=diagnostic::NativeTrialMode(staticMode,GetEnvironmentVariableW(L"G3VR_DX11_STATIC_TRIAL",staticMode,2),probes::Dx11ValidationOnly);
        if(probes::StaticTrial&&staticTrialMode){
            try{wchar_t assets[32768]{},output[32768]{};auto a=GetEnvironmentVariableW(L"G3VR_DX11_STATIC_ASSETS",assets,32768),o=GetEnvironmentVariableW(L"G3VR_DX11_STATIC_OUTPUT",output,32768);
                if(!a||a>=32768||!o||o>=32768)throw std::runtime_error("Missing local static trial asset/output paths");
                auto trial=std::make_unique<g3vr::dx11::NativeStaticTrial>();trial->Initialize(device9.Get(),adapter.Get(),assets,output,width,height,staticTrialMode);staticTrial=std::move(trial);
                ui.worldObserverContext=this;ui.worldObserver=[](void* value,IDirect3DDevice9* d,unsigned kind,diagnostic::temporal::Geometry const* geometry)noexcept{
                    auto& r=*static_cast<Runtime*>(value);if(!r.staticTrial||!r.staticTrial->NeedsMaterialWork())return false;
                    // Isolated references and the native writeback quad need full
                    // resolution, even while the surrounding world uses FFR.
                    bool rearm=r.ffrConfigured&&r.timingContext==1&&r.timingEye>=0,replaced=false;
                    try{if(rearm)r.foveation.EndEye();replaced=r.staticTrial->ObserveMaterialDraw(d,kind,geometry);if(rearm)r.foveation.BeginEye(unsigned(r.timingEye));}
                    catch(...){r.foveation.Reset();r.ffrConfigured=false;}
                    return replaced;};
                log.Line("{\"type\":\"vr_dx11_static_trial_ready\",\"native_draw_suppression_requires_live_color_depth_gate\":true,\"dlss_or_dlaa_enabled_by_trial\":false}");
            }catch(std::exception const& e){log.Line("{\"type\":\"vr_dx11_static_trial_unavailable\",\"native_preserved\":true,\"error\":"+g3vr::Quote(e.what())+"}");}
        }
        upscaler.Initialize(renderer.device.Get(),plan);
        log.Line(plan.fxaa?"{\"type\":\"vr_fxaa\",\"enabled\":true,\"preset\":12,\"subpixel\":0.25,\"eyes\":2,\"hud_filtered\":false,\"temporal_history\":false}":"{\"type\":\"vr_fxaa\",\"enabled\":false}");
        if(plan.smaa){char report[224];sprintf_s(report,"{\"type\":\"vr_smaa\",\"enabled\":true,\"preset\":\"%s\",\"mode\":\"1x\",\"eyes\":2,\"hud_filtered\":false,\"temporal_history\":false}",plan.smaaQuality==2?"Ultra":plan.smaaQuality==1?"High":"Medium");log.Line(report);}
        else log.Line("{\"type\":\"vr_smaa\",\"enabled\":false}");
        if(probes::Enabled&&settings.dlaaExperimental){
#ifdef G3VR_NATIVE_MOTION
            try{
                if(plan.method!=UpscaleMethod::Off||outputWidth!=width||outputHeight!=height||ffrMode)
                    throw std::runtime_error("Experimental DLAA requires native resolution, upscaler Off and FFR Off");
                dlaaClient.Initialize(renderer.device.Get(),renderer.context.Get(),width,height,
                    log.path.parent_path().parent_path()/L"dlaa"/L"G3VRDlaaWorker64.exe");
                for(auto& image:dlaaColor)image=renderer.Color(width,height,DXGI_FORMAT_B8G8R8A8_UNORM);
                dlaaEnabled=true;batchCopies=true;
                log.Line("{\"type\":\"vr_dlaa_ready\",\"mode\":\"experimental_reactive_camera_fallback\",\"scene_motion_coverage_complete\":false,\"native_resolution\":true}");
            }catch(std::exception const& e){dlaaClient.Reset();log.Line("{\"type\":\"vr_dlaa_unavailable\",\"rendering_preserved\":true,\"error\":"+g3vr::Quote(e.what())+"}");}
#else
            log.Line("{\"type\":\"vr_dlaa_unavailable\",\"error\":\"Native motion collectors missing from this build\"}");
#endif
        }
        log.Line("{\"type\":\"vr_upscaler\",\"method\":"+std::to_string(int(plan.method))+",\"input_width\":"+std::to_string(width)+",\"input_height\":"+std::to_string(height)+",\"output_width\":"+std::to_string(outputWidth)+",\"output_height\":"+std::to_string(outputHeight)+",\"sharpness\":"+std::to_string(plan.sharpness)+",\"restart_required\":"+(requested.inputWidth!=width||requested.inputHeight!=height?"true":"false")+"}");
        XrGraphicsBindingD3D11KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};binding.device=renderer.device.Get();XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};ci.next=&binding;ci.systemId=system;
        api.Check(api.CreateSession(api.instance,&ci,&session),"create graphical session");XrReferenceSpaceCreateInfo ri{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};ri.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;ri.poseInReferenceSpace.orientation.w=1;
        api.Check(api.CreateReferenceSpace(session,&ri,&space),"create LOCAL space");
        ri.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_VIEW;api.Check(api.CreateReferenceSpace(session,&ri,&viewSpace),"create UI VIEW space");
        controllers.Initialize(api,session,api.controllerExtensions);
        for(auto const& p:controllers.profileResults)log.Line("{\"type\":\"controller_profile_bindings\",\"profile\":"+g3vr::Quote(p.path)+",\"status\":"+g3vr::Quote(!p.enabled?"extension_disabled":XR_SUCCEEDED(p.result)?"accepted":"unsupported")+",\"result\":"+std::to_string(int(p.result))+",\"accepted_is_not_hardware_validation\":true}");
        log.Line("{\"type\":\"quest_controller_actions_ready\",\"profile_selection\":\"runtime\",\"bindings_preserve_touch_actions\":true}");
        uint32_t count=0;api.Check(api.EnumerateSwapchainFormats(session,0,&count,nullptr),"formats count");
        std::vector<int64_t> formats(count);api.Check(api.EnumerateSwapchainFormats(session,count,&count,formats.data()),"formats");
        if(std::find(formats.begin(),formats.end(),DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)==formats.end())throw std::runtime_error("Selected OpenXR runtime does not expose the required BGRA8 sRGB swapchain format");
        api.Check(api.EnumerateViewConfigurationViews(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"stereo view count");if(count!=2)throw std::runtime_error("Expected two eyes");
        api.Check(api.EnumerateEnvironmentBlendModes(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"blend count");std::vector<XrEnvironmentBlendMode> blends(count);
        api.Check(api.EnumerateEnvironmentBlendModes(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,count,&count,blends.data()),"blend modes");
        if(std::find(blends.begin(),blends.end(),XR_ENVIRONMENT_BLEND_MODE_OPAQUE)==blends.end())throw std::runtime_error("Opaque blend mode required");
        for(auto& eye:eyes){eye.shared.Create(device9.Get(),renderer.device.Get(),width,height,&dxvk);XrSwapchainCreateInfo sc{XR_TYPE_SWAPCHAIN_CREATE_INFO};sc.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            sc.format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;sc.width=outputWidth;sc.height=outputHeight;sc.sampleCount=sc.faceCount=sc.arraySize=sc.mipCount=1;
            api.Check(api.CreateSwapchain(session,&sc,&eye.swapchain),"eye swapchain");api.Check(api.EnumerateSwapchainImages(eye.swapchain,0,&count,nullptr),"eye images count");eye.images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
            api.Check(api.EnumerateSwapchainImages(eye.swapchain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.images.data())),"eye images");}
        ui.Initialize(device9.Get(),renderer.device.Get(),width,height,&dxvk);
        XrSwapchainCreateInfo usc{XR_TYPE_SWAPCHAIN_CREATE_INFO};usc.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        usc.format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;usc.width=width;usc.height=height;usc.sampleCount=usc.faceCount=usc.arraySize=usc.mipCount=1;
        api.Check(api.CreateSwapchain(session,&usc,&uiSwapchain),"UI swapchain");
        api.Check(api.EnumerateSwapchainImages(uiSwapchain,0,&count,nullptr),"UI images count");uiImages.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        api.Check(api.EnumerateSwapchainImages(uiSwapchain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(uiImages.data())),"UI images");
        stereo::Hr(device9->CreateQuery(D3DQUERYTYPE_EVENT,&fence9),"D3D9 copy fence");D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};stereo::Hr(renderer.device->CreateQuery(&q,&fence11),"D3D11 copy fence");
        log.Line("{\"type\":\"game_stereo_ready\",\"adapter\":"+g3vr::Quote(g3vr::Utf8(renderer.adapterDescription.Description))+",\"width\":"+std::to_string(width)+",\"height\":"+std::to_string(height)+",\"interop\":"+g3vr::Quote(dxvk.Active()?"DXVK_Vulkan_import_native_D3D11_BGRA8":wrapperUpdate?"full_D3D11_wrapper_GPU_shared_RGBA_to_BGRA":"D3D9Ex_shared_BGRA8_to_D3D11")+",\"ui\":\"native_GPU_replay_premultiplied_VIEW_quad\"}");
        log.Line(std::string("{\"type\":\"vr_copy_pipeline\",\"batched_copies\":")+(batchCopies?"true":"false")+",\"cross_frame_queue\":false,\"performance_window_seconds\":2}");
        temporalRequest=log.path.parent_path().parent_path()/L"dlaa-capture.request";
        DWORD requestAttributes=probes::Enabled?GetFileAttributesW(temporalRequest.c_str()):INVALID_FILE_ATTRIBUTES;
        if(requestAttributes!=INVALID_FILE_ATTRIBUTES&&!(requestAttributes&FILE_ATTRIBUTE_DIRECTORY)){
            try{ui.temporal.Prepare();temporalRemaining=4;log.Line("{\"type\":\"vr_temporal_probe_armed\",\"stereo_frames\":4,\"world_warmup_frames\":60,\"depth_phases\":[\"opaque_before_occlusion\",\"end_of_eye\"],\"metadata_only\":false,\"depth_snapshot_requested\":true,\"depth_pixels_captured\":false,\"motion_vectors_captured\":false}");}
            catch(...){temporalRemaining=0;log.Line("{\"type\":\"vr_temporal_probe_unavailable\",\"rendering_preserved\":true}");}
        }
    }
    void WaitFor9()
    {
#ifdef G3VR_RENDERER_FEASIBILITY
        auto trialStarted=renderer_trial::experiment.Active()?performance::Now():0;
        g3vr::Scope trialEnd{[&]{if(trialStarted)renderer_trial::experiment.ScopeTime(renderer_trial::Scope::Wait9,trialStarted,performance::Now());}};
#endif
        if(dxvk.Active()){auto start=probes::Now();dxvk.Complete();metrics.Add(performance::Stage::D3D9Wait,probes::Now()-start);return;}
        auto start=probes::Now();auto stats=g3vr::Wait9(fence9.Get());
        metrics.Add(performance::Stage::D3D9Wait,probes::Now()-start);
        metrics.Count(performance::Counter::D3D9Polls,stats.polls);metrics.Count(performance::Counter::D3D9Flushes,stats.flushes);
    }
    bool PrepareTemporalEye(unsigned eye,int context){
        if constexpr(!probes::Enabled)return false;
        if(eye>1||!begun)return false;
        if(temporalPreparedFrames[eye]==frames)return temporalPreparedProbes[eye];
        temporalPreparedFrames[eye]=frames;temporalRasterJitter[eye]={};bool probe=(temporalRemaining||dlaaEnabled)&&temporalGate.Begin(eye,frames,context);temporalPreparedProbes[eye]=probe;
        if(probe){if(temporalJitterOptIn||dlaaEnabled)dlaa::Jitter(frames,temporalRasterJitter[eye][0],temporalRasterJitter[eye][1]);ui.temporal.Prepare();ui.temporal.SetMetadata(temporalRemaining!=0);ui.temporal.Begin(eye,frames,width,height,frame.predictedDisplayTime>0?uint64_t(frame.predictedDisplayTime):0,temporalRasterJitter[eye]);}
        return probe;
    }
    void EvaluateDlaa(){
#ifdef G3VR_NATIVE_MOTION
        if(!dlaaEnabled||dlaaSerial==frames||!dlaaTargets[0]||!dlaaTargets[1])return;
        // Both source copies and both finalized companions belong to this XR
        // frame. Fallback was already copied into each acquired XR image.
        try{
            if(temporalGpuFailed)throw std::runtime_error(temporalGpuError);
            if(!temporalPreparedProbes[0]||!temporalPreparedProbes[1])return;
            if(!temporalGpu||!temporalGpu->Finalized(0,frames)||!temporalGpu->Finalized(1,frames))throw std::runtime_error("DLAA current stereo companions unavailable");
            g3vr::DlaaStereoFrame input;input.serial=frames;input.motionPolicy=g3vr::DlaaMotionPolicy::ExperimentalReactiveFallback;
            input.jitterX=temporalRasterJitter[0][0];input.jitterY=temporalRasterJitter[0][1];
            uint64_t time=frame.predictedDisplayTime>0?uint64_t(frame.predictedDisplayTime):0;
            input.deltaMs=dlaaLastTime&&time>dlaaLastTime?std::clamp(float(double(time-dlaaLastTime)/1000000.),1.f,250.f):1000.f/72;
            for(unsigned e=0;e<2;++e){auto& eye=input.eyes[e];temporal::CameraMotionFrame camera;temporal::GeometryFrameStamp stamp;
                if(!temporalGpu->Camera(e,frames,camera,stamp))throw std::runtime_error("DLAA camera stamp unavailable");
                eye.color=dlaaColor[e].Get();eye.depth=temporalGpu->Depth(e);eye.motion=temporalGpu->Motion(e);eye.reactive=temporalGpu->Reactive(e);
                eye.frame=stamp.frame;eye.epoch=stamp.epoch;eye.displayTimeNs=stamp.displayTimeNs;eye.eye=e;
                eye.readiness=g3vr::DlaaReady&~g3vr::DlaaFullSceneMotion;eye.colorEncoding=g3vr::DlaaColorEncoding::Srgb;
                eye.currentClipToPreviousClip=camera.currentClipToPreviousClip;eye.cameraHistory=camera.historyValid;
                if(!camera.historyValid)input.resetMask|=1u<<e;
                for(unsigned a=0;a<2;++a)eye.previousMinusCurrentRasterJitter[a]=camera.previousRasterJitter[a]-camera.currentRasterJitter[a];
                eye.protectedOverlay=dlaaOverlay[e];
            }
            auto evaluateStarted=probes::Now();auto result=dlaaClient.Evaluate(input);
            for(unsigned e=0;e<2;++e)dlaaClient.Present(e,frames,dlaaTargets[e]);
            dlaaSerial=frames;dlaaLastTime=time;
            if(!dlaaReported||frames-dlaaReported>=120){dlaaReported=frames;
                double ms=1000./double(performanceFrequency);std::ostringstream line;line<<"{\"type\":\"vr_dlaa_frame\",\"frame\":"<<frames<<",\"written_eyes\":2,\"mode\":\"experimental_reactive_camera_fallback\",\"scene_motion_coverage_complete\":false,\"sky_companion_replay\":"<<(temporalRemaining?"true":"false")<<",\"clear_unoccupied_background_raw\":true,\"gpu_ms\":["<<result.gpuMs[0]<<','<<result.gpuMs[1]<<"],\"evaluate_present_wall_ms\":"<<(probes::Now()-evaluateStarted)*ms<<",\"observation_wall_ms\":["<<ui.temporal.ObservationTicks(0)*ms<<','<<ui.temporal.ObservationTicks(1)*ms<<"],\"opaque_replay_wall_ms\":["<<temporalReplayTicks[0]*ms<<','<<temporalReplayTicks[1]*ms<<"],\"opaque_transfer_wall_ms\":["<<temporalTransferTicks[0]*ms<<','<<temporalTransferTicks[1]*ms<<"],\"late_finalize_wall_ms\":["<<temporalFinalizeTicks[0]*ms<<','<<temporalFinalizeTicks[1]*ms<<"],\"reset_mask\":"<<result.resetMask<<'}';log.Line(line.str());}
        }catch(std::exception const& e){
            dlaaEnabled=false;dlaaClient.Reset();
            // If presentation failed after the first eye, restore BOTH eyes.
            for(unsigned e=0;e<2;++e)if(dlaaTargets[e])upscaler.Process(renderer.context.Get(),dlaaTargets[e],dlaaColor[e].Get());
            log.Line("{\"type\":\"vr_dlaa_disabled\",\"native_fallback\":true,\"error\":"+g3vr::Quote(e.what())+"}");
        }
#endif
    }
    void CompleteCopies()
    {
        foveation.EndEye();
        pendingTextures.Drain([&]{WaitFor9();},[&](ID3D11Texture2D* destination,ID3D11Texture2D* source){
            if(wrapperUpdate)stereo::Hr(wrapperUpdate(source),"full D3D11 wrapper GPU eye conversion");
            if(source==eyes[0].shared.texture11.Get()||source==eyes[1].shared.texture11.Get()){
                unsigned eye=source==eyes[1].shared.texture11.Get()?1:0;
                auto feather=MakeFfrFeather(outputWidth,outputHeight,ffrConfigured&&(ffrEyeMask&(1u<<eye))?ffrMode:0,ffrDiameter,ffrTransition,ffrCenters[eye*2],ffrCenters[eye*2+1]);
                bool filtered=false;
                if(feather.enabled&&!ffrFeatherFault){try{
                    auto* target=ffrFeather.Target(renderer.device.Get(),outputWidth,outputHeight);
                    upscaler.Process(renderer.context.Get(),target,source);ffrFeather.Present(renderer.context.Get(),destination,feather);filtered=true;
                }catch(std::exception const& e){ffrFeatherFault=true;log.Line("{\"type\":\"vr_ffr_feather_disabled\",\"native_fallback\":true,\"error\":"+g3vr::Quote(e.what())+"}");}}
                if(!filtered)upscaler.Process(renderer.context.Get(),destination,source);
#ifdef G3VR_STEREO_FRAME_CAPTURE
                try{video.Eye(renderer.context.Get(),source,eye,frames);}catch(std::exception const& e){video.Reset();ui.writeTrace.Stop();log.Line("{\"type\":\"stereo_video_failed\",\"error\":"+g3vr::Quote(e.what())+"}");}
#endif
            }
            else renderer.context->CopyResource(destination,source);
            // Private diagnostic color copies are made while the DXVK source
            // is still owned by D3D11, before ReleaseReads returns its mutex.
            for(unsigned eye=0;eye<2;++eye)if(temporalColor[eye]&&source==eyes[eye].shared.texture11.Get())renderer.context->CopyResource(temporalColor[eye].Get(),source);
            for(unsigned eye=0;eye<2;++eye)if(dlaaEnabled&&source==eyes[eye].shared.texture11.Get()){
                renderer.context->CopyResource(dlaaColor[eye].Get(),source);dlaaTargets[eye]=destination;}
        });
        EvaluateDlaa();
        pendingCopies.Drain([&]{
#ifdef G3VR_RENDERER_FEASIBILITY
            auto trialStarted=renderer_trial::experiment.Active()?performance::Now():0;
#endif
            auto start=probes::Now();auto stats=g3vr::Wait11(renderer.context.Get(),fence11.Get());
#ifdef G3VR_RENDERER_FEASIBILITY
            if(trialStarted)renderer_trial::experiment.ScopeTime(renderer_trial::Scope::Wait11,trialStarted,performance::Now());
#endif
            metrics.Add(performance::Stage::D3D11Wait,probes::Now()-start);
            metrics.Count(performance::Counter::D3D11Polls,stats.polls);metrics.Count(performance::Counter::D3D11Flushes,stats.flushes);
            if(dxvk.Active())dxvk.ReleaseReads();
        },[&](XrSwapchain swapchain){XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};api.Check(api.ReleaseSwapchainImage(swapchain,&release),"release completed copy");});
    }
    void QueueCopy(XrSwapchain swapchain)
    {
        if(!pendingCopies.Add(swapchain))throw std::runtime_error("Duplicate or excessive XR copy in frame");
        metrics.Count(performance::Counter::Copies);
    }
    void QueueTexture(ID3D11Texture2D* destination,ID3D11Texture2D* source)
    {
        if(!pendingTextures.Add(destination,source))throw std::runtime_error("Duplicate or excessive shared texture copy in frame");
        if(dxvk.Active())dxvk.Queue(source);
    }
    void Events()
    {
        for(;;){XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};XrResult result=api.PollEvent(api.instance,&event);if(result==XR_EVENT_UNAVAILABLE)break;api.Check(result,"poll event");
            if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)throw std::runtime_error("OpenXR instance lost");
            if(event.type==XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING){
                auto const& change=*reinterpret_cast<XrEventDataReferenceSpaceChangePending const*>(&event);
                if(change.session==session&&change.referenceSpaceType==XR_REFERENCE_SPACE_TYPE_LOCAL){referenceChanges.push_back(change.changeTime);log.Line("{\"type\":\"local_reference_change_pending\",\"change_time\":"+std::to_string(change.changeTime)+"}");}
                continue;
            }
            if(event.type!=XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)continue;state=reinterpret_cast<XrEventDataSessionStateChanged*>(&event)->state;
            log.Line("{\"type\":\"session_state\",\"state\":"+std::to_string(state)+"}");if(state==XR_SESSION_STATE_FOCUSED)focusedAt=GetTickCount64();
            if(state==XR_SESSION_STATE_READY){XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;api.Check(api.BeginSession(session,&begin),"begin session");running=true;}
            if(state==XR_SESSION_STATE_STOPPING&&running){api.Check(api.EndSession(session),"end session");running=false;}
            if(state==XR_SESSION_STATE_LOSS_PENDING||state==XR_SESSION_STATE_EXITING)throw std::runtime_error("OpenXR session disconnected");}
    }
    bool ReferenceChangeAt(XrTime displayTime)
    {
        bool changed=false;
        referenceChanges.erase(std::remove_if(referenceChanges.begin(),referenceChanges.end(),[&](XrTime time){if(time>displayTime)return false;changed=true;return true;}),referenceChanges.end());
        return changed;
    }
    void ClearTemporalSnapshots()noexcept {
        if constexpr(!probes::Enabled)return;
        for(unsigned eye=0;eye<2;++eye){std::vector<float>().swap(temporalDepth[eye].depth);std::vector<float>().swap(temporalOpaqueDepth[eye].depth);temporalColor[eye].Reset();temporalDepthFrame[eye]=temporalOpaqueFrame[eye]=0;}
        dxvk.ReleaseDepthSnapshotStorage();
    }
    void ResetTemporalGpu()noexcept {
        if constexpr(!probes::Enabled)return;
#ifdef G3VR_NATIVE_MOTION
        temporalGpu.reset();temporalGpuFrames[0]=temporalGpuFrames[1]=0;temporalGpuFailed=false;temporalGpuError.clear();
#endif
    }
    std::string WriteTemporalGpu(unsigned eye){
#ifdef G3VR_NATIVE_MOTION
        if(!temporalGpu||eye>1||temporalGpuFrames[eye]!=frames)return {};
        auto* source=temporalGpu->Motion(eye);if(!source)return {};
        D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        stereo::ComPtr<ID3D11Texture2D> staging;stereo::Hr(renderer.device->CreateTexture2D(&desc,nullptr,&staging),"temporal diagnostic combined image");renderer.context->CopyResource(staging.Get(),source);
        D3D11_MAPPED_SUBRESOURCE map{};stereo::Hr(renderer.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"temporal diagnostic combined readback");
        g3vr::Scope unmap{[&]{renderer.context->Unmap(staging.Get(),0);}};
        if(desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||map.RowPitch<width*8)throw std::runtime_error("Temporal diagnostic combined format/pitch");
        auto name=log.path.stem().string()+"-composed-motion-"+std::to_string(frames)+"-eye"+std::to_string(eye)+".rgba16f";
        std::ofstream output(log.path.parent_path()/name,std::ios::binary);for(unsigned y=0;y<height;++y)output.write(static_cast<char const*>(map.pData)+y*map.RowPitch,width*8);output.close();
        if(!output)throw std::runtime_error("Temporal diagnostic combined write");return name;
#else
        return {};
#endif
    }
    void FinalizeTemporalEye(unsigned eye)noexcept {
#ifdef G3VR_NATIVE_MOTION
        if(eye>1||!temporalGpu||temporalGpuFailed||temporalGpuFrames[eye]!=frames)return;
        try{float const* view=nullptr;float const* projection=nullptr;temporal::GeometryFrameStamp stamp;
            if(!ui.temporal.Camera(eye,frames,view,projection,stamp))throw std::runtime_error("Temporal final camera stamp missing");
            auto started=probes::Now();temporalGpu->Finalize(eye,stamp,ui.temporal.SkySurface(eye),ui.temporal.SkyHistory(eye));temporalFinalizeTicks[eye]=probes::Now()-started;
        }catch(std::exception const& e){temporalGpuFailed=true;temporalGpuError=e.what();temporalGpuFrames[0]=temporalGpuFrames[1]=0;temporalGpu.reset();}
#endif
    }
    std::string WriteTemporalReactive(unsigned eye){
#ifdef G3VR_NATIVE_MOTION
        if(!temporalGpu||eye>1||!temporalGpu->Finalized(eye,frames))return {};auto* source=temporalGpu->Reactive(eye);if(!source)return {};
        D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);if(desc.Format!=DXGI_FORMAT_R8_UNORM||desc.Width!=width||desc.Height!=height)throw std::runtime_error("Temporal reactive mask format/dimensions");desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        stereo::ComPtr<ID3D11Texture2D> staging;stereo::Hr(renderer.device->CreateTexture2D(&desc,nullptr,&staging),"temporal reactive diagnostic staging");renderer.context->CopyResource(staging.Get(),source);D3D11_MAPPED_SUBRESOURCE map{};stereo::Hr(renderer.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"temporal reactive diagnostic readback");g3vr::Scope unmap{[&]{renderer.context->Unmap(staging.Get(),0);}};
        if(map.RowPitch<width)throw std::runtime_error("Temporal reactive mask pitch");auto name=log.path.stem().string()+"-reactive-mask-"+std::to_string(frames)+"-eye"+std::to_string(eye)+".r8";std::ofstream output(log.path.parent_path()/name,std::ios::binary);for(unsigned y=0;y<height;++y)output.write(static_cast<char const*>(map.pData)+y*map.RowPitch,width);output.close();if(!output)throw std::runtime_error("Temporal reactive mask write");return name;
#else
        return {};
#endif
    }
    void CaptureOpaqueDepth(){
        if((!temporalRemaining&&!dlaaEnabled)||!begun||timingEye<0||timingEye>1)return;
        unsigned eye=unsigned(timingEye);
        if(!ui.temporal.Capturing(eye,frames))return;
        // Shadow/low-poly passes may share the callback. Only the observed
        // full-resolution offscreen world target is a candidate here.
        stereo::ComPtr<IDirect3DSurface9> target,depth;stereo::ComPtr<IDirect3DTexture9> texture;
        D3DSURFACE_DESC colorDesc{},depthDesc{};D3DVIEWPORT9 viewport{};
        if(FAILED(device9->GetRenderTarget(0,&target))||FAILED(target->GetDesc(&colorDesc))||
           colorDesc.Width!=width||colorDesc.Height!=height||FAILED(target->GetContainer(IID_PPV_ARGS(&texture)))||
           FAILED(device9->GetViewport(&viewport))||viewport.Width!=width||viewport.Height!=height)return;
        try{
            stereo::Hr(device9->GetDepthStencilSurface(&depth),"opaque native depth");stereo::Hr(depth->GetDesc(&depthDesc),"opaque native depth size");
            if(depthDesc.Width!=width||depthDesc.Height!=height)throw std::runtime_error("Opaque depth dimensions do not match world color");
            auto replayStarted=probes::Now();ui.temporal.CaptureMotion(device9.Get(),depth.Get());temporalReplayTicks[eye]=probes::Now()-replayStarted;
#ifdef G3VR_NATIVE_MOTION
            if(temporalGpuFrames[eye]!=frames&&!temporalGpuFailed){
                try{
                    float const* view=nullptr;float const* projection=nullptr;temporal::GeometryFrameStamp stamp;
                    if(!ui.temporal.Camera(eye,frames,view,projection,stamp))throw std::runtime_error("Temporal live camera missing");
                    if(!temporalGpu){temporalGpu=std::make_unique<g3vr::GameTemporalInputs>();temporalGpu->Initialize(device9.Get(),renderer.device.Get(),renderer.context.Get(),width,height);}
                    auto transferStarted=probes::Now();temporalGpu->Capture(eye,stamp,view,projection,depth.Get(),ui.temporal.SkinSurface(eye),ui.temporal.NativeSurface(eye),temporalRasterJitter[eye]);temporalGpuFrames[eye]=frames;temporalTransferTicks[eye]=probes::Now()-transferStarted;
                }catch(std::exception const& e){temporalGpuFailed=true;temporalGpuError=e.what();temporalGpu.reset();}
            }
#endif
            if(!temporalRemaining||temporalRemaining==4||temporalDepthDone||temporalOpaqueFrame[eye]==frames)return;
            temporalOpaqueFrame[eye]=frames;temporalOpaqueError[eye].clear();
            if(!dxvk.CaptureDepthSnapshot(depth.Get(),temporalOpaqueDepth[eye])){
#ifdef G3VR_NATIVE_MOTION
                // One requested diagnostic frame only. The normal temporal
                // path remains GPU-only; do not mislabel opaque depth as the
                // later end-of-eye depth, which can contain other passes.
                auto* source=temporalGpu&&temporalGpuFrames[eye]==frames?temporalGpu->Depth(eye):nullptr;
                if(!source)throw std::runtime_error("Native opaque temporal depth unavailable");
                D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);if(desc.Format!=DXGI_FORMAT_R32_FLOAT||desc.Width!=width||desc.Height!=height)throw std::runtime_error("Native opaque diagnostic depth format");
                desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
                stereo::ComPtr<ID3D11Texture2D> staging;stereo::Hr(renderer.device->CreateTexture2D(&desc,nullptr,&staging),"native opaque diagnostic staging");renderer.context->CopyResource(staging.Get(),source);
                D3D11_MAPPED_SUBRESOURCE map{};stereo::Hr(renderer.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"native opaque diagnostic readback");g3vr::Scope unmap{[&]{renderer.context->Unmap(staging.Get(),0);}};
                if(map.RowPitch<width*4)throw std::runtime_error("Native opaque diagnostic pitch");auto& snapshot=temporalOpaqueDepth[eye];snapshot.width=width;snapshot.height=height;snapshot.sourceFormat=depthDesc.Format;snapshot.vulkanFormat=0;snapshot.depth.resize(size_t(width)*height);
                for(unsigned y=0;y<height;++y)memcpy(snapshot.depth.data()+size_t(y)*width,static_cast<char const*>(map.pData)+y*map.RowPitch,width*4);
#else
                throw std::runtime_error("Opaque diagnostic depth transport unavailable");
#endif
            }
        }catch(std::exception const& e){temporalOpaqueError[eye]=e.what();std::vector<float>().swap(temporalOpaqueDepth[eye].depth);}
    }
    void FlushTemporalSnapshots(){
        for(unsigned eye=0;eye<2;++eye){
            if(temporalDepthFrame[eye]!=frames)continue;auto& snapshot=temporalDepth[eye];
            std::string prefix=log.path.stem().string()+"-depth-"+std::to_string(frames)+"-eye"+std::to_string(eye);
            bool depthSaved=false,colorSaved=false;float minimum=0,maximum=0;
            if(!snapshot.depth.empty()){
                auto bounds=std::minmax_element(snapshot.depth.begin(),snapshot.depth.end());minimum=*bounds.first;maximum=*bounds.second;
                std::ofstream file(log.path.parent_path()/(prefix+".f32"),std::ios::binary);file.write(reinterpret_cast<char const*>(snapshot.depth.data()),std::streamsize(snapshot.depth.size()*sizeof(float)));file.close();depthSaved=bool(file);
            }
            if(temporalColor[eye]){auto bgr=renderer.Pixels(temporalColor[eye].Get());std::ofstream color(log.path.parent_path()/(prefix+".bgr"),std::ios::binary);color.write(reinterpret_cast<char const*>(bgr.data()),std::streamsize(bgr.size()));color.close();colorSaved=bool(color);}
            std::ostringstream line;line<<"{\"type\":\"vr_temporal_depth_snapshot\",\"xr_frame_serial\":"<<frames<<",\"eye\":"<<eye<<",\"width\":"<<snapshot.width<<",\"height\":"<<snapshot.height<<",\"d3d_format\":"<<snapshot.sourceFormat<<",\"vulkan_format\":"<<snapshot.vulkanFormat<<",\"depth_pixels_captured\":"<<(depthSaved?"true":"false")<<",\"depth_file\":"<<g3vr::Quote(prefix+".f32")<<",\"depth_encoding\":\"little_endian_float32_D3D_0_to_1\",\"minimum\":"<<minimum<<",\"maximum\":"<<maximum<<",\"has_depth_variation\":"<<(maximum-minimum>.000001f?"true":"false")<<",\"color_file\":"<<g3vr::Quote(prefix+".bgr")<<",\"color_encoding\":\"BGR8_top_left\",\"color_saved\":"<<(colorSaved?"true":"false")<<",\"error\":"<<g3vr::Quote(temporalDepthError[eye])<<",\"diagnostic_cpu_readback_only\":true,\"motion_vectors_captured\":false}";log.Line(line.str());
            auto& opaque=temporalOpaqueDepth[eye];bool opaqueSaved=false;minimum=maximum=0;
            if(temporalOpaqueFrame[eye]==frames&&!opaque.depth.empty()){
                auto bounds=std::minmax_element(opaque.depth.begin(),opaque.depth.end());minimum=*bounds.first;maximum=*bounds.second;
                std::ofstream file(log.path.parent_path()/(prefix+"-opaque.f32"),std::ios::binary);file.write(reinterpret_cast<char const*>(opaque.depth.data()),std::streamsize(opaque.depth.size()*sizeof(float)));file.close();opaqueSaved=bool(file);
            }
            if(temporalOpaqueFrame[eye]!=frames)temporalOpaqueError[eye]="Native opaque phase was not observed for this eye";
            std::ostringstream phase;phase<<"{\"type\":\"vr_temporal_depth_snapshot\",\"phase\":\"opaque_before_occlusion\",\"xr_frame_serial\":"<<frames<<",\"eye\":"<<eye<<",\"width\":"<<width<<",\"height\":"<<height<<",\"d3d_format\":"<<opaque.sourceFormat<<",\"vulkan_format\":"<<opaque.vulkanFormat<<",\"depth_pixels_captured\":"<<(opaqueSaved?"true":"false")<<",\"depth_file\":"<<g3vr::Quote(prefix+"-opaque.f32")<<",\"depth_encoding\":\"little_endian_float32_D3D_0_to_1\",\"minimum\":"<<minimum<<",\"maximum\":"<<maximum<<",\"has_depth_variation\":"<<(maximum-minimum>.000001f?"true":"false")<<",\"color_file\":"<<g3vr::Quote(prefix+".bgr")<<",\"color_encoding\":\"BGR8_top_left\",\"color_saved\":"<<(colorSaved?"true":"false")<<",\"color_stage\":\"final_eye_same_frame\",\"error\":"<<g3vr::Quote(temporalOpaqueError[eye])<<",\"diagnostic_cpu_readback_only\":true,\"motion_vectors_captured\":false}";log.Line(phase.str());
        }
        ClearTemporalSnapshots();temporalDepthDone=true;
    }
    void End(bool submit)
    {
        if(!begun)return;
        // Complete ALL D3D11 reads before releasing XR destinations. The next
        // frame cannot overwrite any D3D9 source until this barrier finishes.
        // The same ownership rule applies to partial/cancelled frames.
        CompleteCopies();
        XrCompositionLayerProjectionView pv[2]={{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};
        for(unsigned i=0;i<2;++i){pv[i].pose=views[i].pose;pv[i].fov=views[i].fov;pv[i].subImage.swapchain=eyes[i].swapchain;pv[i].subImage.imageRect.extent={int32_t(outputWidth),int32_t(outputHeight)};}
        XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=space;layer.viewCount=2;layer.views=pv;
        XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};quad.layerFlags=cinemaCaptured?0:XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        quad.space=viewSpace;quad.eyeVisibility=XR_EYE_VISIBILITY_BOTH;quad.pose.orientation.w=1;
        quad.pose.position.z=-uiLayout.distanceMeters;quad.pose.position.y=uiLayout.verticalMeters;
        auto canvas=MenuCanvas(width,height,uiLayout.menu,cinemaCaptured);
        quad.size={uiLayout.widthMeters,uiLayout.widthMeters*float(canvas.height)/float(canvas.width)};
        quad.subImage.swapchain=uiSwapchain;quad.subImage.imageRect.extent={int32_t(canvas.width),int32_t(canvas.height)};
        XrCompositionLayerQuad wheel{XR_TYPE_COMPOSITION_LAYER_QUAD};wheel.layerFlags=XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        wheel.space=viewSpace;wheel.eyeVisibility=XR_EYE_VISIBILITY_BOTH;wheel.pose.orientation.w=1;wheel.pose.position={0,-.10f,-1.3f};wheel.size={.66f*radialScale,.66f*radialScale};
        wheel.subImage.swapchain=radialSwapchain;wheel.subImage.imageRect.extent={RadialPixels,RadialPixels};
        bool statusVisible=!radial.visible&&worldStatus>0;
        if(statusVisible){auto style=WorldStatusVisual(worldStatus,uiLayout.widthMeters,uiLayout.distanceMeters,uiLayout.verticalMeters,float(height)/float(width));wheel.pose.position={style.x,style.y,-StatusDistance};wheel.size={style.metersWide,style.metersHigh};wheel.subImage.imageRect.extent={int32_t(style.width),int32_t(style.height)};}
        XrCompositionLayerBaseHeader const* layers[3]{};unsigned count=0;
        if(submit&&captured==3)layers[count++]=reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer);
        if(submit&&uiCaptured)layers[count++]=reinterpret_cast<XrCompositionLayerBaseHeader*>(&quad);
        if(submit&&uiCaptured&&(radial.visible||statusVisible)&&radialValid)layers[count++]=reinterpret_cast<XrCompositionLayerBaseHeader*>(&wheel);
        XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=frame.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        end.layerCount=count;end.layers=count?layers:nullptr;
        auto endStarted=probes::Now();begun=false;api.Check(api.EndFrame(session,&end),"end frame");metrics.Add(performance::Stage::EndFrame,probes::Now()-endStarted);
#ifdef G3VR_STEREO_FRAME_CAPTURE
        FinishVideo(submit&&captured==3);
#endif
#ifdef G3VR_RENDERER_FEASIBILITY
        renderer_trial::experiment.End(performance::Now(),submit&&captured==3);FlushFeasibility();
#endif
        if(temporalRemaining&&submit&&captured==3&&ui.temporal.Ready(frames)){
            try{
            for(unsigned eye=0;eye<2;++eye){std::ostringstream line;ui.temporal.Write(line,eye,log.path.parent_path(),log.path.stem().string(),temporalRemaining==3);log.Line(line.str());}
#ifdef G3VR_NATIVE_MOTION
            {std::string files[2],reactive[2];bool finalized[2]{};for(unsigned eye=0;eye<2;++eye)finalized[eye]=temporalGpu&&temporalGpu->Finalized(eye,frames);
                if(temporalRemaining==3)for(unsigned eye=0;eye<2;++eye)try{files[eye]=WriteTemporalGpu(eye);reactive[eye]=WriteTemporalReactive(eye);}catch(std::exception const& e){temporalGpuError=e.what();}
                std::ostringstream line;line<<"{\"type\":\"vr_live_temporal_gpu_inputs\",\"frame\":"<<frames<<",\"left_ready\":"<<(finalized[0]?"true":"false")<<",\"right_ready\":"<<(finalized[1]?"true":"false")<<",\"phase\":\"after_late_world_before_hud\",\"width\":"<<width<<",\"height\":"<<height<<",\"left_file\":"<<g3vr::Quote(files[0])<<",\"right_file\":"<<g3vr::Quote(files[1])<<",\"left_reactive_file\":"<<g3vr::Quote(reactive[0])<<",\"right_reactive_file\":"<<g3vr::Quote(reactive[1])<<",\"reactive_encoding\":\"R8_UNORM_top_left_255_bias_current_color\",\"snapshot_cpu_readback_only\":true,\"normal_input_path_gpu_only\":true,\"scene_motion_coverage_complete\":false,\"game_dlaa_enabled\":"<<(dlaaEnabled&&dlaaSerial==frames?"true":"false")<<",\"error\":"<<g3vr::Quote(temporalGpuError)<<",\"transport\":"<<g3vr::Quote(temporalGpu?temporalGpu->Transport():"unavailable")<<'}';log.Line(line.str());}
#endif
            if(!temporalDepthDone&&temporalRemaining<4)FlushTemporalSnapshots();
            --temporalRemaining;ui.temporal.Discard();
            if(!temporalRemaining){
                if(!dlaaEnabled){ResetTemporalGpu();ui.temporal.ReleaseStorage();}
                bool removed=DeleteFileW(temporalRequest.c_str())!=FALSE;log.Line(std::string("{\"type\":\"vr_temporal_probe_complete\",\"request_removed\":")+(removed?"true":"false")+"}");}
            }catch(...){temporalRemaining=0;ResetTemporalGpu();ui.temporal.ReleaseStorage();ClearTemporalSnapshots();try{log.Line("{\"type\":\"vr_temporal_probe_failed\",\"rendering_preserved\":true}");}catch(...){}}
        }
        if(!submit||captured!=3){ResetTemporalGpu();ui.temporal.Discard();ui.temporal.ResetObjectHistory();ClearTemporalSnapshots();}
        if(count){++submitted;if(captured==0)++uiOnlyFrames;performance::Report report;
            if(metrics.Finish(probes::Now(),report)){char line[4096];if(report.Json(line,sizeof(line),batchCopies))log.Line(line);}}
        else metrics.Pause();
    }
    void RequestFoveation(int mode,int diameter,int transition=10) noexcept {
        mode=FfrMode(mode);diameter=std::clamp(diameter,40,95);transition=FfrTransition(transition);
        if(mode==ffrMode&&diameter==ffrDiameter&&transition==ffrTransition)return;
        ffrMode=mode;ffrDiameter=diameter;ffrTransition=transition;ffrDirty=true;
    }
    void UpdateFoveation() {
        if(!ffrDirty)return;
        ffrDirty=false;ffrConfigured=false;
        try{
            foveation.EndEye();
            if(ffrMode){
                if(dlaaEnabled)throw std::runtime_error("Disable experimental DLAA and restart before enabling FFR");
                if(!dxvk.Active())throw std::runtime_error("FFR requires the Gothic 3 VR DXVK backend; native D3D9 is currently active");
                if(!foveation.Ready()&&!foveation.Initialize(device9.Get()))throw std::runtime_error(foveation.Error());
                float centers[4]{};for(unsigned eye=0;eye<2;++eye){auto const& f=views[eye].fov;float l=std::tan(f.angleLeft),r=std::tan(f.angleRight),u=std::tan(f.angleUp),d=std::tan(f.angleDown);centers[eye*2]=-l/(r-l);centers[eye*2+1]=u/(u-d);}
                foveation.Configure(width,height,ffrMode,ffrDiameter,centers[0],centers[1],centers[2],centers[3]);std::copy(centers,centers+4,ffrCenters);ffrConfigured=true;
            }
            log.Line(std::string("{\"type\":\"vr_ffr_applied\",\"active\":")+(ffrConfigured?"true":"false")+",\"mode\":"+std::to_string(ffrMode)+",\"central_diameter_percent\":"+std::to_string(ffrDiameter)+",\"transition_percent\":"+std::to_string(ffrTransition)+",\"transition_pattern\":\"post_aa_pixel_blur\",\"shape\":\"landscape_oval\",\"normalized_axis_ratio\":1.5,\"center_rate\":\"1x1\",\"ui_excluded\":true,\"depth_passes_only\":true}");
        }catch(std::exception const& e){foveation.Reset();log.Line("{\"type\":\"vr_ffr_error\",\"active\":false,\"mode\":"+std::to_string(ffrMode)+",\"central_diameter_percent\":"+std::to_string(ffrDiameter)+",\"message\":"+g3vr::Quote(e.what())+"}");}
    }
    bool Begin(StereoFrame& output,uint64_t generation)
    {
        if(begun)End(false);Events();if(!running){
#ifdef G3VR_RENDERER_FEASIBILITY
            renderer_trial::experiment.Unavailable();FlushFeasibility();
#endif
            controllers.frame={};metrics.Pause();return false;}
        metrics.Begin();auto waitStarted=probes::Now();
#ifdef G3VR_STEREO_FRAME_CAPTURE
        PollVideo();
#endif
#ifdef G3VR_RENDERER_FEASIBILITY
        auto trialWaitStarted=performance::Now();
#endif
        XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};api.Check(api.WaitFrame(session,&wait,&frame),"wait frame");metrics.Add(performance::Stage::XrWait,probes::Now()-waitStarted);
#ifdef G3VR_RENDERER_FEASIBILITY
        auto trialReady=performance::Now();renderer_trial::experiment.RuntimeReady(trialReady,trialReady-trialWaitStarted);
        FlushFeasibility();
#endif
        auto trackingStarted=probes::Now();XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};api.Check(api.BeginFrame(session,&begin),"begin frame");begun=true;captured=0;ffrEyeMask=0;uiCaptured=cinemaCaptured=false;uiStarted=0;++frames;
#ifdef G3VR_STEREO_FRAME_CAPTURE
        if(video.Active()){StereoFrameCapture::Frame sample;sample.serial=frames;LARGE_INTEGER q{};QueryPerformanceCounter(&q);sample.qpc=q.QuadPart;sample.displayTime=frame.predictedDisplayTime;sample.displayPeriod=frame.predictedDisplayPeriod;sample.shouldRender=frame.shouldRender;video.Begin(sample);}
#endif
        dlaaTargets[0]=dlaaTargets[1]=nullptr;dlaaOverlay={};
        for(unsigned e=0;e<2;++e)temporalReplayTicks[e]=temporalTransferTicks[e]=temporalFinalizeTicks[e]=0;
        XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};locate.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;locate.displayTime=frame.predictedDisplayTime;locate.space=space;
        XrViewState vs{XR_TYPE_VIEW_STATE};uint32_t count=0;api.Check(api.LocateViews(session,&locate,&vs,2,&count,views),"locate views");
        bool ready=frame.shouldRender&&count==2&&stereo::ValidViews(vs.viewStateFlags)&&state==XR_SESSION_STATE_FOCUSED&&GetTickCount64()-focusedAt>=100;
        output={};output.focused=state==XR_SESSION_STATE_FOCUSED;
        if(!ready){
#ifdef G3VR_RENDERER_FEASIBILITY
            renderer_trial::experiment.Unavailable();
#endif
            controllers.frame=ControllersDuringRenderWait(controllers.frame,output.focused,GetTickCount64());End(false);return false;}
        output.originChanged=ReferenceChangeAt(frame.predictedDisplayTime);
        if(output.originChanged){ResetTemporalGpu();ui.temporal.ResetObjectHistory();}
        if(output.originChanged)log.Line("{\"type\":\"local_reference_changed\",\"display_time\":"+std::to_string(frame.predictedDisplayTime)+"}");
        UpdateFoveation();
        controllers.Sync(api,session,space,viewSpace,frame.predictedDisplayTime,true);
        for(unsigned i=0;i<2;++i){auto const& p=views[i].pose;output.eyes[i]={{p.position.x,p.position.y,p.position.z},{p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w},{views[i].fov.angleLeft,views[i].fov.angleRight,views[i].fov.angleUp,views[i].fov.angleDown}};}
        auto& h=output.head;LARGE_INTEGER qpc;QueryPerformanceCounter(&qpc);h.qpc=qpc.QuadPart;h.session=generation;h.runtimeConnected=h.deviceConnected=h.valid=true;h.trackingResult=int(vs.viewStateFlags);strcpy_s(h.origin,"local");
        for(unsigned i=0;i<3;++i)h.position[i]=(output.eyes[0].position[i]+output.eyes[1].position[i])*.5f;
        h.quaternion[0]=output.eyes[0].quaternion[3];for(unsigned i=0;i<3;++i)h.quaternion[i+1]=output.eyes[0].quaternion[i];
        metrics.Add(performance::Stage::Tracking,probes::Now()-trackingStarted);metrics.Ready(probes::Now());
        if(probes::Enabled&&GetTickCount64()-lastSample>=1000){lastSample=GetTickCount64();auto const& a=views[0].pose.position;auto const& b=views[1].pose.position;
            std::ostringstream s;s<<"{\"type\":\"game_stereo_frame\",\"frames\":"<<frames<<",\"submissions\":"<<submitted<<",\"left_copies\":"<<eyes[0].copies<<",\"right_copies\":"<<eyes[1].copies<<",\"ui_frames\":"<<uiFrames<<",\"ui_draws\":"<<ui.draws<<",\"ui_only_frames\":"<<uiOnlyFrames<<",\"ipd_m\":"<<std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z))<<'}';log.Line(s.str());}
        if(probes::Enabled&&GetTickCount64()-lastControllerSample>=1000){lastControllerSample=GetTickCount64();
            std::ostringstream s;s<<"{\"type\":\"quest_controller_poses\",\"focused\":"<<controllers.frame.focused<<",\"hands\":[";
            for(unsigned i=0;i<2;++i){auto const& hand=controllers.frame.hands[i];if(i)s<<',';s<<"{\"active\":"<<hand.active<<",\"grip_valid\":"<<hand.grip.valid<<",\"aim_valid\":"<<hand.aimInView.valid<<",\"position_m\":["<<hand.grip.position[0]<<','<<hand.grip.position[1]<<','<<hand.grip.position[2]<<"],\"orientation_xyzw\":["<<hand.grip.quaternion[0]<<','<<hand.grip.quaternion[1]<<','<<hand.grip.quaternion[2]<<','<<hand.grip.quaternion[3]<<"]}";}
            s<<"]}";log.Line(s.str());}
        return true;
    }
    void Capture(unsigned index)
    {
        if(!begun||index>1||(captured&(1u<<index)))throw std::runtime_error("Invalid stereo capture order");auto& eye=eyes[index];stereo::ComPtr<IDirect3DSurface9> backbuffer;
        if(temporalRemaining&&temporalRemaining<4&&!temporalDepthDone&&ui.temporal.Observed(index,frames)){
            temporalDepthFrame[index]=frames;temporalDepthError[index].clear();
            try{stereo::ComPtr<IDirect3DSurface9> depth;stereo::Hr(device9->GetDepthStencilSurface(&depth),"temporal native depth");D3DSURFACE_DESC desc{};stereo::Hr(depth->GetDesc(&desc),"temporal native depth size");
                if(desc.Width!=width||desc.Height!=height)throw std::runtime_error("End-of-eye depth dimensions do not match color");
                if(!dxvk.CaptureDepthSnapshot(depth.Get(),temporalDepth[index]))throw std::runtime_error("Native end-of-eye depth not captured; see separate opaque phase");
            }catch(std::exception const& e){temporalDepthError[index]=e.what();std::vector<float>().swap(temporalDepth[index].depth);}
            // Color is independently useful even when final depth transport is
            // unavailable. Preserve its true stage in the snapshot metadata.
            try{temporalColor[index]=renderer.Color(width,height,DXGI_FORMAT_B8G8R8A8_UNORM);}catch(...){temporalColor[index].Reset();}
        }
        stereo::Hr(device9->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&backbuffer),"capture backbuffer");D3DSURFACE_DESC desc{};backbuffer->GetDesc(&desc);
        if(desc.Width!=width||desc.Height!=height)throw std::runtime_error("Game resolution changed; toggle stereo off/on");
        stereo::Hr(device9->StretchRect(backbuffer.Get(),nullptr,eye.shared.surface9.Get(),nullptr,D3DTEXF_NONE),"copy game eye to shared texture");
        uint32_t image=0;XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};api.Check(api.AcquireSwapchainImage(eye.swapchain,&acquire,&image),"acquire eye");
        auto waitStarted=probes::Now();XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;api.Check(api.WaitSwapchainImage(eye.swapchain,&wait),"wait eye");metrics.Add(performance::Stage::SwapchainWait,probes::Now()-waitStarted);
        XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};g3vr::Scope returnImage{[&]{api.ReleaseSwapchainImage(eye.swapchain,&release);}};
        if(image>=eye.images.size())throw std::runtime_error("Invalid eye image index");QueueCopy(eye.swapchain);returnImage.finish=nullptr;
        QueueTexture(eye.images[image].texture,eye.shared.texture11.Get());
        if(!batchCopies)CompleteCopies();captured|=1u<<index;++eye.copies;
    }
    void BeginUi()
    {
        if(!begun||uiCaptured)throw std::runtime_error("UI begin outside XR frame");uiStarted=probes::Now();uiDrawsAtBegin=ui.draws;ui.Begin();
#ifdef G3VR_UI_COMPOSITION_CAPTURE
        if(!uiDumpFile.empty()&&uiFrames==diagnosticChanged+100&&diagnosticDumps<4)ui.alphaTrace.Begin(device9.Get(),width,height);
#endif
    }
    void CaptureRadial()
    {
        if(!radial.visible&&!worldStatus)return;
        if(radialValid&&radialUploaded.visible==radial.visible&&radialUploaded.page==radial.page&&radialUploaded.selected==radial.selected&&uploadedStatus==worldStatus)return;
        if(!radialSwapchain){XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO};info.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT|XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
            info.format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;info.width=info.height=RadialPixels;info.sampleCount=info.faceCount=info.arraySize=info.mipCount=1;
            api.Check(api.CreateSwapchain(session,&info,&radialSwapchain),"radial swapchain");uint32_t count=0;api.Check(api.EnumerateSwapchainImages(radialSwapchain,0,&count,nullptr),"radial image count");
            radialImages.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});api.Check(api.EnumerateSwapchainImages(radialSwapchain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(radialImages.data())),"radial images");}
        auto started=GetTickCount64();auto pixels=radial.visible?RadialImage(radial):WorldStatusImage(worldStatus);auto drawn=GetTickCount64();uint32_t image=0;XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};api.Check(api.AcquireSwapchainImage(radialSwapchain,&acquire,&image),"acquire radial");
        auto waitStarted=probes::Now();XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;api.Check(api.WaitSwapchainImage(radialSwapchain,&wait),"wait radial");metrics.Add(performance::Stage::SwapchainWait,probes::Now()-waitStarted);
        XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};g3vr::Scope returnImage{[&]{api.ReleaseSwapchainImage(radialSwapchain,&release);}};
        if(image>=radialImages.size())throw std::runtime_error("Invalid radial image index");
        QueueCopy(radialSwapchain);returnImage.finish=nullptr;renderer.context->UpdateSubresource(radialImages[image].texture,0,nullptr,pixels.data(),RadialPixels*4,0);
        if(!batchCopies)CompleteCopies();radialUploaded=radial;uploadedStatus=worldStatus;radialValid=true;
        log.Line("{\"type\":\"radial_upload\",\"page\":"+std::to_string(radial.page)+",\"selected\":"+std::to_string(radial.selected)+",\"draw_ms\":"+std::to_string(drawn-started)+",\"total_ms\":"+std::to_string(GetTickCount64()-started)+"}");
    }
    void EndUi()
    {
        if(uiStarted){metrics.Add(performance::Stage::UiDraw,probes::Now()-uiStarted);metrics.Count(performance::Counter::UiDraws,ui.draws-uiDrawsAtBegin);uiStarted=0;}
        ui.End();if(!begun)throw std::runtime_error("UI end outside XR frame");
        if(ui.error[0])throw std::runtime_error(ui.error);
        // Opt-in GPU diagnostic after a UI state settles, bounded per session.
        // No readback occurs in normal play.
        if((probes::Enabled||!uiDumpFile.empty())&&uiFrames==diagnosticChanged+100&&diagnosticDumps<4){wchar_t dump[MAX_PATH]{};
            if(!uiDumpFile.empty()||GetEnvironmentVariableW(L"G3VR_DUMP_UI_FILE",dump,MAX_PATH)){
                std::wstring name=uiDumpFile.empty()?std::wstring(dump):uiDumpFile;auto dot=name.find_last_of(L'.');if(dot!=std::wstring::npos)name.resize(dot);
                name+=L"-state"+std::to_wstring(diagnosticState)+L"-"+std::to_wstring(++diagnosticDumps)+L".bmp";
#ifdef G3VR_UI_COMPOSITION_CAPTURE
                ui.alphaTrace.Save(name);
#endif
                stereo::ComPtr<IDirect3DSurface9> readback;
                if(SUCCEEDED(device9->CreateOffscreenPlainSurface(width,height,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr))&&
                   SUCCEEDED(device9->GetRenderTargetData(ui.shared.surface9.Get(),readback.Get()))){
                    D3DLOCKED_RECT lock{};if(SUCCEEDED(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY))){
                        FILE* file=nullptr;_wfopen_s(&file,name.c_str(),L"wb");if(file){
                            BITMAPFILEHEADER fh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+width*height*4;
                            BITMAPINFOHEADER ih{};ih.biSize=sizeof(ih);ih.biWidth=width;ih.biHeight=-LONG(height);ih.biPlanes=1;ih.biBitCount=32;ih.biSizeImage=width*height*4;
                            fwrite(&fh,sizeof(fh),1,file);fwrite(&ih,sizeof(ih),1,file);for(UINT y=0;y<height;++y)fwrite(static_cast<char*>(lock.pBits)+y*lock.Pitch,width*4,1,file);fclose(file);
                            log.Line("{\"type\":\"ui_gpu_diagnostic_saved\"}");
                        }readback->UnlockRect();
                    }
                }
            }
        }
        uint32_t image=0;XrSwapchainImageAcquireInfo acquire{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};api.Check(api.AcquireSwapchainImage(uiSwapchain,&acquire,&image),"acquire UI");
        auto waitStarted=probes::Now();XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;api.Check(api.WaitSwapchainImage(uiSwapchain,&wait),"wait UI");metrics.Add(performance::Stage::SwapchainWait,probes::Now()-waitStarted);
        XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};g3vr::Scope returnImage{[&]{api.ReleaseSwapchainImage(uiSwapchain,&release);}};
        if(image>=uiImages.size())throw std::runtime_error("Invalid UI image index");
        QueueCopy(uiSwapchain);returnImage.finish=nullptr;QueueTexture(uiImages[image].texture,ui.shared.texture11.Get());
        if(!batchCopies)CompleteCopies();uiCaptured=true;++uiFrames;CaptureRadial();
    }
    ~Runtime()
    {
#ifdef G3VR_RENDERER_FEASIBILITY
        FlushFeasibility(true);
#endif
        staticTrial.reset(); // Release experimental native imports before device shutdown.
        ui.End();
        try{End(false);}catch(...){}
        ResetTemporalGpu();ui.temporal.ReleaseStorage();ClearTemporalSnapshots();
        // End the running session while its D3D11 binding is valid. Stop event
        // handling after STOPPING; cleanup must never start another session.
        if(running&&api.RequestExitSession&&XR_SUCCEEDED(api.RequestExitSession(session)))
        {
            auto deadline=GetTickCount64()+250;
            while(running&&GetTickCount64()<deadline)
            {
                XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};auto result=api.PollEvent(api.instance,&event);
                if(result==XR_EVENT_UNAVAILABLE){Sleep(1);continue;}if(XR_FAILED(result))break;
                if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
                {
                    auto change=reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
                    if(change->session==session&&change->state==XR_SESSION_STATE_STOPPING)
                    {api.EndSession(session);running=false;}
                }
            }
        }
        if(renderer.context){renderer.context->ClearState();renderer.context->Flush();}
        foveation.Reset();
        upscaler.Reset(); // Drop cached SRVs/RTVs before XR and shared imports.
        ffrFeather.Reset();
        for(auto& eye:eyes){eye.images.clear();if(eye.swapchain&&api.DestroySwapchain)api.DestroySwapchain(eye.swapchain);}
        uiImages.clear();if(uiSwapchain&&api.DestroySwapchain)api.DestroySwapchain(uiSwapchain);
        radialImages.clear();if(radialSwapchain&&api.DestroySwapchain)api.DestroySwapchain(radialSwapchain);
        controllers.Destroy(api);
        if(viewSpace&&api.DestroySpace)api.DestroySpace(viewSpace);
        if(space&&api.DestroySpace)api.DestroySpace(space);if(session&&api.DestroySession)api.DestroySession(session);
        // Drop the D3D11 views first, then the native textures, queries and our
        // device reference before Gothic 3 destroys its own graphics wrapper.
        ui.Reset();dxvk.Reset();for(auto& eye:eyes)eye.shared.Reset();
        gpuMetrics.Reset();fence11.Reset();fence9.Reset();device9.Reset();
        log.Line("{\"type\":\"game_stereo_summary\",\"frames\":"+std::to_string(frames)+",\"submissions\":"+std::to_string(submitted)+",\"left_copies\":"+std::to_string(eyes[0].copies)+",\"right_copies\":"+std::to_string(eyes[1].copies)+",\"ui_frames\":"+std::to_string(uiFrames)+",\"ui_draws\":"+std::to_string(ui.draws)+"}");
    }
};
}
bool StereoRuntime::Connect(IDirect3DDevice9* device,wchar_t const* loader,wchar_t const* logFile,VrSettings const& settings) noexcept
{
    Disconnect();try{auto runtime=std::make_unique<Runtime>(logFile);runtime->Initialize(device,loader,settings);implementation=runtime.release();++generation;return true;}
    catch(std::exception const& e){strcpy_s(error,e.what());return false;}
}
StereoBegin StereoRuntime::Begin(StereoFrame& frame) noexcept
{
    if(!implementation)return StereoBegin::Waiting;try{return static_cast<Runtime*>(implementation)->Begin(frame,generation)?StereoBegin::Ready:StereoBegin::Waiting;}
    catch(std::exception const& e){strcpy_s(error,e.what());Cancel();return StereoBegin::Failed;}
}
bool StereoRuntime::Capture(unsigned eye) noexcept
{
    try{if(!implementation)return false;static_cast<Runtime*>(implementation)->Capture(eye);return true;}
    catch(std::exception const& e){strcpy_s(error,e.what());return false;}
}
bool StereoRuntime::Submit() noexcept
{
    try{if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);if((r.captured!=0&&r.captured!=3)||!r.uiCaptured)throw std::runtime_error("Complete stereo pair or UI-only frame required");r.End(true);return true;}
    catch(std::exception const& e){strcpy_s(error,e.what());return false;}
}
void StereoRuntime::NoteTiming(performance::Stage stage,uint64_t started) noexcept
{
    if constexpr(!probes::Enabled)return;
    if(implementation&&unsigned(stage)<performance::StageCount){auto now=probes::Now();if(now>=started){auto& r=*static_cast<Runtime*>(implementation);r.metrics.Add(stage,now-started);r.CpuSpike(unsigned(stage),performance::Names[unsigned(stage)],started,now-started,r.timingContext);}}
}
void StereoRuntime::BeginCpuUpdate(int context) noexcept {if constexpr(!probes::Enabled)return;if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.timingContext=context;r.updateMetrics.Begin(context,probes::Now());}}
void StereoRuntime::NoteCpuUpdate(performance::UpdateStage stage,uint64_t started) noexcept {
    if constexpr(!probes::Enabled)return;
    if(implementation&&started&&unsigned(stage)<performance::UpdateStageCount){auto now=probes::Now();if(now>=started){auto& r=*static_cast<Runtime*>(implementation);r.updateMetrics.Add(stage,now-started);r.CpuSpike(performance::StageCount+unsigned(stage),performance::UpdateNames[unsigned(stage)],started,now-started,r.timingContext);}}
}
void StereoRuntime::NoteNativePhase(performance::NativePhase stage,uint64_t started,int context) noexcept {
    if constexpr(!probes::Enabled)return;
    try{if(!implementation||!started||unsigned(stage)>=performance::NativePhaseCount)return;
        auto now=probes::Now();if(now<started)return;auto& r=*static_cast<Runtime*>(implementation);
        performance::NativePhaseReport report;
        if(r.nativeMetrics.Add(stage,now-started,now,context,report)){char line[performance::DeferredProfileLines::LineCapacity];if(report.Json(line,sizeof(line)))r.ProfileLine(line);}
        r.CpuSpike(performance::StageCount+performance::UpdateStageCount+unsigned(stage),performance::NativePhaseNames[unsigned(stage)],started,now-started,context);
    }catch(...){}
}
void StereoRuntime::EndCpuUpdate() noexcept {
    if constexpr(!probes::Enabled)return;
    try {
        if(!implementation)return;
        auto& runtime=*static_cast<Runtime*>(implementation);performance::UpdateReport report;
        if(runtime.updateMetrics.End(probes::Now(),report)){
            char line[2048];if(report.Json(line,sizeof(line)))runtime.log.Line(line);
        }
    }catch(...){}
}
void StereoRuntime::NoteSceneStall(char const* scope,uint64_t started,int context,performance::SceneWorkState const& before,performance::SceneWorkState const& after,uint64_t ended) noexcept {
    if constexpr(!probes::Enabled)return;
    try{if(!implementation||!started||ended<started)return;auto& r=*static_cast<Runtime*>(implementation);
        if(!r.performanceFrequency||ended-started<r.performanceFrequency/50)return;
        unsigned key=!strcmp(scope,"pvs_prefetch_process_inclusive")?0:1;
        if(ended<r.sceneStallAfter[key])return;r.sceneStallAfter[key]=ended+r.performanceFrequency/4;
        auto sample=performance::SceneWorkSample::Between(ended-started,r.performanceFrequency,before.work,after.work);char line[1536];
        if(sample.Json(line,sizeof(line),scope,context,r.timingEye,r.frames,started,r.performanceFrequency,before.threading,after.threading,before.jobs,after.jobs,before.workers,after.workers))r.ProfileLine(line);
    }catch(...){}
}
void StereoRuntime::BeginGpuEye(unsigned eye,int context) noexcept {
#ifdef G3VR_STEREO_FRAME_CAPTURE
    if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.ui.writeTrace.Eye(r.frames,int(eye));}
#endif
    if(implementation){auto& r=*static_cast<Runtime*>(implementation);if(r.staticTrial)r.staticTrial->BeginEye(eye,r.frames,context);}
    if(implementation){auto& r=*static_cast<Runtime*>(implementation);try{if(r.ffrConfigured&&context==1&&eye<2){r.foveation.BeginEye(eye);r.ffrEyeMask|=1u<<eye;}else r.foveation.EndEye();}catch(...){r.foveation.Reset();r.ffrConfigured=false;}}
    if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.timingContext=context;r.timingEye=int(eye);if constexpr(probes::Enabled)r.gpuMetrics.BeginEye(eye,context);bool probe=PrepareTemporalEye(eye,context);if(r.drawProfile||probe||(r.staticTrial&&r.staticTrial->MaterialObservationEnabled()))r.ui.BeginWorldTiming();}
}
bool StereoRuntime::PrepareTemporalEye(unsigned eye,int context) noexcept {
    try{return implementation&&static_cast<Runtime*>(implementation)->PrepareTemporalEye(eye,context);}catch(...){if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.ui.temporal.Stop();if(eye<2){r.temporalPreparedProbes[eye]=false;r.temporalRasterJitter[eye]={};}}return false;}
}
bool StereoRuntime::ApplyTemporalProjection(unsigned eye,float* projection) noexcept {
    try{if(!implementation||eye>1||!projection)return false;auto& r=*static_cast<Runtime*>(implementation);
        if(r.temporalPreparedFrames[eye]!=r.frames||!r.temporalPreparedProbes[eye]||(!r.temporalJitterOptIn&&!r.dlaaEnabled))return true;
        std::array<float,16> source{};std::copy_n(projection,16,source.begin());auto result=temporal::JitterProjection(source,r.width,r.height,r.temporalRasterJitter[eye][0],r.temporalRasterJitter[eye][1]);std::copy(result.begin(),result.end(),projection);return true;
    }catch(...){return false;}
}
void StereoRuntime::UnjitterTemporalProjection(unsigned eye,float* projection) noexcept {
    try{if(!implementation||eye>1||!projection)return;auto& r=*static_cast<Runtime*>(implementation);
        if(r.temporalPreparedFrames[eye]!=r.frames||!r.temporalPreparedProbes[eye]||(!r.temporalJitterOptIn&&!r.dlaaEnabled))return;
        std::array<float,16> source{};std::copy_n(projection,16,source.begin());auto result=temporal::JitterProjection(source,r.width,r.height,-r.temporalRasterJitter[eye][0],-r.temporalRasterJitter[eye][1]);std::copy(result.begin(),result.end(),projection);
    }catch(...){}
}
void StereoRuntime::ProtectTemporalReticle(unsigned eye,bool visible,float u,float v) noexcept {
    if(!implementation||eye>1)return;auto& r=*static_cast<Runtime*>(implementation);r.dlaaOverlay[eye]={};
    if(!r.dlaaEnabled||!visible||!std::isfinite(u)||!std::isfinite(v))return;
    float radius=std::max(4.f,r.height*.0035f)+3.f;int x=int(u*r.width),y=int(v*r.height);
    r.dlaaOverlay[eye]={uint32_t(std::clamp(int(std::floor(x-radius)),0,int(r.width))),uint32_t(std::clamp(int(std::floor(y-radius)),0,int(r.height))),uint32_t(std::clamp(int(std::ceil(x+radius+1)),0,int(r.width))),uint32_t(std::clamp(int(std::ceil(y+radius+1)),0,int(r.height)))};
}
bool StereoRuntime::TemporalDiagnosticActive()const noexcept{return implementation&&static_cast<Runtime*>(implementation)->temporalRemaining!=0;}
void StereoRuntime::EndGpuEye(unsigned eye) noexcept {
    try {
        if(!implementation)return;
        auto& r=*static_cast<Runtime*>(implementation);
#ifdef G3VR_STEREO_FRAME_CAPTURE
        r.ui.writeTrace.Eye(r.frames,-1);
#endif
        if constexpr(!probes::Enabled){
            if constexpr(probes::StaticTrial){if(r.staticTrial)r.staticTrial->EndEye();performance::EyeDrawSample unused;r.ui.EndWorldTiming(unused);}
            r.foveation.EndEye();r.timingEye=-1;return;
        }
        performance::EyeDrawSample drawSample;
        if(r.staticTrial)r.staticTrial->EndEye();
        bool drawReady=r.ui.EndWorldTiming(drawSample);r.FinalizeTemporalEye(eye);r.ui.temporal.End(eye);r.foveation.EndEye();r.gpuMetrics.EndEye(eye);
        r.timingEye=-1;r.FlushProfileLines();if(drawReady&&r.drawProfile)r.ReportEyeDraw(drawSample,eye);
        if(eye){performance::GpuReport report;
            if(r.gpuMetrics.Report(probes::Now(),report)){
                char line[1024];if(report.Json(line,sizeof(line)))r.log.Line(line);
            }
        }
    }catch(...){}
}
bool StereoRuntime::TemporalEyeActive() const noexcept {return implementation&&static_cast<Runtime*>(implementation)->ui.temporal.Active();}
#ifdef G3VR_STEREO_FRAME_CAPTURE
void StereoRuntime::CaptureCamera(unsigned eye,float const* view,float const* projection) noexcept {
    if(!implementation||eye>1)return;auto& r=*static_cast<Runtime*>(implementation);if(!r.video.Active())return;
    auto const& p=r.views[eye].pose;auto const& f=r.views[eye].fov;
    float pose[]={p.position.x,p.position.y,p.position.z,p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w};
    float fov[]={f.angleLeft,f.angleRight,f.angleUp,f.angleDown};r.video.Pose(eye,pose,fov);r.video.Camera(eye,view,projection);
}
#endif
void StereoRuntime::RecordTemporalCamera(float const* view,float const* projection) noexcept {if(implementation)static_cast<Runtime*>(implementation)->ui.temporal.Matrices(view,projection);}
void StereoRuntime::CaptureTemporalOpaqueDepth() noexcept {try{if(implementation)static_cast<Runtime*>(implementation)->CaptureOpaqueDepth();}catch(...) {}}
unsigned StereoRuntime::BeginTemporalObject(temporal::Object const& object) noexcept {return implementation?static_cast<Runtime*>(implementation)->ui.temporal.PushObject(object):UINT_MAX;}
void StereoRuntime::EndTemporalObject(unsigned token) noexcept {if(implementation)static_cast<Runtime*>(implementation)->ui.temporal.PopObject(token);}
unsigned StereoRuntime::BeginTemporalInstances(temporal::InstanceEntity const* entities,unsigned count) noexcept {return implementation?static_cast<Runtime*>(implementation)->ui.temporal.PushInstances(entities,count):UINT_MAX;}
void StereoRuntime::EndTemporalInstances(unsigned token) noexcept {if(implementation)static_cast<Runtime*>(implementation)->ui.temporal.PopInstances(token);}
bool StereoRuntime::StaticTrialWantsContext(uintptr_t cookie)const noexcept {if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial&&r.staticTrial->WantsContext(cookie);}
bool StereoRuntime::StaticTrialWantsMaterial(uintptr_t cookie)const noexcept {if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial&&r.staticTrial->WantsMaterial(cookie);}
unsigned StereoRuntime::BeginStaticTrialMaterial(uintptr_t cookie,char const* name)noexcept {if(!implementation)return UINT_MAX;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial?r.staticTrial->BeginMaterial(cookie,name):UINT_MAX;}
bool StereoRuntime::StaticTrialMaterialObservationEnabled()const noexcept {if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial&&r.staticTrial->MaterialObservationEnabled();}
bool StereoRuntime::StaticTrialWantsMaterialEntity(temporal::InstanceEntity const& entity)const noexcept {if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial&&r.staticTrial->WantsMaterialEntity(entity.id.value);}
unsigned StereoRuntime::BeginStaticTrialDeferredMaterial(temporal::InstanceEntity const* entity,char const* name,g3vr::dx11::NativeLightmapSource const& lightmap)noexcept {if(!implementation)return UINT_MAX;auto& r=*static_cast<Runtime*>(implementation);return r.staticTrial?r.staticTrial->BeginDeferredMaterial(entity?entity->id.value:std::array<uint32_t,5>{},entity?entity->world.data():nullptr,name,lightmap):UINT_MAX;}
void StereoRuntime::EndStaticTrialMaterial(unsigned token)noexcept {if(implementation&&token!=UINT_MAX){auto& r=*static_cast<Runtime*>(implementation);if(r.staticTrial)r.staticTrial->EndMaterial(token);}}
void StereoRuntime::ObserveStaticTrial(uintptr_t cookie,g3vr::dx11::NativeStaticSnapshot const& s,float const* view,float const* projection)noexcept {if(implementation){auto& r=*static_cast<Runtime*>(implementation);if(r.staticTrial)r.staticTrial->Observe(cookie,s,view,projection);}}
void StereoRuntime::StaticTrialSolidBoundary(float const* view,float const* projection)noexcept {
    if(!implementation)return;auto& r=*static_cast<Runtime*>(implementation);if(!r.staticTrial||!r.staticTrial->PendingPass(view,projection))return;
    try{r.foveation.EndEye();r.staticTrial->SolidBoundary(view,projection);if(r.ffrConfigured&&r.timingContext==1&&r.timingEye>=0)r.foveation.BeginEye(unsigned(r.timingEye));}
    catch(...){r.staticTrial.reset();r.foveation.Reset();r.ffrConfigured=false;}
}
bool StereoRuntime::BeginUi() noexcept
{
    try{if(!implementation)return false;static_cast<Runtime*>(implementation)->BeginUi();return true;}
    catch(std::exception const& e){strcpy_s(error,e.what());return false;}
}
bool StereoRuntime::CaptureCinema() noexcept
{
    try{if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);
        if(!r.begun)throw std::runtime_error("Cinema outside XR frame");
        r.ui.CopyBackbuffer();r.cinemaCaptured=true;r.EndUi();return true;
    }catch(std::exception const& e){strcpy_s(error,e.what());Cancel();return false;}
    catch(...){strcpy_s(error,"Cinema capture exception");Cancel();return false;}
}
bool StereoRuntime::EndUi() noexcept
{
    try{if(!implementation)return false;static_cast<Runtime*>(implementation)->EndUi();return true;}
    catch(std::exception const& e){strcpy_s(error,e.what());return false;}
}
void StereoRuntime::Cancel() noexcept {try{if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.timingEye=-1;r.gpuMetrics.Cancel();r.ui.End();r.FlushProfileLines();r.End(false);}}catch(...){}}
void StereoRuntime::SetUiLayout(UiLayout const& layout) noexcept {if(implementation)static_cast<Runtime*>(implementation)->uiLayout=layout;}
void StereoRuntime::SetFoveation(int mode,int diameter,int transition) noexcept {if(implementation)static_cast<Runtime*>(implementation)->RequestFoveation(mode,diameter,transition);}
void StereoRuntime::SetUiDiagnosticState(int state) noexcept {if(implementation){auto* r=static_cast<Runtime*>(implementation);if(!probes::Enabled&&r->uiDumpFile.empty())return;if(state!=r->diagnosticState){r->diagnosticState=state;r->diagnosticChanged=r->uiFrames;}}}
void StereoRuntime::SetRadial(RadialState const& state,int scalePercent) noexcept {if(implementation){auto& r=*static_cast<Runtime*>(implementation);r.radial=state;r.radialScale=float(std::clamp(scalePercent,40,120))*.01f;}}
void StereoRuntime::SetWorldStatus(int status) noexcept {if(implementation)static_cast<Runtime*>(implementation)->worldStatus=std::clamp(status,0,4);}
ControllerFrame StereoRuntime::Controllers() const noexcept {return implementation?static_cast<Runtime*>(implementation)->controllers.frame:ControllerFrame{};}
bool StereoRuntime::Haptic(unsigned hand,float amplitude,int milliseconds) noexcept {try{if(!implementation)return false;auto& r=*static_cast<Runtime*>(implementation);bool sent=r.controllers.Pulse(r.api,r.session,hand,amplitude,milliseconds);if constexpr(probes::Enabled)r.log.Line("{\"type\":\"controller_haptic\",\"hand\":"+std::to_string(hand)+",\"result\":"+std::to_string(r.controllers.lastHapticResult)+",\"duration_ms\":"+std::to_string(milliseconds)+",\"sent\":"+(sent?"true":"false")+"}");return sent;}catch(...){return false;}}
void StereoRuntime::Disconnect() noexcept {delete static_cast<Runtime*>(implementation);implementation=nullptr;}
uint64_t StereoRuntime::Submissions() const {return implementation?static_cast<Runtime*>(implementation)->submitted:0;}
}
