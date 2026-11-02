#include "GpuTemporalFinalize.h"
#include "TemporalFinalizeCS.h"
#include <DirectXPackedVector.h>
#include <wrl/client.h>
#include <array>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <limits>
using Microsoft::WRL::ComPtr;
using namespace diagnostic::temporal;
using namespace DirectX::PackedVector;
void Require(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Hr(HRESULT hr){Require(SUCCEEDED(hr),"Finalize test D3D11 operation");}
template<class F>void Reject(F f){bool denied=false;try{f();}catch(...){denied=true;}Require(denied,"Invalid temporal finalization accepted");}
int main(){try{
    constexpr unsigned w=97,h=65;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));
    using P=std::array<float,4>;float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    std::array<P,14> opaque{{{0,0,0,0},{.5f,-.25f,1,1},{.5f,-.25f,1,0},{},{},{1,2,1,1},{0,0,1,.5f},{},{nan,0,1,1},{},{60000,0,1,1},{},{1,0,0,0},{1,2,1,1}}};
    std::array<P,14> sky{{{},{},{},{2,-1,1,.25f},{-2,3,1,1},{1,2,1,0},{},{0,0,.5f,0},{},{inf,0,1,0},{},{60000,0,1,0},{},{0,0,0,1}}};
    std::array<P,14> expected{{{},{.5f,-.25f,1,1},{0,0,1,0},{2,-1,1,1},{-2,3,1,1},{0,0,1,0},{},{},{},{},{0,0,1,0},{0,0,1,0},{},{}}};
    std::array<unsigned char,14> reactive{{255,0,255,64,255,255,255,255,255,255,255,255,255,255}};
    ComPtr<ID3D11Texture2D> layers[2];ComPtr<ID3D11ShaderResourceView> views[2];ComPtr<ID3D11UnorderedAccessView> sourceUav;
    for(unsigned layer=0;layer<2;++layer){std::vector<uint16_t> bytes(size_t(w)*h*4);auto const& pattern=layer?sky:opaque;for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)for(unsigned c=0;c<4;++c)bytes[(size_t(y)*w+x)*4+c]=XMConvertFloatToHalf(pattern[x%14][c]);D3D11_TEXTURE2D_DESC t{};t.Width=w;t.Height=h;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;D3D11_SUBRESOURCE_DATA upload{bytes.data(),w*8,0};Hr(device->CreateTexture2D(&t,&upload,&layers[layer]));Hr(device->CreateShaderResourceView(layers[layer].Get(),nullptr,&views[layer]));}
    Hr(device->CreateUnorderedAccessView(layers[1].Get(),nullptr,&sourceUav));ComPtr<ID3D11ComputeShader> shader;Hr(device->CreateComputeShader(TemporalFinalizeCS,sizeof(TemporalFinalizeCS),nullptr,&shader));context->CSSetShader(shader.Get(),nullptr,0);auto* seed=views[0].Get();context->CSSetShaderResources(0,1,&seed);context->PSSetShaderResources(7,1,&seed);auto* source=sourceUav.Get();context->CSSetUnorderedAccessViews(0,1,&source,nullptr);D3D11_VIEWPORT viewport{3,7,45,31,.1f,.9f};context->RSSetViewports(1,&viewport);
    GpuTemporalFinalize finalizer;finalizer.Initialize(device.Get(),w,h);unsigned pixelsChecked=0;
    for(unsigned mode=0;mode<3;++mode){finalizer.Process(context.Get(),layers[0].Get(),mode==2?nullptr:layers[1].Get(),mode==0);
        ComPtr<ID3D11ComputeShader> restored;context->CSGetShader(&restored,nullptr,nullptr);Require(restored==shader,"Finalize did not restore caller CS");ComPtr<ID3D11ShaderResourceView> restoredView;context->CSGetShaderResources(0,1,&restoredView);Require(restoredView==views[0],"Finalize did not restore caller SRV");ComPtr<ID3D11UnorderedAccessView> restoredUav;context->CSGetUnorderedAccessViews(0,1,&restoredUav);Require(restoredUav==sourceUav,"Finalize did not restore input alias UAV");UINT count=1;D3D11_VIEWPORT vp{};context->RSGetViewports(&count,&vp);Require(count==1&&memcmp(&vp,&viewport,sizeof(vp))==0,"Finalize changed graphics viewport");
        for(unsigned output=0;output<2;++output){auto* image=output?finalizer.Reactive():finalizer.Motion();D3D11_TEXTURE2D_DESC t{};image->GetDesc(&t);t.Usage=D3D11_USAGE_STAGING;t.BindFlags=t.MiscFlags=0;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> readback;Hr(device->CreateTexture2D(&t,nullptr,&readback));context->CopyResource(readback.Get(),image);D3D11_MAPPED_SUBRESOURCE map{};Hr(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&map));
            for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){unsigned type=x%14;auto want=expected[type];unsigned char weight=reactive[type];
                if(mode==1&&(type==3||type==4||type==11)){want={0,0,1,0};weight=255;}
                if(mode==2){want={};weight=255;if(type==1){want=opaque[type];weight=0;}if(type==2||type==10)want={0,0,1,0};if(type==5||type==13){want=opaque[type];weight=0;}}
                auto* row=static_cast<unsigned char*>(map.pData)+map.RowPitch*y;
                if(output)Require(row[x]==weight,"Finalize reactive mask differs from independent cases");else{auto* p=reinterpret_cast<uint16_t*>(row)+x*4;for(unsigned c=0;c<4;++c)Require(XMConvertHalfToFloat(p[c])==want[c],"Finalize motion/occupancy/history differs from independent cases");}++pixelsChecked;}
            context->Unmap(readback.Get(),0);
        }
    }
    Reject([&]{finalizer.Process(context.Get(),nullptr,nullptr,false);});Reject([&]{finalizer.Process(context.Get(),layers[0].Get(),nullptr,true);});Reject([&]{finalizer.Process(context.Get(),layers[0].Get(),layers[0].Get(),true);});Reject([&]{finalizer.Process(context.Get(),finalizer.Motion(),nullptr,false);});
    GpuTemporalFinalize other;other.Initialize(device.Get(),w,h);Require(other.Motion()!=finalizer.Motion()&&other.Reactive()!=finalizer.Reactive(),"Finalized eyes alias");finalizer.Reset();Require(!finalizer.Motion()&&!finalizer.Reactive(),"Finalize reset retains output");context->ClearState();std::printf("PASS late finalization: %u pixel checks, actual sky/opaque/unknown, reactive vs history, conflicts, malformed data, context restore, separate eyes\n",pixelsChecked);return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL finalize %s\n",e.what());return 1;}}
