#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <stdexcept>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "StereoMath.h"
namespace stereo
{
using Microsoft::WRL::ComPtr;
inline void Hr(HRESULT hr,char const* operation)
{
    if(FAILED(hr)){char code[32];sprintf_s(code," 0x%08lx",static_cast<unsigned long>(hr));throw std::runtime_error(std::string(operation)+code);}
}
class Renderer
{
    struct Vertex { float x,y,z,r,g,b; };
    ComPtr<ID3D11VertexShader> vs,mirrorVs;
    ComPtr<ID3D11PixelShader> ps,mirrorPs;
    ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> vertices,constants;
    ComPtr<ID3D11RasterizerState> raster;
    ComPtr<ID3D11DepthStencilState> depthState;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<IDXGISwapChain> mirror;
    ComPtr<ID3D11RenderTargetView> mirrorTarget;
    UINT vertexCount=0;
public:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL feature{};
    DXGI_ADAPTER_DESC adapterDescription{};
    DirectX::XMMATRIX anchor=DirectX::XMMatrixIdentity();
    void Initialize(IDXGIAdapter* adapter=nullptr,D3D_FEATURE_LEVEL minimum=D3D_FEATURE_LEVEL_11_0)
    {
        D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0};
        HRESULT result=D3D11CreateDevice(adapter,adapter?D3D_DRIVER_TYPE_UNKNOWN:D3D_DRIVER_TYPE_HARDWARE,nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels,2,D3D11_SDK_VERSION,&device,&feature,&context);
        if(result==E_INVALIDARG)result=D3D11CreateDevice(adapter,adapter?D3D_DRIVER_TYPE_UNKNOWN:D3D_DRIVER_TYPE_HARDWARE,
            nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,levels+1,1,D3D11_SDK_VERSION,&device,&feature,&context);
        Hr(result,"D3D11CreateDevice");if(feature<minimum)throw std::runtime_error("GPU feature level below OpenXR requirement");
        ComPtr<IDXGIDevice> dxgi;Hr(device.As(&dxgi),"DXGI device");ComPtr<IDXGIAdapter> chosen;Hr(dxgi->GetAdapter(&chosen),"GPU adapter");
        Hr(chosen->GetDesc(&adapterDescription),"GPU description");
        char const* shader=R"(
cbuffer Camera:register(b0){row_major float4x4 mvp;}
struct Input{float3 position:POSITION;float3 color:COLOR;};
struct Output{float4 position:SV_POSITION;float3 color:COLOR;};
Output VS(Input v){Output o;o.position=mul(float4(v.position,1),mvp);o.color=v.color;return o;}
float4 PS(Output i):SV_TARGET{return float4(i.color,1);}
)";
        ComPtr<ID3DBlob> vcode,pcode,errors;
        Hr(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"VS","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&vcode,&errors),"compile geometry VS");
        Hr(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"PS","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&pcode,&errors),"compile geometry PS");
        Hr(device->CreateVertexShader(vcode->GetBufferPointer(),vcode->GetBufferSize(),nullptr,&vs),"geometry VS");
        Hr(device->CreatePixelShader(pcode->GetBufferPointer(),pcode->GetBufferSize(),nullptr,&ps),"geometry PS");
        D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
            {"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
        Hr(device->CreateInputLayout(elements,2,vcode->GetBufferPointer(),vcode->GetBufferSize(),&layout),"vertex layout");
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=64;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        Hr(device->CreateBuffer(&cb,nullptr,&constants),"camera constants");
        D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;
        Hr(device->CreateRasterizerState(&rs,&raster),"raster state");
        D3D11_DEPTH_STENCIL_DESC ds{};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS;
        Hr(device->CreateDepthStencilState(&ds,&depthState),"depth state");
        std::vector<Vertex> mesh;
        auto cube=[&](float x,float y,float z,float sx,float sy,float sz,float r,float g,float b)
        {
            constexpr float corners[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
            constexpr unsigned faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
            for(unsigned face=0;face<6;++face)for(unsigned index:faces[face])
            {
                float shade=.55f+.075f*face;auto const& c=corners[index];
                mesh.push_back({x+c[0]*sx*.5f,y+c[1]*sy*.5f,z+c[2]*sz*.5f,r*shade,g*shade,b*shade});
            }
        };
        cube(0,-1.62f,-3,12,.04f,12,.09f,.12f,.15f);
        for(int i=-5;i<=5;++i){cube(float(i),-1.593f,-3,.012f,.01f,12,.3f,.38f,.4f);cube(0,-1.593f,float(i)-3,12,.01f,.012f,.3f,.38f,.4f);}
        cube(-.38f,-.15f,-.85f,.25f,.25f,.25f,1,.12f,.08f);
        cube(.42f,-.12f,-1.7f,.4f,.4f,.4f,.08f,1,.22f);
        cube(0,-.15f,-3,.65f,.65f,.65f,.12f,.35f,1);
        cube(-1.2f,-.2f,-4.5f,.9f,.9f,.9f,1,.65f,.08f);
        cube(1.8f,-.1f,-6,1.1f,1.1f,1.1f,.65f,.12f,1);
        // Posts around the viewer make 360-degree head rotation evident.
        for(int i=0;i<12;++i){float angle=i*DirectX::XM_2PI/12;cube(std::sin(angle)*4,-.85f,std::cos(angle)*4,.07f,1.5f,.07f,.2f,.55f,.65f);}
        vertexCount=static_cast<UINT>(mesh.size());D3D11_BUFFER_DESC vb{};vb.ByteWidth=UINT(mesh.size()*sizeof(Vertex));vb.Usage=D3D11_USAGE_IMMUTABLE;vb.BindFlags=D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=mesh.data();Hr(device->CreateBuffer(&vb,&data,&vertices),"scene mesh");
    }
    ComPtr<ID3D11Texture2D> Color(UINT width,UINT height,DXGI_FORMAT format)
    {
        D3D11_TEXTURE2D_DESC d{};d.Width=width;d.Height=height;d.MipLevels=d.ArraySize=1;d.Format=format;d.SampleDesc.Count=1;
        d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> texture;Hr(device->CreateTexture2D(&d,nullptr,&texture),"color texture");return texture;
    }
    ComPtr<ID3D11RenderTargetView> Target(ID3D11Texture2D* texture,DXGI_FORMAT format)
    {
        D3D11_RENDER_TARGET_VIEW_DESC d{};d.Format=format;d.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2D;
        ComPtr<ID3D11RenderTargetView> target;Hr(device->CreateRenderTargetView(texture,&d,&target),"render target");return target;
    }
    ComPtr<ID3D11DepthStencilView> Depth(UINT width,UINT height)
    {
        D3D11_TEXTURE2D_DESC d{};d.Width=width;d.Height=height;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_D32_FLOAT;d.SampleDesc.Count=1;
        d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_DEPTH_STENCIL;ComPtr<ID3D11Texture2D> texture;
        Hr(device->CreateTexture2D(&d,nullptr,&texture),"depth texture");ComPtr<ID3D11DepthStencilView> depth;
        Hr(device->CreateDepthStencilView(texture.Get(),nullptr,&depth),"depth view");return depth;
    }
    void Draw(ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,UINT width,UINT height,XrView const& eye)
    {
        float background[]={.025f,.04f,.065f,1};context->ClearRenderTargetView(target,background);
        context->ClearDepthStencilView(depth,D3D11_CLEAR_DEPTH,1,0);context->OMSetRenderTargets(1,&target,depth);
        context->OMSetDepthStencilState(depthState.Get(),0);context->OMSetBlendState(nullptr,nullptr,~0u);
        D3D11_VIEWPORT viewport{0,0,float(width),float(height),0,1};context->RSSetViewports(1,&viewport);context->RSSetState(raster.Get());
        UINT stride=sizeof(Vertex),offset=0;ID3D11Buffer* vb=vertices.Get();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
        context->IASetInputLayout(layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);
        DirectX::XMFLOAT4X4 matrix;DirectX::XMStoreFloat4x4(&matrix,anchor*View(eye.pose)*Projection(eye.fov));
        context->UpdateSubresource(constants.Get(),0,nullptr,&matrix,0,0);ID3D11Buffer* cb=constants.Get();context->VSSetConstantBuffers(0,1,&cb);
        context->Draw(vertexCount,0);context->OMSetRenderTargets(0,nullptr,nullptr);
    }
    void CreateMirror(HWND window)
    {
        ComPtr<IDXGIDevice> dxgi;Hr(device.As(&dxgi),"mirror DXGI");ComPtr<IDXGIAdapter> adapter;Hr(dxgi->GetAdapter(&adapter),"mirror adapter");
        ComPtr<IDXGIFactory> factory;Hr(adapter->GetParent(IID_PPV_ARGS(&factory)),"mirror factory");
        DXGI_SWAP_CHAIN_DESC d{};d.BufferDesc.Width=1000;d.BufferDesc.Height=550;d.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        d.SampleDesc.Count=1;d.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;d.BufferCount=2;d.OutputWindow=window;d.Windowed=TRUE;d.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
        Hr(factory->CreateSwapChain(device.Get(),&d,&mirror),"mirror swapchain");factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER);
        ComPtr<ID3D11Texture2D> texture;Hr(mirror->GetBuffer(0,IID_PPV_ARGS(&texture)),"mirror backbuffer");mirrorTarget=Target(texture.Get(),d.BufferDesc.Format);
        char const* shader=R"(
Texture2D image:register(t0);SamplerState sampleImage:register(s0);
struct Out{float4 position:SV_POSITION;float2 uv:TEXCOORD;};
Out VS(uint id:SV_VertexID){float2 uv=float2(id&1,id>>1);Out o;o.position=float4(uv.x*2-1,1-uv.y*2,0,1);o.uv=uv;return o;}
float4 PS(Out i):SV_TARGET{float3 c=image.Sample(sampleImage,i.uv).rgb;return float4(pow(max(c,0),1.0/2.2),1);}
)";
        ComPtr<ID3DBlob> code,errors;Hr(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"VS","vs_5_0",0,0,&code,&errors),"mirror VS compile");
        Hr(device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&mirrorVs),"mirror VS");
        Hr(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"PS","ps_5_0",0,0,&code,&errors),"mirror PS compile");
        Hr(device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&mirrorPs),"mirror PS");
        D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
        Hr(device->CreateSamplerState(&sd,&sampler),"mirror sampler");
    }
    void Mirror(ID3D11ShaderResourceView* left,ID3D11ShaderResourceView* right)
    {
        if(!mirror)return;float black[]={0,0,0,1};context->ClearRenderTargetView(mirrorTarget.Get(),black);
        ID3D11RenderTargetView* target=mirrorTarget.Get();context->OMSetRenderTargets(1,&target,nullptr);context->OMSetDepthStencilState(nullptr,0);
        context->IASetInputLayout(nullptr);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->VSSetShader(mirrorVs.Get(),nullptr,0);context->PSSetShader(mirrorPs.Get(),nullptr,0);
        ID3D11SamplerState* s=sampler.Get();context->PSSetSamplers(0,1,&s);
        ID3D11ShaderResourceView* images[]={left,right};
        for(unsigned i=0;i<2;++i){D3D11_VIEWPORT v{float(i*500),0,500,550,0,1};context->RSSetViewports(1,&v);context->PSSetShaderResources(0,1,&images[i]);context->Draw(4,0);}
        ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(0,1,&empty);context->OMSetRenderTargets(0,nullptr,nullptr);
        Hr(mirror->Present(0,0),"mirror Present");
    }
    std::vector<unsigned char> Pixels(ID3D11Texture2D* texture)
    {
        D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;d.MiscFlags=0;
        ComPtr<ID3D11Texture2D> staging;Hr(device->CreateTexture2D(&d,nullptr,&staging),"self-test staging");context->CopyResource(staging.Get(),texture);
        D3D11_MAPPED_SUBRESOURCE map{};Hr(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"self-test pixels");
        std::vector<unsigned char> rgb(size_t(d.Width)*d.Height*3);
        for(UINT y=0;y<d.Height;++y)for(UINT x=0;x<d.Width;++x)for(UINT c=0;c<3;++c)
            rgb[(size_t(y)*d.Width+x)*3+c]=static_cast<unsigned char const*>(map.pData)[y*map.RowPitch+x*4+c];
        context->Unmap(staging.Get(),0);return rgb;
    }
};
}
