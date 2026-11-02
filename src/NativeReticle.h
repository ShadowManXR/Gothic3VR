#pragma once
#include <g3sdk/Script.h>
#include <d3d9.h>
#include <wrl/client.h>
#include "VirtualHook.h"
namespace diagnostic {
// CP 1.75 eCImageAdmin::GetImageInfo fills 24 bytes: a bCString, source
// rectangle and texture index (also consumed by native LoadBitmapA).
// Reuse the installed game's named image/atlas, never ship a copied asset.
struct NativeReticleImageInfo {bCString name;bCRect rect;int texture=-1;};
static_assert(sizeof(NativeReticleImageInfo)==24);
struct ReticleArtwork {
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
    IDirect3DDevice9* device=nullptr;
    float u0=0,v0=0,u1=1,v1=1,aspect=1;
    void Reset(){texture.Reset();device=nullptr;}
};
class NativeReticle {
    int group=-1,image=-1;uint64_t retry=0;
public:
    ReticleArtwork art;
    void ResetFrame(){art.Reset();}
    bool Prepare(){
        art.Reset();auto now=GetTickCount64();
        auto& images=eCImageAdmin::GetInstance();
        if(group<0||image<0){
            if(now<retry)return false;retry=now+1000;
            // The triple 's' is the original game's actual resource name.
            bCString name("G3_HUD_CrosssHair.tga");
            if(!images.FindImageIndex(name,group,image)){group=image=-1;return false;}
        }
        NativeReticleImageInfo info;
        if(!images.GetImageInfo(group,image,reinterpret_cast<eSImage&>(info))){group=image=-1;return false;}
        auto* node=images.GetTexture(info.texture);if(!Readable(node,sizeof(void*)))return false;
        eCGfxImageData* data=nullptr;memcpy(&data,node,sizeof(data));
        if(!Readable(data,sizeof(eCGfxImageData)))return false;
        auto* api=static_cast<eCDX9Texture*>(data->m_pAPITexture);
        if(!Readable(api,sizeof(eCDX9Texture))||!api->m_pTexture||!api->m_pDirect3DDevice)return false;
        D3DSURFACE_DESC desc{};if(FAILED(api->m_pTexture->GetLevelDesc(0,&desc))||!desc.Width||!desc.Height)return false;
        int x0=info.rect.GetLeft(),y0=info.rect.GetTop(),x1=info.rect.GetRight(),y1=info.rect.GetBottom();
        if(x0<0||y0<0||x1<=x0||y1<=y0||unsigned(x1)>desc.Width||unsigned(y1)>desc.Height)return false;
        // This reference lives for this tracked frame only, including both
        // eyes. ResetFrame releases it on every disabled/changed weapon frame.
        art.texture=api->m_pTexture;art.device=api->m_pDirect3DDevice;
        art.u0=(x0+.5f)/desc.Width;art.v0=(y0+.5f)/desc.Height;
        art.u1=(x1-.5f)/desc.Width;art.v1=(y1-.5f)/desc.Height;
        art.aspect=float(x1-x0)/float(y1-y0);return true;
    }
};
}
