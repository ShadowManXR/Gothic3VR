#include <windows.h>
#include <d3d9.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>
#include <cstring>
using Microsoft::WRL::ComPtr;
namespace {
void Check(HRESULT hr,char const* step){if(FAILED(hr))throw std::runtime_error(std::string(step)+" HRESULT="+std::to_string(unsigned(hr)));}
void Need(bool ok,char const* step){if(!ok)throw std::runtime_error(step);}
struct Library{HMODULE value=nullptr;~Library(){if(value)FreeLibrary(value);}};
struct Window{HWND value=CreateWindowExW(0,L"STATIC",L"Isolated full DX11 wrapper compatibility",WS_POPUP,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
}
int wmain(int argc,wchar_t** argv){std::filesystem::path output;
try{
    Need(argc==3||argc==5,"Usage: FullWrapperProbe <absolute d3d9.dll> <output> [width height]");unsigned width=argc==5?std::stoul(argv[3]):320,height=argc==5?std::stoul(argv[4]):240;Need(width>=64&&height>=64&&uint64_t(width)*height<=16777216,"Wrapper target size budget");output=argv[2];std::filesystem::create_directories(output);
    std::ofstream steps(output/L"steps.log");auto step=[&](char const* s){steps<<s<<'\n';steps.flush();};
    step("load_backend");Library library;library.value=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Need(library.value!=nullptr,"Backend DLL load failed");
    auto factory=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(library.value,"Direct3DCreate9Ex"));Need(factory!=nullptr,"Missing Direct3DCreate9Ex export");
    ComPtr<IDirect3D9Ex> api;Check(factory(D3D_SDK_VERSION,&api),"Direct3DCreate9Ex");step("api9ex_created");
    LUID luid{};Check(api->GetAdapterLUID(0,&luid),"GetAdapterLUID");Window window;Need(window.value!=nullptr,"Hidden test window failed");
    D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=320;pp.BackBufferHeight=240;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9Ex> d9;Check(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d9),"CreateDeviceEx");step("device9ex_created");
    bool wrapperLoadedD11=GetModuleHandleW(L"d3d11.dll")!=nullptr;
    ComPtr<ID3D11Device> queriedDevice;auto deviceQuery=d9.As(&queriedDevice);
    steps<<"underlying_d11_query_hr="<<unsigned(deviceQuery)<<" d3d11_loaded_by_wrapper="<<wrapperLoadedD11<<'\n';steps.flush();
    ComPtr<IDXGIFactory1> dxgi;Check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> next;Need(dxgi->EnumAdapters1(i,&next)!=DXGI_ERROR_NOT_FOUND,"Matching DXGI adapter missing");DXGI_ADAPTER_DESC1 desc{};next->GetDesc1(&desc);if(!memcmp(&desc.AdapterLuid,&luid,sizeof(luid))){adapter=next;break;}}
    ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&d11,&level,&context),"VR-owned D3D11 device");step("vr_device11_created");
    HANDLE shared=nullptr;ComPtr<IDirect3DTexture9> texture9;ComPtr<IDirect3DSurface9> surface9;ComPtr<ID3D11Texture2D> texture11;
    using CustomEye=HRESULT(WINAPI*)(IDirect3DDevice9Ex*,ID3D11Device*,UINT,UINT,IDirect3DTexture9**,IDirect3DSurface9**,ID3D11Texture2D**);
    auto custom=reinterpret_cast<CustomEye>(GetProcAddress(library.value,"G3VR_CreateWrapperSharedEye"));
    if(custom){auto hr=custom(d9.Get(),d11.Get(),width,height,&texture9,&surface9,&texture11);using Diagnostics=void(WINAPI*)(unsigned*);auto diagnostic=reinterpret_cast<Diagnostics>(GetProcAddress(library.value,"G3VR_WrapperDiagnostics"));if(diagnostic){unsigned v[8]{};diagnostic(v);steps<<"proxy_diagnostics=";for(auto x:v)steps<<x<<',';steps<<'\n';steps.flush();}Check(hr,"Wrapper direct GPU eye bridge");step("wrapper_gpu_eye_opened");}
    else {
    Check(d9->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture9,&shared),"Create shared eye texture");
    steps<<"shared_handle="<<reinterpret_cast<uintptr_t>(shared)<<'\n';steps.flush();
    Check(texture9->GetSurfaceLevel(0,&surface9),"Eye surface");
    ComPtr<ID3D11Texture2D> queriedTexture;auto textureQuery=texture9.As(&queriedTexture);steps<<"underlying_texture11_query_hr="<<unsigned(textureQuery)<<'\n';steps.flush();
    Need(shared!=nullptr,"Wrapper did not return a D3D9Ex shared eye handle");
    Check(d11->OpenSharedResource(shared,IID_PPV_ARGS(&texture11)),"Open shared eye on VR D3D11 device");step("shared_eye_opened");}
    D3D11_TEXTURE2D_DESC desc{};texture11->GetDesc(&desc);Need(desc.Width==width&&desc.Height==height&&(desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS||desc.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS)&&desc.SampleDesc.Count==1,"Incompatible shared-eye format");bool rgba=desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS;
    desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;Check(d11->CreateTexture2D(&desc,nullptr,&staging),"Diagnostic readback");
    ComPtr<IDirect3DQuery9> fence;Check(d9->CreateQuery(D3DQUERYTYPE_EVENT,&fence),"Native completion event");
    for(unsigned frame=0;frame<8;++frame){
        Check(d9->SetDepthStencilSurface(nullptr),"Depth off");Check(d9->SetRenderTarget(0,surface9.Get()),"Shared target bind");
        DWORD colors[]{0x00123456,0x40123456,0x80123456,0xff123456,0x00a56219,0x40a56219,0x80a56219,0xffa56219};DWORD color=colors[frame];Check(d9->Clear(0,nullptr,D3DCLEAR_TARGET,color,1,0),"Distinct eye color clear");
        Check(fence->Issue(D3DISSUE_END),"Native event issue");auto started=GetTickCount64();
        for(;;){auto hr=fence->GetData(nullptr,0,D3DGETDATA_FLUSH);if(hr==S_OK)break;Check(hr,"Native event wait");Need(GetTickCount64()-started<5000,"Native event timed out");SwitchToThread();}
        using Update=HRESULT(WINAPI*)(ID3D11Texture2D*);auto update=reinterpret_cast<Update>(GetProcAddress(library.value,"G3VR_UpdateWrapperEye"));if(update)Check(update(texture11.Get()),"Wrapper GPU channel conversion");
        context->CopyResource(staging.Get(),texture11.Get());D3D11_MAPPED_SUBRESOURCE map{};Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"Diagnostic eye map");
        DWORD expected=rgba?((color&0xff00ff00)|((color&0xff)<<16)|((color>>16)&0xff)):color;
        bool correct=true;for(unsigned y=0;y<height;++y){auto row=reinterpret_cast<DWORD const*>(static_cast<char*>(map.pData)+size_t(y)*map.RowPitch);for(unsigned x=0;x<width;++x)correct=correct&&row[x]==expected;}
        context->Unmap(staging.Get(),0);Need(correct,"Stale or incorrect shared-eye pixels");
    }
    step("eight_distinct_eye_frames_correct");
    using Release=void(WINAPI*)(ID3D11Texture2D*);auto release=reinterpret_cast<Release>(GetProcAddress(library.value,"G3VR_ReleaseWrapperEye"));if(release)release(texture11.Get());
    if(custom&&release){surface9.Reset();texture9.Reset();texture11.Reset();for(unsigned cycle=0;cycle<10;++cycle){Check(custom(d9.Get(),d11.Get(),width,height,&texture9,&surface9,&texture11),"Repeated VR target creation");release(texture11.Get());surface9.Reset();texture9.Reset();texture11.Reset();}step("ten_target_release_recreate_cycles");}
    std::ofstream result(output/L"result.json");result<<"{\"result\":\"PASS\",\"shared_vr_eye_transport\":true,\"width\":"<<width<<",\"height\":"<<height<<",\"frames_checked\":8,\"alpha_preserved\":true,\"release_recreate_cycles\":10,\"game_loaded\":false,\"headset_validated\":false,\"performance_gain_proven\":false,\"wrapper_loaded_d3d11\":"<<(wrapperLoadedD11?"true":"false")<<"}\n";
    std::cout<<"PASS wrapper D3D9Ex / GPU shared-eye transport\n";return 0;
}catch(std::exception const& e){if(!output.empty()){std::ofstream result(output/L"result.json");std::string reason=e.what();for(char& c:reason)if(c=='"'||c=='\\')c=' ';result<<"{\"result\":\"REFUSED\",\"reason\":\""<<reason<<"\",\"game_loaded\":false,\"headset_validated\":false}\n";}std::cerr<<e.what()<<'\n';return 1;}}
