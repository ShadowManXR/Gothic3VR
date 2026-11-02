#pragma once
#include "TemporalInstanceIdentity.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <array>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace g3vr {
// Bounded program evidence, separate from motion eligibility. In particular a
// visible EQUAL/ZWRITE=0 color pass must not disappear from a depth-only catalog.
class D3D9ShaderSurvey {
    // Exact program IDs observed in preserved captures. This label only
    // prioritizes evidence; it does not authorize sky motion or clear-depth
    // classification. The current skydome runs AFTER the opaque boundary.
    static bool SkyProgram(uint64_t hash)noexcept{return hash==0x67ecae639f0848acull||hash==0x9cd6b9e1f1627a6full;}
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    static constexpr D3DRENDERSTATETYPE Types[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,D3DRS_COLORWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_ALPHAFUNC,D3DRS_ALPHAREF,D3DRS_CULLMODE,D3DRS_STENCILENABLE,D3DRS_CLIPPLANEENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS,D3DRS_FILLMODE,D3DRS_STENCILFUNC,D3DRS_STENCILREF,
        D3DRS_STENCILMASK,D3DRS_STENCILWRITEMASK,D3DRS_STENCILFAIL,D3DRS_STENCILZFAIL,D3DRS_STENCILPASS,D3DRS_TWOSIDEDSTENCILMODE,D3DRS_CCW_STENCILFUNC,D3DRS_CCW_STENCILFAIL,D3DRS_CCW_STENCILZFAIL,D3DRS_CCW_STENCILPASS};
    static constexpr char const* Names[]={"z_enable","z_write","z_func","color_write","alpha_blend","alpha_test","alpha_func","alpha_ref","cull","stencil","clip_planes","scissor","depth_bias_bits","slope_bias_bits","fill","stencil_func","stencil_ref",
        "stencil_read_mask","stencil_write_mask","stencil_fail","stencil_zfail","stencil_pass","two_sided_stencil","ccw_stencil_func","ccw_stencil_fail","ccw_stencil_zfail","ccw_stencil_pass"};
    static constexpr D3DSAMPLERSTATETYPE SamplerTypes[]={D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW,D3DSAMP_BORDERCOLOR,D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_MAXMIPLEVEL,D3DSAMP_MAXANISOTROPY,D3DSAMP_SRGBTEXTURE};
    struct Sampler {
        uintptr_t identity=0;unsigned type=0,levels=0;D3DSURFACE_DESC level0{};
        std::array<DWORD,std::size(SamplerTypes)> states{};bool textureValid=false,statesValid=false;
    };
    struct Sample {
        uint64_t frame=0;unsigned draws=0,priority=0,drawKind=1;bool afterOpaque=false;
        D3DVIEWPORT9 viewport{};RECT scissor{};std::array<DWORD,std::size(Types)> states{};bool statesValid=false;
        std::array<float,1024> vertexConstants{};std::array<float,896> pixelConstants{};HRESULT vertexResult=E_FAIL,pixelResult=E_FAIL;
        Ptr<IDirect3DPixelShader9> pixel;
        std::array<Sampler,16> samplers;
        std::array<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1> declaration{};UINT elements=0;
        UINT offset=0,stride=0,frequency=0;D3DVERTEXBUFFER_DESC vertex{};D3DINDEXBUFFER_DESC index{};
        diagnostic::temporal::Geometry geometry;unsigned entityCount=0;
        std::array<diagnostic::temporal::InstanceEntity,2> entities;
    };
    struct Entry {Ptr<IDirect3DVertexShader9> vertex;uint64_t hash=0;std::array<Sample,2> eyes;};
    std::array<Entry,64> entries;unsigned count=0,width=0,height=0,eye=2;uint64_t frame=0;
    std::array<unsigned char,32768> code{};
    struct Program {uint64_t hash=0;unsigned bytes=0;bool written=false;std::string name;};
    template<class T>Program ProgramInfo(T* shader,bool vertex,std::filesystem::path const& directory,bool save){
        Program info;if(!shader)return info;UINT bytes=0;if(FAILED(shader->GetFunction(nullptr,&bytes))||!bytes||bytes>code.size())return info;
        UINT capacity=bytes;if(FAILED(shader->GetFunction(code.data(),&capacity))||capacity!=bytes)return info;
        info.bytes=bytes;info.hash=14695981039346656037ull;for(unsigned i=0;i<bytes;++i){info.hash^=code[i];info.hash*=1099511628211ull;}
        std::ostringstream name;name<<"temporal-"<<(vertex?"vs-":"ps-")<<std::hex<<info.hash<<".cso";info.name=name.str();
        if(save){std::ofstream out(directory/info.name,std::ios::binary);out.write(reinterpret_cast<char const*>(code.data()),bytes);out.close();info.written=bool(out);}return info;
    }
public:
    unsigned overflow=0;
    void Begin(unsigned index,uint64_t serial,unsigned w,unsigned h)noexcept{
        eye=index<2?index:2;frame=serial;width=w;height=h;
        if(eye<2)for(auto& e:entries){auto& s=e.eyes[eye];s=Sample{};s.frame=serial;}
    }
    void Observe(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry,bool afterOpaque,
                 diagnostic::temporal::InstanceEntity const* entities,unsigned entityCount,unsigned drawKind=1){
        if(eye>1||!d)return;Ptr<IDirect3DVertexShader9> vs;if(FAILED(d->GetVertexShader(&vs))||!vs)return;
        Entry* found=nullptr;for(unsigned i=0;i<count;++i)if(entries[i].vertex.Get()==vs.Get()){found=&entries[i];break;}
        if(!found){if(count==entries.size()){++overflow;return;}found=&entries[count++];found->vertex=vs;
            UINT bytes=0;if(SUCCEEDED(vs->GetFunction(nullptr,&bytes))&&bytes&&bytes<=code.size()){
                UINT capacity=bytes;if(SUCCEEDED(vs->GetFunction(code.data(),&capacity))&&capacity==bytes){found->hash=14695981039346656037ull;for(unsigned i=0;i<bytes;++i){found->hash^=code[i];found->hash*=1099511628211ull;}}}}
        auto& s=found->eyes[eye];++s.draws;if(s.priority>=3)return;
        D3DVIEWPORT9 viewport{};DWORD z=0,color=0,blend=0;
        if(FAILED(d->GetViewport(&viewport))||viewport.X||viewport.Y||viewport.Width!=width||viewport.Height!=height||
           FAILED(d->GetRenderState(D3DRS_ZENABLE,&z))||FAILED(d->GetRenderState(D3DRS_COLORWRITEENABLE,&color))||FAILED(d->GetRenderState(D3DRS_ALPHABLENDENABLE,&blend)))return;
        bool rgb=(color&7)==7,sky=SkyProgram(found->hash);
        unsigned priority=sky&&rgb?4:z?(rgb?(blend?2:3):1):rgb?1:0;if(!priority||priority<=s.priority)return;
        s.priority=priority;s.frame=frame;s.afterOpaque=afterOpaque;s.viewport=viewport;s.geometry=geometry;s.entityCount=entityCount;s.drawKind=drawKind;
        for(unsigned i=0;i<2;++i)s.entities[i]=entities&&i<entityCount?entities[i]:diagnostic::temporal::InstanceEntity{};
        s.statesValid=true;for(unsigned i=0;i<s.states.size();++i)if(FAILED(d->GetRenderState(Types[i],&s.states[i])))s.statesValid=false;
        d->GetScissorRect(&s.scissor);s.vertexResult=d->GetVertexShaderConstantF(0,s.vertexConstants.data(),256);s.pixelResult=d->GetPixelShaderConstantF(0,s.pixelConstants.data(),224);
        s.pixel.Reset();d->GetPixelShader(&s.pixel);Ptr<IDirect3DVertexDeclaration9> declaration;s.elements=0;
        for(unsigned slot=0;slot<s.samplers.size();++slot){
            auto& sampler=s.samplers[slot];sampler={};Ptr<IDirect3DBaseTexture9> texture;
            sampler.textureValid=SUCCEEDED(d->GetTexture(slot,&texture));
            if(texture){sampler.identity=reinterpret_cast<uintptr_t>(texture.Get());sampler.type=unsigned(texture->GetType());sampler.levels=texture->GetLevelCount();
                Ptr<IDirect3DTexture9> image;if(SUCCEEDED(texture.As(&image)))sampler.textureValid=SUCCEEDED(image->GetLevelDesc(0,&sampler.level0));}
            sampler.statesValid=true;for(unsigned k=0;k<sampler.states.size();++k)if(FAILED(d->GetSamplerState(slot,SamplerTypes[k],&sampler.states[k])))sampler.statesValid=false;
        }
        if(SUCCEEDED(d->GetVertexDeclaration(&declaration))&&declaration){s.elements=UINT(s.declaration.size());if(FAILED(declaration->GetDeclaration(s.declaration.data(),&s.elements))||s.elements>s.declaration.size())s.elements=0;}
        Ptr<IDirect3DVertexBuffer9> vb;Ptr<IDirect3DIndexBuffer9> ib;s.vertex={};s.index={};s.offset=s.stride=s.frequency=0;
        // UP calls consume caller memory, not the bound VB/IB. Do not label a
        // stale stream from an earlier command as this draw's source geometry.
        if(drawKind<2){if(SUCCEEDED(d->GetStreamSource(0,&vb,&s.offset,&s.stride))&&vb)vb->GetDesc(&s.vertex);d->GetStreamSourceFreq(0,&s.frequency);
            if(drawKind==1&&SUCCEEDED(d->GetIndices(&ib))&&ib)ib->GetDesc(&s.index);}
    }
    void Write(std::ostream& out,unsigned index,std::filesystem::path const& directory,bool save){
        if(index>1){out<<"null";return;}out<<"{\"limit\":64,\"overflow\":"<<overflow<<",\"programs\":[";bool first=true;
        auto values=[&](auto const& data){out<<'[';bool comma=false;for(auto value:data){if(comma)out<<',';comma=true;if(std::isfinite(double(value)))out<<+value;else out<<"null";}out<<']';};
        auto program=[&](Program const& p){out<<"{\"file\":"<<std::quoted(p.name)<<",\"bytes\":"<<p.bytes<<",\"written\":"<<(p.written?"true":"false")<<'}';};
        for(unsigned i=0;i<count;++i){auto const& entry=entries[i];auto const& s=entry.eyes[index];if(!s.draws)continue;if(!first)out<<',';first=false;
            auto vs=ProgramInfo(entry.vertex.Get(),true,directory,save),ps=ProgramInfo(s.pixel.Get(),false,directory,save);
            out<<"{\"frame\":"<<s.frame<<",\"draws\":"<<s.draws<<",\"sample_priority\":"<<s.priority<<",\"after_opaque_boundary\":"<<(s.afterOpaque?"true":"false")<<",\"vertex\":";program(vs);out<<",\"pixel\":";program(ps);
            out<<",\"draw_kind\":"<<s.drawKind<<",\"user_pointer_geometry_not_retained\":"<<(s.drawKind>=2?"true":"false")<<",\"native_sky_program\":"<<(SkyProgram(entry.hash)?"true":"false");
            out<<",\"states_valid\":"<<(s.statesValid?"true":"false")<<",\"states\":{";for(unsigned k=0;k<s.states.size();++k){if(k)out<<',';out<<std::quoted(Names[k])<<':'<<s.states[k];}out<<"},\"vertex_constant_hresult\":"<<int32_t(s.vertexResult)<<",\"pixel_constant_hresult\":"<<int32_t(s.pixelResult);
            out<<",\"pixel_samplers\":[";for(unsigned slot=0;slot<s.samplers.size();++slot){if(slot)out<<',';auto const& sampler=s.samplers[slot];
                out<<"{\"slot\":"<<slot<<",\"texture_identity\":"<<sampler.identity<<",\"type\":"<<sampler.type<<",\"levels\":"<<sampler.levels<<",\"texture_valid\":"<<(sampler.textureValid?"true":"false")<<",\"states_valid\":"<<(sampler.statesValid?"true":"false")<<",\"level0\":["<<sampler.level0.Width<<','<<sampler.level0.Height<<','<<sampler.level0.Format<<','<<sampler.level0.Usage<<','<<sampler.level0.Pool<<"],\"states_addressUVW_border_mag_min_mip_lod_maxmip_aniso_srgb\":";values(sampler.states);out<<",\"texture_pixels_captured\":false}";}out<<']';
            out<<",\"vertex_constants_c0_c255\":";values(s.vertexConstants);out<<",\"pixel_constants_c0_c223\":";values(s.pixelConstants);
            out<<",\"viewport\":["<<s.viewport.X<<','<<s.viewport.Y<<','<<s.viewport.Width<<','<<s.viewport.Height<<','<<s.viewport.MinZ<<','<<s.viewport.MaxZ<<"],\"scissor_rect\":["<<s.scissor.left<<','<<s.scissor.top<<','<<s.scissor.right<<','<<s.scissor.bottom<<"],\"draw\":["<<s.geometry.primitive<<','<<s.geometry.base<<','<<s.geometry.minimum<<','<<s.geometry.vertices<<','<<s.geometry.first<<','<<s.geometry.count<<']';
            out<<",\"stream0\":["<<s.offset<<','<<s.stride<<','<<s.frequency<<','<<s.vertex.Size<<','<<s.vertex.Usage<<"],\"index\":["<<s.index.Size<<','<<s.index.Format<<"],\"declaration\":[";
            for(unsigned k=0;k<s.elements;++k){if(k)out<<',';auto const& e=s.declaration[k];out<<'['<<e.Stream<<','<<e.Offset<<','<<unsigned(e.Type)<<','<<unsigned(e.Method)<<','<<unsigned(e.Usage)<<','<<unsigned(e.UsageIndex)<<']';}
            out<<"],\"source_entity_count\":"<<s.entityCount<<",\"sdk_world_samples\":[";for(unsigned k=0;k<std::min(s.entityCount,2u);++k){if(k)out<<',';out<<"{\"id\":";values(s.entities[k].id.value);out<<",\"world\":";values(s.entities[k].world);out<<'}';}out<<"]}";
        }out<<"]}";
    }
};
}
