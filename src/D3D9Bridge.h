#pragma once
#include <d3d9.h>
#include "StereoRenderer.h"
#include "FencePolling.h"
#include "DxvkGpuBridge.h"
namespace g3vr
{
// D3D9Ex owns this shared render texture; D3D11 only opens the shared resource.
// No NT handle is created, so there is no CloseHandle on the legacy shared handle.
struct SharedEye
{
    SharedEye()=default;
    SharedEye(SharedEye const&)=delete;
    SharedEye& operator=(SharedEye const&)=delete;
    ~SharedEye(){Reset();}
    stereo::ComPtr<IDirect3DTexture9> texture9;
    stereo::ComPtr<IDirect3DSurface9> surface9;
    stereo::ComPtr<ID3D11Texture2D> texture11;
    void Reset() noexcept
    {
        if(texture11){using Release=void(WINAPI*)(ID3D11Texture2D*);auto release=reinterpret_cast<Release>(GetProcAddress(GetModuleHandleW(L"d3d9.dll"),"G3VR_ReleaseWrapperEye"));if(release)release(texture11.Get());}
        texture11.Reset();surface9.Reset();texture9.Reset();
    }
    void Create(IDirect3DDevice9Ex* device9,ID3D11Device* device11,UINT width,UINT height,DxvkGpuBridge* dxvk=nullptr)
    {
        if(dxvk&&dxvk->Active()){dxvk->Create(device9,device11,width,height,texture9,surface9,texture11);return;}
        using WrapperEye=HRESULT(WINAPI*)(IDirect3DDevice9Ex*,ID3D11Device*,UINT,UINT,IDirect3DTexture9**,IDirect3DSurface9**,ID3D11Texture2D**);
        auto wrapper=reinterpret_cast<WrapperEye>(GetProcAddress(GetModuleHandleW(L"d3d9.dll"),"G3VR_CreateWrapperSharedEye"));
        if(wrapper){stereo::Hr(wrapper(device9,device11,width,height,&texture9,&surface9,&texture11),"full D3D11 wrapper shared eye");return;}
        HANDLE shared=nullptr;
        stereo::Hr(device9->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture9,&shared),"create shared D3D9Ex texture");
        if(!shared)throw std::runtime_error("No D3D9Ex shared handle");
        stereo::Hr(texture9->GetSurfaceLevel(0,&surface9),"shared D3D9 surface");
        stereo::Hr(device11->OpenSharedResource(shared,IID_PPV_ARGS(&texture11)),"open D3D9Ex texture in D3D11");
        D3D11_TEXTURE2D_DESC desc{};texture11->GetDesc(&desc);
        if(desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM||desc.Width!=width||desc.Height!=height||desc.SampleDesc.Count!=1)
            throw std::runtime_error("Unexpected shared texture format/dimensions");
    }
};
inline FencePollStats Wait9(IDirect3DQuery9* query)
{
    stereo::Hr(query->Issue(D3DISSUE_END),"D3D9 GPU event");auto start=GetTickCount64();
    FencePollSchedule schedule;FencePollStats stats;
    // Submit once, then poll without repeatedly forcing the driver's command
    // buffer. A bounded 2 ms re-flush preserves progress on older drivers.
    for(;;){auto now=GetTickCount64();bool flush=schedule.Flush(now);++stats.polls;if(flush)++stats.flushes;
        HRESULT hr=query->GetData(nullptr,0,flush?D3DGETDATA_FLUSH:0);if(hr==S_OK)return stats;stereo::Hr(hr,"D3D9 GPU event wait");
        if(now-start>2000)throw std::runtime_error("D3D9 GPU event timeout");SwitchToThread();}
}
inline FencePollStats Wait11(ID3D11DeviceContext* context,ID3D11Query* query)
{
    context->End(query);context->Flush();auto start=GetTickCount64();
    FencePollStats stats;stats.flushes=1;
    for(;;){++stats.polls;BOOL complete=FALSE;HRESULT hr=context->GetData(query,&complete,sizeof(complete),D3D11_ASYNC_GETDATA_DONOTFLUSH);stereo::Hr(hr,"D3D11 GPU event wait");if(hr==S_OK&&complete)return stats;
        if(GetTickCount64()-start>2000)throw std::runtime_error("D3D11 GPU event timeout");SwitchToThread();}
}
}
