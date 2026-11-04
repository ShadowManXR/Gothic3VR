#pragma once
#include "RadialItems.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <cstring>
namespace diagnostic {
// Some drivers cannot let D3DX lock compressed DEFAULT-pool textures. Sample
// those through D3D9 into a private 64px target, then copy that target to RAM.
// This runs only for uncached icons; all native device state is restored.
inline bool RenderRadialTexture(IDirect3DDevice9* device,IDirect3DTexture9* texture,RECT const& source,IDirect3DSurface9* output){
    using Microsoft::WRL::ComPtr;
    ComPtr<IDirect3DStateBlock9> state;ComPtr<IDirect3DSurface9> target,depth,temporary;
    D3DCAPS9 caps{};D3DSURFACE_DESC desc{};D3DVIEWPORT9 viewport{};
    if(FAILED(device->GetDeviceCaps(&caps))||FAILED(texture->GetLevelDesc(0,&desc))||
       FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))||FAILED(state->Capture())||
       FAILED(device->GetRenderTarget(0,&target))||FAILED(device->GetViewport(&viewport)))return false;
    HRESULT depthResult=device->GetDepthStencilSurface(&depth);
    if(FAILED(depthResult)&&depthResult!=D3DERR_NOTFOUND)return false;
    ComPtr<IDirect3DSurface9> extra[3];
    unsigned targets=(caps.NumSimultaneousRTs<4?caps.NumSimultaneousRTs:4);
    for(unsigned i=1;i<targets;++i){auto hr=device->GetRenderTarget(i,&extra[i-1]);if(FAILED(hr)&&hr!=D3DERR_NOTFOUND)return false;}
    if(FAILED(device->CreateRenderTarget(RadialIconPixels,RadialIconPixels,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&temporary,nullptr)))return false;
    bool ownScene=SUCCEEDED(device->BeginScene());
    if(!ownScene&&FAILED(device->TestCooperativeLevel()))return false;
    struct Restore {
        IDirect3DDevice9* device;IDirect3DStateBlock9* state;IDirect3DSurface9* target;IDirect3DSurface9* depth;
        ComPtr<IDirect3DSurface9>* extra;unsigned targets;D3DVIEWPORT9 viewport;bool ownScene;
        ~Restore(){device->SetRenderTarget(0,target);for(unsigned i=1;i<targets;++i)device->SetRenderTarget(i,extra[i-1].Get());
            device->SetDepthStencilSurface(depth);state->Apply();device->SetViewport(&viewport);if(ownScene)device->EndScene();}
    } restore{device,state.Get(),target.Get(),depth.Get(),extra,targets,viewport,ownScene};
    bool ok=true;auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
    set(device->SetDepthStencilSurface(nullptr));for(unsigned i=1;i<targets;++i)set(device->SetRenderTarget(i,nullptr));
    set(device->SetRenderTarget(0,temporary.Get()));D3DVIEWPORT9 view{0,0,RadialIconPixels,RadialIconPixels,0,1};set(device->SetViewport(&view));
    set(device->SetVertexShader(nullptr));set(device->SetPixelShader(nullptr));set(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));set(device->SetStreamSourceFreq(0,1));
    for(auto type:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,
                  D3DRS_SCISSORTESTENABLE,D3DRS_FOGENABLE,D3DRS_LIGHTING,D3DRS_SRGBWRITEENABLE})set(device->SetRenderState(type,FALSE));
    set(device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));set(device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
    set(device->SetRenderState(D3DRS_COLORWRITEENABLE,15));set(device->SetRenderState(D3DRS_CLIPPLANEENABLE,0));
    set(device->SetTexture(0,texture));set(device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1));
    set(device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));set(device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1));
    set(device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE));set(device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0));
    set(device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE));set(device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
    set(device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR));set(device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));
    set(device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE));set(device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));
    set(device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));set(device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE));
    struct Vertex {float x,y,z,w,u,v;};float right=float(RadialIconPixels)-.5f;
    float u0=float(source.left)/desc.Width,v0=float(source.top)/desc.Height,u1=float(source.right)/desc.Width,v1=float(source.bottom)/desc.Height;
    Vertex vertices[]={{-.5f,-.5f,0,1,u0,v0},{right,-.5f,0,1,u1,v0},{-.5f,right,0,1,u0,v1},{right,right,0,1,u1,v1}};
    if(!ok||FAILED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex))))return false;
    return SUCCEEDED(device->GetRenderTargetData(temporary.Get(),output));
}
inline std::shared_ptr<RadialIcon const> CopyRadialTexture(IDirect3DTexture9* texture,RECT const& source){
    using Convert=HRESULT(WINAPI*)(IDirect3DSurface9*,PALETTEENTRY const*,RECT const*,IDirect3DSurface9*,PALETTEENTRY const*,RECT const*,DWORD,D3DCOLOR);
    auto convert=reinterpret_cast<Convert>(GetProcAddress(GetModuleHandleW(L"d3dx9_40.dll"),"D3DXLoadSurfaceFromSurface"));
    if(!texture||!convert)return {};
    D3DSURFACE_DESC desc{};
    if(FAILED(texture->GetLevelDesc(0,&desc))||source.left<0||source.top<0||source.right<=source.left||source.bottom<=source.top||
       unsigned(source.right)>desc.Width||unsigned(source.bottom)>desc.Height)return {};
    Microsoft::WRL::ComPtr<IDirect3DDevice9> device;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> from,to;
    if(FAILED(texture->GetDevice(&device))||FAILED(texture->GetSurfaceLevel(0,&from))||
       FAILED(device->CreateOffscreenPlainSurface(RadialIconPixels,RadialIconPixels,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&to,nullptr)))return {};
    if(FAILED(convert(to.Get(),nullptr,nullptr,from.Get(),nullptr,&source,3,0))&&
       !RenderRadialTexture(device.Get(),texture,source,to.Get()))return {};
    auto result=std::make_shared<RadialIcon>();D3DLOCKED_RECT locked{};
    if(FAILED(to->LockRect(&locked,nullptr,D3DLOCK_READONLY)))return {};
    for(unsigned y=0;y<RadialIconPixels;++y)
        memcpy(result->data()+y*RadialIconPixels,static_cast<unsigned char*>(locked.pBits)+y*locked.Pitch,RadialIconPixels*4);
    to->UnlockRect();return result;
}
}
