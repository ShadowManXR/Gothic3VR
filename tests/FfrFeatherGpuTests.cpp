#include "GpuFfrFeather.h"
#include "GpuUpscaler.h"
#include <wrl/client.h>
#include <d3d11sdklayers.h>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <string>
using namespace diagnostic;using Microsoft::WRL::ComPtr;
void Need(bool ok,char const* text){if(!ok)throw std::runtime_error(text);}
void Hr(HRESULT r){Need(SUCCEEDED(r),"D3D11 operation failed");}
ComPtr<ID3D11Texture2D> Texture(ID3D11Device* device,unsigned w,unsigned h,DXGI_FORMAT format,UINT bind){
    D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=format;d.BindFlags=bind;ComPtr<ID3D11Texture2D> t;Hr(device->CreateTexture2D(&d,nullptr,&t));return t;
}
std::vector<uint32_t> Read(ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* image){
    D3D11_TEXTURE2D_DESC d{};image->GetDesc(&d);d.Format=DXGI_FORMAT_B8G8R8A8_UNORM;d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> stage;Hr(device->CreateTexture2D(&d,nullptr,&stage));context->CopyResource(stage.Get(),image);D3D11_MAPPED_SUBRESOURCE m{};Hr(context->Map(stage.Get(),0,D3D11_MAP_READ,0,&m));
    std::vector<uint32_t> out(d.Width*d.Height);for(unsigned y=0;y<d.Height;++y)memcpy(out.data()+y*d.Width,static_cast<char*>(m.pData)+y*m.RowPitch,d.Width*4);context->Unmap(stage.Get(),0);return out;
}
bool Same(uint32_t a,uint32_t b){for(unsigned s:{0u,8u,16u})if(abs(int((a>>s)&255)-int((b>>s)&255))>1)return false;return (a>>24)==(b>>24);}
template<class Run>double Time(ID3D11Device* device,ID3D11DeviceContext* context,Run run){
    D3D11_QUERY_DESC q{D3D11_QUERY_TIMESTAMP_DISJOINT,0};ComPtr<ID3D11Query> disjoint,start,end;Hr(device->CreateQuery(&q,&disjoint));q.Query=D3D11_QUERY_TIMESTAMP;Hr(device->CreateQuery(&q,&start));Hr(device->CreateQuery(&q,&end));
    context->Begin(disjoint.Get());context->End(start.Get());for(unsigned i=0;i<16;++i)run();context->End(end.Get());context->End(disjoint.Get());context->Flush();
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT info{};uint64_t a=0,b=0;auto until=GetTickCount64()+10000;
    while(context->GetData(disjoint.Get(),&info,sizeof(info),0)==S_FALSE){Need(GetTickCount64()<until,"GPU timing timeout");Sleep(1);}Hr(context->GetData(start.Get(),&a,sizeof(a),0));Hr(context->GetData(end.Get(),&b,sizeof(b),0));Need(!info.Disjoint&&info.Frequency&&b>a,"Invalid GPU timing");return double(b-a)*1000/double(info.Frequency)/16;
}
int main(){try{
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_DEBUG,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context)))Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context));
    ComPtr<ID3D11InfoQueue> debug;device.As(&debug);if(debug)debug->ClearStoredMessages();
    for(unsigned aa=0;aa<=4;++aa)for(auto method:{UpscaleMethod::Off,UpscaleMethod::NIS,UpscaleMethod::CAS}){
        auto plan=MakeUpscalePlan(513,385,method,75,20,true,aa==1,aa>=2,aa>=2?int(aa)-2:0);GpuUpscaler up;up.Initialize(device.Get(),plan);GpuFfrFeather feather;auto* scene=feather.Target(device.Get(),plan.outputWidth,plan.outputHeight);
        auto input=Texture(device.Get(),plan.inputWidth,plan.inputHeight,DXGI_FORMAT_B8G8R8A8_UNORM,D3D11_BIND_SHADER_RESOURCE);
        ComPtr<ID3D11Texture2D> output[2];for(auto& out:output)out=Texture(device.Get(),plan.outputWidth,plan.outputHeight,DXGI_FORMAT_B8G8R8A8_TYPELESS,D3D11_BIND_RENDER_TARGET);
        for(unsigned frame=0;frame<2;++frame){std::vector<uint32_t> pixels(plan.inputWidth*plan.inputHeight);for(unsigned y=0;y<plan.inputHeight;++y)for(unsigned x=0;x<plan.inputWidth;++x){bool high=(x/4+y/4+frame)%2!=0;pixels[y*plan.inputWidth+x]=high?0xFFDCB482:0xFF172349;}
            context->UpdateSubresource(input.Get(),0,nullptr,pixels.data(),plan.inputWidth*4,0);up.Process(context.Get(),scene,input.Get());auto original=Read(device.Get(),context.Get(),scene);
            auto p=MakeFfrFeather(plan.outputWidth,plan.outputHeight,2,60,20);feather.Present(context.Get(),output[0].Get(),p);auto blurred=Read(device.Get(),context.Get(),output[0].Get());
            unsigned changed=0,protectedPixels=0;for(unsigned y=0;y<plan.outputHeight;++y)for(unsigned x=0;x<plan.outputWidth;++x){auto i=y*plan.outputWidth+x;auto amount=FfrFeatherAmount(p,(x+.5f)/plan.outputWidth,(y+.5f)/plan.outputHeight);
                if(amount==0){++protectedPixels;Need(Same(original[i],blurred[i]),"Center color or AA changed during feather");}if(amount>.95f)changed+=!Same(original[i],blurred[i]);Need((original[i]>>24)==(blurred[i]>>24),"Feather changed alpha");}
            Need(protectedPixels>20000&&changed>5000,"Feather did not blur peripheral pixels");
            feather.Present(context.Get(),output[1].Get(),p);Need(Read(device.Get(),context.Get(),output[1].Get())==blurred,"Repeated eye result is unstable");
            auto off=MakeFfrFeather(plan.outputWidth,plan.outputHeight,2,60,0);feather.Present(context.Get(),output[0].Get(),off);auto bypass=Read(device.Get(),context.Get(),output[0].Get());for(unsigned i=0;i<bypass.size();++i)Need(Same(bypass[i],original[i]),"Zero width failed to restore scene");
        }
    }
    // Measure GPU cost outside the game; no CPU pixel readback in this loop.
    {unsigned w=3840,h=2160;GpuFfrFeather feather;auto* scene=feather.Target(device.Get(),w,h);auto source=Texture(device.Get(),w,h,DXGI_FORMAT_B8G8R8A8_UNORM,D3D11_BIND_SHADER_RESOURCE);auto out=Texture(device.Get(),w,h,DXGI_FORMAT_B8G8R8A8_TYPELESS,D3D11_BIND_RENDER_TARGET);
        auto p=MakeFfrFeather(w,h,2,60,10);context->CopyResource(scene,source.Get());feather.Present(context.Get(),out.Get(),p);
        double direct=Time(device.Get(),context.Get(),[&]{context->CopyResource(out.Get(),source.Get());});double filtered=Time(device.Get(),context.Get(),[&]{context->CopyResource(scene,source.Get());feather.Present(context.Get(),out.Get(),p);});
        std::printf("4K per-eye GPU transfer %.3f ms; transfer plus feather %.3f ms; extra %.3f ms\n",direct,filtered,filtered-direct);
    }
    context->ClearState();context->Flush();if(debug)for(UINT64 i=0;i<debug->GetNumStoredMessagesAllowedByRetrievalFilter();++i){SIZE_T size=0;debug->GetMessage(i,nullptr,&size);std::vector<unsigned char> bytes(size);auto* message=reinterpret_cast<D3D11_MESSAGE*>(bytes.data());Hr(debug->GetMessage(i,message,&size));if(message->Severity<=D3D11_MESSAGE_SEVERITY_WARNING){std::fprintf(stderr,"GPU debug: %s\n",message->pDescription);Need(false,"D3D11 validation warning/error");}}
    std::puts("PASS pixel feather after Off/NIS/CAS and all AA profiles: stable eyes, inner center, alpha, live zero width, changing frames");return 0;
}catch(std::exception const& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
