#include "GpuTemporalDepth.h"
#include "TemporalDepthFloatCS.h"
#include "TemporalDepthUintCS.h"
#include <stdexcept>
#include <cstdint>
namespace diagnostic::temporal {
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Hr(HRESULT hr,char const* why){Require(SUCCEEDED(hr),why);}
struct State {
    ID3D11DeviceContext* c;ComPtr<ID3D11ComputeShader> shader;ComPtr<ID3D11ShaderResourceView> input;ComPtr<ID3D11UnorderedAccessView> output;
    ID3D11ClassInstance* classes[256]{};UINT count=256;
    explicit State(ID3D11DeviceContext* context):c(context){c->CSGetShader(&shader,classes,&count);c->CSGetShaderResources(0,1,&input);c->CSGetUnorderedAccessViews(0,1,&output);}
    ~State(){ID3D11UnorderedAccessView* empty=nullptr;c->CSSetUnorderedAccessViews(0,1,&empty,nullptr);c->CSSetShader(shader.Get(),classes,count);auto* s=input.Get();c->CSSetShaderResources(0,1,&s);auto* u=output.Get();c->CSSetUnorderedAccessViews(0,1,&u,nullptr);for(unsigned i=0;i<count;++i)if(classes[i])classes[i]->Release();}
};
}
struct GpuTemporalDepth::Impl {
    ComPtr<ID3D11Device> device;unsigned width,height;DXGI_FORMAT format;
    ComPtr<ID3D11ComputeShader> shader;ComPtr<ID3D11Texture2D> output;ComPtr<ID3D11UnorderedAccessView> uav;
    // Bounded two-image cache, matching the two eyes. No repeated SRV creation
    // on the normal GPU path. Native pointer is never retained without a ref.
    struct Input{ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;};Input inputs[2];
    Impl(ID3D11Device* d,unsigned w,unsigned h,g3vr::TemporalEncoding encoding):device(d),width(w),height(h){
        Require(d&&w&&h&&w<=8192&&h<=8192&&uint64_t(w)*h<=33554432,"Temporal decode dimensions/device");Require(d->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_0,"Temporal decode feature level");
        switch(encoding){case g3vr::TemporalEncoding::Depth16Unorm:format=DXGI_FORMAT_R16_UNORM;break;case g3vr::TemporalEncoding::Depth24Uint:format=DXGI_FORMAT_R32_UINT;break;case g3vr::TemporalEncoding::Depth32Float:format=DXGI_FORMAT_R32_FLOAT;break;default:throw std::runtime_error("Not a transported depth encoding");}
        bool packed=encoding==g3vr::TemporalEncoding::Depth24Uint;Hr(d->CreateComputeShader(packed?TemporalDepthUintCS:TemporalDepthFloatCS,packed?sizeof(TemporalDepthUintCS):sizeof(TemporalDepthFloatCS),nullptr,&shader),"Temporal depth shader");
        D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R32_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
        Hr(d->CreateTexture2D(&t,nullptr,&output),"Temporal decoded image");Hr(d->CreateUnorderedAccessView(output.Get(),nullptr,&uav),"Temporal decoded output");
    }
    ID3D11ShaderResourceView* View(ID3D11Texture2D* texture){
        Require(texture&&texture!=output.Get(),"Temporal depth null/output alias");ComPtr<ID3D11Device> owner;texture->GetDevice(&owner);Require(owner.Get()==device.Get(),"Temporal decode device mismatch");
        D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);Require(d.Width==width&&d.Height==height&&d.Format==format&&d.MipLevels==1&&d.ArraySize==1&&d.SampleDesc.Count==1&&(d.BindFlags&D3D11_BIND_SHADER_RESOURCE),"Temporal raw depth description");
        for(auto& input:inputs)if(input.texture.Get()==texture)return input.view.Get();
        for(auto& input:inputs)if(!input.texture){Input next;next.texture=texture;Hr(device->CreateShaderResourceView(texture,nullptr,&next.view),"Temporal raw depth view");input=std::move(next);return input.view.Get();}
        throw std::runtime_error("Temporal decode input cache full; reset on resource change");
    }
};
GpuTemporalDepth::GpuTemporalDepth()=default;GpuTemporalDepth::~GpuTemporalDepth()=default;
void GpuTemporalDepth::Initialize(ID3D11Device* d,unsigned w,unsigned h,g3vr::TemporalEncoding encoding){impl=std::make_unique<Impl>(d,w,h,encoding);}
void GpuTemporalDepth::Reset()noexcept{impl.reset();}
ID3D11Texture2D* GpuTemporalDepth::Texture()const noexcept{return impl?impl->output.Get():nullptr;}
void GpuTemporalDepth::Process(ID3D11DeviceContext* c,ID3D11Texture2D* raw){
    Require(impl&&c&&c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Temporal decode initialization/context");auto& p=*impl;ComPtr<ID3D11Device> owner;c->GetDevice(&owner);Require(owner.Get()==p.device.Get(),"Temporal decode context device mismatch");
    auto* source=p.View(raw);State saved(c);ID3D11UnorderedAccessView* empty=nullptr;c->CSSetUnorderedAccessViews(0,1,&empty,nullptr);c->CSSetShader(p.shader.Get(),nullptr,0);c->CSSetShaderResources(0,1,&source);auto* u=p.uav.Get();c->CSSetUnorderedAccessViews(0,1,&u,nullptr);c->Dispatch((p.width+7)/8,(p.height+7)/8,1);
}
}
