#include "D3D9SkinnedMotion.h"
#include "SkinMotionVS.h"
#include "SkinMotionPS.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace g3vr {
namespace {
void Check(HRESULT hr,char const* label){if(FAILED(hr))throw std::runtime_error(std::string(label)+" (HRESULT "+std::to_string(int32_t(hr))+")");}
DWORD Bits(float value){DWORD bits;memcpy(&bits,&value,4);return bits;}
struct RestoreState {
    using Surface=Microsoft::WRL::ComPtr<IDirect3DSurface9>;
    IDirect3DDevice9* device;Microsoft::WRL::ComPtr<IDirect3DStateBlock9> block;
    std::array<Surface,4> targets;Surface depth;D3DVIEWPORT9 viewport{};RECT scissor{};unsigned count;bool armed=false;
    Microsoft::WRL::ComPtr<IDirect3DBaseTexture9> vertexTexture;
    inline static constexpr D3DSAMPLERSTATETYPE Samplers[]={D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER,D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_SRGBTEXTURE,D3DSAMP_MAXMIPLEVEL,D3DSAMP_MIPMAPLODBIAS};
    std::array<DWORD,8> sampler{};
    RestoreState(IDirect3DDevice9* d,unsigned n):device(d),count(n){
        Check(d->CreateStateBlock(D3DSBT_ALL,&block),"motion state block");Check(block->Capture(),"motion state capture");
        for(unsigned i=0;i<n;++i){auto hr=d->GetRenderTarget(i,&targets[i]);if(i||hr!=D3DERR_NOTFOUND){if(hr!=D3DERR_NOTFOUND)Check(hr,"motion save target");}}
        auto hr=d->GetDepthStencilSurface(&depth);if(hr!=D3DERR_NOTFOUND)Check(hr,"motion save depth");
        Check(d->GetViewport(&viewport),"motion save viewport");Check(d->GetScissorRect(&scissor),"motion save scissor");
        Check(d->GetTexture(D3DVERTEXTEXTURESAMPLER0,&vertexTexture),"motion save vertex texture");
        for(unsigned i=0;i<sampler.size();++i)Check(d->GetSamplerState(D3DVERTEXTEXTURESAMPLER0,Samplers[i],&sampler[i]),"motion save vertex sampler");armed=true;
    }
    HRESULT Apply()noexcept {HRESULT first=S_OK;auto save=[&](HRESULT h){if(FAILED(h)&&SUCCEEDED(first))first=h;};
        save(device->SetDepthStencilSurface(nullptr));for(unsigned i=1;i<count;++i)save(device->SetRenderTarget(i,nullptr));
        save(device->SetRenderTarget(0,targets[0].Get()));for(unsigned i=1;i<count;++i)save(device->SetRenderTarget(i,targets[i].Get()));save(device->SetDepthStencilSurface(depth.Get()));
        save(block->Apply());save(device->SetTexture(D3DVERTEXTEXTURESAMPLER0,vertexTexture.Get()));
        for(unsigned i=0;i<sampler.size();++i)save(device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0,Samplers[i],sampler[i]));
        save(device->SetViewport(&viewport));save(device->SetScissorRect(&scissor));return first;}
    void Finish(){auto hr=Apply();armed=false;Check(hr,"motion native state restoration");}
    ~RestoreState(){if(armed)Apply();}
};
void Validate(SkinnedMotionDraw const& draw){
    auto const& g=draw.geometry;if(!draw.vertices||!draw.indices||draw.stride<32||!draw.current.Valid()||(draw.historyValid&&!draw.previous.Valid())||
       !std::isfinite(draw.depthBias)||!std::isfinite(draw.slopeBias)||draw.cull<D3DCULL_NONE||draw.cull>D3DCULL_CCW||
       (g.primitive!=D3DPT_TRIANGLELIST&&g.primitive!=D3DPT_TRIANGLESTRIP)||!g.vertices||!g.count)throw std::runtime_error("Invalid motion draw");
    D3DVERTEXBUFFER_DESC vb{};D3DINDEXBUFFER_DESC ib{};Check(draw.vertices->GetDesc(&vb),"motion vertex description");Check(draw.indices->GetDesc(&ib),"motion index description");
    int64_t first=int64_t(g.base)+g.minimum,end=first+g.vertices;
    uint64_t indices=g.primitive==D3DPT_TRIANGLELIST?uint64_t(g.count)*3:uint64_t(g.count)+2;
    unsigned indexBytes=ib.Format==D3DFMT_INDEX16?2:ib.Format==D3DFMT_INDEX32?4:0;
    if(first<0||end<=0||uint64_t(draw.offset)+uint64_t(end-1)*draw.stride+32>vb.Size||!indexBytes||
       (uint64_t(g.first)+indices)*indexBytes>ib.Size)throw std::runtime_error("Motion geometry exceeds its native buffers");
}
}
void D3D9SkinnedMotion::Reset()noexcept {surface.Reset();output.Reset();palette.Reset();declaration.Reset();pixelShader.Reset();vertexShader.Reset();device.Reset();width=height=targets=0;}
void D3D9SkinnedMotion::Initialize(IDirect3DDevice9* native,unsigned w,unsigned h){
    Reset();if(!native||!w||!h||w>8192||h>8192||uint64_t(w)*h>16777216)throw std::runtime_error("Invalid motion target size");
    try{
        device=native;D3DCAPS9 caps{};Check(device->GetDeviceCaps(&caps),"motion caps");
        if(caps.VertexShaderVersion<D3DVS_VERSION(3,0)||caps.PixelShaderVersion<D3DPS_VERSION(3,0)||w>caps.MaxTextureWidth||h>caps.MaxTextureHeight)throw std::runtime_error("SM3 motion shader unavailable");
        targets=std::min(4u,unsigned(caps.NumSimultaneousRTs));width=w;height=h;
        Check(device->CreateVertexShader(reinterpret_cast<DWORD const*>(SkinMotionVS),&vertexShader),"motion vertex shader");
        Check(device->CreatePixelShader(reinterpret_cast<DWORD const*>(SkinMotionPS),&pixelShader),"motion pixel shader");
        D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,12,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_BLENDWEIGHT,0},{0,28,D3DDECLTYPE_UBYTE4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_BLENDINDICES,0},D3DDECL_END()};
        Check(device->CreateVertexDeclaration(elements,&declaration),"motion position declaration");
        Check(device->CreateTexture(256,Limit,1,D3DUSAGE_DYNAMIC,D3DFMT_A32B32G32R32F,D3DPOOL_DEFAULT,&palette,nullptr),"previous-bone GPU palette");
        Check(device->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&output,nullptr),"motion RGBA16F target");
        Check(output->GetSurfaceLevel(0,&surface),"motion target surface");
    }catch(...){Reset();throw;}
}
D3D9SkinnedMotion::Report D3D9SkinnedMotion::Render(IDirect3DSurface9* opaqueDepth,SkinnedMotionDraw const* draws,unsigned count,std::array<float,2> currentJitter,std::array<float,2> previousJitter){
    if(!device||!opaqueDepth||count>Limit||(count&&!draws))throw std::runtime_error("Invalid motion frame");
    for(float f:currentJitter)if(!std::isfinite(f)||std::fabs(f)>.5f)throw std::runtime_error("Invalid current raster jitter");
    for(float f:previousJitter)if(!std::isfinite(f)||std::fabs(f)>.5f)throw std::runtime_error("Invalid previous raster jitter");
    D3DSURFACE_DESC depthDesc{};Check(opaqueDepth->GetDesc(&depthDesc),"motion source depth");
    if(depthDesc.Width!=width||depthDesc.Height!=height||depthDesc.MultiSampleType!=D3DMULTISAMPLE_NONE||!(depthDesc.Usage&D3DUSAGE_DEPTHSTENCIL))throw std::runtime_error("Motion depth does not match eye");
    for(unsigned i=0;i<count;++i)Validate(draws[i]);
    struct ReplayGuard {unsigned& value;explicit ReplayGuard(unsigned& v):value(v){++value;}~ReplayGuard(){--value;}} replay(replayDepth);
    // One bounded CPU upload of previous matrices; no geometry/pixel readback.
    if(count){D3DLOCKED_RECT mapped{};Check(palette->LockRect(0,&mapped,nullptr,D3DLOCK_DISCARD),"motion palette upload");
        if(mapped.Pitch<256*16){palette->UnlockRect(0);throw std::runtime_error("Invalid palette row pitch");}
        for(unsigned i=0;i<count;++i){auto const& p=draws[i].historyValid?draws[i].previous:draws[i].current;memcpy(static_cast<unsigned char*>(mapped.pBits)+i*mapped.Pitch,p.bones.data(),sizeof(p.bones));}
        Check(palette->UnlockRect(0),"motion palette upload complete");}
    RestoreState restore(device.Get(),targets);
    Check(device->SetDepthStencilSurface(nullptr),"motion unbind depth");for(unsigned i=1;i<targets;++i)Check(device->SetRenderTarget(i,nullptr),"motion disable MRT");
    Check(device->SetRenderTarget(0,surface.Get()),"motion bind target");Check(device->SetDepthStencilSurface(opaqueDepth),"motion bind opaque depth");
    D3DVIEWPORT9 viewport{0,0,width,height,0,1};Check(device->SetViewport(&viewport),"motion viewport");
    auto state=[&](D3DRENDERSTATETYPE key,DWORD value){Check(device->SetRenderState(key,value),"motion render state");};
    state(D3DRS_ZENABLE,TRUE);state(D3DRS_ZWRITEENABLE,FALSE);state(D3DRS_ZFUNC,D3DCMP_EQUAL);
    state(D3DRS_ALPHABLENDENABLE,FALSE);state(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);state(D3DRS_ALPHATESTENABLE,FALSE);state(D3DRS_STENCILENABLE,FALSE);
    state(D3DRS_COLORWRITEENABLE,15);state(D3DRS_SRGBWRITEENABLE,FALSE);state(D3DRS_FOGENABLE,FALSE);state(D3DRS_SCISSORTESTENABLE,FALSE);
    state(D3DRS_CLIPPLANEENABLE,0);state(D3DRS_FILLMODE,D3DFILL_SOLID);state(D3DRS_CLIPPING,TRUE);
    Check(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"motion clear output only");
    Check(device->SetVertexDeclaration(declaration.Get()),"motion vertex declaration");Check(device->SetVertexShader(vertexShader.Get()),"motion VS");Check(device->SetPixelShader(pixelShader.Get()),"motion PS");
    Check(device->SetTexture(D3DVERTEXTEXTURESAMPLER0,palette.Get()),"motion previous palette binding");
    for(auto setting:std::array<std::pair<D3DSAMPLERSTATETYPE,DWORD>,8>{{{D3DSAMP_MINFILTER,D3DTEXF_POINT},{D3DSAMP_MAGFILTER,D3DTEXF_POINT},{D3DSAMP_MIPFILTER,D3DTEXF_NONE},{D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP},{D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP},{D3DSAMP_SRGBTEXTURE,FALSE},{D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0}}})
        Check(device->SetSamplerState(D3DVERTEXTEXTURESAMPLER0,setting.first,setting.second),"motion palette sampler");
    float pixel[4]{float(width)*.5f,-float(height)*.5f,previousJitter[0]-currentJitter[0],previousJitter[1]-currentJitter[1]};
    Check(device->SetPixelShaderConstantF(0,pixel,1),"motion pixel units");Report report;
    for(unsigned i=0;i<count;++i){auto const& d=draws[i];auto const& previous=d.historyValid?d.previous:d.current;
        Check(device->SetStreamSource(0,d.vertices.Get(),d.offset,d.stride),"motion primary vertex stream");Check(device->SetStreamSourceFreq(0,1),"motion primary frequency");Check(device->SetIndices(d.indices.Get()),"motion indices");
        state(D3DRS_CULLMODE,d.cull);state(D3DRS_DEPTHBIAS,Bits(d.depthBias));state(D3DRS_SLOPESCALEDEPTHBIAS,Bits(d.slopeBias));
        Check(device->SetVertexShaderConstantF(0,d.current.bones.data(),192),"motion current bones");Check(device->SetVertexShaderConstantF(192,d.current.clip.data(),4),"motion current projection");
        Check(device->SetVertexShaderConstantF(196,previous.clip.data(),4),"motion previous projection");
        float atlas[4]{1.f/256,1.f/Limit,float(i)+.5f,d.historyValid?1.f:0.f};Check(device->SetVertexShaderConstantF(200,atlas,1),"motion palette row");
        auto const& g=d.geometry;Check(device->DrawIndexedPrimitive(D3DPRIMITIVETYPE(g.primitive),g.base,g.minimum,g.vertices,g.first,g.count),"motion raster draw");
        ++report.draws;if(d.historyValid)++report.historyDraws;
    }
    report.paletteBytes=count*sizeof(diagnostic::temporal::SkinPose::bones);restore.Finish();return report;
}
}
