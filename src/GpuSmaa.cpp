#include "GpuSmaa.h"
#include <wrl/client.h>
#include <stdexcept>
#include "AreaTex.h"
#include "SearchTex.h"
#include "SmaaMediumEdgeVS.h"
#include "SmaaMediumEdgePS.h"
#include "SmaaMediumWeightVS.h"
#include "SmaaMediumWeightPS.h"
#include "SmaaMediumBlendVS.h"
#include "SmaaMediumBlendPS.h"
#include "SmaaHighEdgeVS.h"
#include "SmaaHighEdgePS.h"
#include "SmaaHighWeightVS.h"
#include "SmaaHighWeightPS.h"
#include "SmaaHighBlendVS.h"
#include "SmaaHighBlendPS.h"
#include "SmaaUltraEdgeVS.h"
#include "SmaaUltraEdgePS.h"
#include "SmaaUltraWeightVS.h"
#include "SmaaUltraWeightPS.h"
#include "SmaaUltraBlendVS.h"
#include "SmaaUltraBlendPS.h"
namespace diagnostic {
using Microsoft::WRL::ComPtr;
namespace {void CheckSmaa(HRESULT hr,char const* action){if(FAILED(hr))throw std::runtime_error(action);}}
struct GpuSmaa::Impl {
    unsigned width,height;bool rgba;
    ComPtr<ID3D11Texture2D> color,edges,weights,area,search;
    ComPtr<ID3D11ShaderResourceView> encoded,linear,edgeView,weightView,areaView,searchView;
    ComPtr<ID3D11RenderTargetView> edgeTarget,weightTarget;
    ComPtr<ID3D11VertexShader> vs[3];ComPtr<ID3D11PixelShader> ps[3];
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11SamplerState> samplers[2];
    ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11BlendState> blend;
    Impl(ID3D11Device* d,unsigned w,unsigned h,bool r,int quality):width(w),height(h),rgba(r){
        auto texture=[&](unsigned x,unsigned y,DXGI_FORMAT format,unsigned bind,ComPtr<ID3D11Texture2D>& result,void const* bytes=nullptr,unsigned pitch=0){
            D3D11_TEXTURE2D_DESC t{};t.Width=x;t.Height=y;t.Format=format;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.BindFlags=bind;t.Usage=bytes?D3D11_USAGE_IMMUTABLE:D3D11_USAGE_DEFAULT;
            D3D11_SUBRESOURCE_DATA data{};data.pSysMem=bytes;data.SysMemPitch=pitch;
            CheckSmaa(d->CreateTexture2D(&t,bytes?&data:nullptr,&result),"SMAA texture");
        };
        // A typed shared D3D9 BGRA input cannot always expose an sRGB view.
        // One GPU-only typeless copy provides both correct read encodings.
        texture(w,h,r?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_TYPELESS,D3D11_BIND_SHADER_RESOURCE,color);
        D3D11_SHADER_RESOURCE_VIEW_DESC sv{};sv.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
        sv.Format=r?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_B8G8R8A8_UNORM;CheckSmaa(d->CreateShaderResourceView(color.Get(),&sv,&encoded),"SMAA encoded color");
        sv.Format=r?DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;CheckSmaa(d->CreateShaderResourceView(color.Get(),&sv,&linear),"SMAA linear color");
        // RGBA8 follows upstream's recommendation, including NVIDIA hardware.
        texture(w,h,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET,edges);
        texture(w,h,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET,weights);
        CheckSmaa(d->CreateShaderResourceView(edges.Get(),nullptr,&edgeView),"SMAA edge view");CheckSmaa(d->CreateShaderResourceView(weights.Get(),nullptr,&weightView),"SMAA weight view");
        CheckSmaa(d->CreateRenderTargetView(edges.Get(),nullptr,&edgeTarget),"SMAA edge target");CheckSmaa(d->CreateRenderTargetView(weights.Get(),nullptr,&weightTarget),"SMAA weight target");
        texture(AREATEX_WIDTH,AREATEX_HEIGHT,DXGI_FORMAT_R8G8_UNORM,D3D11_BIND_SHADER_RESOURCE,area,areaTexBytes,AREATEX_PITCH);
        texture(SEARCHTEX_WIDTH,SEARCHTEX_HEIGHT,DXGI_FORMAT_R8_UNORM,D3D11_BIND_SHADER_RESOURCE,search,searchTexBytes,SEARCHTEX_PITCH);
        CheckSmaa(d->CreateShaderResourceView(area.Get(),nullptr,&areaView),"SMAA area lookup");CheckSmaa(d->CreateShaderResourceView(search.Get(),nullptr,&searchView),"SMAA search lookup");
        struct ShaderCode {void const* bytes;size_t size;};
        ShaderCode vertex[3][3]={
            {{SmaaMediumEdgeVS,sizeof(SmaaMediumEdgeVS)},{SmaaMediumWeightVS,sizeof(SmaaMediumWeightVS)},{SmaaMediumBlendVS,sizeof(SmaaMediumBlendVS)}},
            {{SmaaHighEdgeVS,sizeof(SmaaHighEdgeVS)},{SmaaHighWeightVS,sizeof(SmaaHighWeightVS)},{SmaaHighBlendVS,sizeof(SmaaHighBlendVS)}},
            {{SmaaUltraEdgeVS,sizeof(SmaaUltraEdgeVS)},{SmaaUltraWeightVS,sizeof(SmaaUltraWeightVS)},{SmaaUltraBlendVS,sizeof(SmaaUltraBlendVS)}}
        };
        ShaderCode pixel[3][3]={
            {{SmaaMediumEdgePS,sizeof(SmaaMediumEdgePS)},{SmaaMediumWeightPS,sizeof(SmaaMediumWeightPS)},{SmaaMediumBlendPS,sizeof(SmaaMediumBlendPS)}},
            {{SmaaHighEdgePS,sizeof(SmaaHighEdgePS)},{SmaaHighWeightPS,sizeof(SmaaHighWeightPS)},{SmaaHighBlendPS,sizeof(SmaaHighBlendPS)}},
            {{SmaaUltraEdgePS,sizeof(SmaaUltraEdgePS)},{SmaaUltraWeightPS,sizeof(SmaaUltraWeightPS)},{SmaaUltraBlendPS,sizeof(SmaaUltraBlendPS)}}
        };
        quality=quality>=0&&quality<=2?quality:0;
        for(unsigned i=0;i<3;++i){auto v=vertex[quality][i],p=pixel[quality][i];CheckSmaa(d->CreateVertexShader(v.bytes,v.size,nullptr,&vs[i]),"SMAA vertex shader");CheckSmaa(d->CreatePixelShader(p.bytes,p.size,nullptr,&ps[i]),"SMAA pixel shader");}
        float metrics[]={1.f/w,1.f/h,float(w),float(h)};D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(metrics);cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;cb.Usage=D3D11_USAGE_IMMUTABLE;D3D11_SUBRESOURCE_DATA data{};data.pSysMem=metrics;CheckSmaa(d->CreateBuffer(&cb,&data,&constants),"SMAA metrics");
        for(unsigned i=0;i<2;++i){D3D11_SAMPLER_DESC sd{};sd.Filter=i?D3D11_FILTER_MIN_MAG_MIP_POINT:D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;CheckSmaa(d->CreateSamplerState(&sd,&samplers[i]),"SMAA sampler");}
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;CheckSmaa(d->CreateRasterizerState(&rs,&raster),"SMAA raster");
        D3D11_DEPTH_STENCIL_DESC ds{};CheckSmaa(d->CreateDepthStencilState(&ds,&depth),"SMAA depth");D3D11_BLEND_DESC bs{};bs.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;CheckSmaa(d->CreateBlendState(&bs,&blend),"SMAA blend");
    }
};
GpuSmaa::GpuSmaa(ID3D11Device* d,unsigned w,unsigned h,bool rgba,int quality):impl(std::make_unique<Impl>(d,w,h,rgba,quality)){}
GpuSmaa::~GpuSmaa()=default;
void GpuSmaa::Process(ID3D11DeviceContext* context,ID3D11RenderTargetView* output,ID3D11Texture2D* input){
    auto& p=*impl;D3D11_TEXTURE2D_DESC desc{};input->GetDesc(&desc);
    auto format=p.rgba?DXGI_FORMAT_R8G8B8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_UNORM;
    if(desc.Width!=p.width||desc.Height!=p.height||desc.Format!=format||desc.ArraySize!=1||desc.SampleDesc.Count!=1)throw std::runtime_error("SMAA input mismatch");
    context->ClearState();context->CopyResource(p.color.Get(),input);
    float clear[4]{};context->ClearRenderTargetView(p.edgeTarget.Get(),clear);context->ClearRenderTargetView(p.weightTarget.Get(),clear);
    ID3D11RenderTargetView* targets[]={p.edgeTarget.Get(),p.weightTarget.Get(),output};
    auto* constants=p.constants.Get();ID3D11SamplerState* samplers[]={p.samplers[0].Get(),p.samplers[1].Get()};
    for(unsigned pass=0;pass<3;++pass){
        // Unbind previous pass outputs before binding them as inputs.
        context->ClearState();context->OMSetRenderTargets(1,&targets[pass],nullptr);
        context->OMSetDepthStencilState(p.depth.Get(),0);context->OMSetBlendState(p.blend.Get(),nullptr,~0u);context->RSSetState(p.raster.Get());
        D3D11_VIEWPORT viewport{0,0,float(p.width),float(p.height),0,1};context->RSSetViewports(1,&viewport);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(p.vs[pass].Get(),nullptr,0);context->PSSetShader(p.ps[pass].Get(),nullptr,0);context->VSSetConstantBuffers(0,1,&constants);context->PSSetConstantBuffers(0,1,&constants);context->PSSetSamplers(0,2,samplers);
        ID3D11ShaderResourceView* views[]={pass==2?p.linear.Get():p.encoded.Get(),pass==1?p.edgeView.Get():nullptr,pass==1?p.areaView.Get():nullptr,pass==1?p.searchView.Get():nullptr,pass==2?p.weightView.Get():nullptr};
        context->PSSetShaderResources(0,5,views);context->Draw(3,0);
    }
    context->ClearState();
}
}
