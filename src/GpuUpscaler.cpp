#include "GpuUpscaler.h"
#include "GpuSmaa.h"
#include <wrl/client.h>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <cmath>
#include "NIS_Config.h"
#define A_CPU 1
#include "ffx_a.h"
#include "ffx_cas.h"
#include "NisScale.h"
#include "NisSharp.h"
#include "CasScale.h"
#include "CasSharp.h"
#include "UpscaleVS.h"
#include "UpscalePS.h"
#include "FxaaPS.h"
namespace diagnostic {
using Microsoft::WRL::ComPtr;
namespace {void Check(HRESULT hr,char const* text){if(FAILED(hr))throw std::runtime_error(text);}}
struct GpuUpscaler::Impl {
    UpscalePlan plan;ComPtr<ID3D11Device> device;std::unique_ptr<GpuSmaa> smaa;
    ComPtr<ID3D11ComputeShader> compute;ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11Buffer> constants,fxaaConstants;ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11Texture2D> scratch,coefficients[2];ComPtr<ID3D11UnorderedAccessView> uav;
    ComPtr<ID3D11ShaderResourceView> scratchView,coefficientViews[2];
    ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11BlendState> blend;
    struct Source {ComPtr<ID3D11Texture2D> image,copy;ComPtr<ID3D11ShaderResourceView> view;};
    struct Target {ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11RenderTargetView> view;};
    std::vector<Source> sources;std::vector<Target> targets;
    explicit Impl(ID3D11Device* d,UpscalePlan const& p):plan(p),device(d){
        if(!p.inputWidth||!p.inputHeight||p.inputWidth>p.outputWidth||p.inputHeight>p.outputHeight||p.outputWidth>16384||p.outputHeight>16384)throw std::runtime_error("Invalid upscale dimensions");
        if(p.method==UpscaleMethod::Off){if(p.Scaled())throw std::runtime_error("Upscaling is disabled");if(!p.fxaa&&!p.smaa)return;}
        if(d->GetFeatureLevel()<D3D_FEATURE_LEVEL_11_0)throw std::runtime_error("Scene filters require D3D11 feature level 11");
        if(p.method!=UpscaleMethod::Off){
        bool scale=p.Scaled();
        auto code=p.method==UpscaleMethod::NIS?(scale?NisScale:NisSharp):(scale?CasScale:CasSharp);
        auto bytes=p.method==UpscaleMethod::NIS?(scale?sizeof(NisScale):sizeof(NisSharp)):(scale?sizeof(CasScale):sizeof(CasSharp));
        Check(d->CreateComputeShader(code,bytes,nullptr,&compute),"Create NIS/CAS shader");
        NISConfig nis{};struct CasConstants {uint32_t a[4],b[4],input[2],output[2];} cas{};
        D3D11_BUFFER_DESC cb{};cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cb.Usage=D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA data{};
        if(p.method==UpscaleMethod::NIS){
            if(!NVScalerUpdateConfig(nis,p.sharpness*.01f,0,0,p.inputWidth,p.inputHeight,p.inputWidth,p.inputHeight,0,0,p.outputWidth,p.outputHeight,p.outputWidth,p.outputHeight))throw std::runtime_error("NIS configuration");
            cb.ByteWidth=sizeof(nis);data.pSysMem=&nis;
            for(unsigned i=0;i<2;++i){D3D11_TEXTURE2D_DESC t{};t.Width=kFilterSize/4;t.Height=kPhaseCount;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;t.Usage=D3D11_USAGE_IMMUTABLE;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
                D3D11_SUBRESOURCE_DATA c{};c.pSysMem=i?static_cast<void const*>(coef_usm):static_cast<void const*>(coef_scale);c.SysMemPitch=kFilterSize*sizeof(float);
                Check(d->CreateTexture2D(&t,&c,&coefficients[i]),"NIS coefficients");Check(d->CreateShaderResourceView(coefficients[i].Get(),nullptr,&coefficientViews[i]),"NIS coefficient view");}
        }else{CasSetup(cas.a,cas.b,p.sharpness*.01f,float(p.inputWidth),float(p.inputHeight),float(p.outputWidth),float(p.outputHeight));cas.input[0]=p.inputWidth;cas.input[1]=p.inputHeight;cas.output[0]=p.outputWidth;cas.output[1]=p.outputHeight;cb.ByteWidth=sizeof(cas);data.pSysMem=&cas;}
        Check(d->CreateBuffer(&cb,&data,&constants),"Upscale constants");
        D3D11_TEXTURE2D_DESC t{};t.Width=p.outputWidth;t.Height=p.outputHeight;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R8G8B8A8_TYPELESS;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
        Check(d->CreateTexture2D(&t,nullptr,&scratch),"Upscale scratch");
        D3D11_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_R8G8B8A8_UNORM;u.ViewDimension=D3D11_UAV_DIMENSION_TEXTURE2D;Check(d->CreateUnorderedAccessView(scratch.Get(),&u,&uav),"Upscale UAV");
        // Compute operates on gamma-encoded SDR. Decode the stored result for
        // the final sRGB render target, avoiding a second gamma encoding.
        D3D11_SHADER_RESOURCE_VIEW_DESC s{};s.Format=p.fxaa?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;s.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;s.Texture2D.MipLevels=1;
        Check(d->CreateShaderResourceView(scratch.Get(),&s,&scratchView),"Upscale result view");
        }
        if(p.smaa)smaa=std::make_unique<GpuSmaa>(d,p.outputWidth,p.outputHeight,p.method!=UpscaleMethod::Off,p.smaaQuality);
        if(p.fxaa){float reciprocal[4]={1.f/p.outputWidth,1.f/p.outputHeight,0,0};D3D11_BUFFER_DESC cb{};cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cb.Usage=D3D11_USAGE_IMMUTABLE;cb.ByteWidth=sizeof(reciprocal);D3D11_SUBRESOURCE_DATA data{};data.pSysMem=reciprocal;Check(d->CreateBuffer(&cb,&data,&fxaaConstants),"FXAA constants");}
        Check(d->CreateVertexShader(UpscaleVS,sizeof(UpscaleVS),nullptr,&vs),"Upscale blit VS");Check(d->CreatePixelShader(p.fxaa?FxaaPS:UpscalePS,p.fxaa?sizeof(FxaaPS):sizeof(UpscalePS),nullptr,&ps),"Scene filter PS");
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;Check(d->CreateSamplerState(&sd,&sampler),"NIS sampler");
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;Check(d->CreateRasterizerState(&rs,&raster),"Upscale raster");
        D3D11_DEPTH_STENCIL_DESC ds{};Check(d->CreateDepthStencilState(&ds,&depth),"Upscale depth");D3D11_BLEND_DESC bs{};bs.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;Check(d->CreateBlendState(&bs,&blend),"Upscale blend");
    }
    Source& Input(ID3D11Texture2D* input){
        for(auto& s:sources)if(s.image.Get()==input)return s;
        if(sources.size()>=2)throw std::runtime_error("Unexpected extra upscale input");
        Source s;s.image=input;D3D11_TEXTURE2D_DESC d{};input->GetDesc(&d);
        if(d.Width!=plan.inputWidth||d.Height!=plan.inputHeight||d.SampleDesc.Count!=1||d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)throw std::runtime_error("Unsupported upscale input");
        if(FAILED(device->CreateShaderResourceView(input,nullptr,&s.view))){d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=d.CPUAccessFlags=0;d.Usage=D3D11_USAGE_DEFAULT;Check(device->CreateTexture2D(&d,nullptr,&s.copy),"Upscale input copy");Check(device->CreateShaderResourceView(s.copy.Get(),nullptr,&s.view),"Upscale input view");}
        sources.push_back(std::move(s));return sources.back();
    }
    ID3D11RenderTargetView* Output(ID3D11Texture2D* output){
        for(auto& t:targets)if(t.image.Get()==output)return t.view.Get();
        if(targets.size()>=32)throw std::runtime_error("Too many upscale output images");
        D3D11_TEXTURE2D_DESC d{};output->GetDesc(&d);if(d.Width!=plan.outputWidth||d.Height!=plan.outputHeight||(d.Format!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB&&d.Format!=DXGI_FORMAT_B8G8R8A8_TYPELESS)||d.SampleDesc.Count!=1||d.ArraySize!=1)throw std::runtime_error("Unsupported upscale output");
        // VDXR allocates typeless images even for an sRGB XR swapchain.
        D3D11_RENDER_TARGET_VIEW_DESC r{};r.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;r.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
        Target t;t.image=output;Check(device->CreateRenderTargetView(output,&r,&t.view),"Upscale XR target view");targets.push_back(std::move(t));return targets.back().view.Get();
    }
};
GpuUpscaler::GpuUpscaler()=default;GpuUpscaler::~GpuUpscaler()=default;
void GpuUpscaler::Initialize(ID3D11Device* device,UpscalePlan const& plan){impl=std::make_unique<Impl>(device,plan);}
void GpuUpscaler::Reset() noexcept{impl.reset();}
void GpuUpscaler::Process(ID3D11DeviceContext* context,ID3D11Texture2D* output,ID3D11Texture2D* input){
    if(!impl)throw std::runtime_error("Upscaler not initialized");auto& p=*impl;
    if(p.plan.method==UpscaleMethod::Off&&!p.plan.fxaa&&!p.plan.smaa){context->CopyResource(output,input);return;}
    if(p.plan.method==UpscaleMethod::Off&&p.smaa){p.smaa->Process(context,p.Output(output),input);return;}
    auto& src=p.Input(input);auto* target=p.Output(output);
    context->ClearState();if(src.copy)context->CopyResource(src.copy.Get(),input);
    ID3D11ShaderResourceView* result=src.view.Get();
    if(p.plan.method!=UpscaleMethod::Off){
    ID3D11ShaderResourceView* views[]={src.view.Get(),p.coefficientViews[0].Get(),p.coefficientViews[1].Get()};auto* cb=p.constants.Get();auto* sampler=p.sampler.Get();auto* uav=p.uav.Get();
    context->CSSetShader(p.compute.Get(),nullptr,0);context->CSSetConstantBuffers(0,1,&cb);context->CSSetShaderResources(0,3,views);context->CSSetSamplers(0,1,&sampler);context->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
    unsigned block=p.plan.method==UpscaleMethod::NIS?32:16,blockY=p.plan.method==UpscaleMethod::NIS&&p.plan.Scaled()?24:block;
    context->Dispatch((p.plan.outputWidth+block-1)/block,(p.plan.outputHeight+blockY-1)/blockY,1);
    result=p.scratchView.Get();
    }
    if(p.smaa){p.smaa->Process(context,target,p.scratch.Get());return;}
    context->ClearState();context->OMSetRenderTargets(1,&target,nullptr);context->OMSetDepthStencilState(p.depth.Get(),0);context->OMSetBlendState(p.blend.Get(),nullptr,~0u);context->RSSetState(p.raster.Get());
    D3D11_VIEWPORT viewport{0,0,float(p.plan.outputWidth),float(p.plan.outputHeight),0,1};context->RSSetViewports(1,&viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(p.vs.Get(),nullptr,0);context->PSSetShader(p.ps.Get(),nullptr,0);context->PSSetShaderResources(0,1,&result);
    if(p.plan.fxaa){auto* cb=p.fxaaConstants.Get();auto* sampler=p.sampler.Get();context->PSSetConstantBuffers(0,1,&cb);context->PSSetSamplers(0,1,&sampler);}
    context->Draw(3,0);context->ClearState();
}
}
