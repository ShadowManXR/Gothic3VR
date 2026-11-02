#include "D3D9NativeMotion.h"
#include "NativeMotion0.h"
#include "NativeMotion1.h"
#include "NativeMotion2.h"
#include "NativeMotion3.h"
#include "NativeMotionPS.h"
#include "NativeMotionMaskedWind.h"
#include "NativeMaskedMotionPS.h"
#include <cstring>
#include <stdexcept>
#include <string>
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
void Validate(NativeMotionDraw const& d){
    using Kind=diagnostic::temporal::NativeGeometryKind;auto const& g=d.geometry;
    if(!d.vertices||!d.indices||!d.declaration||!d.stride||(d.frequency!=1&&d.frequency!=(D3DSTREAMSOURCE_INDEXEDDATA|1))||unsigned(d.kind)>3||!d.current.Valid()||(d.historyValid&&!d.previous.Valid())||!std::isfinite(d.depthBias)||!std::isfinite(d.slopeBias)||d.cull<D3DCULL_NONE||d.cull>D3DCULL_CCW||
       (g.primitive!=D3DPT_TRIANGLELIST&&g.primitive!=D3DPT_TRIANGLESTRIP)||!g.vertices||!g.count)throw std::runtime_error("Invalid native geometry motion draw");
    UINT count=MAXD3DDECLLENGTH+1;D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH+1]{};Check(d.declaration->GetDeclaration(elements,&count),"native motion declaration");
    bool position=false,auxiliary=d.kind==Kind::RigidWorld,uv=!d.masked;std::array<unsigned,16> ends{};
    if(d.masked&&(!d.alphaTexture||d.kind!=Kind::Wind||d.alphaReference>255))throw std::runtime_error("Invalid native masked material");
    if(count<2||count>(d.masked?4u:3u))throw std::runtime_error("Native motion requires a minimal position declaration");
    for(unsigned i=0;i+1<count;++i){auto const& e=elements[i];
        if(e.Stream>=16||e.Method||e.UsageIndex>4)throw std::runtime_error("Unsupported native position stream/method");
        bool p=e.Stream==0&&e.Usage==D3DDECLUSAGE_POSITION&&e.UsageIndex==0&&e.Type==D3DDECLTYPE_FLOAT3;
        bool a=d.kind!=Kind::RigidWorld&&e.Usage==D3DDECLUSAGE_TEXCOORD&&e.UsageIndex==(d.kind==Kind::Wind?1:4)&&e.Type<=D3DDECLTYPE_FLOAT4;
        bool t=d.masked&&e.Usage==D3DDECLUSAGE_TEXCOORD&&e.UsageIndex==0&&e.Type==D3DDECLTYPE_FLOAT2;
        if(!p&&!a&&!t)throw std::runtime_error("Unsupported native position element");position|=p;auxiliary|=a;uv|=t;
        ends[e.Stream]=std::max(ends[e.Stream],unsigned(e.Offset)+(e.Type+1)*4);
    }
    if(!position||!auxiliary||!uv||ends[0]>d.stride)throw std::runtime_error("Native position declaration missing inputs");
    D3DVERTEXBUFFER_DESC vb{};D3DINDEXBUFFER_DESC ib{};Check(d.vertices->GetDesc(&vb),"native motion VB");Check(d.indices->GetDesc(&ib),"native motion IB");
    int64_t first=int64_t(g.base)+g.minimum,last=first+g.vertices;
    unsigned indexBytes=ib.Format==D3DFMT_INDEX16?2:ib.Format==D3DFMT_INDEX32?4:0;uint64_t indices=g.primitive==D3DPT_TRIANGLELIST?uint64_t(g.count)*3:uint64_t(g.count)+2;
    if(first<0||last<=0||!indexBytes||uint64_t(d.offset)+uint64_t(last-1)*d.stride+ends[0]>vb.Size||(uint64_t(g.first)+indices)*indexBytes>ib.Size)throw std::runtime_error("Native motion geometry exceeds buffers");
    for(unsigned stream=1;stream<16;++stream)if(ends[stream]){auto const& input=d.auxiliaryStreams[stream-1];if(!input.vertices||input.frequency!=1||!input.stride||ends[stream]>input.stride)throw std::runtime_error("Native auxiliary stream missing/instanced");D3DVERTEXBUFFER_DESC desc{};Check(input.vertices->GetDesc(&desc),"native auxiliary stream description");if(uint64_t(input.offset)+uint64_t(last-1)*input.stride+ends[stream]>desc.Size)throw std::runtime_error("Native auxiliary stream exceeds buffer");}
}
}
void D3D9NativeMotion::Reset()noexcept{surface.Reset();output.Reset();pixel.Reset();maskedPixel.Reset();maskedWind.Reset();for(auto& s:shaders)s.Reset();device.Reset();width=height=targets=0;}
void D3D9NativeMotion::Initialize(IDirect3DDevice9* d,unsigned w,unsigned h){
    Reset();if(!d||!w||!h||w>8192||h>8192||uint64_t(w)*h>16777216)throw std::runtime_error("Invalid native motion target");
    try{device=d;D3DCAPS9 caps{};Check(d->GetDeviceCaps(&caps),"native motion capabilities");if(caps.VertexShaderVersion<D3DVS_VERSION(3,0)||caps.PixelShaderVersion<D3DPS_VERSION(3,0)||caps.MaxVertexShaderConst<256||w>caps.MaxTextureWidth||h>caps.MaxTextureHeight)throw std::runtime_error("Native motion SM3 unavailable");
        void const* code[]{NativeMotion0,NativeMotion1,NativeMotion2,NativeMotion3};for(unsigned i=0;i<4;++i)Check(d->CreateVertexShader(reinterpret_cast<DWORD const*>(code[i]),&shaders[i]),"native position companion VS");
        Check(d->CreateVertexShader(reinterpret_cast<DWORD const*>(NativeMotionMaskedWind),&maskedWind),"native masked wind VS");
        Check(d->CreatePixelShader(reinterpret_cast<DWORD const*>(NativeMaskedMotionPS),&maskedPixel),"native masked motion PS");
        Check(d->CreatePixelShader(reinterpret_cast<DWORD const*>(NativeMotionPS),&pixel),"native motion PS");Check(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&output,nullptr),"native motion output");Check(output->GetSurfaceLevel(0,&surface),"native motion surface");width=w;height=h;targets=std::min(4u,unsigned(caps.NumSimultaneousRTs));
    }catch(...){Reset();throw;}
}
D3D9NativeMotion::Report D3D9NativeMotion::Render(IDirect3DSurface9* depth,NativeMotionDraw const* draws,unsigned count,std::array<float,2> currentJitter,std::array<float,2> previousJitter){
    if(!device||!depth||count>Limit||(count&&!draws))throw std::runtime_error("Invalid native motion frame");for(auto jitter:{currentJitter,previousJitter})for(float value:jitter)if(!std::isfinite(value)||std::abs(value)>.5f)throw std::runtime_error("Invalid native motion jitter");
    D3DSURFACE_DESC desc{};Check(depth->GetDesc(&desc),"native motion opaque depth");if(desc.Width!=width||desc.Height!=height||desc.MultiSampleType!=D3DMULTISAMPLE_NONE||!(desc.Usage&D3DUSAGE_DEPTHSTENCIL))throw std::runtime_error("Native motion depth mismatch");for(unsigned i=0;i<count;++i)Validate(draws[i]);
    struct Guard{unsigned& n;explicit Guard(unsigned& v):n(v){++n;}~Guard(){--n;}} guard(replayDepth);RestoreState restore(device.Get(),targets);
    Check(device->SetDepthStencilSurface(nullptr),"native motion unbind depth");for(unsigned i=1;i<targets;++i)Check(device->SetRenderTarget(i,nullptr),"native motion unbind MRT");Check(device->SetRenderTarget(0,surface.Get()),"native motion RT");Check(device->SetDepthStencilSurface(depth),"native motion bind depth");
    D3DVIEWPORT9 vp{0,0,width,height,0,1};Check(device->SetViewport(&vp),"native motion viewport");auto state=[&](D3DRENDERSTATETYPE k,DWORD v){Check(device->SetRenderState(k,v),"native motion state");};
    state(D3DRS_ZENABLE,TRUE);state(D3DRS_ZWRITEENABLE,FALSE);state(D3DRS_ZFUNC,D3DCMP_EQUAL);
    for(auto key:{D3DRS_ALPHABLENDENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_CLIPPLANEENABLE})state(key,FALSE);
    state(D3DRS_COLORWRITEENABLE,15);state(D3DRS_FILLMODE,D3DFILL_SOLID);state(D3DRS_CLIPPING,TRUE);Check(device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"native motion clear private output");Check(device->SetPixelShader(pixel.Get()),"native motion PS bind");
    for(auto wrap:{D3DRS_WRAP0,D3DRS_WRAP1,D3DRS_WRAP2,D3DRS_WRAP3})state(wrap,0);
    float parameters[]{float(width)*.5f,-float(height)*.5f,previousJitter[0]-currentJitter[0],previousJitter[1]-currentJitter[1]};Check(device->SetPixelShaderConstantF(0,parameters,1),"native motion pixel units");Report report;
    for(unsigned i=0;i<count;++i){auto const& d=draws[i];auto const& old=d.historyValid?d.previous:d.current;
        Check(device->SetVertexShader(d.masked?maskedWind.Get():shaders[unsigned(d.kind)].Get()),"native companion VS bind");
        Check(device->SetPixelShader(d.masked?maskedPixel.Get():pixel.Get()),"native per-draw motion PS");
        if(d.masked){Check(device->SetTexture(0,d.alphaTexture.Get()),"native alpha texture");for(unsigned j=0;j<d.alphaSampler.size();++j)Check(device->SetSamplerState(0,NativeMotionDraw::AlphaSamplerStates[j],d.alphaSampler[j]),"native alpha sampler");float alpha[]{float(d.alphaReference)/255.f,0,0,0};Check(device->SetPixelShaderConstantF(1,alpha,1),"native alpha reference");}
        Check(device->SetVertexDeclaration(d.declaration.Get()),"native position declaration bind");Check(device->SetStreamSource(0,d.vertices.Get(),d.offset,d.stride),"native position stream bind");Check(device->SetStreamSourceFreq(0,1),"native position stream frequency");Check(device->SetIndices(d.indices.Get()),"native position index bind");
        for(unsigned j=0;j<d.auxiliaryStreams.size();++j)if(d.auxiliaryStreams[j].vertices){auto const& input=d.auxiliaryStreams[j];Check(device->SetStreamSource(j+1,input.vertices.Get(),input.offset,input.stride),"native auxiliary stream bind");Check(device->SetStreamSourceFreq(j+1,1),"native auxiliary stream frequency");}
        state(D3DRS_CULLMODE,d.cull);state(D3DRS_DEPTHBIAS,Bits(d.depthBias));state(D3DRS_SLOPESCALEDEPTHBIAS,Bits(d.slopeBias));
        Check(device->SetVertexShaderConstantF(0,d.current.constants.data(),83),"native current constants");Check(device->SetVertexShaderConstantF(96,old.constants.data(),83),"native previous constants");float valid[]{d.historyValid?1.f:0.f,0,0,0};Check(device->SetVertexShaderConstantF(251,valid,1),"native previous validity");
        if(d.kind==diagnostic::temporal::NativeGeometryKind::InstancesA||d.kind==diagnostic::temporal::NativeGeometryKind::InstancesB){std::array<float,17*4> tags{};for(unsigned j=0;j<17;++j)tags[j*4]=d.historyValid&&d.instanceHistory[j]?1.f:0.f;Check(device->SetVertexShaderConstantF(230,tags.data(),17),"native previous instance identities");report.constantBytes+=sizeof(tags);}
        auto const& g=d.geometry;Check(device->DrawIndexedPrimitive(D3DPRIMITIVETYPE(g.primitive),g.base,g.minimum,g.vertices,g.first,g.count),"native motion companion raster");++report.draws;report.historyDraws+=d.historyValid;report.constantBytes+=83*16*2+16;
    }
    restore.Finish();return report;
}
}
