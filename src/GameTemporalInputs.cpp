#include "GameTemporalInputs.h"
#include "NativeTemporalBridge.h"
#include <windows.h>
#include <stdexcept>
#include <cmath>
using Microsoft::WRL::ComPtr;
using namespace diagnostic::temporal;
namespace g3vr {
namespace {
void Require(bool value,char const* why){if(!value)throw std::runtime_error(why);}
void Check(HRESULT value,char const* why){Require(SUCCEEDED(value),why);}
Matrix ViewProjection(float const* view,float const* projection){
    Require(view&&projection,"Temporal camera matrices missing");Matrix result{};
    for(unsigned i=0;i<16;++i)Require(std::isfinite(view[i])&&std::isfinite(projection[i]),"Temporal camera matrices nonfinite");
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)result[r*4+c]+=double(view[r*4+k])*projection[k*4+c];return result;
}
}
struct GameTemporalInputs::Impl {
    struct Eye {
        std::array<ComPtr<IDirect3DSurface9>,3> sources;
        std::array<ComPtr<ID3D11Texture2D>,3> inputs;
        std::array<ComPtr<ID3D11ShaderResourceView>,2> motionViews;
        ComPtr<ID3D11ShaderResourceView> depthView;
        GpuTemporalDepth decoder;GpuTemporalMotion composer;
        GpuTemporalFinalize finalizer;ComPtr<IDirect3DSurface9> skySource;ComPtr<ID3D11Texture2D> skyInput;
        CameraMotionFrame camera;
        Matrix previousVP{};GeometryFrameStamp previousStamp;std::array<float,2> previousJitter{};bool ready=false,finalReady=false;
    };
    DxvkTemporalBridge bridge;NativeTemporalBridge nativeBridge;bool nativeTransport=false;
    DxvkTemporalBridge lateBridge;NativeTemporalBridge lateNativeBridge;bool lateInitialized=false,lateOutstanding=false;
    ComPtr<IDirect3DDevice9Ex> nativeDevice;
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Query> fence;ComPtr<ID3D11Texture2D> unknown;ComPtr<ID3D11ShaderResourceView> unknownView;
    std::array<Eye,2> eyes;unsigned width=0,height=0;bool outstanding=false,failed=false;
    void Finish(){
        if(!outstanding&&!lateOutstanding)return;
        context->End(fence.Get());context->Flush();auto start=GetTickCount64();
        for(;;){BOOL done=FALSE;auto hr=context->GetData(fence.Get(),&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);Check(hr,"Temporal D3D11 completion fence");
            if(hr==S_OK&&done)break;Require(GetTickCount64()-start<2000,"Temporal D3D11 completion timeout");SwitchToThread();}
        if(outstanding){if(nativeTransport)nativeBridge.ReleaseReads();else bridge.ReleaseReads();outstanding=false;}
        if(lateOutstanding){if(nativeTransport)lateNativeBridge.ReleaseReads();else lateBridge.ReleaseReads();lateOutstanding=false;}
    }
    void Bind(Eye& eye,IDirect3DSurface9* depth,IDirect3DSurface9* skin,IDirect3DSurface9* native){
        std::array<IDirect3DSurface9*,3> sources{{depth,skin,native}};
        if(eye.sources[0]){for(unsigned i=0;i<3;++i)Require(eye.sources[i].Get()==sources[i],"Temporal native surfaces changed; reset input history");return;}
        for(unsigned i=0;i<3;++i){
            auto encoding=nativeTransport?nativeBridge.Register(sources[i],device.Get(),eye.inputs[i]):bridge.Register(sources[i],device.Get(),eye.inputs[i]);eye.sources[i]=sources[i];
            if(!i){eye.decoder.Initialize(device.Get(),width,height,encoding);Check(device->CreateShaderResourceView(eye.decoder.Texture(),nullptr,&eye.depthView),"Temporal decoded depth view");}
            else{Require(encoding==TemporalEncoding::Color16F,"Temporal motion source format");Check(device->CreateShaderResourceView(eye.inputs[i].Get(),nullptr,&eye.motionViews[i-1]),"Temporal motion view");}
        }
        eye.composer.Initialize(device.Get(),width,height);
        eye.finalizer.Initialize(device.Get(),width,height);
    }
    ~Impl(){try{Finish();}catch(...){/* Device-loss teardown cannot wait indefinitely. */}}
};
GameTemporalInputs::GameTemporalInputs()=default;
GameTemporalInputs::~GameTemporalInputs()=default;
void GameTemporalInputs::Initialize(IDirect3DDevice9Ex* native,ID3D11Device* device,ID3D11DeviceContext* context,unsigned width,unsigned height){
    Reset();Require(native&&device&&context&&width&&height&&width<=8192&&height<=8192&&uint64_t(width)*height<=33554432,"Temporal GPU input initialization");
    ComPtr<ID3D11Device> owner;context->GetDevice(&owner);Require(owner.Get()==device&&context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Temporal GPU context ownership");
    auto p=std::make_unique<Impl>();p->device=device;p->context=context;p->nativeDevice=native;p->width=width;p->height=height;
    if(!p->bridge.Initialize(native)){Require(p->nativeBridge.Initialize(native),"Temporal GPU input transport unavailable (DXVK or native NVIDIA required)");p->nativeTransport=true;}
    D3D11_QUERY_DESC q{D3D11_QUERY_EVENT,0};Check(device->CreateQuery(&q,&p->fence),"Temporal input fence");
    D3D11_TEXTURE2D_DESC t{};t.Width=width;t.Height=height;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R8_UNORM;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    Check(device->CreateTexture2D(&t,nullptr,&p->unknown),"Temporal unknown coverage image");ComPtr<ID3D11RenderTargetView> clear;Check(device->CreateRenderTargetView(p->unknown.Get(),nullptr,&clear),"Temporal unknown coverage clear");float zero[4]{};context->ClearRenderTargetView(clear.Get(),zero);
    Check(device->CreateShaderResourceView(p->unknown.Get(),nullptr,&p->unknownView),"Temporal unknown coverage view");impl=std::move(p);
}
GameTemporalInputs::Result GameTemporalInputs::Capture(unsigned index,GeometryFrameStamp const& stamp,float const* view,float const* projection,IDirect3DSurface9* opaqueDepth,IDirect3DSurface9* skinMotion,IDirect3DSurface9* nativeMotion,std::array<float,2> jitter){
    Require(impl&&!impl->failed&&index<2&&stamp.eye==index&&stamp.frame&&stamp.epoch&&stamp.displayTimeNs&&opaqueDepth&&skinMotion&&nativeMotion,"Temporal GPU capture inputs");
    auto& p=*impl;auto& eye=p.eyes[index];Require(!eye.ready||stamp.frame>eye.previousStamp.frame,"Temporal duplicate or reversed eye frame");auto vp=ViewProjection(view,projection);
    for(float value:jitter)Require(std::isfinite(value)&&std::abs(value)<=.5f,"Temporal input raster jitter");
    eye.finalReady=false;
    try{
        p.Bind(eye,opaqueDepth,skinMotion,nativeMotion);
        for(auto const& input:eye.inputs){if(p.nativeTransport)p.nativeBridge.Queue(input.Get());else p.bridge.Queue(input.Get());}
        if(p.nativeTransport)p.nativeBridge.Complete();else p.bridge.Complete();p.outstanding=true;
        eye.decoder.Process(p.context.Get(),eye.inputs[0].Get());CameraMotionFrame camera;camera.historyValid=eye.ready&&stamp.Follows(eye.previousStamp);
        camera.currentRasterJitter=jitter;camera.previousRasterJitter=eye.previousJitter; // Native geometry layers are already de-jittered.
        if(camera.historyValid)camera.currentClipToPreviousClip=CameraReprojection(vp,eye.previousVP);
        else for(unsigned i=0;i<4;++i)camera.currentClipToPreviousClip[i*5]=1;
        eye.composer.ProcessLayers(p.context.Get(),eye.depthView.Get(),eye.motionViews[0].Get(),eye.motionViews[1].Get(),p.unknownView.Get(),camera);
        p.Finish();eye.camera=camera;eye.previousVP=vp;eye.previousStamp=stamp;eye.previousJitter=jitter;eye.ready=true;return {stamp.frame,index,camera.historyValid};
    }catch(...){p.failed=true;eye.ready=false;try{p.Finish();}catch(...){}throw;}
}
void GameTemporalInputs::Finalize(unsigned index,GeometryFrameStamp const& stamp,IDirect3DSurface9* sky,bool history){
    Require(impl&&!impl->failed&&index<2&&stamp.eye==index,"Temporal late finalization inputs");auto& p=*impl;auto& eye=p.eyes[index];
    Require(eye.ready&&!eye.finalReady&&stamp.frame==eye.previousStamp.frame&&stamp.epoch==eye.previousStamp.epoch&&stamp.displayTimeNs==eye.previousStamp.displayTimeNs,"Temporal late eye/stamp mismatch or duplicate finalization");Require(sky||!history,"Temporal sky history without source");
    try{
        if(sky){
            if(!p.lateInitialized){Require(p.nativeTransport?p.lateNativeBridge.Initialize(p.nativeDevice.Get()):p.lateBridge.Initialize(p.nativeDevice.Get()),"Temporal late GPU transport unavailable");p.lateInitialized=true;}
            if(!eye.skySource){auto encoding=p.nativeTransport?p.lateNativeBridge.Register(sky,p.device.Get(),eye.skyInput):p.lateBridge.Register(sky,p.device.Get(),eye.skyInput);Require(encoding==TemporalEncoding::Color16F,"Temporal late sky encoding");eye.skySource=sky;}
            else Require(eye.skySource.Get()==sky,"Temporal late sky surface changed; reset history");
            if(p.nativeTransport){p.lateNativeBridge.Queue(eye.skyInput.Get());p.lateNativeBridge.Complete();}else{p.lateBridge.Queue(eye.skyInput.Get());p.lateBridge.Complete();}p.lateOutstanding=true;
        }
        eye.finalizer.Process(p.context.Get(),eye.composer.Texture(),sky?eye.skyInput.Get():nullptr,history);p.Finish();eye.finalReady=true;
    }catch(...){p.failed=true;eye.finalReady=false;try{p.Finish();}catch(...){}throw;}
}
ID3D11Texture2D* GameTemporalInputs::Motion(unsigned eye)const noexcept{return impl&&!impl->failed&&eye<2&&impl->eyes[eye].ready?(impl->eyes[eye].finalReady?impl->eyes[eye].finalizer.Motion():impl->eyes[eye].composer.Texture()):nullptr;}
ID3D11Texture2D* GameTemporalInputs::Depth(unsigned eye)const noexcept{return impl&&!impl->failed&&eye<2&&impl->eyes[eye].ready?impl->eyes[eye].decoder.Texture():nullptr;}
ID3D11Texture2D* GameTemporalInputs::Reactive(unsigned eye)const noexcept{return impl&&!impl->failed&&eye<2&&impl->eyes[eye].finalReady?impl->eyes[eye].finalizer.Reactive():nullptr;}
bool GameTemporalInputs::Finalized(unsigned eye,uint64_t frame)const noexcept{return impl&&!impl->failed&&eye<2&&impl->eyes[eye].finalReady&&impl->eyes[eye].previousStamp.frame==frame;}
bool GameTemporalInputs::Camera(unsigned eye,uint64_t frame,CameraMotionFrame& camera,GeometryFrameStamp& stamp)const noexcept{
    if(!Finalized(eye,frame))return false;camera=impl->eyes[eye].camera;stamp=impl->eyes[eye].previousStamp;return true;
}
char const* GameTemporalInputs::Transport()const noexcept{return impl?(impl->nativeTransport?"native_NVIDIA_D3D9_shared_D3D11":"DXVK_Vulkan_D3D11"):"unavailable";}
void GameTemporalInputs::Reset()noexcept{impl.reset();}
}
