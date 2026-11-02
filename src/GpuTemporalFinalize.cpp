#include "GpuTemporalFinalize.h"
#include "TemporalFinalizeCS.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <stdexcept>
#include <cstdint>
namespace diagnostic::temporal {
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Check(HRESULT hr,char const* why){Require(SUCCEEDED(hr),why);}
}
struct GpuTemporalFinalize::Impl {
    ComPtr<ID3D11Device> device;unsigned width,height;
    ComPtr<ID3D11ComputeShader> shader;ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11Texture2D> outputs[2],sources[2];ComPtr<ID3D11UnorderedAccessView> uavs[2];ComPtr<ID3D11ShaderResourceView> views[2];
    ComPtr<ID3DDeviceContextState> state;
    Impl(ID3D11Device* d,unsigned w,unsigned h):device(d),width(w),height(h){
        Require(d&&w&&h&&w<=8192&&h<=8192&&uint64_t(w)*h<=33554432,"Temporal finalize initialization");
        Check(d->CreateComputeShader(TemporalFinalizeCS,sizeof(TemporalFinalizeCS),nullptr,&shader),"Temporal finalize shader");D3D11_BUFFER_DESC b{};b.ByteWidth=16;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Check(d->CreateBuffer(&b,nullptr,&constants),"Temporal finalize constants");
        for(unsigned i=0;i<2;++i){D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=i?DXGI_FORMAT_R8_UNORM:DXGI_FORMAT_R16G16B16A16_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;Check(d->CreateTexture2D(&t,nullptr,&outputs[i]),"Temporal final motion/reactive output");Check(d->CreateUnorderedAccessView(outputs[i].Get(),nullptr,&uavs[i]),"Temporal finalize UAV");}
        ComPtr<ID3D11Device1> d1;Check(device.As(&d1),"Temporal finalize D3D11.1 device");D3D_FEATURE_LEVEL level=D3D_FEATURE_LEVEL_11_0;Check(d1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),nullptr,&state),"Temporal finalize private state");
    }
    void Validate(ID3D11Texture2D* image){
        Require(image&&image!=outputs[0].Get()&&image!=outputs[1].Get(),"Temporal finalize missing/aliased source");ComPtr<ID3D11Device> owner;image->GetDevice(&owner);Require(owner.Get()==device.Get(),"Temporal finalize source device");D3D11_TEXTURE2D_DESC t{};image->GetDesc(&t);
        Require(t.Width==width&&t.Height==height&&t.Format==DXGI_FORMAT_R16G16B16A16_FLOAT&&t.SampleDesc.Count==1&&t.ArraySize==1&&t.MipLevels==1&&(t.BindFlags&D3D11_BIND_SHADER_RESOURCE),"Temporal finalize source format/dimensions");
    }
    void Bind(unsigned slot,ID3D11Texture2D* image){
        if(!image){views[slot].Reset();sources[slot].Reset();return;}
        if(sources[slot].Get()==image)return;ComPtr<ID3D11ShaderResourceView> next;Check(device->CreateShaderResourceView(image,nullptr,&next),"Temporal finalize source view");sources[slot]=image;views[slot]=next;
    }
};
GpuTemporalFinalize::GpuTemporalFinalize()=default;GpuTemporalFinalize::~GpuTemporalFinalize()=default;
void GpuTemporalFinalize::Initialize(ID3D11Device* d,unsigned w,unsigned h){impl=std::make_unique<Impl>(d,w,h);}
void GpuTemporalFinalize::Process(ID3D11DeviceContext* context,ID3D11Texture2D* opaque,ID3D11Texture2D* sky,bool history){
    Require(impl&&context&&context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Temporal finalize context");auto& p=*impl;ComPtr<ID3D11Device> owner;context->GetDevice(&owner);Require(owner.Get()==p.device.Get(),"Temporal finalize context device");p.Validate(opaque);if(sky){p.Validate(sky);Require(sky!=opaque,"Temporal finalize source layer alias");}else Require(!history,"Sky history without sky image");
    p.Bind(0,opaque);p.Bind(1,sky);ComPtr<ID3D11DeviceContext1> c1;Check(context->QueryInterface(IID_PPV_ARGS(&c1)),"Temporal finalize D3D11.1 context");ComPtr<ID3DDeviceContextState> before;c1->SwapDeviceContextState(p.state.Get(),&before);
    struct Restore{ID3D11DeviceContext1* c;ID3DDeviceContextState* old;~Restore(){c->SwapDeviceContextState(old,nullptr);}} restore{c1.Get(),before.Get()};context->ClearState();uint32_t values[]{p.width,p.height,sky?1u:0u,history?1u:0u};context->UpdateSubresource(p.constants.Get(),0,nullptr,values,0,0);
    context->CSSetShader(p.shader.Get(),nullptr,0);auto* cb=p.constants.Get();context->CSSetConstantBuffers(0,1,&cb);ID3D11ShaderResourceView* srvs[]{p.views[0].Get(),p.views[1].Get()};ID3D11UnorderedAccessView* uavs[]{p.uavs[0].Get(),p.uavs[1].Get()};context->CSSetShaderResources(0,2,srvs);context->CSSetUnorderedAccessViews(0,2,uavs,nullptr);context->Dispatch((p.width+7)/8,(p.height+7)/8,1);context->ClearState();
}
ID3D11Texture2D* GpuTemporalFinalize::Motion()const noexcept{return impl?impl->outputs[0].Get():nullptr;}
ID3D11Texture2D* GpuTemporalFinalize::Reactive()const noexcept{return impl?impl->outputs[1].Get():nullptr;}
void GpuTemporalFinalize::Reset()noexcept{impl.reset();}
}
