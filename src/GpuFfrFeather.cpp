#include "GpuFfrFeather.h"
#include <wrl/client.h>
#include <vector>
#include <stdexcept>
#include "UpscaleVS.h"
#include "FfrFeatherPS.h"
namespace diagnostic {
using Microsoft::WRL::ComPtr;
namespace {void Need(HRESULT r,char const* what){if(FAILED(r))throw std::runtime_error(what);}}
struct GpuFfrFeather::Impl {
    ComPtr<ID3D11Device> device;unsigned width,height;
    ComPtr<ID3D11Texture2D> scene;ComPtr<ID3D11ShaderResourceView> view;
    ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11SamplerState> sampler;ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11BlendState> blend;
    struct Output {ComPtr<ID3D11Texture2D> image;ComPtr<ID3D11RenderTargetView> view;};std::vector<Output> outputs;
    Impl(ID3D11Device* d,unsigned w,unsigned h):device(d),width(w),height(h){
        if(!d||!w||!h||w>16384||h>16384)throw std::runtime_error("Invalid FFR feather dimensions");
        D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_B8G8R8A8_TYPELESS;t.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        Need(d->CreateTexture2D(&t,nullptr,&scene),"FFR feather scene target");
        D3D11_SHADER_RESOURCE_VIEW_DESC s{};s.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;s.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;s.Texture2D.MipLevels=1;
        Need(d->CreateShaderResourceView(scene.Get(),&s,&view),"FFR feather scene view");
        Need(d->CreateVertexShader(UpscaleVS,sizeof(UpscaleVS),nullptr,&vs),"FFR feather VS");Need(d->CreatePixelShader(FfrFeatherPS,sizeof(FfrFeatherPS),nullptr,&ps),"FFR feather PS");
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(FfrFeatherParameters);cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Need(d->CreateBuffer(&cb,nullptr,&constants),"FFR feather constants");
        D3D11_SAMPLER_DESC ss{};ss.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;ss.AddressU=ss.AddressV=ss.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;ss.MaxLOD=D3D11_FLOAT32_MAX;Need(d->CreateSamplerState(&ss,&sampler),"FFR feather sampler");
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;Need(d->CreateRasterizerState(&rs,&raster),"FFR feather raster");
        D3D11_DEPTH_STENCIL_DESC ds{};Need(d->CreateDepthStencilState(&ds,&depth),"FFR feather depth");D3D11_BLEND_DESC bs{};bs.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;Need(d->CreateBlendState(&bs,&blend),"FFR feather blend");
    }
    ID3D11RenderTargetView* OutputView(ID3D11Texture2D* image){
        for(auto& o:outputs)if(o.image.Get()==image)return o.view.Get();
        D3D11_TEXTURE2D_DESC t{};image->GetDesc(&t);
        if(image==scene.Get()||t.Width!=width||t.Height!=height||t.SampleDesc.Count!=1||t.ArraySize!=1||(t.Format!=DXGI_FORMAT_B8G8R8A8_TYPELESS&&t.Format!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)||outputs.size()>=32)throw std::runtime_error("Unsupported FFR feather output");
        Output o;o.image=image;D3D11_RENDER_TARGET_VIEW_DESC r{};r.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;r.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;Need(device->CreateRenderTargetView(image,&r,&o.view),"FFR feather XR view");outputs.push_back(std::move(o));return outputs.back().view.Get();
    }
};
GpuFfrFeather::GpuFfrFeather()=default;GpuFfrFeather::~GpuFfrFeather()=default;
ID3D11Texture2D* GpuFfrFeather::Target(ID3D11Device* device,unsigned width,unsigned height){
    if(!impl)impl=std::make_unique<Impl>(device,width,height);
    if(impl->device.Get()!=device||impl->width!=width||impl->height!=height)throw std::runtime_error("FFR feather target changed without session restart");return impl->scene.Get();
}
void GpuFfrFeather::Present(ID3D11DeviceContext* context,ID3D11Texture2D* output,FfrFeatherParameters const& p){
    if(!impl||!context||!output)throw std::runtime_error("FFR feather not initialized");auto& s=*impl;auto* target=s.OutputView(output);
    context->ClearState();context->UpdateSubresource(s.constants.Get(),0,nullptr,&p,0,0);
    context->OMSetRenderTargets(1,&target,nullptr);context->OMSetDepthStencilState(s.depth.Get(),0);context->OMSetBlendState(s.blend.Get(),nullptr,~0u);context->RSSetState(s.raster.Get());
    D3D11_VIEWPORT viewport{0,0,float(s.width),float(s.height),0,1};context->RSSetViewports(1,&viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(s.vs.Get(),nullptr,0);context->PSSetShader(s.ps.Get(),nullptr,0);
    auto* srv=s.view.Get();auto* cb=s.constants.Get();auto* sampler=s.sampler.Get();context->PSSetShaderResources(0,1,&srv);context->PSSetConstantBuffers(0,1,&cb);context->PSSetSamplers(0,1,&sampler);context->Draw(3,0);context->ClearState();
}
void GpuFfrFeather::Reset()noexcept{impl.reset();}
}
