#include "DxvkTemporalBridge.h"
#include "GpuTemporalDepth.h"
#include "GpuTemporalMotion.h"
#include "D3D9Bridge.h"
#include "TemporalTransportFixturePS.h"
#include "TemporalDepthFloatCS.h"
#include <dxgi1_2.h>
#include <DirectXPackedVector.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <limits>
using Microsoft::WRL::ComPtr;
using namespace DirectX::PackedVector;
using namespace diagnostic::temporal;
void Require(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Hr(HRESULT h){Require(SUCCEEDED(h),"Temporal transport fixture API failure");}
template<class F>void Reject(F f){bool bad=false;try{f();}catch(std::exception const&){bad=true;}Require(bad,"Invalid transport operation accepted");}
struct Module{HMODULE value=nullptr;~Module(){if(value)FreeLibrary(value);}};
struct Window{HWND value=CreateWindowW(L"STATIC",L"Temporal GPU transport",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);~Window(){if(value)DestroyWindow(value);}};
struct Eye {
    ComPtr<IDirect3DTexture9> texture,maskTexture;ComPtr<IDirect3DSurface9> color,depth,maskSurface;
    ComPtr<ID3D11Texture2D> motion,rawDepth,mask;GpuTemporalDepth decode;GpuTemporalMotion compose;
    ComPtr<ID3D11ShaderResourceView> depthView,motionView;
    g3vr::TemporalEncoding encoding{};
};
std::array<float,4> Color(unsigned eye,unsigned phase,unsigned quad){return {-3.25f-float(eye*3+phase*2+quad)*.5f,5.5f+float(phase+quad)*.25f,1,1};}
void Draw(IDirect3DDevice9Ex* d,IDirect3DPixelShader9* ps,unsigned w,unsigned h,unsigned eye,unsigned phase){
    d->SetDepthStencilSurface(nullptr);d->SetVertexShader(nullptr);d->SetPixelShader(ps);d->SetFVF(D3DFVF_XYZRHW);
    d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    Hr(d->BeginScene());
    for(unsigned quad=0;quad<4;++quad){unsigned x=quad%2,y=quad/2;float left=float(x?w/2:0)-.5f,right=float(x?w:w/2)-.5f,top=float(y?h/2:0)-.5f,bottom=float(y?h:h/2)-.5f;
        struct V{float x,y,z,rhw;};V vertices[]={{left,top,0,1},{right,top,0,1},{left,bottom,0,1},{right,bottom,0,1}};
        auto c=Color(eye,phase,quad);Hr(d->SetPixelShaderConstantF(0,c.data(),1));Hr(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)));}
    Hr(d->EndScene());
}
ComPtr<ID3D11Texture2D> Staging(ID3D11Device* d,ID3D11Texture2D* source){D3D11_TEXTURE2D_DESC desc{};source->GetDesc(&desc);desc.BindFlags=desc.MiscFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> out;Hr(d->CreateTexture2D(&desc,nullptr,&out));return out;}
void Verify(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Texture2D* source,unsigned w,unsigned h,unsigned eye,unsigned phase,bool depth,g3vr::TemporalEncoding encoding,std::vector<float> const* nativeDepth=nullptr){
    auto staging=Staging(d,source);c->CopyResource(staging.Get(),source);D3D11_MAPPED_SUBRESOURCE map{};Hr(c->Map(staging.Get(),0,D3D11_MAP_READ,0,&map));
    double error=0;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){auto* row=static_cast<char*>(map.pData)+y*map.RowPitch;
        if(depth){float got=reinterpret_cast<float*>(row)[x];float expected=(x<w/2&&y<h/2)?(.6f+phase*.05f):(.15f+eye*.1f+phase*.05f);
            // Ground truth is the separately extracted native depth. Float
            // Clear-to-D16 quantization may differ by one ULP near ties;
            // transport must reproduce the ACTUAL stored depth, not a guess.
            Require(nativeDepth&&nativeDepth->size()==size_t(w)*h,"Native depth reference missing");float stored=(*nativeDepth)[size_t(y)*w+x];
            Require(std::abs(stored-expected)<(encoding==g3vr::TemporalEncoding::Depth16Unorm?1.1f/65535.f:1.e-6f),"Native clear/reference depth differs");
            double delta=std::abs(got-stored);error=std::max(error,delta);if(!std::isfinite(got)||delta>=2.e-7)std::printf("DEPTH mismatch eye%u phase%u at%u,%u got%.10f native%.10f encoding%u\n",eye,phase,x,y,got,stored,unsigned(encoding));Require(std::isfinite(got)&&delta<2.e-7,"Transported/decoded depth differs from actual native storage");}
        else{auto expected=Color(eye,phase,(x>=w/2?1:0)+(y>=h/2?2:0));auto* p=reinterpret_cast<uint16_t*>(row)+x*4;
            for(unsigned k=0;k<4;++k)Require(p[k]==XMConvertFloatToHalf(expected[k]),"RGBA16F motion changed/stale/flipped across GPU bridge");}}
    c->Unmap(staging.Get(),0);std::printf("PASS eye%u phase%u %s %ux%u encoding%u max_error=%.9f\n",eye,phase,depth?"depth":"motion",w,h,unsigned(encoding),error);
}
void DecodeEdges(ID3D11Device* d,ID3D11DeviceContext* c){
    // D24/D16 conversion is tested even on GPUs that promote the requested
    // native surface to D32. Packed high-byte poison must never affect depth.
    for(auto encoding:{g3vr::TemporalEncoding::Depth16Unorm,g3vr::TemporalEncoding::Depth24Uint,g3vr::TemporalEncoding::Depth32Float}){
        constexpr unsigned W=9,H=3;std::vector<uint32_t> values(W*H);std::vector<uint16_t> depth16(W*H);std::vector<float> floats(W*H);
        for(unsigned i=0;i<W*H;++i){values[i]=0xa5000000|((i*7919)%0xffffff);depth16[i]=uint16_t(i*1237);floats[i]=float(i)/(W*H-1);}
        floats[0]=std::numeric_limits<float>::quiet_NaN();floats[1]=-1;floats[2]=std::numeric_limits<float>::infinity();values[0]=0xff000000;values[1]=0xffffffff;depth16[0]=0;depth16[1]=65535;
        D3D11_TEXTURE2D_DESC t{};t.Width=W;t.Height=H;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=encoding==g3vr::TemporalEncoding::Depth16Unorm?DXGI_FORMAT_R16_UNORM:encoding==g3vr::TemporalEncoding::Depth24Uint?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R32_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=encoding==g3vr::TemporalEncoding::Depth16Unorm?static_cast<void*>(depth16.data()):encoding==g3vr::TemporalEncoding::Depth24Uint?static_cast<void*>(values.data()):static_cast<void*>(floats.data());data.SysMemPitch=W*(encoding==g3vr::TemporalEncoding::Depth16Unorm?2:4);
        ComPtr<ID3D11Texture2D> image;Hr(d->CreateTexture2D(&t,&data,&image));GpuTemporalDepth decode;decode.Initialize(d,W,H,encoding);
        ComPtr<ID3D11ComputeShader> seed;Hr(d->CreateComputeShader(TemporalDepthFloatCS,sizeof(TemporalDepthFloatCS),nullptr,&seed));c->CSSetShader(seed.Get(),nullptr,0);
        ComPtr<ID3D11ShaderResourceView> seedSource;Hr(d->CreateShaderResourceView(image.Get(),nullptr,&seedSource));auto* seeded=seedSource.Get();c->CSSetShaderResources(0,1,&seeded);
        auto targetDesc=t;targetDesc.Format=DXGI_FORMAT_R32_FLOAT;targetDesc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;ComPtr<ID3D11Texture2D> untouched;ComPtr<ID3D11UnorderedAccessView> seedUav;Hr(d->CreateTexture2D(&targetDesc,nullptr,&untouched));Hr(d->CreateUnorderedAccessView(untouched.Get(),nullptr,&seedUav));auto* seededUav=seedUav.Get();c->CSSetUnorderedAccessViews(0,1,&seededUav,nullptr);
        decode.Process(c,image.Get());ComPtr<ID3D11ComputeShader> gotShader;UINT classes=0;c->CSGetShader(&gotShader,nullptr,&classes);Require(gotShader==seed,"Depth decode did not restore shader");ComPtr<ID3D11ShaderResourceView> gotSource;c->CSGetShaderResources(0,1,&gotSource);Require(gotSource==seedSource,"Depth decode did not restore source");ComPtr<ID3D11UnorderedAccessView> gotUav;c->CSGetUnorderedAccessViews(0,1,&gotUav);Require(gotUav==seedUav,"Depth decode did not restore UAV");
        auto result=Staging(d,decode.Texture());c->CopyResource(result.Get(),decode.Texture());D3D11_MAPPED_SUBRESOURCE map{};Hr(c->Map(result.Get(),0,D3D11_MAP_READ,0,&map));
        for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){unsigned i=y*W+x;double expected=encoding==g3vr::TemporalEncoding::Depth16Unorm?depth16[i]/65535.:encoding==g3vr::TemporalEncoding::Depth24Uint?(values[i]&0xffffff)/16777215.:((std::isfinite(floats[i])&&floats[i]>=0&&floats[i]<=1)?floats[i]:1);
            float got=reinterpret_cast<float*>(static_cast<char*>(map.pData)+y*map.RowPitch)[x];Require(std::abs(got-expected)<2.e-7,"D16/D24/D32 decoder edge/packing error");}c->Unmap(result.Get(),0);
        Reject([&]{decode.Process(c,decode.Texture());});decode.Reset();Require(!decode.Texture(),"Depth decoder reset failed");c->ClearState();
    }
}
void SequentialSharedDepth(IDirect3DDevice9Ex* native,IDirect3DPixelShader9* ps,ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Query* fence,unsigned w,unsigned h){
    // Actual Gothic capture405 uses depth identity7332088 for BOTH eyes.
    // Export that topology: one native depth/color, separate persistent eye
    // outputs. Left stays GPU read-owned while right overwrites native data.
    ComPtr<IDirect3DSurface9> previousTarget;Hr(native->GetRenderTarget(0,&previousTarget));g3vr::DxvkTemporalBridge bridge;Require(bridge.Initialize(native),"Shared-source bridge");g3vr::DxvkGpuBridge reference;Require(reference.Initialize(native),"Shared-source reference");Eye eyes[2];
    Hr(native->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&eyes[0].texture,nullptr));Hr(eyes[0].texture->GetSurfaceLevel(0,&eyes[0].color));Hr(native->CreateDepthStencilSurface(w,h,D3DFMT_D24S8,D3DMULTISAMPLE_NONE,0,TRUE,&eyes[0].depth,nullptr));eyes[1].color=eyes[0].color;eyes[1].depth=eyes[0].depth;
    for(auto& e:eyes){Require(bridge.Register(e.color.Get(),device,e.motion)==g3vr::TemporalEncoding::Color16F,"Shared source motion registration");e.encoding=bridge.Register(e.depth.Get(),device,e.rawDepth);e.decode.Initialize(device,w,h,e.encoding);}
    Require(eyes[0].motion!=eyes[1].motion&&eyes[0].rawDepth!=eyes[1].rawDepth,"Shared native source aliased eye outputs");
    std::vector<float> stored[2];
    for(unsigned eye=0;eye<2;++eye){auto& e=eyes[eye];Hr(native->SetRenderTarget(0,e.color.Get()));Draw(native,ps,w,h,eye,0);Hr(native->SetDepthStencilSurface(e.depth.Get()));Hr(native->Clear(0,nullptr,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,.15f+eye*.1f,0x5a));D3DRECT corner{0,0,LONG(w/2),LONG(h/2)};Hr(native->Clear(1,&corner,D3DCLEAR_ZBUFFER,0,.6f,0));
        bridge.Queue(e.motion.Get());if(!eye)Reject([&]{bridge.Queue(eyes[1].motion.Get());});bridge.Queue(e.rawDepth.Get());if(!eye)Reject([&]{bridge.Queue(eyes[1].rawDepth.Get());});bridge.Complete();
        g3vr::DepthSnapshot snapshot;Require(reference.CaptureDepthSnapshot(e.depth.Get(),snapshot),"Shared source exact native depth");stored[eye]=std::move(snapshot.depth);
    }
    for(unsigned eye=0;eye<2;++eye){auto& e=eyes[eye];e.decode.Process(context,e.rawDepth.Get());Verify(device,context,e.decode.Texture(),w,h,eye,0,true,e.encoding,&stored[eye]);Verify(device,context,e.motion.Get(),w,h,eye,0,false,e.encoding);}
    g3vr::Wait11(context,fence);bridge.ReleaseReads();Hr(native->SetDepthStencilSurface(nullptr));Hr(native->SetRenderTarget(0,previousTarget.Get()));bridge.Reset();
    std::puts("PASS SAME native depth/color overwritten between eyes: left preserved, right fresh, aliased same-batch capture rejected");
}
int wmain(int argc,wchar_t** argv){try{
    Require(argc==4,"Pass absolute DXVK x32 DLL,width,height");unsigned w=_wtoi(argv[2]),h=_wtoi(argv[3]);Require(w&&h&&w<=5120&&h<=2880,"Fixture dimensions");
    Module module;module.value=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);Require(module.value!=nullptr,"Load actual DXVK");auto create=reinterpret_cast<HRESULT(WINAPI*)(UINT,IDirect3D9Ex**)>(GetProcAddress(module.value,"Direct3DCreate9Ex"));Require(create!=nullptr,"DXVK entry");
    Window window;ComPtr<IDirect3D9Ex> api;Hr(create(D3D_SDK_VERSION,&api));D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window.value;p.BackBufferWidth=w;p.BackBufferHeight=h;p.BackBufferFormat=D3DFMT_A8R8G8B8;
    ComPtr<IDirect3DDevice9Ex> native;Hr(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&p,nullptr,&native));
    LUID luid{};Hr(api->GetAdapterLUID(0,&luid));ComPtr<IDXGIFactory1> factory;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter1> adapter;
    for(unsigned i=0;;++i){ComPtr<IDXGIAdapter1> next;if(factory->EnumAdapters1(i,&next)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 desc{};Hr(next->GetDesc1(&desc));if(!memcmp(&luid,&desc.AdapterLuid,sizeof(luid))){adapter=next;break;}}Require(adapter.Get()!=nullptr,"Matching native DXVK adapter");
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));DecodeEdges(device.Get(),context.Get());
    ComPtr<IDirect3DPixelShader9> ps;Hr(native->CreatePixelShader(reinterpret_cast<DWORD const*>(TemporalTransportFixturePS),&ps));ComPtr<IDirect3DSurface9> original;Hr(native->GetRenderTarget(0,&original));
    D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};ComPtr<ID3D11Query> fence;Hr(device->CreateQuery(&q,&fence));
    unsigned testedFormats=0;
    for(D3DFORMAT format:{D3DFMT_D16,D3DFMT_D24S8,D3DFMT_D32F_LOCKABLE}){
        ComPtr<IDirect3DSurface9> probe;auto supported=native->CreateDepthStencilSurface(w,h,format,D3DMULTISAMPLE_NONE,0,FALSE,&probe,nullptr);
        if(FAILED(supported)&&format==D3DFMT_D32F_LOCKABLE){std::printf("Optional native D32F unavailable; decoder fixture still tested\n");continue;}
        Hr(supported);++testedFormats;
        g3vr::DxvkTemporalBridge bridge;Require(bridge.Initialize(native.Get()),"Temporal DXVK initialization");g3vr::DxvkGpuBridge reference;Require(reference.Initialize(native.Get()),"Native depth test reference");Eye eyes[2];
        // Zero is an explicit UNKNOWN static-classification fixture, never a
        // real-game assertion that all non-skin pixels are static.
        std::vector<unsigned char> unknown(size_t(w)*h);D3D11_TEXTURE2D_DESC mt{};mt.Width=w;mt.Height=h;mt.MipLevels=mt.ArraySize=mt.SampleDesc.Count=1;mt.Format=DXGI_FORMAT_R8_UNORM;mt.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA md{unknown.data(),w,0};ComPtr<ID3D11Texture2D> staticMask;ComPtr<ID3D11ShaderResourceView> staticView;Hr(device->CreateTexture2D(&mt,&md,&staticMask));Hr(device->CreateShaderResourceView(staticMask.Get(),nullptr,&staticView));
        for(unsigned eye=0;eye<2;++eye){auto& e=eyes[eye];Hr(native->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&e.texture,nullptr));Hr(e.texture->GetSurfaceLevel(0,&e.color));if(eye==0)e.depth=std::move(probe);else Hr(native->CreateDepthStencilSurface(w,h,format,D3DMULTISAMPLE_NONE,0,FALSE,&e.depth,nullptr));
            Require(bridge.Register(e.color.Get(),device.Get(),e.motion)==g3vr::TemporalEncoding::Color16F,"Motion format encoding");e.encoding=bridge.Register(e.depth.Get(),device.Get(),e.rawDepth);e.decode.Initialize(device.Get(),w,h,e.encoding);
            Hr(native->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&e.maskTexture,nullptr));Hr(e.maskTexture->GetSurfaceLevel(0,&e.maskSurface));Require(bridge.Register(e.maskSurface.Get(),device.Get(),e.mask)==g3vr::TemporalEncoding::Color16F,"Mask format");
            e.compose.Initialize(device.Get(),w,h);Hr(device->CreateShaderResourceView(e.motion.Get(),nullptr,&e.motionView));Hr(device->CreateShaderResourceView(e.decode.Texture(),nullptr,&e.depthView));}
        Reject([&]{ComPtr<ID3D11Texture2D> output;bridge.Register(eyes[0].color.Get(),device.Get(),output);});
        for(unsigned phase=0;phase<3;++phase){
            for(unsigned eye=0;eye<2;++eye){auto& e=eyes[eye];Hr(native->SetRenderTarget(0,e.maskSurface.Get()));Draw(native.Get(),ps.Get(),w,h,eye+2,phase);Hr(native->SetRenderTarget(0,e.color.Get()));Draw(native.Get(),ps.Get(),w,h,eye,phase);Hr(native->SetDepthStencilSurface(e.depth.Get()));
                float background=.15f+eye*.1f+phase*.05f,foreground=.6f+phase*.05f;Hr(native->Clear(0,nullptr,D3DCLEAR_ZBUFFER|(format==D3DFMT_D24S8?D3DCLEAR_STENCIL:0),0,background,0x5a));D3DRECT corner{0,0,LONG(w/2),LONG(h/2)};Hr(native->Clear(1,&corner,D3DCLEAR_ZBUFFER,0,foreground,0));}
            if(phase==2)Hr(native->BeginScene());
            Reject([&]{bridge.Queue(eyes[0].compose.Texture());});bridge.Queue(eyes[1].motion.Get());Reject([&]{bridge.Queue(eyes[1].motion.Get());});bridge.Queue(eyes[0].rawDepth.Get());bridge.Queue(eyes[0].motion.Get());bridge.Queue(eyes[1].rawDepth.Get());bridge.Queue(eyes[1].mask.Get());bridge.Queue(eyes[0].mask.Get());bridge.Complete();
            if(phase==2)Hr(native->EndScene());Reject([&]{bridge.Queue(eyes[0].motion.Get());});
            for(unsigned eye=0;eye<2;++eye){auto& e=eyes[eye];e.decode.Process(context.Get(),e.rawDepth.Get());g3vr::DepthSnapshot nativeDepth;Require(reference.CaptureDepthSnapshot(e.depth.Get(),nativeDepth),"Actual stored depth reference extraction");Verify(device.Get(),context.Get(),e.decode.Texture(),w,h,eye,phase,true,e.encoding,&nativeDepth.depth);Verify(device.Get(),context.Get(),e.motion.Get(),w,h,eye,phase,false,e.encoding);Verify(device.Get(),context.Get(),e.mask.Get(),w,h,eye+2,phase,false,e.encoding);
                CameraMotionFrame frame;for(unsigned k=0;k<4;++k)frame.currentClipToPreviousClip[k*5]=1;frame.historyValid=true;e.compose.Process(context.Get(),e.depthView.Get(),e.motionView.Get(),staticView.Get(),frame);Verify(device.Get(),context.Get(),e.compose.Texture(),w,h,eye,phase,false,e.encoding);}
            g3vr::Wait11(context.Get(),fence.Get());bridge.ReleaseReads();bridge.ReleaseReads();
        }
        Hr(native->SetDepthStencilSurface(nullptr));Hr(native->SetRenderTarget(0,original.Get()));native->SetPixelShader(nullptr);context->ClearState();g3vr::Wait11(context.Get(),fence.Get());bridge.Reset();bridge.Reset();
    }
    Require(testedFormats>=2,"Native temporal transport formats were skipped");SequentialSharedDepth(native.Get(),ps.Get(),device.Get(),context.Get(),fence.Get(),w,h);Hr(native->SetRenderTarget(0,original.Get()));native->SetPixelShader(nullptr);original.Reset();ps.Reset();context->ClearState();g3vr::Wait11(context.Get(),fence.Get());fence.Reset();
    auto* pointer=native.Detach();Require(pointer->Release()==0,"Temporal bridge leaked native D3D9 references");
    std::puts("PASS actual DXVK -> device-local Vulkan -> D3D11 temporal transport, two eyes, signed RGBA16F, GPU D16/D24/D32 decode, changing frames, native reuse, BeginScene and lifetime. No game/NGX coverage claim.");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
