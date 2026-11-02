// Independent x86 geometric-stereo client. The installed Gothic 3 plugin is untouched.
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include "StereoRenderer.h"
#include <openxr/openxr_platform.h>
#include <shellapi.h>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <functional>
#include "StereoOpenXrApi.h"
namespace fs=std::filesystem;
using stereo::ComPtr;
using g3vr::Log; using g3vr::Scope; using g3vr::Api; using g3vr::Quote; using g3vr::Utf8;
struct Eye
{
    XrSwapchain swapchain=XR_NULL_HANDLE;uint32_t width=0,height=0;std::vector<XrSwapchainImageD3D11KHR> images;
    std::vector<ComPtr<ID3D11RenderTargetView>> targets;ComPtr<ID3D11DepthStencilView> depth;
    ComPtr<ID3D11Texture2D> mirror;ComPtr<ID3D11ShaderResourceView> srv;
};
struct Probe
{
    Log& log;Api api;stereo::Renderer renderer;XrSystemId system=XR_NULL_SYSTEM_ID;XrSession session=XR_NULL_HANDLE;XrSpace space=XR_NULL_HANDLE;
    Eye eyes[2];HWND window=nullptr;bool quit=false,recenter=true,running=false,anchored=false;XrSessionState state=XR_SESSION_STATE_UNKNOWN;uint64_t focusedAt=0;
    uint64_t frames=0,submitted=0,draws[2]={},skipped=0;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
    explicit Probe(Log& l):log(l){}
    static LRESULT CALLBACK WindowProc(HWND w,UINT msg,WPARAM key,LPARAM data)
    {
        auto p=reinterpret_cast<Probe*>(GetWindowLongPtrW(w,GWLP_USERDATA));
        if(msg==WM_NCCREATE){p=static_cast<Probe*>(reinterpret_cast<CREATESTRUCTW*>(data)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(p));}
        if(p){if(msg==WM_CLOSE){p->quit=true;return 0;}if(msg==WM_KEYDOWN){if(key==VK_ESCAPE)p->quit=true;if(key=='R')p->recenter=true;return 0;}}
        return DefWindowProcW(w,msg,key,data);
    }
    void Graphics(fs::path const& loader)
    {
        api.Initialize(loader,log);XrSystemGetInfo info{XR_TYPE_SYSTEM_GET_INFO};info.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        api.Check(api.GetSystem(api.instance,&info,&system),"xrGetSystem (Quest connected and awake)");
        XrSystemProperties props{XR_TYPE_SYSTEM_PROPERTIES};api.Check(api.GetSystemProperties(api.instance,system,&props),"system properties");
        XrGraphicsRequirementsD3D11KHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};api.Check(api.GetD3D11GraphicsRequirementsKHR(api.instance,system,&req),"D3D11 requirements");
        ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
        for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> candidate;HRESULT hr=factory->EnumAdapters1(i,&candidate);if(hr==DXGI_ERROR_NOT_FOUND)break;stereo::Hr(hr,"enumerate GPU");
            DXGI_ADAPTER_DESC1 desc{};candidate->GetDesc1(&desc);if(!memcmp(&desc.AdapterLuid,&req.adapterLuid,sizeof(LUID))){adapter=candidate;break;}}
        if(!adapter)throw std::runtime_error("VDXR GPU adapter not found");renderer.Initialize(adapter.Get(),req.minFeatureLevel);
        log.Line("{\"type\":\"graphics_ready\",\"headset\":"+Quote(props.systemName)+",\"adapter\":"+Quote(Utf8(renderer.adapterDescription.Description))+"}");
    }
    void Session(float scale)
    {
        XrGraphicsBindingD3D11KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};binding.device=renderer.device.Get();
        XrSessionCreateInfo si{XR_TYPE_SESSION_CREATE_INFO};si.next=&binding;si.systemId=system;api.Check(api.CreateSession(api.instance,&si,&session),"xrCreateSession D3D11");
        XrReferenceSpaceCreateInfo ri{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};ri.referenceSpaceType=XR_REFERENCE_SPACE_TYPE_LOCAL;ri.poseInReferenceSpace.orientation.w=1;
        api.Check(api.CreateReferenceSpace(session,&ri,&space),"LOCAL reference space");uint32_t count=0;
        api.Check(api.EnumerateEnvironmentBlendModes(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"blend count");
        std::vector<XrEnvironmentBlendMode> blends(count);api.Check(api.EnumerateEnvironmentBlendModes(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,count,&count,blends.data()),"blend modes");
        if(std::find(blends.begin(),blends.end(),XR_ENVIRONMENT_BLEND_MODE_OPAQUE)==blends.end())throw std::runtime_error("Opaque VR not supported");
        api.Check(api.EnumerateViewConfigurationViews(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,0,&count,nullptr),"stereo views count");
        if(count!=2)throw std::runtime_error("Expected two stereo views");XrViewConfigurationView views[2]={{XR_TYPE_VIEW_CONFIGURATION_VIEW},{XR_TYPE_VIEW_CONFIGURATION_VIEW}};
        api.Check(api.EnumerateViewConfigurationViews(api.instance,system,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,views),"stereo configuration");
        api.Check(api.EnumerateSwapchainFormats(session,0,&count,nullptr),"format count");std::vector<int64_t> formats(count);api.Check(api.EnumerateSwapchainFormats(session,count,&count,formats.data()),"formats");
        for(auto preferred:{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,DXGI_FORMAT_B8G8R8A8_UNORM_SRGB})if(std::find(formats.begin(),formats.end(),preferred)!=formats.end()){format=preferred;break;}
        if(format==DXGI_FORMAT_UNKNOWN)throw std::runtime_error("No supported sRGB color format");
        for(unsigned i=0;i<2;++i){auto& e=eyes[i];e.width=std::max(1u,uint32_t(std::ceil(views[i].recommendedImageRectWidth*scale)));e.height=std::max(1u,uint32_t(std::ceil(views[i].recommendedImageRectHeight*scale)));
            XrSwapchainCreateInfo ci{XR_TYPE_SWAPCHAIN_CREATE_INFO};ci.usageFlags=XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;ci.format=format;ci.sampleCount=1;ci.width=e.width;ci.height=e.height;ci.faceCount=ci.arraySize=ci.mipCount=1;
            api.Check(api.CreateSwapchain(session,&ci,&e.swapchain),"create eye swapchain");api.Check(api.EnumerateSwapchainImages(e.swapchain,0,&count,nullptr),"eye images count");
            e.images.resize(count,{XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});api.Check(api.EnumerateSwapchainImages(e.swapchain,count,&count,reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data())),"eye images");
            for(auto const& image:e.images)e.targets.push_back(renderer.Target(image.texture,format));e.depth=renderer.Depth(e.width,e.height);e.mirror=renderer.Color(e.width,e.height,format);
            stereo::Hr(renderer.device->CreateShaderResourceView(e.mirror.Get(),nullptr,&e.srv),"eye mirror SRV");
            log.Line("{\"type\":\"eye_swapchain\",\"eye\":"+std::to_string(i)+",\"width\":"+std::to_string(e.width)+",\"height\":"+std::to_string(e.height)+",\"images\":"+std::to_string(count)+"}");}
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=WindowProc;wc.lpszClassName=L"Gothic3VRStereoTest";wc.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
        if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Register window");
        window=CreateWindowW(wc.lpszClassName,L"Gothic3VR stereo test - R: recenter | Esc: exit",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,1016,589,nullptr,nullptr,wc.hInstance,this);
        if(!window)throw std::runtime_error("Create mirror window");renderer.CreateMirror(window);ShowWindow(window,SW_SHOW);UpdateWindow(window);
    }
    void Events()
    {
        for(;;){XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};XrResult result=api.PollEvent(api.instance,&event);if(result==XR_EVENT_UNAVAILABLE)break;api.Check(result,"poll event");
            if(event.type==XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)quit=true;
            if(event.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){auto& change=*reinterpret_cast<XrEventDataSessionStateChanged*>(&event);state=change.state;
                log.Line("{\"type\":\"session_state\",\"state\":"+std::to_string(state)+"}");
                if(state==XR_SESSION_STATE_FOCUSED)focusedAt=GetTickCount64();
                if(state==XR_SESSION_STATE_READY&&!quit){XrSessionBeginInfo begin{XR_TYPE_SESSION_BEGIN_INFO};begin.primaryViewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;api.Check(api.BeginSession(session,&begin),"begin stereo session");running=true;}
                if(state==XR_SESSION_STATE_STOPPING&&running){api.Check(api.EndSession(session),"end session");running=false;}
                if(state==XR_SESSION_STATE_EXITING||state==XR_SESSION_STATE_LOSS_PENDING)quit=true;}}
    }
    void Frame(bool sample)
    {
        XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};XrFrameState frame{XR_TYPE_FRAME_STATE};api.Check(api.WaitFrame(session,&wi,&frame),"wait frame");
        XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};api.Check(api.BeginFrame(session,&bi),"begin frame");++frames;
        XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO};end.displayTime=frame.predictedDisplayTime;end.environmentBlendMode=XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        Scope close{[&]{api.EndFrame(session,&end);}};
        XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};XrViewState vs{XR_TYPE_VIEW_STATE};XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO};
        locate.viewConfigurationType=XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;locate.displayTime=frame.predictedDisplayTime;locate.space=space;uint32_t count=0;
        api.Check(api.LocateViews(session,&locate,&vs,2,&count,views),"locate eye poses");
        bool valid=count==2&&stereo::ValidViews(vs.viewStateFlags);
        if(sample){std::ostringstream s;s<<"{\"type\":\"frame_sample\",\"frame\":"<<frames<<",\"should_render\":"<<(frame.shouldRender?"true":"false")<<",\"flags\":"<<vs.viewStateFlags<<",\"submitted\":"<<submitted;
            if(valid){auto const& a=views[0].pose.position;auto const& b=views[1].pose.position;float distance=std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
                s<<",\"ipd_m\":"<<distance<<",\"left_position_m\":["<<a.x<<','<<a.y<<','<<a.z<<"],\"left_orientation_xyzw\":["<<views[0].pose.orientation.x<<','<<views[0].pose.orientation.y<<','<<views[0].pose.orientation.z<<','<<views[0].pose.orientation.w<<']';}s<<'}';log.Line(s.str());}
        // VDXR may report a valid identity pose during startup, before the headset is focused.
        // Anchor only after a short focused interval; later frames keep the world fixed.
        bool readyForAnchor=state==XR_SESSION_STATE_FOCUSED&&GetTickCount64()-focusedAt>=100;
        if(!frame.shouldRender||!valid||(!anchored&&!readyForAnchor)){++skipped;close.finish=nullptr;api.Check(api.EndFrame(session,&end),"end empty frame");return;}
        if(recenter&&readyForAnchor){renderer.anchor=stereo::SceneAnchor(views);recenter=false;anchored=true;log.Line("{\"type\":\"scene_recenter\"}");}
        XrCompositionLayerProjectionView pv[2]={{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW},{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW}};
        for(unsigned i=0;i<2;++i){auto& e=eyes[i];uint32_t image=0;XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};api.Check(api.AcquireSwapchainImage(e.swapchain,&ai,&image),"acquire eye image");
            XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};wait.timeout=XR_INFINITE_DURATION;api.Check(api.WaitSwapchainImage(e.swapchain,&wait),"wait eye image");
            XrSwapchainImageReleaseInfo release{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};Scope returnImage{[&]{api.ReleaseSwapchainImage(e.swapchain,&release);}};
            if(image>=e.targets.size())throw std::runtime_error("Invalid swapchain image index");renderer.Draw(e.targets[image].Get(),e.depth.Get(),e.width,e.height,views[i]);++draws[i];
            renderer.context->CopyResource(e.mirror.Get(),e.images[image].texture);returnImage.finish=nullptr;api.Check(api.ReleaseSwapchainImage(e.swapchain,&release),"release eye image");
            pv[i].pose=views[i].pose;pv[i].fov=views[i].fov;pv[i].subImage.swapchain=e.swapchain;pv[i].subImage.imageRect.extent={int32_t(e.width),int32_t(e.height)};}
        renderer.context->Flush();XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};layer.space=space;layer.viewCount=2;layer.views=pv;
        XrCompositionLayerBaseHeader const* layers[]={reinterpret_cast<XrCompositionLayerBaseHeader*>(&layer)};end.layerCount=1;end.layers=layers;
        close.finish=nullptr;api.Check(api.EndFrame(session,&end),"submit stereo projection");++submitted;renderer.Mirror(eyes[0].srv.Get(),eyes[1].srv.Get());
    }
    int Run(unsigned seconds)
    {
        uint64_t start=GetTickCount64(),last=0;
        while(!quit&&GetTickCount64()-start<uint64_t(seconds)*1000){MSG msg;while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)quit=true;TranslateMessage(&msg);DispatchMessageW(&msg);}Events();if(quit)break;
            if(running){auto now=GetTickCount64();bool sample=now-last>=1000;if(sample)last=now;Frame(sample);}else Sleep(10);}
        quit=true;if(running){api.Check(api.RequestExitSession(session),"request session exit");auto deadline=GetTickCount64()+2000;while(running&&GetTickCount64()<deadline){Events();if(running)Sleep(10);}}
        log.Line("{\"type\":\"summary\",\"frames\":"+std::to_string(frames)+",\"stereo_submissions\":"+std::to_string(submitted)+",\"left_draws\":"+std::to_string(draws[0])+",\"right_draws\":"+std::to_string(draws[1])+",\"skipped\":"+std::to_string(skipped)+"}");return submitted?0:2;
    }
    ~Probe()
    {
        if(renderer.context){renderer.context->ClearState();renderer.context->Flush();}
        for(auto& e:eyes){e.targets.clear();e.srv.Reset();e.mirror.Reset();e.depth.Reset();e.images.clear();if(e.swapchain&&api.DestroySwapchain)api.DestroySwapchain(e.swapchain);}
        if(space&&api.DestroySpace)api.DestroySpace(space);if(session&&api.DestroySession)api.DestroySession(session);if(window)DestroyWindow(window);
    }
};
int SelfTest(Log& log)
{
    stereo::Renderer r;r.Initialize();XrView views[2]={{XR_TYPE_VIEW},{XR_TYPE_VIEW}};std::vector<unsigned char> pixels[2];
    for(int i=0;i<2;++i){views[i].pose.orientation.w=1;views[i].pose.position.x=i?.032f:-.032f;views[i].fov={-.7f,.7f,.7f,-.7f};auto color=r.Color(512,512,DXGI_FORMAT_R8G8B8A8_UNORM);auto target=r.Target(color.Get(),DXGI_FORMAT_R8G8B8A8_UNORM);auto depth=r.Depth(512,512);
        r.Draw(target.Get(),depth.Get(),512,512,views[i]);pixels[i]=r.Pixels(color.Get());std::ofstream ppm(log.path.parent_path()/(i?"stereo-right.ppm":"stereo-left.ppm"),std::ios::binary);ppm<<"P6\n512 512\n255\n";ppm.write(reinterpret_cast<char const*>(pixels[i].data()),pixels[i].size());}
    unsigned changed=0,scene=0;for(size_t j=0;j<pixels[0].size();j+=3){if(pixels[0][j]!=pixels[1][j]||pixels[0][j+1]!=pixels[1][j+1]||pixels[0][j+2]!=pixels[1][j+2])++changed;
        if(pixels[0][j]>30||pixels[0][j+1]>30||pixels[0][j+2]>30)++scene;}
    log.Line("{\"type\":\"gpu_self_test\",\"adapter\":"+Quote(Utf8(r.adapterDescription.Description))+",\"different_eye_pixels\":"+std::to_string(changed)+",\"scene_pixels\":"+std::to_string(scene)+"}");
    if(changed<100||scene<1000)throw std::runtime_error("GPU stereo self-test produced insufficient geometry/disparity");r.context->ClearState();return 0;
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int)
{
    wchar_t executable[32768];GetModuleFileNameW(nullptr,executable,32768);fs::path directory=fs::path(executable).parent_path();
    fs::path loader=directory/L"openxr_loader.dll",logPath=directory/L"logs"/(L"stereo-"+std::to_wstring(GetTickCount64())+L".jsonl");unsigned seconds=120;float scale=.6f;bool self=false,check=false;
    int argc=0;wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);Scope args{[&]{LocalFree(argv);}};
    try{for(int i=1;i<argc;++i){std::wstring arg=argv[i];if(arg==L"--self-test")self=true;else if(arg==L"--check-runtime")check=true;
            else if(i+1<argc&&arg==L"--log")logPath=fs::absolute(argv[++i]);else if(i+1<argc&&arg==L"--loader")loader=fs::absolute(argv[++i]);
            else if(i+1<argc&&arg==L"--seconds")seconds=std::stoul(argv[++i]);else if(i+1<argc&&arg==L"--scale")scale=std::stof(argv[++i]);else throw std::runtime_error("Invalid argument");}
        if(seconds<1||seconds>600||!std::isfinite(scale)||scale<.25f||scale>1)throw std::runtime_error("Invalid duration/scale");
        Log log(logPath);try{log.Line("{\"type\":\"start\",\"version\":\"0.1.0\",\"scene\":\"geometric_stereo\"}");if(self)return SelfTest(log);
            Probe probe(log);probe.Graphics(loader);if(check){log.Line("{\"type\":\"runtime_check_passed\",\"rendered\":false}");return 0;}probe.Session(scale);return probe.Run(seconds);}
        catch(std::exception const& e){log.Line("{\"type\":\"error\",\"message\":"+Quote(e.what())+"}");return 1;}}
    catch(std::exception const&){return 1;}
}
