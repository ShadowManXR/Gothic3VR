#include "D3D9Bridge.h"
#include "FrameCopyBatch.h"
#include "FramePerformance.h"
#include "AsyncGpuTiming.h"
#include <array>
#include <cstring>
#include <cstdio>
using stereo::ComPtr;
void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
struct HiddenWindow {
    HWND handle=CreateWindowExW(0,L"STATIC",L"Gothic3VR bridge validation",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    ~HiddenWindow(){if(handle)DestroyWindow(handle);}
};
int main(){try{
    // Standalone, hidden GPU check. No game hooks, runtime, headset, settings,
    // registry changes or on-screen window are involved.
    HiddenWindow window;Require(window.handle!=nullptr,"Cannot create hidden window");
    ComPtr<IDirect3D9Ex> api9;stereo::Hr(Direct3DCreate9Ex(D3D_SDK_VERSION,&api9),"D3D9Ex");
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.hDeviceWindow=window.handle;
    ComPtr<IDirect3DDevice9Ex> d9;stereo::Hr(api9->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.handle,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,nullptr,&d9),"D3D9 device");
    LUID luid{};stereo::Hr(api9->GetAdapterLUID(0,&luid),"adapter LUID");ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");ComPtr<IDXGIAdapter1> adapter;
    for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> next;if(factory->EnumAdapters1(i,&next)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};stereo::Hr(next->GetDesc1(&desc),"adapter description");if(!std::memcmp(&luid,&desc.AdapterLuid,sizeof(luid))){adapter=next;break;}}
    Require(adapter.Get()!=nullptr,"Matching GPU not found");ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    stereo::Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d11,&level,&context),"D3D11 device");
    constexpr UINT width=3840,height=2160;g3vr::SharedEye shared[3];ComPtr<ID3D11Texture2D> images[3],readback;
    D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    for(unsigned i=0;i<3;++i){shared[i].Create(d9.Get(),d11.Get(),width,height);stereo::Hr(d11->CreateTexture2D(&desc,nullptr,&images[i]),"destination texture");}
    desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;stereo::Hr(d11->CreateTexture2D(&desc,nullptr,&readback),"readback");
    ComPtr<IDirect3DSurface9> source;stereo::Hr(d9->CreateRenderTarget(width,height,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&source,nullptr),"native source");
    ComPtr<IDirect3DQuery9> q9;stereo::Hr(d9->CreateQuery(D3DQUERYTYPE_EVENT,&q9),"9 fence");ComPtr<ID3D11Query> q11;D3D11_QUERY_DESC qdesc{D3D11_QUERY_EVENT,0};stereo::Hr(d11->CreateQuery(&qdesc,&q11),"11 fence");
    uint64_t checks=0;double medians[2]{};
    diagnostic::performance::AsyncGpuTiming gpuTimer;bool timestamps=gpuTimer.Initialize(d9.Get(),1);
    for(unsigned mode=0;mode<2;++mode){bool batched=mode!=0;std::array<double,32> times{};uint64_t waits9=0,waits11=0,polls9=0,flushes9=0;
        for(unsigned frame=0;frame<36;++frame){g3vr::FrameCopyBatch<unsigned> ownership;g3vr::DeferredFrameCopies<ID3D11Texture2D> copies;unsigned released=0;DWORD colors[3]{};
            auto complete=[&]{copies.Drain([&]{auto stats=g3vr::Wait9(q9.Get());++waits9;polls9+=stats.polls;flushes9+=stats.flushes;},[&](ID3D11Texture2D* destination,ID3D11Texture2D* origin){context->CopyResource(destination,origin);});
                ownership.Drain([&]{g3vr::Wait11(context.Get(),q11.Get());++waits11;},[&](unsigned h){Require(h==++released,"Unexpected release order");});};
            auto start=diagnostic::performance::Now();
            for(unsigned i=0;i<3;++i){colors[i]=D3DCOLOR_ARGB(255,20+frame*3,40+i*70,10+frame+i);
                if(i<2)gpuTimer.BeginEye(i,1);
                stereo::Hr(d9->SetRenderTarget(0,source.Get()),"source target");stereo::Hr(d9->Clear(0,nullptr,D3DCLEAR_TARGET,colors[i],1,0),"native render pattern");
                stereo::Hr(d9->StretchRect(source.Get(),nullptr,shared[i].surface9.Get(),nullptr,D3DTEXF_NONE),"shared copy");
                if(i<2)gpuTimer.EndEye(i);
                Require(ownership.Add(i+1)&&copies.Add(images[i].Get(),shared[i].texture11.Get()),"queue full");if(!batched)complete();}
            complete();auto end=diagnostic::performance::Now();Require(released==3&&!ownership.Count()&&!copies.Count(),"Incomplete frame ownership");
            gpuTimer.Poll();
            if(frame>=4)times[frame-4]=double(end-start)*1000/double(diagnostic::performance::Frequency());
            // Readback is TEST ONLY, outside the timed pipeline. Check each
            // eye/UI and changing frame colors, catching stale/torn transfers.
            for(unsigned i=0;i<3;++i){context->CopyResource(readback.Get(),images[i].Get());context->Flush();D3D11_MAPPED_SUBRESOURCE mapped{};stereo::Hr(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"verify pixels");
                bool valid=true;for(unsigned j=0;j<5;++j){UINT x=j*(width-1)/4,y=j*(height-1)/4;DWORD pixel;std::memcpy(&pixel,static_cast<char*>(mapped.pData)+y*mapped.RowPitch+x*4,4);valid=valid&&pixel==colors[i];++checks;}
                context->Unmap(readback.Get(),0);Require(valid,"Stale or corrupt frame pixels");}
        }
        std::sort(times.begin(),times.end());medians[mode]=(times[15]+times[16])*.5;
        Require(waits9==(batched?36u:108u)&&waits11==waits9,"Unexpected per-frame fence count");
        std::printf("{\"mode\":\"%s\",\"resolution\":\"3840x2160\",\"frames\":36,\"wait9_calls\":%llu,\"wait11_calls\":%llu,\"d3d9_polls\":%llu,\"d3d9_flush_requests\":%llu,\"pipeline_median_ms\":%.3f}\n",batched?"batched":"immediate",static_cast<unsigned long long>(waits9),static_cast<unsigned long long>(waits11),static_cast<unsigned long long>(polls9),static_cast<unsigned long long>(flushes9),medians[mode]);
    }
    auto gpu=gpuTimer.Current();if(timestamps)Require(gpu.samples>=4&&gpu.sum[0]>=0&&gpu.sum[1]>=0,"GPU timestamps never became ready without an extra flush");
    std::printf("GPU timestamp support=%d samples=%llu discarded=%llu mean_left_ms=%.4f mean_right_ms=%.4f; GetData flags0, no query wait or flush\n",timestamps,static_cast<unsigned long long>(gpu.samples),static_cast<unsigned long long>(gpu.discarded),gpu.samples?gpu.sum[0]/gpu.samples:0,gpu.samples?gpu.sum[1]/gpu.samples:0);
    gpuTimer.Reset();context->ClearState();context->Flush();std::printf("PASS native D3D9Ex/D3D11 three-layer 4K transfers: %llu pixel checks, same-frame source ownership; GPU bridge timings are not game FPS\n",static_cast<unsigned long long>(checks));return 0;
}catch(std::exception const& e){std::printf("FAIL native GPU bridge: %s\n",e.what());return 1;}}
