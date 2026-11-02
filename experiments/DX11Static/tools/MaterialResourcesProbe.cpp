#include "NativeMaterialResources.h"
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using Microsoft::WRL::ComPtr;
using g3vr::dx11::NativeMaterialResources;
namespace {
void Need(bool value,char const* why) { if(!value)throw std::runtime_error(why); }
void Hr(HRESULT hr,char const* why) { if(FAILED(hr))throw std::runtime_error(std::string(why)+" HRESULT="+std::to_string(unsigned(hr))); }
template<class F>void Reject(F f,char const* why) { bool rejected=false;try{f();}catch(std::exception const&){rejected=true;}Need(rejected,why); }
struct Library { HMODULE value{}; ~Library(){if(value)FreeLibrary(value);} };
struct Window { HWND value=CreateWindowExW(0,L"STATIC",L"Isolated native material resource test",WS_OVERLAPPEDWINDOW,0,0,800,600,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);} };
struct Target {
    ComPtr<IDirect3DTexture9> color,depth;
    ComPtr<IDirect3DSurface9> cs,ds;
    unsigned w{},h{};
    void Create(IDirect3DDevice9Ex* d,unsigned width,unsigned height,D3DFORMAT format) {
        w=width;h=height;cs.Reset();ds.Reset();color.Reset();depth.Reset();
        Hr(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&color,nullptr),"native test color");
        Hr(d->CreateTexture(w,h,1,D3DUSAGE_DEPTHSTENCIL,format,D3DPOOL_DEFAULT,&depth,nullptr),"native test depth");
        Hr(color->GetSurfaceLevel(0,&cs),"native color surface");Hr(depth->GetSurfaceLevel(0,&ds),"native depth surface");
    }
    void Bind(IDirect3DDevice9Ex* d) {
        Hr(d->SetDepthStencilSurface(nullptr),"unbind previous test depth");Hr(d->SetRenderTarget(0,cs.Get()),"bind test color");Hr(d->SetDepthStencilSurface(ds.Get()),"bind test depth");
        D3DVIEWPORT9 vp{0,0,w,h,0,1};Hr(d->SetViewport(&vp),"test viewport");
    }
    float Base(unsigned iteration) const { return .075f+float(iteration)*.013f; }
    float Expected(unsigned x,unsigned y,unsigned iteration) const { return Base(iteration)+float((x>=w/2?1:0)+(y>=h/2?2:0))*.19f; }
    uint32_t Color(unsigned x,unsigned y,unsigned iteration) const { return 0x67193a50u+iteration*0x10101u+uint32_t((x>=w/2?1:0)+(y>=h/2?2:0))*0x000b0703u; }
    void Fill(IDirect3DDevice9Ex* d,unsigned iteration) {
        Bind(d);Hr(d->BeginScene(),"begin native resource clear");
        for(unsigned q=0;q<4;++q) {
            D3DRECT r{LONG(q&1?w/2:0),LONG(q&2?h/2:0),LONG(q&1?w:w/2),LONG(q&2?h:h/2)};
            Hr(d->Clear(1,&r,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,Color(unsigned(r.x1),unsigned(r.y1),iteration),Expected(unsigned(r.x1),unsigned(r.y1),iteration),0),"native material pattern clear");
        }
        Hr(d->EndScene(),"end native resource clear");
    }
};
template<class T>std::vector<T> Read(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11ShaderResourceView* view,unsigned w,unsigned h) {
    ComPtr<ID3D11Resource> source;view->GetResource(&source);ComPtr<ID3D11Texture2D> texture;Hr(source.As(&texture),"test texture view");
    D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);Need(desc.Width==w&&desc.Height==h,"Transferred texture dimensions differ");
    desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;Hr(d->CreateTexture2D(&desc,nullptr,&staging),"one-shot test staging texture");c->CopyResource(staging.Get(),source.Get());
    D3D11_MAPPED_SUBRESOURCE map{};Hr(c->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"one-shot test readback");
    std::vector<T> result(size_t(w)*h);for(unsigned y=0;y<h;++y)memcpy(result.data()+size_t(y)*w,static_cast<char const*>(map.pData)+size_t(y)*map.RowPitch,w*sizeof(T));c->Unmap(staging.Get(),0);return result;
}
}
int wmain(int argc,wchar_t** argv) { try {
    Need(argc==3,"Usage: G3DX11MaterialResourcesProbe <DXVK DLL> <output>");std::filesystem::path output=argv[2];std::filesystem::create_directories(output);{std::ofstream initial(output/L"result.json");initial<<"{\"status\":\"INCOMPLETE\",\"live_game_test\":false}\n";}
    bool dxvk=std::wstring(argv[1])!=L"native";
    Library library;library.value=dxvk?LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS):LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);Need(library.value!=nullptr,"Load explicit test API");
    auto factory=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Need(factory!=nullptr,"DXVK D3D9Ex factory");
    ComPtr<IDirect3D9Ex> api;Hr(factory(D3D_SDK_VERSION,&api),"test native API");Window window;Need(window.value!=nullptr,"Owned hidden test window");
    D3DPRESENT_PARAMETERS p{};p.BackBufferWidth=p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.BackBufferCount=1;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window.value;p.Windowed=TRUE;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9Ex> native;Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&p,nullptr,&native),"owned DXVK device");
    LUID luid{};Hr(api->GetAdapterLUID(0,&luid),"native adapter ID");ComPtr<IDXGIFactory1> dxgi;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"test DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(unsigned i=0;;++i) { ComPtr<IDXGIAdapter1> candidate;Need(dxgi->EnumAdapters1(i,&candidate)!=DXGI_ERROR_NOT_FOUND,"Matching test adapter missing");DXGI_ADAPTER_DESC1 desc{};Hr(candidate->GetDesc1(&desc),"test adapter description");if(!memcmp(&desc.AdapterLuid,&luid,sizeof(luid))){adapter=candidate;break;} }
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level{};
    Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"owned native D3D11 device");Need(level>=D3D_FEATURE_LEVEL_11_0,"Compute-capable test GPU required");
    NativeMaterialResources bridge;
    if(!dxvk) { Need(!bridge.Initialize(native.Get(),device.Get(),context.Get()),"Native D3D9 unsupported interop did not fail open");std::ofstream out(output/L"result.json");out<<"{\"status\":\"PASS\",\"native_d3d9_fail_open\":true,\"live_game_test\":false}\n";std::cout<<"PASS unsupported native D3D9 fails open\n";return 0; }
    Need(bridge.Initialize(native.Get(),device.Get(),context.Get()),"DXVK material interop unavailable");
    Reject([&]{bridge.Shadow(0);},"Uncaptured shadow accepted");
    Target lightmap,shadow,detail;lightmap.Create(native.Get(),512,512,D3DFMT_D24S8);shadow.Create(native.Get(),512,512,D3DFMT_D24X8);detail.Create(native.Get(),1408,1408,D3DFMT_D24S8);
    unsigned checks=0;float maxError=0;unsigned formats[2]{};
    for(unsigned iteration=0;iteration<8;++iteration) {
        if(iteration==2)shadow.Create(native.Get(),512,512,D3DFMT_D24X8);
        if(iteration==3)shadow.Create(native.Get(),8,8,D3DFMT_D24X8);
        if(iteration==4)detail.Create(native.Get(),65,41,D3DFMT_D24S8);
        if(iteration==6)lightmap.Create(native.Get(),8,8,D3DFMT_D24S8);
        lightmap.Fill(native.Get(),iteration);shadow.Fill(native.Get(),iteration);detail.Fill(native.Get(),iteration);
        // Refusals precede source layout changes and leave native state intact.
        if(iteration==0) {
            Reject([&]{bridge.Capture(nullptr,shadow.depth.Get(),detail.depth.Get());},"Null material source accepted");
            Reject([&]{bridge.Capture(lightmap.color.Get(),shadow.depth.Get(),shadow.depth.Get());},"Aliased shadow sources accepted");
            Reject([&]{bridge.Capture(lightmap.color.Get(),lightmap.color.Get(),detail.depth.Get());},"Color source accepted as shadow");
            ComPtr<IDirect3DTexture9> mips;Hr(native->CreateTexture(32,32,2,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&mips,nullptr),"unsupported mip fixture");
            Reject([&]{bridge.Capture(mips.Get(),shadow.depth.Get(),detail.depth.Get());},"Mipmapped native lightmap accepted");
            Need(bridge.Allocations()==0&&bridge.Captures()==0,"Rejected inputs allocated/submitted resources");
        }
        ComPtr<IDirect3DSurface9> rt,ds;Hr(native->GetRenderTarget(0,&rt),"saved native target");Hr(native->GetDepthStencilSurface(&ds),"saved native depth");D3DVIEWPORT9 vp{};Hr(native->GetViewport(&vp),"saved native viewport");
        bridge.Capture(lightmap.color.Get(),shadow.depth.Get(),detail.depth.Get());
        auto colors=Read<uint32_t>(device.Get(),context.Get(),bridge.Lightmap(),lightmap.w,lightmap.h);
        for(unsigned y=0;y<lightmap.h;++y)for(unsigned x=0;x<lightmap.w;++x) { Need(colors[size_t(y)*lightmap.w+x]==lightmap.Color(x,y,iteration),"GPU lightmap bytes differ");++checks; }
        Target* targets[]{&shadow,&detail};for(unsigned i=0;i<2;++i) { formats[i]=bridge.ShadowBackingFormat(i);auto* t=targets[i];auto values=Read<float>(device.Get(),context.Get(),bridge.Shadow(i),t->w,t->h);
            for(unsigned y=0;y<t->h;++y)for(unsigned x=0;x<t->w;++x) { auto error=std::abs(values[size_t(y)*t->w+x]-t->Expected(x,y,iteration));Need(std::isfinite(error)&&error<=2.f/16777215,"GPU normalized shadow depth differs");maxError=std::max(maxError,error);++checks; }
        }
        ComPtr<IDirect3DSurface9> afterRT,afterDS;Hr(native->GetRenderTarget(0,&afterRT),"native target after capture");Hr(native->GetDepthStencilSurface(&afterDS),"native depth after capture");D3DVIEWPORT9 afterVP{};Hr(native->GetViewport(&afterVP),"native viewport after capture");
        Need(rt.Get()==afterRT.Get()&&ds.Get()==afterDS.Get()&&!memcmp(&vp,&afterVP,sizeof(vp)),"Native material bridge changed rendering state");
        bridge.FinishReads();Reject([&]{bridge.Lightmap();},"Released shared lightmap remained readable");
        Need(bridge.Allocations()==(iteration<3?3u:iteration==3?4u:iteration<6?5u:6u),"Native GPU transfer allocated on unchanged dimensions");
    }
    Need(bridge.Captures()==8,"Incomplete GPU material batches");auto allocations=bridge.Allocations();bridge.Reset();
    // Reconnect validates ownership cleanup. Native DX9 fail-open runs in a
    // separate process: Windows caches DLL modules by basename.
    Need(bridge.Initialize(native.Get(),device.Get(),context.Get()),"Material bridge reconnect failed");
    bridge.Capture(lightmap.color.Get(),shadow.depth.Get(),detail.depth.Get());bridge.FinishReads();bridge.Reset();
    std::ofstream out(output/L"result.json");out<<"{\"status\":\"PASS\",\"batches\":9,\"pixel_checks\":"<<checks<<",\"allocations_first_connection\":"<<allocations<<",\"max_depth_error\":"<<maxError<<",\"vulkan_shadow_formats\":["<<formats[0]<<","<<formats[1]<<"],\"gpu_only_bridge\":true,\"diagnostic_readback_only_in_test\":true,\"native_state_preserved\":true,\"live_game_test\":false}\n";
    std::cout<<"PASS GPU-only native material resources: "<<checks<<" pixel checks, max depth error "<<maxError<<"\n";return 0;
} catch(std::exception const& e) { std::cerr<<"FAIL "<<e.what()<<"\n";return 1; } }
