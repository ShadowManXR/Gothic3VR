#pragma once
#include <d3d9.h>
#include "HandHudPose.h"
#include <vector>
#include <cstring>

namespace diagnostic {
struct HandHudQuad {
    struct Vertex {float x,y,z;DWORD color;float u,v;};
    IDirect3DTexture9* texture=nullptr;
    Vertex vertices[4];
};
inline void AddHandHudQuad(std::vector<HandHudQuad>& quads,arms::Mat const& pose,
    float panelWidth,float panelHeight,float pixelsWide,float pixelsHigh,
    float x,float y,float w,float h,IDirect3DTexture9* texture,DWORD color=0xffffffff,
    float u0=0,float v0=0,float u1=1,float v1=1){
    HandHudQuad quad;quad.texture=texture;
    for(unsigned i=0;i<4;++i){bool right=(i&1)!=0,bottom=(i&2)!=0;
        auto p=arms::Point(pose,{((x+(right?w:0))/pixelsWide-.5f)*panelWidth,
                               (.5f-(y+(bottom?h:0))/pixelsHigh)*panelHeight,0});
        quad.vertices[i]={p.x,p.y,p.z,color,right?u1:u0,bottom?v1:v0};}
    quads.push_back(quad);
}
inline bool DrawHandHudQuads(IDirect3DDevice9* device,float const* view,float const* projection,
                             std::vector<HandHudQuad> const& quads){
    if(!device||quads.empty())return false;
    IDirect3DStateBlock9* state=nullptr;if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state)))return false;
    if(FAILED(state->Capture())){state->Release();return false;}
    bool scene=SUCCEEDED(device->BeginScene());
    D3DMATRIX identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    D3DMATRIX v,p;memcpy(&v,view,sizeof(v));memcpy(&p,projection,sizeof(p));
    device->SetTransform(D3DTS_WORLD,&identity);device->SetTransform(D3DTS_VIEW,&v);device->SetTransform(D3DTS_PROJECTION,&p);
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);
    device->SetRenderState(D3DRS_ZENABLE,TRUE);device->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    device->SetRenderState(D3DRS_FOGENABLE,FALSE);device->SetRenderState(D3DRS_COLORWRITEENABLE,15);
    device->SetRenderState(D3DRS_STENCILENABLE,FALSE);device->SetRenderState(D3DRS_CLIPPLANEENABLE,0);
    device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    bool ok=true;
    for(auto const& q:quads){device->SetTexture(0,q.texture);
        device->SetTextureStageState(0,D3DTSS_COLOROP,q.texture?D3DTOP_MODULATE:D3DTOP_SELECTARG2);
        device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
        device->SetTextureStageState(0,D3DTSS_ALPHAOP,q.texture?D3DTOP_MODULATE:D3DTOP_SELECTARG2);
        device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
        ok=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,q.vertices,sizeof(HandHudQuad::Vertex)))&&ok;}
    if(scene)device->EndScene();state->Apply();state->Release();return ok;
}
}
