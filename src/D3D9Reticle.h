#pragma once
#include <d3d9.h>
#include <algorithm>
#include "NativeReticle.h"
namespace diagnostic {
inline float ReticleRadius(UINT height){return std::max(4.f,height*.0035f);}
inline bool DrawReticle(IDirect3DDevice9* device,float u,float v,bool target){
    if(!device)return false;
        D3DVIEWPORT9 viewport{};if(FAILED(device->GetViewport(&viewport))||!viewport.Width||!viewport.Height)return false;
        IDirect3DStateBlock9* state=nullptr;if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state)))return false;
        bool ownScene=SUCCEEDED(device->BeginScene());
        device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetTexture(0,nullptr);
        device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
        device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
        device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);
        device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
        device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
        device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        struct Vertex{float x,y,z,rhw;DWORD color;};Vertex vertices[4*6];
        float x=viewport.X+u*viewport.Width-.5f,y=viewport.Y+v*viewport.Height-.5f,r=ReticleRadius(viewport.Height),inner=r*.55f;
        DWORD color=target?D3DCOLOR_XRGB(90,255,130):D3DCOLOR_XRGB(255,245,200);
        float dx[4]{0,1,0,-1},dy[4]{-1,0,1,0};
        for(unsigned i=0;i<4;++i){unsigned next=(i+1)%4;Vertex a{x+r*dx[i],y+r*dy[i],0,1,color},b{x+r*dx[next],y+r*dy[next],0,1,color},c{x+inner*dx[next],y+inner*dy[next],0,1,color},d{x+inner*dx[i],y+inner*dy[i],0,1,color};
            vertices[i*6]=a;vertices[i*6+1]=b;vertices[i*6+2]=c;vertices[i*6+3]=a;vertices[i*6+4]=c;vertices[i*6+5]=d;}
        HRESULT result=device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,8,vertices,sizeof(Vertex));
        if(ownScene)device->EndScene();state->Apply();state->Release();
    return SUCCEEDED(result);
}
inline bool DrawReticleArtwork(IDirect3DDevice9* device,float u,float v,ReticleArtwork const& art){
    if(!device||!art.texture||art.device!=device)return false;
    D3DVIEWPORT9 viewport{};if(FAILED(device->GetViewport(&viewport))||!viewport.Width||!viewport.Height)return false;
    IDirect3DStateBlock9* state=nullptr;if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state)))return false;
    bool ownScene=SUCCEEDED(device->BeginScene());
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetTexture(0,art.texture.Get());
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    device->SetRenderState(D3DRS_FOGENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    device->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    float x=viewport.X+u*viewport.Width-.5f,y=viewport.Y+v*viewport.Height-.5f;
    float ry=ReticleRadius(viewport.Height),rx=ry*art.aspect;
    struct Vertex{float x,y,z,rhw;DWORD color;float u,v;};constexpr DWORD white=0xffffffff;
    Vertex p[4]{{x-rx,y-ry,0,1,white,art.u0,art.v0},{x+rx,y-ry,0,1,white,art.u1,art.v0},
                {x-rx,y+ry,0,1,white,art.u0,art.v1},{x+rx,y+ry,0,1,white,art.u1,art.v1}};
    HRESULT result=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,p,sizeof(Vertex));
    if(ownScene)device->EndScene();state->Apply();state->Release();return SUCCEEDED(result);
}

}
