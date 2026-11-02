#pragma once
#include "TemporalObjectHistory.h"
#include "D3D9SkinnedMotion.h"
#include "D3D9BufferObserver.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <ostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string>
#include <stdexcept>

namespace g3vr {
class D3D9AnimatedMotionProbe {
    using Object=diagnostic::temporal::Object;
    using Layout=diagnostic::temporal::SkinLayout;
    struct Shader {uintptr_t identity=0;uint64_t hash=0;Layout layout;};
    std::array<Shader,16> shaders{};unsigned shaderCount=0;
    std::array<unsigned char,32768> code{};
    std::array<float,1024> constants{};
    diagnostic::temporal::SkinPose pose;
    std::array<Object,8> scopes;unsigned depth=0;int eye=-1;
    inline static constexpr char const* RejectNames[]={"viewport_or_render_state","target_or_depth_surface","vertex_stream_or_frequency","indices_or_declaration","multiple_vertex_streams","constant_read"};
    struct Rejection {unsigned count=0;uint64_t shader=0;std::array<uint32_t,6> first{};};
    struct Counters {unsigned scopedDraws=0,queries=0,unsupported=0,rejected=0;std::array<Rejection,6> reasons;};
    std::array<Counters,2> counts;
    diagnostic::temporal::ObjectHistory history;
    std::array<uint64_t,2> displayTimes{},epochs{{1,1}};
    std::array<std::array<float,2>,2> currentJitter{},previousJitter{};
    struct Resources {Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> vertices;Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> indices;float bias=0,slope=0;};
    std::array<std::array<Resources,diagnostic::temporal::ObjectHistory::Limit>,2> resources;
    std::array<SkinnedMotionDraw,diagnostic::temporal::ObjectHistory::Limit> jobs;
    struct Snapshot {std::unique_ptr<D3D9SkinnedMotion> renderer;uint64_t frame=0;bool attempted=false,written=false,writeAttempted=false;D3D9SkinnedMotion::Report report;std::string file,error;};
    std::array<Snapshot,2> snapshots;
    std::array<uint64_t,2> rasterFrames{};
    D3D9BufferObserver* buffers=nullptr;
    static void Reject(Counters& c,unsigned reason,uint64_t shader,std::array<uint32_t,6> values={})noexcept {
        ++c.rejected;auto& r=c.reasons[reason];if(!r.count){r.shader=shader;r.first=values;}++r.count;
    }
    static uint64_t Hash(void const* data,unsigned bytes)noexcept {auto p=static_cast<unsigned char const*>(data);uint64_t h=14695981039346656037ull;for(unsigned i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;}
    Shader Describe(IDirect3DVertexShader9* shader)noexcept {
        if(!shader)return {};auto identity=reinterpret_cast<uintptr_t>(shader);
        for(unsigned i=0;i<shaderCount;++i)if(shaders[i].identity==identity)return shaders[i];
        if(shaderCount>=shaders.size())return {};
        Shader out;out.identity=identity;UINT bytes=0;
        if(SUCCEEDED(shader->GetFunction(nullptr,&bytes))&&bytes&&bytes<=code.size()){
            UINT size=bytes;if(SUCCEEDED(shader->GetFunction(code.data(),&size))&&size==bytes){out.hash=Hash(code.data(),bytes);out.layout=diagnostic::temporal::Layout(out.hash);}}
        shaders[shaderCount++]=out;return out;
    }
    static void Floats(std::ostream& out,float const* data,unsigned count){out<<'[';for(unsigned i=0;i<count;++i){if(i)out<<',';out<<data[i];}out<<']';}
    static void Pose(std::ostream& out,diagnostic::temporal::SkinPose const& p){
        out<<"{\"bone_c0_c191\":";Floats(out,p.bones.data(),unsigned(p.bones.size()));
        out<<",\"clip_dp4_columns\":";Floats(out,p.clip.data(),16);out<<",\"root_sdk_row_major\":";Floats(out,p.world.data(),16);out<<'}';
    }
public:
    void UseBufferObserver(D3D9BufferObserver* observer)noexcept{buffers=observer;}
    void Begin(unsigned index,uint64_t frame,uint64_t displayTime=0,std::array<float,2> jitter={})noexcept {eye=index<2?int(index):-1;depth=0;shaderCount=0;if(eye>=0){
        previousJitter[index]=currentJitter[index];currentJitter[index]=jitter;
        if(displayTime&&displayTimes[index]&&(displayTime<=displayTimes[index]||displayTime-displayTimes[index]>250000000ull))++epochs[index];
        displayTimes[index]=displayTime;counts[index]={};for(auto& r:resources[index])r={};history.Begin(index,frame,epochs[index]);
        auto& s=snapshots[index];s.frame=frame;s.attempted=s.written=s.writeAttempted=false;s.report={};s.file.clear();s.error.clear();}}
    void Stop()noexcept {eye=-1;depth=0;}
    void ResetHistory()noexcept {history.Reset();depth=0;displayTimes={};rasterFrames={};currentJitter={};previousJitter={};for(auto& epoch:epochs)++epoch;for(auto& eye:resources)for(auto& r:eye)r={};for(auto& d:jobs){d.vertices.Reset();d.indices.Reset();}for(auto& snapshot:snapshots)snapshot={};}
    unsigned Push(Object const& object)noexcept {unsigned token=depth;if(depth<scopes.size())scopes[depth]=object;++depth;return token;}
    void Pop(unsigned token)noexcept {depth=token<=depth?token:0;}
    void Observe(IDirect3DDevice9* device,diagnostic::temporal::Geometry const& geometry)noexcept {
        if(eye<0||!device||snapshots[unsigned(eye)].attempted||!depth||depth>scopes.size()||!scopes[depth-1].Valid())return;
        auto& c=counts[unsigned(eye)];++c.scopedDraws;if(c.queries>=diagnostic::temporal::ObjectHistory::Limit*4)return;++c.queries;
        Microsoft::WRL::ComPtr<IDirect3DVertexShader9> vs;
        if(FAILED(device->GetVertexShader(&vs)))return;auto shader=Describe(vs.Get());if(!shader.layout){++c.unsupported;return;}
        D3DVIEWPORT9 viewport{};DWORD z=0,write=0,color=0,zfunc=0,cull=0,alphaTest=0;
        if(FAILED(device->GetViewport(&viewport))||FAILED(device->GetRenderState(D3DRS_ZENABLE,&z))||
           FAILED(device->GetRenderState(D3DRS_ZWRITEENABLE,&write))||FAILED(device->GetRenderState(D3DRS_COLORWRITEENABLE,&color))||
           FAILED(device->GetRenderState(D3DRS_ZFUNC,&zfunc))||FAILED(device->GetRenderState(D3DRS_CULLMODE,&cull))||FAILED(device->GetRenderState(D3DRS_ALPHATESTENABLE,&alphaTest))||
           !z||!color||viewport.X||viewport.Y||viewport.Width!=width||viewport.Height!=height){Reject(c,0,shader.hash,{z,write,color,viewport.X,viewport.Width,viewport.Height});return;}
        // A visible color draw can legitimately test the already populated
        // world depth without writing it again. ZWRITE is part of pass
        // identity, not a requirement for observing animation correspondence.
        Microsoft::WRL::ComPtr<IDirect3DSurface9> target,depthSurface;D3DSURFACE_DESC rt{},ds{};
        if(FAILED(device->GetRenderTarget(0,&target))||FAILED(target->GetDesc(&rt))||FAILED(device->GetDepthStencilSurface(&depthSurface))||
           FAILED(depthSurface->GetDesc(&ds))||rt.Width!=width||rt.Height!=height||ds.Width!=width||ds.Height!=height){Reject(c,1,shader.hash,{rt.Width,rt.Height,ds.Width,ds.Height});return;}
        Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> vb;Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> ib;Microsoft::WRL::ComPtr<IDirect3DVertexDeclaration9> decl;
        UINT offset=0,stride=0,frequency=0,elements=MAXD3DDECLLENGTH+1;D3DINDEXBUFFER_DESC index{};D3DVERTEXELEMENT9 declaration[MAXD3DDECLLENGTH+1]{};
        if(FAILED(device->GetStreamSource(0,&vb,&offset,&stride))||!vb||!stride||FAILED(device->GetStreamSourceFreq(0,&frequency))||
           (frequency!=1&&frequency!=(D3DSTREAMSOURCE_INDEXEDDATA|1))){Reject(c,2,shader.hash,{offset,stride,frequency,vb?1u:0u});return;}
        if(FAILED(device->GetIndices(&ib))||!ib||FAILED(ib->GetDesc(&index))||FAILED(device->GetVertexDeclaration(&decl))||!decl||
           FAILED(decl->GetDeclaration(declaration,&elements))||!elements||elements>MAXD3DDECLLENGTH+1){Reject(c,3,shader.hash,{ib?1u:0u,decl?1u:0u,elements,unsigned(index.Format)});return;}
        // Native lighting appends COLOR0/D3DCOLOR in stream1 to the seven
        // stream0 skin elements. The four whitelisted shaders derive clip
        // position only from POSITION/BLENDWEIGHT/BLENDINDICES in stream0.
        // Lighting-buffer rotation must not break geometry correspondence.
        for(unsigned i=0;i+1<elements;++i){auto const& element=declaration[i];
            if(element.Stream&&(element.Stream>=16||element.Usage!=D3DDECLUSAGE_COLOR||element.Type!=D3DDECLTYPE_D3DCOLOR||element.Method!=D3DDECLMETHOD_DEFAULT)){
                Reject(c,4,shader.hash,{i,element.Stream,element.Offset,element.Type,element.Usage});return;}}
        auto constantResult=device->GetVertexShaderConstantF(0,constants.data(),256);
        if(FAILED(constantResult)){Reject(c,5,shader.hash,{uint32_t(constantResult)});return;}
        auto const& object=scopes[depth-1];std::copy_n(constants.begin(),768,pose.bones.begin());
        std::copy_n(constants.begin()+shader.layout.projectionRegister*4,16,pose.clip.begin());pose.world=object.world;
        diagnostic::temporal::DrawKey key;key.id=object.id;key.context=object.context;key.visual=object.visual;
        key.vertices=reinterpret_cast<uintptr_t>(vb.Get());key.indices=reinterpret_cast<uintptr_t>(ib.Get());key.shader=shader.hash;
        key.declaration=Hash(declaration,elements*sizeof(D3DVERTEXELEMENT9));key.offset=offset;key.stride=stride;key.indexFormat=index.Format;key.draw=geometry;
        if(buffers){
            if(!buffers->Observe(vb.Get())||!buffers->Observe(ib.Get())){Reject(c,2,shader.hash);return;}
            auto version=buffers->Version(vb.Get(),ib.Get());if(!version.valid){Reject(c,2,shader.hash);return;}
            key.geometryVersion={version.vertexObject,version.vertexContent,version.indexObject,version.indexContent};
        }
        key.target=reinterpret_cast<uintptr_t>(target.Get());key.depth=reinterpret_cast<uintptr_t>(depthSurface.Get());key.pass={write,zfunc,cull,alphaTest,color};
        auto entry=history.Add(unsigned(eye),key,pose);
        if(entry&&!snapshots[unsigned(eye)].attempted&&viewport.MinZ==0&&viewport.MaxZ==1){
            bool position=false,weights=false,indices=false;for(unsigned i=0;i+1<elements;++i){auto const& e=declaration[i];if(e.Stream||e.UsageIndex||e.Method)continue;
                position|=e.Usage==D3DDECLUSAGE_POSITION&&e.Type==D3DDECLTYPE_FLOAT3&&e.Offset==0;
                weights|=e.Usage==D3DDECLUSAGE_BLENDWEIGHT&&e.Type==D3DDECLTYPE_FLOAT4&&e.Offset==12;
                indices|=e.Usage==D3DDECLUSAGE_BLENDINDICES&&e.Type==D3DDECLTYPE_UBYTE4&&e.Offset==28;}
            if(position&&weights&&indices){DWORD bias=0,slope=0;
                if(SUCCEEDED(device->GetRenderState(D3DRS_DEPTHBIAS,&bias))&&SUCCEEDED(device->GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,&slope))){
                    unsigned slot=unsigned(entry-history.Current(unsigned(eye)).entries.data());auto& r=resources[unsigned(eye)][slot];r.vertices=vb;r.indices=ib;memcpy(&r.bias,&bias,4);memcpy(&r.slope,&slope,4);}}
        }
    }
    unsigned width=0,height=0;
    void CaptureRaster(IDirect3DDevice9* device,IDirect3DSurface9* opaqueDepth)noexcept {
        if(eye<0||!device||!opaqueDepth)return;auto index=unsigned(eye);auto& snapshot=snapshots[index];if(snapshot.attempted)return;
        auto const& frame=history.Current(index);unsigned count=0,ready=0;
        for(unsigned i=0;i<frame.count;++i){auto const& e=frame.entries[i];auto const& r=resources[index][i];
            if(e.ambiguous||!r.vertices||!r.indices||e.key.depth!=reinterpret_cast<uintptr_t>(opaqueDepth))continue;
            if(buffers){auto v=buffers->Version(r.vertices.Get(),r.indices.Get());if(!v.valid||e.key.geometryVersion!=std::array<uint64_t,4>{v.vertexObject,v.vertexContent,v.indexObject,v.indexContent})continue;}
            auto& d=jobs[count++];d.vertices=r.vertices;d.indices=r.indices;d.geometry=e.key.draw;d.offset=e.key.offset;d.stride=e.key.stride;d.cull=e.key.pass[2];d.depthBias=r.bias;d.slopeBias=r.slope;
            d.current=e.pose;d.historyValid=e.previous>=0&&rasterFrames[index]&&rasterFrames[index]+1==frame.serial;if(d.historyValid){d.previous=history.Previous(index).entries[unsigned(e.previous)].pose;++ready;}else d.previous=e.pose;
        }
        snapshot.attempted=true;snapshot.frame=frame.serial;
        try{if(!snapshot.renderer){snapshot.renderer=std::make_unique<D3D9SkinnedMotion>();snapshot.renderer->Initialize(device,width,height);}snapshot.report=snapshot.renderer->Render(opaqueDepth,jobs.data(),count,currentJitter[index],previousJitter[index]);rasterFrames[index]=frame.serial;}
        catch(std::exception const& e){try{snapshot.error=e.what();}catch(...){}snapshot.renderer.reset();}
        catch(...){snapshot.renderer.reset();}
        for(unsigned i=0;i<count;++i){jobs[i].vertices.Reset();jobs[i].indices.Reset();}
        for(auto& r:resources[index])r={};
    }
    bool WriteRaster(std::ostream& out,unsigned index,std::filesystem::path const& directory,std::string const& prefix){
        auto& s=snapshots[index];if(!s.attempted||s.frame!=history.Current(index).serial){out<<"null";return false;}
        if(!s.writeAttempted){s.writeAttempted=true;s.file=prefix+"-motion-"+std::to_string(s.frame)+"-eye"+std::to_string(index)+".rgba16f";
            if(s.renderer)try{Microsoft::WRL::ComPtr<IDirect3DDevice9> d;Microsoft::WRL::ComPtr<IDirect3DSurface9> staging;
                auto check=[](HRESULT hr,char const* why){if(FAILED(hr))throw std::runtime_error(why);};
                check(s.renderer->Texture()->GetDevice(&d),"motion readback device");check(d->CreateOffscreenPlainSurface(width,height,D3DFMT_A16B16G16R16F,D3DPOOL_SYSTEMMEM,&staging,nullptr),"motion diagnostic staging allocation");
                check(d->GetRenderTargetData(s.renderer->Surface(),staging.Get()),"motion diagnostic readback");D3DLOCKED_RECT map{};check(staging->LockRect(&map,nullptr,D3DLOCK_READONLY),"motion diagnostic map");
                struct Unlock {IDirect3DSurface9* surface;~Unlock(){surface->UnlockRect();}} unlock{staging.Get()};
                if(map.Pitch<int(width*8))throw std::runtime_error("motion diagnostic row pitch");
                std::ofstream file(directory/s.file,std::ios::binary);for(unsigned y=0;y<height;++y)file.write(static_cast<char const*>(map.pBits)+y*map.Pitch,width*8);file.close();s.written=bool(file);if(!s.written)s.error="motion file write failed";
            }catch(std::exception const& e){s.error=e.what();}}
        out<<"{\"xr_frame_serial\":"<<s.frame<<",\"eye\":"<<index<<",\"width\":"<<width<<",\"height\":"<<height<<",\"file\":"<<std::quoted(s.file)<<",\"file_written\":"<<(s.written?"true":"false")
           <<",\"encoding\":\"RGBA16F_little_endian_top_left\",\"rg_units\":\"previous_minus_current_pixels_unjittered\",\"b_channel\":\"supported_skin_geometry_occupied\",\"a_channel\":\"previous_skin_history_valid\",\"draws\":"<<s.report.draws
           <<",\"history_draws\":"<<s.report.historyDraws<<",\"previous_palette_upload_bytes\":"<<s.report.paletteBytes<<",\"source\":\"native_game_indexed_geometry_gpu_replay\",\"scene_motion_coverage_complete\":false,\"game_dlaa_enabled\":false,\"error\":"<<std::quoted(s.error)<<'}';return s.written;
    }
    IDirect3DSurface9* Surface(unsigned index)const noexcept{return index<2&&snapshots[index].attempted&&snapshots[index].renderer?snapshots[index].renderer->Surface():nullptr;}
    void Write(std::ostream& out,unsigned index)const {
        auto const& f=history.Current(index);auto const& previous=history.Previous(index);auto const& c=counts[index];
        out<<"{\"schema\":2,\"epoch\":"<<f.epoch<<",\"maximum_history_gap_ms\":250,\"diagnostic_only\":true,\"raster_motion_vectors\":false,\"coverage_complete\":false,\"position_skin_stream\":0,\"auxiliary_d3dcolor_streams_allowed\":true,\"scoped_indexed_draws\":"<<c.scopedDraws
           <<",\"queried_draws\":"<<c.queries<<",\"unsupported_shader_draws\":"<<c.unsupported<<",\"rejected_pass_draws\":"<<c.rejected
           <<",\"dropped_draws\":"<<f.dropped<<",\"invalid_poses\":"<<f.invalid<<",\"draw_limit\":"<<diagnostic::temporal::ObjectHistory::Limit<<",\"native_buffer_writes_observed\":"<<(buffers?"true":"false")<<",\"rejection_details\":[";
        bool first=true;for(unsigned i=0;i<c.reasons.size();++i){auto const& r=c.reasons[i];if(!r.count)continue;if(!first)out<<',';first=false;
            out<<"{\"reason\":\""<<RejectNames[i]<<"\",\"count\":"<<r.count<<",\"first_shader\":\""<<std::hex<<r.shader<<std::dec<<"\",\"first_values\":[";
            for(unsigned j=0;j<r.first.size();++j){if(j)out<<',';out<<r.first[j];}out<<"]}";}
        out<<"],\"draws\":[";
        for(unsigned i=0;i<f.count;++i){if(i)out<<',';auto const& e=f.entries[i];auto const& k=e.key;
            out<<"{\"entity_id\":[";for(unsigned j=0;j<5;++j){if(j)out<<',';out<<k.id[j];}
            out<<"],\"context\":"<<k.context<<",\"visual\":"<<k.visual<<",\"vertex_buffer\":"<<k.vertices<<",\"index_buffer\":"<<k.indices
               <<",\"shader_hash\":\""<<std::hex<<k.shader<<std::dec<<"\",\"declaration_hash\":\""<<std::hex<<k.declaration<<std::dec
               <<"\",\"offset\":"<<k.offset<<",\"stride\":"<<k.stride<<",\"index_format\":"<<k.indexFormat
               <<",\"render_target\":"<<k.target<<",\"depth_surface\":"<<k.depth<<",\"pass_state\":[";
            for(unsigned j=0;j<k.pass.size();++j){if(j)out<<',';out<<k.pass[j];}
            out<<']'
               <<",\"geometry_version\":["<<k.geometryVersion[0]<<','<<k.geometryVersion[1]<<','<<k.geometryVersion[2]<<','<<k.geometryVersion[3]<<']'
               <<",\"geometry\":["<<k.draw.primitive<<','<<k.draw.base<<','<<k.draw.minimum<<','<<k.draw.vertices<<','<<k.draw.first<<','<<k.draw.count
               <<"],\"observations\":"<<e.observations<<",\"history_status\":\""<<diagnostic::temporal::Reason(e.reason)<<"\",\"current\":";Pose(out,e.pose);
            if(e.previous>=0){out<<",\"previous_frame\":"<<previous.serial<<",\"previous\":";Pose(out,previous.entries[unsigned(e.previous)].pose);}out<<'}';
        }out<<"]}";
    }
};
}
