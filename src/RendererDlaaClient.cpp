#include "RendererDlaaClient.h"
#include "dlaa/Pipe.h"
#include "dlaa/SharedTexture.h"
#include "RendererDlaaPackCS.h"
#include "RendererDlaaPresentVS.h"
#include "RendererDlaaPresentPS.h"
#include <array>
#include <algorithm>
namespace g3vr {
using namespace dlaa;
struct RendererDlaaClient::Impl {
    Gpu gpu;unsigned width,height;DlaaInputSource source;
    std::unique_ptr<WorkerProcess> worker;
    SharedTexture shared[2][5];ComPtr<ID3D11Texture2D> output[2];
    ComPtr<ID3D11UnorderedAccessView> colorUav[2],motionUav[2],reactiveUav[2];
    ComPtr<ID3D11Texture2D> rawBackground[2];ComPtr<ID3D11ShaderResourceView> rawBackgroundView[2];ComPtr<ID3D11UnorderedAccessView> rawBackgroundUav[2];
    ComPtr<ID3D11ComputeShader> pack;ComPtr<ID3D11Buffer> constants,presentConstants;
    struct Packing {uint32_t width,height,srgb,policy;float transform[16],camera[4];uint32_t overlay[4],previousOverlay[4];};
    struct Presentation {uint32_t overlay[4],previousOverlay[4],srgb,padding[3];};
    static_assert(sizeof(Packing)==128&&sizeof(Presentation)==48);
    ComPtr<ID3D11VertexShader> presentVs;ComPtr<ID3D11PixelShader> presentPs;
    ComPtr<ID3D11ShaderResourceView> outputViews[2];
    ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depthState;ComPtr<ID3D11BlendState> blend;
    struct Target {ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11RenderTargetView> view;};
    std::vector<Target> targets;
    ComPtr<ID3D11DeviceContext1> context1;ComPtr<ID3DDeviceContextState> privateState;
    struct Views {ComPtr<ID3D11Texture2D> color,motion,depth,reactive;ComPtr<ID3D11ShaderResourceView> colorView,motionView,depthView,reactiveView;};
    Views views[2];uint64_t previousSerial=0,previousFrame=0,previousEpoch=0,previousTime=0;
    std::array<std::array<uint32_t,4>,2> currentOverlay{},previousOverlay{};
    std::array<uint32_t,2> currentSrgb{};DlaaMotionPolicy previousPolicy=DlaaMotionPolicy::ExactScene;
    DWORD thread=GetCurrentThreadId();bool failed=false,ready=false;
    struct StateScope {
        ID3D11DeviceContext1* context;ComPtr<ID3DDeviceContextState> previous;
        StateScope(Impl& p):context(p.context1.Get()){context->SwapDeviceContextState(p.privateState.Get(),&previous);}
        ~StateScope(){context->SwapDeviceContextState(previous.Get(),nullptr);}
    };
    Impl(ID3D11Device* device,ID3D11DeviceContext* context,unsigned w,unsigned h,
        std::filesystem::path const& executable,DlaaInputSource origin):gpu(device,context),width(w),height(h),source(origin){
        Require(w>=64&&h>=64&&w<=8192&&h<=8192&&uint64_t(w)*h<=16777216,"DLAA renderer dimensions");
        Require(origin==DlaaInputSource::GameRenderer||origin==DlaaInputSource::SyntheticValidation,"DLAA source identity");
        ComPtr<ID3D11Device1> device1;Hr(gpu.device.As(&device1),"DLAA D3D11.1 device");Hr(gpu.context.As(&context1),"DLAA D3D11.1 context");
        D3D_FEATURE_LEVEL level=D3D_FEATURE_LEVEL_11_0;
        Hr(device1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),nullptr,&privateState),"DLAA private pipeline state");
        Hr(device->CreateComputeShader(RendererDlaaPackCS,sizeof(RendererDlaaPackCS),nullptr,&pack),"DLAA input packing shader");
        Hr(device->CreateVertexShader(RendererDlaaPresentVS,sizeof(RendererDlaaPresentVS),nullptr,&presentVs),"DLAA presentation vertex shader");
        Hr(device->CreatePixelShader(RendererDlaaPresentPS,sizeof(RendererDlaaPresentPS),nullptr,&presentPs),"DLAA presentation pixel shader");
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;Hr(device->CreateRasterizerState(&rs,&raster),"DLAA presentation raster");
        D3D11_DEPTH_STENCIL_DESC ds{};Hr(device->CreateDepthStencilState(&ds,&depthState),"DLAA presentation depth");
        D3D11_BLEND_DESC bs{};bs.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;Hr(device->CreateBlendState(&bs,&blend),"DLAA presentation blend");
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(Packing);cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Hr(device->CreateBuffer(&cb,nullptr,&constants),"DLAA packing constants");cb.ByteWidth=sizeof(Presentation);Hr(device->CreateBuffer(&cb,nullptr,&presentConstants),"DLAA overlay presentation constants");
        worker=std::make_unique<WorkerProcess>(executable);
        dlaa::Initialize config;config.width=w;config.height=h;config.adapterLow=gpu.description.AdapterLuid.LowPart;config.adapterHigh=gpu.description.AdapterLuid.HighPart;
        config.source=origin==DlaaInputSource::GameRenderer?Source::GameRenderer:Source::SyntheticValidationScene;
        constexpr DXGI_FORMAT formats[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8_UNORM};
        for(unsigned e=0;e<2;++e){
            for(unsigned b=0;b<5;++b){shared[e][b].Create(gpu,w,h,formats[b]);config.textures[e][b]=worker->Duplicate(shared[e][b].shared.value);}
            Hr(device->CreateUnorderedAccessView(shared[e][0].texture.Get(),nullptr,&colorUav[e]),"DLAA packed color UAV");
            Hr(device->CreateUnorderedAccessView(shared[e][2].texture.Get(),nullptr,&motionUav[e]),"DLAA packed motion UAV");
            Hr(device->CreateUnorderedAccessView(shared[e][4].texture.Get(),nullptr,&reactiveUav[e]),"DLAA packed reactive UAV");
            output[e]=gpu.Texture(w,h,formats[3],D3D11_BIND_SHADER_RESOURCE);
            Hr(device->CreateShaderResourceView(output[e].Get(),nullptr,&outputViews[e]),"DLAA eye output view");
            rawBackground[e]=gpu.Texture(w,h,DXGI_FORMAT_R8_UNORM,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS);
            Hr(device->CreateShaderResourceView(rawBackground[e].Get(),nullptr,&rawBackgroundView[e]),"DLAA native background view");
            Hr(device->CreateUnorderedAccessView(rawBackground[e].Get(),nullptr,&rawBackgroundUav[e]),"DLAA native background output");
        }
        Require(Valid(config),"DLAA renderer protocol initialization");worker->pipe.Send(Message::Initialize,config);
        auto reply=worker->pipe.Receive<Reply>(Message::Reply);Require(reply.success&&reply.serial==0&&reply.writtenEyes==0,reply.detail);
    }
    D3D11_TEXTURE2D_DESC Texture(ID3D11Texture2D* image)const {
        Require(image!=nullptr,"DLAA input texture missing");ComPtr<ID3D11Device> owner;image->GetDevice(&owner);Require(owner.Get()==gpu.device.Get(),"DLAA input belongs to another device");
        D3D11_TEXTURE2D_DESC d{};image->GetDesc(&d);
        Require(d.Width==width&&d.Height==height&&d.ArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&d.Usage==D3D11_USAGE_DEFAULT&&d.CPUAccessFlags==0,"DLAA input dimensions / resource contract");return d;
    }
    void Validate(DlaaStereoFrame const& frame)const {
        Require(!failed&&worker&&thread==GetCurrentThreadId(),"DLAA session failed / wrong renderer thread");
        Frame protocol;protocol.serial=frame.serial;protocol.validInputs=AllInputs;protocol.jitterX=frame.jitterX;protocol.jitterY=frame.jitterY;protocol.deltaMs=frame.deltaMs;protocol.resetMask=frame.resetMask;
        Require(Valid(protocol,previousSerial),"DLAA invalid serial / jitter / frame time");
        Require(frame.motionPolicy==DlaaMotionPolicy::ExactScene||frame.motionPolicy==DlaaMotionPolicy::ExperimentalReactiveFallback,"DLAA unknown motion policy");
        auto const& a=frame.eyes[0];auto const& b=frame.eyes[1];
        Require(a.frame&&a.epoch&&a.displayTimeNs&&a.frame==b.frame&&a.epoch==b.epoch&&a.displayTimeNs==b.displayTimeNs,"DLAA stereo stamps mismatch");
        Require(!previousSerial||a.epoch!=previousEpoch||(a.frame>previousFrame&&a.displayTimeNs>previousTime),"DLAA stale renderer frame");
        for(unsigned e=0;e<2;++e){auto const& input=frame.eyes[e];
            auto required=frame.motionPolicy==DlaaMotionPolicy::ExactScene?uint32_t(DlaaReady):uint32_t(DlaaReady&~DlaaFullSceneMotion);
            Require(input.eye==e&&(input.readiness&required)==required&&!(input.readiness&~uint32_t(DlaaReady))&&input.colorEncoding!=DlaaColorEncoding::Unknown,"DLAA incomplete scene coverage / jitter / color contract");
            if(frame.motionPolicy==DlaaMotionPolicy::ExactScene)Require(input.readiness==DlaaReady,"DLAA exact scene motion required");
            else {for(float v:input.currentClipToPreviousClip)Require(std::isfinite(v),"DLAA nonfinite fallback camera");for(float v:input.previousMinusCurrentRasterJitter)Require(std::isfinite(v)&&std::abs(v)<=1,"DLAA invalid fallback jitter delta");}
            auto const& rect=input.protectedOverlay;Require(rect[0]<=rect[2]&&rect[1]<=rect[3]&&rect[2]<=width&&rect[3]<=height,"DLAA protected overlay bounds");
            Require(input.colorEncoding==DlaaColorEncoding::Linear||input.colorEncoding==DlaaColorEncoding::Srgb,"DLAA color encoding");
            auto color=Texture(input.color),depth=Texture(input.depth),motion=Texture(input.motion),reactive=Texture(input.reactive);
            Require(input.color!=input.depth&&input.color!=input.motion&&input.depth!=input.motion,"DLAA aliased inputs");
            Require(reactive.Format==DXGI_FORMAT_R8_UNORM&&(reactive.BindFlags&D3D11_BIND_SHADER_RESOURCE)&&input.reactive!=input.color&&input.reactive!=input.depth&&input.reactive!=input.motion,"DLAA missing/aliased reactive mask");
            Require(color.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||color.Format==DXGI_FORMAT_B8G8R8A8_UNORM||color.Format==DXGI_FORMAT_R8G8B8A8_UNORM,"DLAA unsupported color texture");
            Require(color.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT||input.colorEncoding==DlaaColorEncoding::Linear,"DLAA float color must be linear");
            Require((color.BindFlags&D3D11_BIND_SHADER_RESOURCE)&&depth.Format==DXGI_FORMAT_R32_FLOAT&&(motion.BindFlags&D3D11_BIND_SHADER_RESOURCE),"DLAA depth / shader input format");
            Require(motion.Format==DXGI_FORMAT_R16G16B16A16_FLOAT||(source==DlaaInputSource::SyntheticValidation&&motion.Format==DXGI_FORMAT_R16G16_FLOAT),"DLAA native motion flags required");
            if(frame.motionPolicy==DlaaMotionPolicy::ExperimentalReactiveFallback)Require(motion.Format==DXGI_FORMAT_R16G16B16A16_FLOAT&&(depth.BindFlags&D3D11_BIND_SHADER_RESOURCE),"DLAA fallback requires geometry flags and sampleable depth");
        }
        for(auto* left:{a.color,a.depth,a.motion,a.reactive})for(auto* right:{b.color,b.depth,b.motion,b.reactive})Require(left!=right,"DLAA stereo input alias");
    }
    void Prepare(unsigned e,DlaaEyeInput const& input){
        auto& v=views[e];
        if(v.color.Get()!=input.color){ComPtr<ID3D11ShaderResourceView> view;Hr(gpu.device->CreateShaderResourceView(input.color,nullptr,&view),"DLAA source color SRV");v.color=input.color;v.colorView=view;}
        if(v.motion.Get()!=input.motion){ComPtr<ID3D11ShaderResourceView> view;Hr(gpu.device->CreateShaderResourceView(input.motion,nullptr,&view),"DLAA source motion SRV");v.motion=input.motion;v.motionView=view;}
        if(v.depth.Get()!=input.depth){ComPtr<ID3D11ShaderResourceView> view;Hr(gpu.device->CreateShaderResourceView(input.depth,nullptr,&view),"DLAA source depth SRV");v.depth=input.depth;v.depthView=view;}
        if(v.reactive.Get()!=input.reactive){ComPtr<ID3D11ShaderResourceView> view;Hr(gpu.device->CreateShaderResourceView(input.reactive,nullptr,&view),"DLAA source reactive SRV");v.reactive=input.reactive;v.reactiveView=view;}
    }
    Result Evaluate(DlaaStereoFrame const& frame){
        ready=false; // A rejected frame must never expose the preceding image.
        Validate(frame);for(unsigned e=0;e<2;++e)Prepare(e,frame.eyes[e]);
        uint32_t resets=frame.resetMask;auto const& stamp=frame.eyes[0];
        if(!previousSerial||stamp.epoch!=previousEpoch||previousFrame==UINT64_MAX||stamp.frame!=previousFrame+1||stamp.displayTimeNs-previousTime>250000000ull)resets=3;
        if(previousSerial&&frame.motionPolicy!=previousPolicy)resets=3;
        auto oldOverlay=(resets==3)?std::array<std::array<uint32_t,4>,2>{}:currentOverlay;
        ready=false;
        try {
            // Switching context states also unbinds any renderer RTV/SRV that
            // aliases an input. The complete caller state is restored on exit.
            StateScope state(*this);
            gpu.context->ClearState(); // Only our private context state is cleared.
            {std::unique_ptr<TextureLease> leases[2][5];
                for(unsigned e=0;e<2;++e)for(unsigned b:{0u,1u,2u,4u})leases[e][b]=std::make_unique<TextureLease>(shared[e][b],0,1);
                for(unsigned e=0;e<2;++e){
                    auto const& input=frame.eyes[e];Packing cb{};cb.width=width;cb.height=height;cb.srgb=input.colorEncoding==DlaaColorEncoding::Srgb?1u:0u;cb.policy=frame.motionPolicy==DlaaMotionPolicy::ExperimentalReactiveFallback?1u:0u;std::copy(input.currentClipToPreviousClip.begin(),input.currentClipToPreviousClip.end(),cb.transform);cb.camera[0]=input.previousMinusCurrentRasterJitter[0];cb.camera[1]=input.previousMinusCurrentRasterJitter[1];cb.camera[2]=input.cameraHistory?1.f:0.f;std::copy(input.protectedOverlay.begin(),input.protectedOverlay.end(),cb.overlay);std::copy(oldOverlay[e].begin(),oldOverlay[e].end(),cb.previousOverlay);
                    gpu.context->UpdateSubresource(constants.Get(),0,nullptr,&cb,0,0);
                    ID3D11Buffer* buffer=constants.Get();gpu.context->CSSetConstantBuffers(0,1,&buffer);gpu.context->CSSetShader(pack.Get(),nullptr,0);
                    ID3D11ShaderResourceView* srv[]{views[e].colorView.Get(),views[e].motionView.Get(),views[e].depthView.Get(),views[e].reactiveView.Get()};gpu.context->CSSetShaderResources(0,4,srv);
                    ID3D11UnorderedAccessView* uav[]{colorUav[e].Get(),motionUav[e].Get(),reactiveUav[e].Get(),rawBackgroundUav[e].Get()};gpu.context->CSSetUnorderedAccessViews(0,4,uav,nullptr);
                    gpu.context->Dispatch((width+7)/8,(height+7)/8,1);
                    ID3D11UnorderedAccessView* noneUav[4]{};ID3D11ShaderResourceView* noneSrv[4]{};
                    gpu.context->CSSetUnorderedAccessViews(0,4,noneUav,nullptr);gpu.context->CSSetShaderResources(0,4,noneSrv);
                    gpu.context->CopyResource(shared[e][1].texture.Get(),frame.eyes[e].depth);
                }
                gpu.Wait(2000); // Producer completion before handing ownership to NGX.
            }
            Frame request;request.serial=frame.serial;request.validInputs=AllInputs;request.resetMask=resets;request.jitterX=frame.jitterX;request.jitterY=frame.jitterY;request.deltaMs=frame.deltaMs;
            worker->pipe.Send(Message::Frame,request);auto reply=worker->pipe.Receive<Reply>(Message::Reply);
            Require(reply.success&&reply.serial==frame.serial&&reply.writtenEyes==3,reply.success?"DLAA stale / incomplete stereo output":reply.detail);
            {TextureLease left(shared[0][3],1,0),right(shared[1][3],1,0);
                for(unsigned e=0;e<2;++e)gpu.context->CopyResource(output[e].Get(),shared[e][3].texture.Get());
                gpu.Wait(2000); // Output copy finished before the worker reuses it.
            }
            previousSerial=frame.serial;previousFrame=stamp.frame;previousEpoch=stamp.epoch;previousTime=stamp.displayTimeNs;ready=true;
            previousPolicy=frame.motionPolicy;previousOverlay=oldOverlay;for(unsigned e=0;e<2;++e){currentOverlay[e]=frame.eyes[e].protectedOverlay;currentSrgb[e]=frame.eyes[e].colorEncoding==DlaaColorEncoding::Srgb?1u:0u;}
            Result result;result.serial=frame.serial;result.resetMask=resets;std::copy_n(reply.gpuMs,2,result.gpuMs);return result;
        } catch(...){failed=true;ready=false;worker.reset();throw;}
    }
    void Present(unsigned eye,uint64_t serial,ID3D11Texture2D* target){
        Require(!failed&&ready&&thread==GetCurrentThreadId()&&eye<2&&serial==previousSerial,"DLAA presentation stale / unavailable eye");
        auto d=Texture(target);Require((d.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB||d.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS)&&(d.BindFlags&D3D11_BIND_RENDER_TARGET),"DLAA requires XR sRGB BGRA target");
        Target* found=nullptr;for(auto& entry:targets)if(entry.image.Get()==target){found=&entry;break;}
        if(!found){Require(targets.size()<32,"DLAA too many XR output images");Target entry;entry.image=target;D3D11_RENDER_TARGET_VIEW_DESC r{};r.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;r.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
            Hr(gpu.device->CreateRenderTargetView(target,&r,&entry.view),"DLAA XR presentation view");targets.push_back(std::move(entry));found=&targets.back();}
        StateScope state(*this);gpu.context->ClearState();auto* rt=found->view.Get();gpu.context->OMSetRenderTargets(1,&rt,nullptr);gpu.context->OMSetDepthStencilState(depthState.Get(),0);gpu.context->OMSetBlendState(blend.Get(),nullptr,~0u);gpu.context->RSSetState(raster.Get());
        D3D11_VIEWPORT viewport{0,0,float(width),float(height),0,1};gpu.context->RSSetViewports(1,&viewport);gpu.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gpu.context->VSSetShader(presentVs.Get(),nullptr,0);gpu.context->PSSetShader(presentPs.Get(),nullptr,0);ID3D11ShaderResourceView* src[]{outputViews[eye].Get(),views[eye].colorView.Get(),rawBackgroundView[eye].Get()};gpu.context->PSSetShaderResources(0,3,src);
        Presentation pc{};std::copy(currentOverlay[eye].begin(),currentOverlay[eye].end(),pc.overlay);std::copy(previousOverlay[eye].begin(),previousOverlay[eye].end(),pc.previousOverlay);pc.srgb=currentSrgb[eye];gpu.context->UpdateSubresource(presentConstants.Get(),0,nullptr,&pc,0,0);auto* cb=presentConstants.Get();gpu.context->PSSetConstantBuffers(0,1,&cb);gpu.context->Draw(3,0);
        gpu.context->ClearState(); // Unbind output before its XR owner releases it.
    }
    ~Impl(){if(worker)try{worker->Finish();}catch(...){/* Own-child teardown is bounded. */}}
};
RendererDlaaClient::RendererDlaaClient()=default;
RendererDlaaClient::~RendererDlaaClient()=default;
void RendererDlaaClient::Initialize(ID3D11Device* d,ID3D11DeviceContext* c,unsigned w,unsigned h,std::filesystem::path const& worker,DlaaInputSource source){Reset();impl=std::make_unique<Impl>(d,c,w,h,worker,source);}
RendererDlaaClient::Result RendererDlaaClient::Evaluate(DlaaStereoFrame const& frame){Require(impl!=nullptr,"DLAA renderer client not initialized");return impl->Evaluate(frame);}
ID3D11Texture2D* RendererDlaaClient::Output(unsigned eye)const noexcept{return impl&&!impl->failed&&impl->ready&&eye<2?impl->output[eye].Get():nullptr;}
void RendererDlaaClient::Present(unsigned eye,uint64_t serial,ID3D11Texture2D* target){Require(impl!=nullptr,"DLAA renderer client not initialized");impl->Present(eye,serial,target);}
void RendererDlaaClient::Reset()noexcept{impl.reset();}
}
