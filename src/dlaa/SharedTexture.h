#pragma once
#include "Gpu.h"
namespace dlaa {
struct SharedTexture {
    ComPtr<ID3D11Texture2D> texture;ComPtr<IDXGIKeyedMutex> mutex;Handle shared;
    void Create(Gpu& gpu,unsigned w,unsigned h,DXGI_FORMAT format){
        texture=gpu.Texture(w,h,format,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS,D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX);
        Hr(texture.As(&mutex),"Export texture keyed mutex");ComPtr<IDXGIResource1> resource;Hr(texture.As(&resource),"Export NT resource");
        Hr(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&shared.value),"Export texture NT handle");
    }
    void Open(Gpu& gpu,uint64_t remote,unsigned w,unsigned h,DXGI_FORMAT expected){
        Require(remote<=UINTPTR_MAX&&remote!=0,"Truncated/invalid texture handle");Handle owned(reinterpret_cast<HANDLE>(uintptr_t(remote)));ComPtr<ID3D11Device1> device;Hr(gpu.device.As(&device),"D3D11.1 for NT import");
        Hr(device->OpenSharedResource1(owned.value,IID_PPV_ARGS(&texture)),"Open shared GPU texture in x64 worker");D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);
        Require(d.Width==w&&d.Height==h&&d.Format==expected&&d.ArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&
            (d.BindFlags&D3D11_BIND_SHADER_RESOURCE)&&(d.BindFlags&D3D11_BIND_UNORDERED_ACCESS),"Shared texture contract mismatch");Hr(texture.As(&mutex),"Import keyed mutex");
    }
    void Acquire(uint64_t key){auto hr=mutex->AcquireSync(key,5000);if(hr!=S_OK)throw std::runtime_error("Texture mutex acquisition failed: "+Hex(uint32_t(hr)));}
    void Release(uint64_t key){Hr(mutex->ReleaseSync(key),"Texture mutex release");}
};
struct TextureLease {
    SharedTexture& texture;uint64_t release;bool held=false;
    TextureLease(SharedTexture& t,uint64_t acquire,uint64_t done):texture(t),release(done){texture.Acquire(acquire);held=true;}
    ~TextureLease(){if(held)texture.mutex->ReleaseSync(release);}
    TextureLease(TextureLease const&)=delete;
};
struct GpuTimer {
    ComPtr<ID3D11Query> disjoint,start,end;
    explicit GpuTimer(Gpu& gpu){D3D11_QUERY_DESC d{D3D11_QUERY_TIMESTAMP_DISJOINT,0};Hr(gpu.device->CreateQuery(&d,&disjoint),"Timestamp disjoint");d.Query=D3D11_QUERY_TIMESTAMP;Hr(gpu.device->CreateQuery(&d,&start),"Timestamp start");Hr(gpu.device->CreateQuery(&d,&end),"Timestamp end");}
    void Begin(Gpu& gpu){gpu.context->Begin(disjoint.Get());gpu.context->End(start.Get());}
    void End(Gpu& gpu){gpu.context->End(end.Get());gpu.context->End(disjoint.Get());}
    double Read(Gpu& gpu){D3D11_QUERY_DATA_TIMESTAMP_DISJOINT d{};UINT64 a=0,b=0;
        Require(gpu.context->GetData(disjoint.Get(),&d,sizeof(d),0)==S_OK&&!d.Disjoint&&d.Frequency,"Invalid/disjoint GPU timestamps");
        Require(gpu.context->GetData(start.Get(),&a,sizeof(a),0)==S_OK&&gpu.context->GetData(end.Get(),&b,sizeof(b),0)==S_OK&&b>=a,"GPU timestamps unavailable");return double(b-a)*1000/double(d.Frequency);}
};
}
