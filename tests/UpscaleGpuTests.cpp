#include "GpuUpscaler.h"
#include <wrl/client.h>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstdint>
#include <d3d11sdklayers.h>
using Microsoft::WRL::ComPtr;using namespace diagnostic;
void Require(bool b,char const* s){if(!b)throw std::runtime_error(s);}
void Hr(HRESULT h){Require(SUCCEEDED(h),"D3D11 test operation failed");}
#include "FxaaBenchmark.h"
int main(int argc,char** argv){try{
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_DEBUG,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)))
        Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));
    ComPtr<ID3D11InfoQueue> debug;device.As(&debug);if(debug)debug->ClearStoredMessages();
    if(argc==2&&std::string(argv[1])=="--benchmark"){FxaaBenchmark(device.Get(),context.Get());return 0;}
    for(unsigned aa:{0u,1u,2u,3u,4u})for(auto method:{UpscaleMethod::Off,UpscaleMethod::NIS,UpscaleMethod::CAS})for(int scale:{100,75,50})for(int copy:{0,1}){
        // Deliberately odd output dimensions exercise shader edge stores.
        auto p=MakeUpscalePlan(257,145,method,scale,25,true,aa==1,aa>=2,aa>=2?int(aa)-2:0);GpuUpscaler up;up.Initialize(device.Get(),p);
        D3D11_TEXTURE2D_DESC d{};d.Width=p.inputWidth;d.Height=p.inputHeight;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.BindFlags=copy?D3D11_BIND_RENDER_TARGET:D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> sources[2];for(auto& source:sources)Hr(device->CreateTexture2D(&d,nullptr,&source));
        d.Width=p.outputWidth;d.Height=p.outputHeight;d.Format=copy?DXGI_FORMAT_B8G8R8A8_TYPELESS:DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;d.BindFlags=D3D11_BIND_RENDER_TARGET;ComPtr<ID3D11Texture2D> target;Hr(device->CreateTexture2D(&d,nullptr,&target));
        d.Format=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;Hr(device->CreateTexture2D(&d,nullptr,&staging));
        for(unsigned step=0;step<8;++step){unsigned patterns[]={4,0,1,2,3,4,5,0},pattern=patterns[step];auto* source=sources[step%2].Get();std::vector<uint32_t> pixels(p.inputWidth*p.inputHeight);
            for(unsigned y=0;y<p.inputHeight;++y)for(unsigned x=0;x<p.inputWidth;++x){unsigned r=pattern==0?23:pattern==1?197:x*255/(p.inputWidth-1),g=pattern<2?73:y*255/(p.inputHeight-1),b=pattern<2?151:pattern==2?51:((x/8+y/8)%2)*255;if(pattern==4||pattern==5){bool bright=y<float(x)*.53f+9;r=g=b=pattern==5?(bright?112:96):(bright?230:16);}unsigned alpha=pattern==3&&aa>=2&&method==UpscaleMethod::Off?((x+y)%256):255;pixels[y*p.inputWidth+x]=(alpha<<24)|(r<<16)|(g<<8)|b;}
            context->UpdateSubresource(source,0,nullptr,pixels.data(),p.inputWidth*4,0);up.Process(context.Get(),target.Get(),source);context->CopyResource(staging.Get(),target.Get());
            D3D11_MAPPED_SUBRESOURCE m{};Hr(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m));
            unsigned softened=0;
            for(unsigned y=0;y<p.outputHeight;++y)for(unsigned x=0;x<p.outputWidth;++x){auto got=reinterpret_cast<uint32_t*>(static_cast<char*>(m.pData)+y*m.RowPitch)[x];
                unsigned alpha=pattern==3&&aa>=2&&method==UpscaleMethod::Off?((x+y)%256):255;Require((got>>24)==alpha,"Unwritten output edge or invalid alpha");
                if(method==UpscaleMethod::Off&&!aa)Require(got==pixels[y*p.inputWidth+x],"Off path not exact");
                if(pattern==4&&(got&255)>18&&(got&255)<228)++softened;
                if(pattern==5&&(got&255)>96&&(got&255)<112)++softened;
                if(pattern<2){auto expected=pixels[0];for(unsigned shift:{0,8,16})Require(abs(int((got>>shift)&255)-int((expected>>shift)&255))<=1,"Channel swap, gamma error or stale image");}
                else if(pattern==2&&x>12&&y>12&&x+12<p.outputWidth&&y+12<p.outputHeight){unsigned r=(got>>16)&255,g=(got>>8)&255,b=got&255;Require(abs(int(r)-int(x*255/(p.outputWidth-1)))<6&&abs(int(g)-int(y*255/(p.outputHeight-1)))<6&&abs(int(b)-51)<2,"Gradient orientation or scaling incorrect");}
            }context->Unmap(staging.Get(),0);
            if(pattern==4&&aa&&method==UpscaleMethod::Off)Require(softened>30,"Spatial AA did not smooth the diagonal edge");
            if(pattern==5&&aa>=2&&method==UpscaleMethod::Off){if(aa==4)Require(softened>20,"Ultra did not detect its lower-contrast edge");else Require(softened==0,"Medium/High threshold changed");}
        }
    }
    context->ClearState();context->Flush();
    if(debug)for(UINT64 i=0;i<debug->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T size=0;debug->GetMessage(i,nullptr,&size);std::vector<unsigned char> data(size);auto* message=reinterpret_cast<D3D11_MESSAGE*>(data.data());Hr(debug->GetMessage(i,message,&size));if(message->Severity<=D3D11_MESSAGE_SEVERITY_WARNING){std::fprintf(stderr,"GPU debug: %s\n",message->pDescription);Require(false,"D3D11 validation warning/error");}}
    std::puts("PASS real GPU NIS/CAS/FXAA/SMAA: Off exact,100/75/50%,odd edges,SRV/copy input,color/gamma,changing frames,diagonal AA,gradient/checker");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
