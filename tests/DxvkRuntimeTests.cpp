// Real DXVK/Vulkan/D3D11 GPU work through the actual Runtime capture/drain
// methods. OpenXR acquisition/submission is a fixture: no headset or game.
#include "../src/StereoRuntime.cpp"
#include <array>
#include <cstdio>
using stereo::ComPtr;
void Require(bool condition,char const* message){if(!condition)throw std::runtime_error(message);}
struct Module {HMODULE handle{};~Module(){if(handle)FreeLibrary(handle);}};
struct Window {HWND handle=CreateWindowW(L"STATIC",L"DXVK runtime GPU test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(handle)DestroyWindow(handle);}};
struct Fixture {
    ComPtr<ID3D11Texture2D> targets[4];bool acquired[4]{};
    unsigned releases=0,endCalls=0,expectedLayers=0;bool failRelease=false;
    ID3D11Device* device{};
    unsigned Index(XrSwapchain handle){Require(handle>=14&&handle<=17,"Unknown fixture swapchain");return unsigned(handle-14);}
} fixture;
XrResult XRAPI_CALL Acquire(XrSwapchain h,XrSwapchainImageAcquireInfo const*,uint32_t* index){auto i=fixture.Index(h);Require(!fixture.acquired[i],"Image acquired twice");fixture.acquired[i]=true;*index=0;return XR_SUCCESS;}
XrResult XRAPI_CALL Wait(XrSwapchain h,XrSwapchainImageWaitInfo const*){Require(fixture.acquired[fixture.Index(h)],"Waiting on unowned image");return XR_SUCCESS;}
XrResult XRAPI_CALL Release(XrSwapchain h,XrSwapchainImageReleaseInfo const*){auto i=fixture.Index(h);Require(fixture.acquired[i],"Image released twice");if(fixture.failRelease){fixture.failRelease=false;return XR_ERROR_RUNTIME_FAILURE;}fixture.acquired[i]=false;++fixture.releases;return XR_SUCCESS;}
XrResult XRAPI_CALL End(XrSession,XrFrameEndInfo const* info){
    Require(info->layerCount==fixture.expectedLayers,"Unexpected submitted layer count");
    for(bool owned:fixture.acquired)Require(!owned,"XR frame ended while copy destination is still acquired");
    for(unsigned i=0;i<info->layerCount;++i){auto* layer=info->layers[i];if(layer->type==XR_TYPE_COMPOSITION_LAYER_PROJECTION){auto* projection=reinterpret_cast<XrCompositionLayerProjection const*>(layer);
        D3D11_TEXTURE2D_DESC d{};fixture.targets[0]->GetDesc(&d);for(unsigned eye=0;eye<2;++eye)Require(projection->views[eye].subImage.imageRect.extent.width==int(d.Width)&&projection->views[eye].subImage.imageRect.extent.height==int(d.Height),"XR projection cropped to native input size");}}
    ++fixture.endCalls;return XR_SUCCESS;
}
ComPtr<ID3D11Texture2D> Texture(ID3D11Device* device,UINT width,UINT height,bool staging=false){
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
    desc.Format=staging?DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:DXGI_FORMAT_B8G8R8A8_TYPELESS;desc.BindFlags=staging?0:D3D11_BIND_RENDER_TARGET;
    if(staging){desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;}
    ComPtr<ID3D11Texture2D> result;stereo::Hr(device->CreateTexture2D(&desc,nullptr,&result),"fixture target");return result;
}
XrResult XRAPI_CALL CreateSwapchain(XrSession,XrSwapchainCreateInfo const* info,XrSwapchain* handle){fixture.targets[3]=Texture(fixture.device,info->width,info->height);*handle=17;return XR_SUCCESS;}
XrResult XRAPI_CALL Enumerate(XrSwapchain handle,uint32_t capacity,uint32_t* count,XrSwapchainImageBaseHeader* out){Require(handle==17,"Unexpected radial swapchain");*count=1;if(capacity){auto* image=reinterpret_cast<XrSwapchainImageD3D11KHR*>(out);image->texture=fixture.targets[3].Get();}return XR_SUCCESS;}
void Pixels(diagnostic::Runtime& runtime,unsigned layer,ComPtr<ID3D11Texture2D> const& staging,DWORD expected,bool transparentBackground=false){
    runtime.renderer.context->CopyResource(staging.Get(),fixture.targets[layer].Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};stereo::Hr(runtime.renderer.context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped),"verify runtime frame");
    D3D11_TEXTURE2D_DESC imageSize{};staging->GetDesc(&imageSize);
    DWORD center{},corner{};memcpy(&center,static_cast<char*>(mapped.pData)+(imageSize.Height/2)*mapped.RowPitch+(imageSize.Width/2)*4,4);memcpy(&corner,mapped.pData,4);
    runtime.renderer.context->Unmap(staging.Get(),0);
    bool same=true;for(unsigned shift:{0,8,16,24})same&=abs(int((center>>shift)&255)-int((expected>>shift)&255))<=(layer<2?1:0);
    if(!same)std::printf("layer%u pixel %08lx expected %08lx\n",layer,center,expected);
    Require(same,"Stale/corrupt eye/UI/cinema pixel in actual Runtime");
    if(transparentBackground)Require(corner==0,"UI background lost transparency");
}
void DrawUi(IDirect3DDevice9Ex* device,UINT w,UINT h){
    struct V{float x,y,z,rhw;DWORD color;};
    V vertices[]={{w*.25f,h*.25f,0,1,0x80ff0000},{w*.75f,h*.25f,0,1,0x80ff0000},{w*.25f,h*.75f,0,1,0x80ff0000},{w*.75f,h*.75f,0,1,0x80ff0000}};
    stereo::Hr(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE),"UI FVF");
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetTexture(0,nullptr);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    stereo::Hr(device->BeginScene(),"UI scene");stereo::Hr(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)),"UI quad");stereo::Hr(device->EndScene(),"UI scene end");
}
int wmain(int argc,wchar_t** argv){try{
    diagnostic::temporal::WorldGate gate;
    for(uint64_t frame=1;frame<=60;++frame){bool ready=frame==60;
        Require(gate.Begin(0,frame,1)==ready&&gate.Begin(0,frame,1)==ready&&gate.Begin(1,frame,1)==ready,"Temporal capture did not wait for stable world frames");}
    Require(!gate.Begin(0,61,2)&&!gate.Begin(1,61,2)&&!gate.Begin(0,62,1),"Menu did not reset temporal capture warmup");
    Require(!gate.Begin(0,300,1),"Missed XR frames did not reset temporal warmup");
    Require(argc>=4&&argc<=6,"Pass absolute DXVK x32 d3d9.dll, width and height");
    auto method=diagnostic::ValidUpscaleMethod(argc>=5?_wtoi(argv[4]):0);
    UINT width=UINT(_wtoi(argv[2])),height=UINT(_wtoi(argv[3]));UINT outWidth=method==diagnostic::UpscaleMethod::Off?width:width*4/3,outHeight=method==diagnostic::UpscaleMethod::Off?height:height*4/3;Require(width&&height&&width<=5120&&height<=4096,"Invalid test dimensions");
    Module module;module.handle=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Require(module.handle!=nullptr,"Load supplied DXVK");
    auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.handle,"Direct3DCreate9Ex"));Require(create!=nullptr,"DXVK export");
    Window window;Require(window.handle!=nullptr,"Hidden test window");ComPtr<IDirect3D9Ex> api;stereo::Hr(create(D3D_SDK_VERSION,&api),"DXVK API");
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.handle;pp.BackBufferWidth=width;pp.BackBufferHeight=height;pp.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> native;stereo::Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.handle,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&native),"DXVK device");
    auto baseline=native->AddRef();native->Release();
    auto table=*reinterpret_cast<void***>(native.Get());void* originals[4]{};for(unsigned i=0;i<4;++i)originals[i]=table[81+i];
    wchar_t temp[MAX_PATH]{},logPath[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"dxv",0,logPath);
    for(unsigned cycle=0;cycle<2;++cycle){
        diagnostic::Runtime runtime(logPath);runtime.device9=native;runtime.width=width;runtime.height=height;
        Require(runtime.dxvk.Initialize(native.Get())&&runtime.dxvk.Active(),"DXVK automatic backend detection");
        LUID luid{};stereo::Hr(api->GetAdapterLUID(0,&luid),"DXVK LUID");ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
        for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> next;if(factory->EnumAdapters1(i,&next)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};next->GetDesc1(&desc);if(!memcmp(&luid,&desc.AdapterLuid,sizeof(luid))){adapter=next;break;}}
        Require(adapter.Get()!=nullptr,"Matching DXGI GPU");runtime.renderer.Initialize(adapter.Get());fixture.device=runtime.renderer.device.Get();
        runtime.outputWidth=outWidth;runtime.outputHeight=outHeight;runtime.upscaler.Initialize(runtime.renderer.device.Get(),diagnostic::UpscalePlan{width,height,outWidth,outHeight,method,25});
        runtime.api.AcquireSwapchainImage=Acquire;runtime.api.WaitSwapchainImage=Wait;runtime.api.ReleaseSwapchainImage=Release;runtime.api.EndFrame=End;
        runtime.api.CreateSwapchain=CreateSwapchain;runtime.api.EnumerateSwapchainImages=Enumerate;
        runtime.space=11;runtime.viewSpace=12;runtime.frame.predictedDisplayTime=123456;
        for(unsigned i=0;i<2;++i){auto& eye=runtime.eyes[i];eye.swapchain=14+i;eye.shared.Create(native.Get(),runtime.renderer.device.Get(),width,height,&runtime.dxvk);
            fixture.targets[i]=Texture(runtime.renderer.device.Get(),outWidth,outHeight);eye.images.push_back({XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR,nullptr,fixture.targets[i].Get()});}
        runtime.uiSwapchain=16;fixture.targets[2]=Texture(runtime.renderer.device.Get(),width,height);runtime.uiImages.push_back({XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR,nullptr,fixture.targets[2].Get()});
        runtime.ui.Initialize(native.Get(),runtime.renderer.device.Get(),width,height,&runtime.dxvk);
        D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};stereo::Hr(runtime.renderer.device->CreateQuery(&q,&runtime.fence11),"runtime copy fence");
        if(!cycle){
            // A real stand-alone D24S8 surface has no texture container, just
            // like the observed game depth. Verify values and reuse after the
            // external Vulkan read, including DXVK's deferred full clears.
            ComPtr<IDirect3DSurface9> previousDepth,depth;HRESULT oldDepth=native->GetDepthStencilSurface(&previousDepth);Require(SUCCEEDED(oldDepth)||oldDepth==D3DERR_NOTFOUND,"Depth test original surface");
            stereo::Hr(native->CreateDepthStencilSurface(width,height,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,FALSE,&depth,nullptr),"Depth test D24S8 surface");
            stereo::Hr(native->SetDepthStencilSurface(depth.Get()),"Depth test bind");
            for(unsigned pass=0;pass<2;++pass){
                if(pass){stereo::Hr(native->SetDepthStencilSurface(previousDepth.Get()),"Unbind tested depth");depth.Reset();stereo::Hr(native->CreateDepthStencilSurface(width,height,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&depth,nullptr),"Discardable depth test surface");stereo::Hr(native->SetDepthStencilSurface(depth.Get()),"Bind discardable depth");}
                float background=pass?.35f:.15f,foreground=pass?.85f:.65f;
                stereo::Hr(native->Clear(0,nullptr,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,background,0x55),"Depth test lazy clear");
                D3DRECT region{LONG(width/2),LONG(height/2),LONG(width),LONG(height)};
                stereo::Hr(native->Clear(1,&region,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,foreground,0xaa),"Depth test rectangle");
                g3vr::DepthSnapshot snapshot;Require(runtime.dxvk.CaptureDepthSnapshot(depth.Get(),snapshot),"Depth Vulkan extraction unavailable");
                Require(snapshot.width==width&&snapshot.height==height&&snapshot.sourceFormat==D3DFMT_D24S8&&snapshot.depth.size()==size_t(width)*height,"Depth snapshot dimensions / format");
                for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x){float expected=x>=width/2&&y>=height/2?foreground:background;Require(std::fabs(snapshot.depth[size_t(y)*width+x]-expected)<.00001f,"Depth pixels corrupt / stale / vertically inverted");}
                ComPtr<IDirect3DSurface9> actual;stereo::Hr(native->GetDepthStencilSurface(&actual),"Depth binding after read");Require(actual.Get()==depth.Get(),"Depth capture changed native binding");
                std::printf("Depth %ux%u D24S8 -> Vulkan format%u: two regions verified (pass%u)\n",width,height,snapshot.vulkanFormat,pass);
            }
            if(width==64&&height==64){
                // Exercise the actual armed Runtime path, including a depth
                // snapshot while a previous eye color is queued, UI transition,
                // private color copies, file output and immediate memory release.
                runtime.temporalRemaining=1;runtime.frames=17;runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;
                ComPtr<IDirect3DSurface9> originalTarget,worldTarget;ComPtr<IDirect3DTexture9> worldTexture;
                stereo::Hr(native->GetRenderTarget(0,&originalTarget),"Armed original color target");
                stereo::Hr(native->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&worldTexture,nullptr),"Armed world color texture");
                stereo::Hr(worldTexture->GetSurfaceLevel(0,&worldTarget),"Armed world color surface");
                float identity[16]{};for(unsigned i=0;i<4;++i)identity[i*5]=1;
                for(unsigned eye=0;eye<2;++eye){
                    runtime.timingEye=int(eye);stereo::Hr(native->SetRenderTarget(0,worldTarget.Get()),"Armed world target bind");
                    stereo::Hr(native->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,eye?0xff886644:0xff224466,eye?.7f:.2f,0x55),"Armed capture known eye");
                    runtime.ui.temporal.Begin(eye,17,width,height);runtime.ui.temporal.Matrices(identity,identity);runtime.ui.temporal.Observe(native.Get());
                    stereo::Hr(native->BeginScene(),"Opaque capture during native scene");runtime.CaptureOpaqueDepth();
                    Require(!runtime.temporalOpaqueDepth[eye].depth.empty()&&std::fabs(runtime.temporalOpaqueDepth[eye].depth[0]-(eye?.7f:.2f))<.00001f,"Opaque native phase depth did not match its eye");
                    // Simulate the engine's later invisible occludee write. It
                    // changes depth but must not change the earlier snapshot.
                    stereo::Hr(native->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,eye?.1f:.05f,0),"Late invisible depth pass");runtime.CaptureOpaqueDepth();
                    Require(std::fabs(runtime.temporalOpaqueDepth[eye].depth[0]-(eye?.7f:.2f))<.00001f,"Duplicate phase overwrote opaque world depth");
                    stereo::Hr(native->EndScene(),"Opaque capture native scene end");runtime.ui.temporal.End(eye);
                    stereo::Hr(native->StretchRect(worldTarget.Get(),nullptr,originalTarget.Get(),nullptr,D3DTEXF_NONE),"Armed final world color");
                    stereo::Hr(native->SetRenderTarget(0,originalTarget.Get()),"Armed final target restore");runtime.Capture(eye);
                    Require(!runtime.temporalDepth[eye].depth.empty()&&std::fabs(runtime.temporalDepth[eye].depth[0]-(eye?.1f:.05f))<.00001f,"End-of-eye diagnostic reference did not see the later depth pass");}
                runtime.timingEye=-1;
                runtime.BeginUi();DrawUi(native.Get(),width,height);runtime.EndUi();fixture.expectedLayers=2;runtime.End(true);
                Require(runtime.temporalDepthDone&&!runtime.temporalRemaining&&runtime.temporalDepth[0].depth.capacity()==0&&runtime.temporalDepth[1].depth.capacity()==0&&!runtime.temporalColor[0]&&!runtime.temporalColor[1],"Armed Runtime capture retained large snapshots");
                Require(runtime.temporalOpaqueDepth[0].depth.capacity()==0&&runtime.temporalOpaqueDepth[1].depth.capacity()==0,"Opaque capture retained large snapshots");
                std::ifstream record(logPath);std::string line;unsigned snapshots=0;while(std::getline(record,line))if(line.find("\"type\":\"vr_temporal_depth_snapshot\"")!=std::string::npos){
                    Require(line.find("\"depth_pixels_captured\":true")!=std::string::npos&&line.find("\"color_saved\":true")!=std::string::npos,"Runtime snapshot files missing");
                    unsigned eye=line.find("\"eye\":0")!=std::string::npos?0:1;
                    auto filename=[&](char const* key){std::string token=std::string("\"")+key+"\":\"";auto start=line.find(token);Require(start!=std::string::npos,"Snapshot path missing");start+=token.size();auto end=line.find('"',start);Require(end!=std::string::npos,"Snapshot path invalid");return runtime.log.path.parent_path()/line.substr(start,end-start);};
                    bool opaque=line.find("\"phase\":\"opaque_before_occlusion\"")!=std::string::npos;
                    float expected=opaque?(eye?.7f:.2f):(eye?.1f:.05f);
                    float value=0;std::ifstream depthFile(filename("depth_file"),std::ios::binary);depthFile.read(reinterpret_cast<char*>(&value),sizeof(value));Require(bool(depthFile)&&std::fabs(value-expected)<.00001f,"Saved depth does not match its eye and native phase");
                    unsigned char color[3]{};std::ifstream colorFile(filename("color_file"),std::ios::binary);colorFile.read(reinterpret_cast<char*>(color),3);Require(bool(colorFile)&&color[0]==(eye?0x44:0x66)&&color[1]==(eye?0x66:0x44)&&color[2]==(eye?0x88:0x22),"Saved color does not match depth eye / BGR encoding");++snapshots;
                }Require(snapshots==4,"Expected matching two-eye opaque and end-of-eye depth snapshots");
            }
            runtime.dxvk.ReleaseDepthSnapshotStorage();stereo::Hr(native->SetDepthStencilSurface(previousDepth.Get()),"Restore native depth");
        }
        if(argc==6){for(auto& view:runtime.views)view.fov={-.8f,.9f,.85f,-.75f};
            runtime.RequestFoveation(1,75);runtime.UpdateFoveation();Require(runtime.ffrConfigured,"Runtime FFR enabled on custom DXVK");}
        auto staging=Texture(runtime.renderer.device.Get(),outWidth,outHeight,true);auto uiStaging=Texture(runtime.renderer.device.Get(),width,height,true);
        for(unsigned frame=0;frame<8;++frame){
            if(argc==6){int mode=frame==3?0:1+int(frame%3),diameter=frame%2?95:40;
                runtime.RequestFoveation(mode,diameter);runtime.UpdateFoveation();
                Require(runtime.ffrConfigured==(mode!=0)&&!runtime.ffrDirty,"Live FFR Off/On/diameter apply at frame boundary");
                runtime.RequestFoveation(mode,diameter);Require(!runtime.ffrDirty,"Unchanged settings must not upload every frame");}
            runtime.batchCopies=(frame%2)==0;runtime.begun=true;runtime.captured=0;runtime.ffrEyeMask=runtime.ffrConfigured?3:0;runtime.uiCaptured=false;runtime.cinemaCaptured=false;
            DWORD colors[2]={D3DCOLOR_ARGB(255,10+frame*15,30,70),D3DCOLOR_ARGB(255,80,15+frame*15,60)};
            for(unsigned i=0;i<2;++i){if(runtime.ffrConfigured)runtime.foveation.BeginEye(i);stereo::Hr(native->Clear(0,nullptr,D3DCLEAR_TARGET,colors[i],1,0),"game eye");runtime.Capture(i);}
            runtime.BeginUi();DrawUi(native.Get(),width,height);runtime.EndUi();fixture.expectedLayers=2;runtime.End(true);
            Pixels(runtime,0,staging,colors[0]);Pixels(runtime,1,staging,colors[1]);Pixels(runtime,2,uiStaging,0x80800000,true);
            Require(!runtime.pendingTextures.Count()&&!runtime.pendingCopies.Count(),"Frame copies escaped to next frame");
        }
        // Partial/cancelled frame must still finish and release its one eye.
        runtime.batchCopies=true;runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;native->Clear(0,nullptr,D3DCLEAR_TARGET,0xff112233,1,0);runtime.Capture(0);
        fixture.expectedLayers=0;runtime.End(false);Pixels(runtime,0,staging,0xff112233);
        // UI-only menu and opaque RGB cinema paths use the same DXVK bridge.
        runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;runtime.BeginUi();DrawUi(native.Get(),width,height);runtime.EndUi();fixture.expectedLayers=1;runtime.End(true);Pixels(runtime,2,uiStaging,0x80800000,true);
        runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;native->Clear(0,nullptr,D3DCLEAR_TARGET,0x00123456,1,0);runtime.ui.CopyBackbuffer();runtime.cinemaCaptured=true;runtime.EndUi();runtime.End(true);Pixels(runtime,2,uiStaging,0x00123456);
        // A release error retains only the unreleased acquired destination;
        // cancellation retries without resubmitting or double-releasing.
        runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;runtime.cinemaCaptured=false;runtime.Capture(1);fixture.failRelease=true;bool failed=false;
        try{runtime.CompleteCopies();}catch(std::exception const&){failed=true;}
        Require(failed&&runtime.pendingCopies.Count()==1&&!runtime.pendingTextures.Count(),"Failed release did not retain ownership");fixture.expectedLayers=0;runtime.End(false);
        // Status/radial uploads bypass DXVK but share the final ownership fence.
        runtime.begun=true;runtime.captured=0;runtime.uiCaptured=false;runtime.BeginUi();runtime.worldStatus=2;runtime.EndUi();fixture.expectedLayers=2;runtime.End(true);
        runtime.worldStatus=0;runtime.ui.Reset();runtime.dxvk.Reset();runtime.dxvk.Reset();
        for(auto& target:fixture.targets)target.Reset();
    }
    for(unsigned i=0;i<4;++i)Require(table[81+i]==originals[i],"DXVK UI hook not restored at shutdown");
    auto after=native->AddRef();native->Release();Require(after==baseline,"DXVK device retained after repeated Runtime teardown");
    DeleteFileW(logPath);
    std::printf("PASS DXVK actual Runtime %ux%u: two-eye colors, UI alpha, batched/immediate copies, menus, cinema, cancellation, release retry, red skull layer, reinitialization and native reference/hook restoration\n",width,height);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
