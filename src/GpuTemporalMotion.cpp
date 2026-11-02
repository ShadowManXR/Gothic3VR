#include "GpuTemporalMotion.h"
#include "TemporalMotionCS.h"
#include <wrl/client.h>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <limits>
namespace diagnostic::temporal {
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
void Hr(HRESULT hr,char const* message){Require(SUCCEEDED(hr),message);}
struct Constants {float transform[16],pixel[4];uint32_t history,native,geometryJitter,padding;};
static_assert(sizeof(Constants)==96);
struct ComputeState {
    ID3D11DeviceContext* context;
    ComPtr<ID3D11ComputeShader> shader;
    ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11UnorderedAccessView> output;
    ID3D11ShaderResourceView* resources[4]{};
    ID3D11ClassInstance* classes[256]{};UINT classCount=256;
    explicit ComputeState(ID3D11DeviceContext* c):context(c){
        c->CSGetShader(&shader,classes,&classCount);c->CSGetConstantBuffers(0,1,&constants);
        c->CSGetShaderResources(0,4,resources);c->CSGetUnorderedAccessViews(0,1,&output);
    }
    ~ComputeState(){
        ID3D11UnorderedAccessView* empty=nullptr;context->CSSetUnorderedAccessViews(0,1,&empty,nullptr);
        context->CSSetShader(shader.Get(),classes,classCount);
        auto* cb=constants.Get();context->CSSetConstantBuffers(0,1,&cb);
        context->CSSetShaderResources(0,4,resources);
        auto* u=output.Get();context->CSSetUnorderedAccessViews(0,1,&u,nullptr);
        for(auto* r:resources)if(r)r->Release();
        for(unsigned i=0;i<classCount;++i)if(classes[i])classes[i]->Release();
    }
};
}
std::array<float,16> CameraReprojection(Matrix const& now,Matrix const& before){
    double a[4][8]{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){
        Require(std::isfinite(now[r*4+c])&&std::isfinite(before[r*4+c]),"Nonfinite camera matrix");
        a[r][c]=now[r*4+c];a[r][c+4]=(r==c)?1:0;
    }
    for(unsigned c=0;c<4;++c){
        unsigned pivot=c;for(unsigned r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[pivot][c]))pivot=r;
        Require(std::abs(a[pivot][c])>1.e-14,"Singular camera matrix");
        if(pivot!=c)for(unsigned j=0;j<8;++j)std::swap(a[pivot][j],a[c][j]);
        double scale=a[c][c];for(double& value:a[c])value/=scale;
        for(unsigned r=0;r<4;++r)if(r!=c){scale=a[r][c];for(unsigned j=0;j<8;++j)a[r][j]-=scale*a[c][j];}
    }
    std::array<float,16> result{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){
        double value=0;for(unsigned k=0;k<4;++k)value+=a[r][k+4]*before[k*4+c];
        Require(std::isfinite(value)&&std::abs(value)<=std::numeric_limits<float>::max(),"Camera transform overflow");
        result[r*4+c]=float(value);
    }
    // Reject near-singular inversions whose reconstructed identity is unstable.
    double residual=0;
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){double v=0;for(unsigned k=0;k<4;++k)v+=now[r*4+k]*a[k][c+4];residual=std::max(residual,std::abs(v-(r==c?1.:0.)));}
    Require(residual<1.e-5,"Unstable camera inverse");return result;
}
struct GpuTemporalMotion::Impl {
    ComPtr<ID3D11Device> device;unsigned width,height;
    ComPtr<ID3D11ComputeShader> shader;ComPtr<ID3D11Buffer> constants;
    ComPtr<ID3D11Texture2D> output;ComPtr<ID3D11UnorderedAccessView> uav;
    Impl(ID3D11Device* d,unsigned w,unsigned h):device(d),width(w),height(h){
        Require(d&&w&&h&&w<=8192&&h<=8192&&uint64_t(w)*h<=33554432,"Invalid temporal dimensions/device");
        Require(d->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_0,"Temporal compute needs feature level 11");
        Hr(d->CreateComputeShader(TemporalMotionCS,sizeof(TemporalMotionCS),nullptr,&shader),"Temporal shader");
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(Constants);cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        Hr(d->CreateBuffer(&cb,nullptr,&constants),"Temporal constants");
        D3D11_TEXTURE2D_DESC image{};image.Width=w;image.Height=h;image.MipLevels=image.ArraySize=image.SampleDesc.Count=1;
        image.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;image.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
        Hr(d->CreateTexture2D(&image,nullptr,&output),"Temporal image");
        Hr(d->CreateUnorderedAccessView(output.Get(),nullptr,&uav),"Temporal output view");
    }
    void Validate(ID3D11ShaderResourceView* view,DXGI_FORMAT format){
        Require(view!=nullptr,"Missing temporal input");D3D11_SHADER_RESOURCE_VIEW_DESC v{};view->GetDesc(&v);
        Require(v.Format==format&&v.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&v.Texture2D.MostDetailedMip==0&&v.Texture2D.MipLevels==1,"Temporal input view/format");
        ComPtr<ID3D11Resource> resource;view->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;
        Hr(resource.As(&texture),"Temporal input is not 2D");Require(texture.Get()!=output.Get(),"Temporal output aliases input");
        ComPtr<ID3D11Device> owner;texture->GetDevice(&owner);Require(owner.Get()==device.Get(),"Temporal input device mismatch");
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
        Require(desc.Width==width&&desc.Height==height&&desc.MipLevels==1&&desc.ArraySize==1&&desc.SampleDesc.Count==1,"Temporal input dimensions");
    }
};
GpuTemporalMotion::GpuTemporalMotion()=default;GpuTemporalMotion::~GpuTemporalMotion()=default;
void GpuTemporalMotion::Initialize(ID3D11Device* d,unsigned w,unsigned h){impl=std::make_unique<Impl>(d,w,h);}
void GpuTemporalMotion::Reset()noexcept{impl.reset();}
ID3D11Texture2D* GpuTemporalMotion::Texture()const noexcept{return impl?impl->output.Get():nullptr;}
void GpuTemporalMotion::Process(ID3D11DeviceContext* context,ID3D11ShaderResourceView* depth,ID3D11ShaderResourceView* skin,ID3D11ShaderResourceView* knownStatic,CameraMotionFrame const& frame){
    ProcessLayers(context,depth,skin,nullptr,knownStatic,frame);
}
void GpuTemporalMotion::ProcessLayers(ID3D11DeviceContext* context,ID3D11ShaderResourceView* depth,ID3D11ShaderResourceView* skin,ID3D11ShaderResourceView* nativeGeometry,ID3D11ShaderResourceView* knownStatic,CameraMotionFrame const& frame){
    Require(impl&&context,"Temporal composer not initialized/context missing");auto& p=*impl;
    Require(context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Temporal needs immediate context");
    ComPtr<ID3D11Device> owner;context->GetDevice(&owner);Require(owner.Get()==p.device.Get(),"Temporal context device mismatch");
    p.Validate(depth,DXGI_FORMAT_R32_FLOAT);p.Validate(skin,DXGI_FORMAT_R16G16B16A16_FLOAT);p.Validate(knownStatic,DXGI_FORMAT_R8_UNORM);
    if(nativeGeometry)p.Validate(nativeGeometry,DXGI_FORMAT_R16G16B16A16_FLOAT);
    Constants cb{};for(unsigned i=0;i<16;++i){Require(std::isfinite(frame.currentClipToPreviousClip[i]),"Nonfinite reprojection");cb.transform[i]=frame.currentClipToPreviousClip[i];}
    cb.pixel[0]=float(p.width);cb.pixel[1]=float(p.height);cb.history=frame.historyValid?1:0;
    cb.native=nativeGeometry?1:0;
    cb.geometryJitter=frame.geometryIncludesRasterJitter?1:0;
    for(unsigned i=0;i<2;++i){Require(std::isfinite(frame.currentRasterJitter[i])&&std::isfinite(frame.previousRasterJitter[i])&&std::abs(frame.currentRasterJitter[i])<=8&&std::abs(frame.previousRasterJitter[i])<=8,"Invalid raster jitter");cb.pixel[i+2]=frame.previousRasterJitter[i]-frame.currentRasterJitter[i];}
    ComputeState saved(context);context->UpdateSubresource(p.constants.Get(),0,nullptr,&cb,0,0);
    context->CSSetShader(p.shader.Get(),nullptr,0);auto* buffer=p.constants.Get();context->CSSetConstantBuffers(0,1,&buffer);
    // Release caller UAV before binding sources, avoiding an implicit hazard
    // unbind which would otherwise lose its original binding on restoration.
    ID3D11UnorderedAccessView* empty=nullptr;context->CSSetUnorderedAccessViews(0,1,&empty,nullptr);
    ID3D11ShaderResourceView* views[]{depth,skin,knownStatic,nativeGeometry};context->CSSetShaderResources(0,4,views);
    auto* output=p.uav.Get();context->CSSetUnorderedAccessViews(0,1,&output,nullptr);
    context->Dispatch((p.width+7)/8,(p.height+7)/8,1);
}
}
