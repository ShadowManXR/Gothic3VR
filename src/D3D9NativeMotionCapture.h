#pragma once
#include "D3D9NativeMotion.h"
#include "D3D9SkinnedMotion.h"
#include "D3D9BufferObserver.h"
#include "D3D9ShaderSurvey.h"
#include "NativeShaderProfiles.h"
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace g3vr {
// Actual draw observer. It stays dormant outside an explicitly enabled temporal
// eye scope. Captures bounded native inputs; never claims full-scene coverage.
class D3D9NativeMotionCapture {
    using Kind=diagnostic::temporal::NativeGeometryKind;
    using Stamp=diagnostic::temporal::GeometryFrameStamp;
    using Version=diagnostic::temporal::GeometryVersion;
    using Entity=diagnostic::temporal::InstanceEntity;
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Entry {
        NativeMotionDraw draw;diagnostic::temporal::DrawKey key;
        Ptr<IDirect3DSurface9> target,depth;
        Version version;std::array<Version,15> auxiliary;
        diagnostic::temporal::InstanceRows rows;bool ambiguous=false;
        std::array<Entity,2> sourceEntities;unsigned sourceCount=0;
    };
    struct Frame {Stamp stamp;std::vector<Entry> entries;std::array<float,2> rasterJitter{};bool closed=false;};
    struct Eye {
        Frame current,previous;std::unique_ptr<D3D9NativeMotion> renderer;
        D3D9NativeMotion::Report raster;bool captured=false;std::string error;
    };
    struct Shader {Ptr<IDirect3DVertexShader9> value;uint64_t hash=0;int kind=-1;};
    struct Pixel {Ptr<IDirect3DPixelShader9> value;uint64_t hash=0;};
    struct Scope {unsigned first=0,count=0;};
    std::array<Eye,2> eyes;std::array<Shader,1024> shaders;unsigned shaderCount=0;
    std::array<Pixel,256> pixels;unsigned pixelCount=0;
    std::array<unsigned char,32768> code{};D3D9BufferObserver buffers;
    std::array<Entity,4096> entities;std::array<Scope,8> scopes;unsigned entityCount=0,scopeDepth=0;
    struct Declaration {uint64_t hash=0;Ptr<IDirect3DVertexDeclaration9> value;};
    std::array<Declaration,64> declarations;
    std::vector<NativeMotionDraw> jobs;int eye=-1;unsigned width=0,height=0;
    uint64_t epoch=1,lastFrame=0;
    std::unique_ptr<D3D9ShaderSurvey> survey;bool surveyEnabled=true;
    enum class Refusal:unsigned {Viewport,StateQuery,Depth,Color,Blend,Alpha,Stencil,Clip,Scissor,Count};
    void Reject(Refusal reason)noexcept{auto& count=stats[unsigned(eye)];++count.rejected;++count.refusal[unsigned(reason)];}
    Shader Describe(IDirect3DVertexShader9* value){
        for(unsigned i=0;i<shaderCount;++i)if(shaders[i].value.Get()==value)return shaders[i];
        if(!value||shaderCount==shaders.size())return {};
        Shader result;result.value=value;UINT bytes=0;
        if(SUCCEEDED(value->GetFunction(nullptr,&bytes))&&bytes&&bytes<=code.size()){
            UINT capacity=bytes;if(SUCCEEDED(value->GetFunction(code.data(),&capacity))&&capacity==bytes){
                result.hash=Hash(code.data(),bytes);if(auto profile=diagnostic::temporal::FindNativeShaderProfile(result.hash))result.kind=int(profile->kind);
            }
        }shaders[shaderCount++]=result;return result;
    }
    static uint64_t Hash(void const* value,unsigned bytes)noexcept{
        uint64_t h=14695981039346656037ull;auto p=static_cast<unsigned char const*>(value);
        for(unsigned i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;
    }
    bool AlphaMaterial(IDirect3DDevice9* d,uint64_t vertex,NativeMotionDraw& draw){
        if(vertex!=0x15e5561db503fe47ull)return false;
        Ptr<IDirect3DPixelShader9> ps;if(FAILED(d->GetPixelShader(&ps))||!ps)return false;uint64_t hash=0;
        for(unsigned i=0;i<pixelCount;++i)if(pixels[i].value==ps){hash=pixels[i].hash;break;}
        if(!hash){if(pixelCount==pixels.size())return false;UINT bytes=0;if(FAILED(ps->GetFunction(nullptr,&bytes))||!bytes||bytes>code.size())return false;UINT length=bytes;
            if(FAILED(ps->GetFunction(code.data(),&length))||length!=bytes)return false;hash=Hash(code.data(),bytes);pixels[pixelCount++]={ps,hash};}
        // This exact pair has alpha = saturate(diffuse s3.a * native fade).
        // Shader recognition is not permission to bypass other material tests.
        if(hash!=0x0cef3519ea7bd097ull)return false;
        DWORD func=0,wrap=0;if(FAILED(d->GetRenderState(D3DRS_ALPHAFUNC,&func))||func!=D3DCMP_GREATER||FAILED(d->GetRenderState(D3DRS_ALPHAREF,&draw.alphaReference))||draw.alphaReference>255||FAILED(d->GetRenderState(D3DRS_WRAP0,&wrap))||wrap)return false;
        Ptr<IDirect3DBaseTexture9> texture;if(FAILED(d->GetTexture(3,&texture))||!texture||FAILED(texture.As(&draw.alphaTexture)))return false;
        D3DSURFACE_DESC desc{};Ptr<IDirect3DDevice9> owner;if(FAILED(draw.alphaTexture->GetLevelDesc(0,&desc))||desc.Usage||FAILED(draw.alphaTexture->GetDevice(&owner))||owner.Get()!=d)return false;
        for(unsigned i=0;i<draw.alphaSampler.size();++i)if(FAILED(d->GetSamplerState(3,NativeMotionDraw::AlphaSamplerStates[i],&draw.alphaSampler[i])))return false;
        draw.masked=true;return true;
    }
    static auto IdSet(diagnostic::temporal::InstanceRows const& rows)noexcept {
        std::array<std::array<uint32_t,5>,17> ids{};unsigned count=0;
        for(unsigned i=0;i<17;++i)if(rows.identified[i])ids[count++]=rows.ids[i].value;
        std::sort(ids.begin(),ids.end());return ids;
    }
    static bool SameGeometry(Entry const& a,Entry const& b)noexcept{
        if(!(a.key==b.key)||!a.version.Same(b.version))return false;
        if(a.draw.masked!=b.draw.masked||a.draw.alphaTexture!=b.draw.alphaTexture||a.draw.alphaReference!=b.draw.alphaReference||a.draw.alphaSampler!=b.draw.alphaSampler)return false;
        if((a.draw.kind==Kind::InstancesA||a.draw.kind==Kind::InstancesB)&&IdSet(a.rows)!=IdSet(b.rows))return false;
        for(unsigned i=0;i<15;++i){auto const& x=a.draw.auxiliaryStreams[i];auto const& y=b.draw.auxiliaryStreams[i];
            if(bool(x.vertices)!=bool(y.vertices))return false;
            if(x.vertices&&(!a.auxiliary[i].Same(b.auxiliary[i])||x.offset!=y.offset||x.stride!=y.stride||x.frequency!=y.frequency))return false;
        }return true;
    }
    bool Unchanged(Entry const& entry){
        if(!entry.version.Same(buffers.Version(entry.draw.vertices.Get(),entry.draw.indices.Get())))return false;
        for(unsigned i=0;i<15;++i)if(entry.draw.auxiliaryStreams[i].vertices&&!entry.auxiliary[i].Same(buffers.Version(entry.draw.auxiliaryStreams[i].vertices.Get(),entry.draw.indices.Get())))return false;
        return true;
    }
    void ObserveImpl(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry){
        using namespace diagnostic::temporal;auto& output=eyes[unsigned(eye)].current;
        Ptr<IDirect3DVertexShader9> vs;if(FAILED(d->GetVertexShader(&vs)))return;auto shader=Describe(vs.Get());
        if(shader.kind<0){++stats[unsigned(eye)].unsupported;return;}
        if(output.entries.size()==D3D9NativeMotion::Limit){++stats[unsigned(eye)].overflow;return;}
        Entry entry;auto& draw=entry.draw;draw.kind=Kind(shader.kind);draw.geometry=geometry;
        D3DVIEWPORT9 viewport{};DWORD z=0,write=0,zfunc=0,color=0,blend=0,alpha=0,cull=0,bias=0,slope=0,stencil=0,scissor=0,clip=0;
        auto state=[&](D3DRENDERSTATETYPE type,DWORD& value){return SUCCEEDED(d->GetRenderState(type,&value));};
        if(FAILED(d->GetViewport(&viewport))||viewport.X||viewport.Y||viewport.Width!=width||viewport.Height!=height||viewport.MinZ!=0||viewport.MaxZ!=1){Reject(Refusal::Viewport);return;}
        if(!state(D3DRS_ZENABLE,z)||!state(D3DRS_ZWRITEENABLE,write)||!state(D3DRS_ZFUNC,zfunc)||
           !state(D3DRS_COLORWRITEENABLE,color)||!state(D3DRS_ALPHABLENDENABLE,blend)||!state(D3DRS_ALPHATESTENABLE,alpha)||
           !state(D3DRS_CULLMODE,cull)||!state(D3DRS_DEPTHBIAS,bias)||!state(D3DRS_SLOPESCALEDEPTHBIAS,slope)||
           !state(D3DRS_STENCILENABLE,stencil)||!state(D3DRS_CLIPPLANEENABLE,clip)||!state(D3DRS_SCISSORTESTENABLE,scissor)){Reject(Refusal::StateQuery);return;}
        if(!z||(zfunc!=D3DCMP_LESSEQUAL&&zfunc!=D3DCMP_EQUAL)){Reject(Refusal::Depth);return;}
        if((color&7)!=7){Reject(Refusal::Color);return;}if(blend){Reject(Refusal::Blend);return;}if(alpha&&!AlphaMaterial(d,shader.hash,draw)){Reject(Refusal::Alpha);return;}
        if(stencil){
            // Native opaque passes often write stencil with an ALWAYS test.
            // This cannot discard a fragment. Replay tests final opaque Z and
            // disables stencil writes; the original draw still owns stencil.
            // Unknown/two-sided filtering remains rejected, never inferred.
            DWORD func=0,twoSided=0,backFunc=D3DCMP_ALWAYS;
            if(!state(D3DRS_STENCILFUNC,func)||!state(D3DRS_TWOSIDEDSTENCILMODE,twoSided)||
               (twoSided&&!state(D3DRS_CCW_STENCILFUNC,backFunc))){Reject(Refusal::StateQuery);return;}
            if(func!=D3DCMP_ALWAYS||backFunc!=D3DCMP_ALWAYS){Reject(Refusal::Stencil);return;}
        }
        if(clip){Reject(Refusal::Clip);return;}
        if(scissor){RECT rect{};if(FAILED(d->GetScissorRect(&rect))||rect.left>0||rect.top>0||rect.right<LONG(width)||rect.bottom<LONG(height)){Reject(Refusal::Scissor);return;}}
        Ptr<IDirect3DSurface9> target,depth;D3DSURFACE_DESC rt{},ds{};
        if(FAILED(d->GetRenderTarget(0,&target))||!target||FAILED(target->GetDesc(&rt))||FAILED(d->GetDepthStencilSurface(&depth))||!depth||FAILED(depth->GetDesc(&ds))||
           rt.Width!=width||rt.Height!=height||ds.Width!=width||ds.Height!=height){++stats[unsigned(eye)].rejected;return;}
        Ptr<IDirect3DVertexDeclaration9> nativeDecl;D3DVERTEXELEMENT9 elements[MAXD3DDECLLENGTH+1]{};UINT count=MAXD3DDECLLENGTH+1;
        if(FAILED(d->GetVertexDeclaration(&nativeDecl))||!nativeDecl||FAILED(nativeDecl->GetDeclaration(elements,&count))||!count||count>MAXD3DDECLLENGTH+1||
           FAILED(d->GetStreamSource(0,&draw.vertices,&draw.offset,&draw.stride))||!draw.vertices||!draw.stride||FAILED(d->GetStreamSourceFreq(0,&draw.frequency))||
           (draw.frequency!=1&&draw.frequency!=(D3DSTREAMSOURCE_INDEXEDDATA|1))||FAILED(d->GetIndices(&draw.indices))||!draw.indices){++stats[unsigned(eye)].rejected;return;}
        D3DVERTEXELEMENT9 minimal[4]{};unsigned used=0;bool position=false,auxiliary=shader.kind==0,uv=!draw.masked;
        for(unsigned i=0;i+1<count;++i){auto const& e=elements[i];if(e.Method||e.UsageIndex>7)continue;
            bool p=e.Usage==D3DDECLUSAGE_POSITION&&e.UsageIndex==0;
            bool a=shader.kind!=0&&e.Usage==D3DDECLUSAGE_TEXCOORD&&e.UsageIndex==(shader.kind==3?1:4);
            bool t=draw.masked&&e.Usage==D3DDECLUSAGE_TEXCOORD&&e.UsageIndex==0;
            if(!p&&!a&&!t)continue;
            if(used==3||(p&&(position||e.Stream||e.Type!=D3DDECLTYPE_FLOAT3))||
               (a&&(auxiliary||e.Stream>=16||e.Type>D3DDECLTYPE_FLOAT4))||(t&&(uv||e.Stream>=16||e.Type!=D3DDECLTYPE_FLOAT2))){++stats[unsigned(eye)].rejected;return;}
            minimal[used++]=e;position|=p;auxiliary|=a;uv|=t;
            if(e.Stream){auto& s=draw.auxiliaryStreams[e.Stream-1];
                if(FAILED(d->GetStreamSource(e.Stream,&s.vertices,&s.offset,&s.stride))||!s.vertices||!s.stride||FAILED(d->GetStreamSourceFreq(e.Stream,&s.frequency))||s.frequency!=1||!buffers.Observe(s.vertices.Get())){++stats[unsigned(eye)].rejected;return;}
                entry.auxiliary[e.Stream-1]=buffers.Version(s.vertices.Get(),draw.indices.Get());
            }
        }
        if(!position||!auxiliary||!uv||!buffers.Observe(draw.vertices.Get())||!buffers.Observe(draw.indices.Get())){++stats[unsigned(eye)].rejected;return;}
        // An auxiliary version could only be queried after the IB was observed.
        for(unsigned i=0;i<15;++i)if(draw.auxiliaryStreams[i].vertices)entry.auxiliary[i]=buffers.Version(draw.auxiliaryStreams[i].vertices.Get(),draw.indices.Get());
        entry.version=buffers.Version(draw.vertices.Get(),draw.indices.Get());if(!entry.version.valid)return;
        minimal[used]=D3DDECL_END();
        auto declarationHash=Hash(minimal,(used+1)*sizeof(minimal[0]));
        for(auto const& cached:declarations)if(cached.value&&cached.hash==declarationHash){draw.declaration=cached.value;break;}
        if(!draw.declaration){for(auto& cached:declarations)if(!cached.value){
            if(FAILED(d->CreateVertexDeclaration(minimal,&cached.value)))break;cached.hash=declarationHash;draw.declaration=cached.value;break;
        }}
        auto profile=FindNativeShaderProfile(shader.hash);std::array<float,256*4> constants{};
        if(!draw.declaration||!profile||FAILED(d->GetVertexShaderConstantF(0,constants.data(),profile->canonical?83:256))||
           !NormalizeNativePose(*profile,constants.data(),profile->canonical?83:256,draw.current)){++stats[unsigned(eye)].rejected;return;}
        draw.cull=cull;memcpy(&draw.depthBias,&bias,4);memcpy(&draw.slopeBias,&slope,4);
        entry.target=target;entry.depth=depth;
        auto& key=entry.key;key.shader=shader.hash;key.declaration=Hash(minimal,(used+1)*sizeof(minimal[0]));key.vertices=reinterpret_cast<uintptr_t>(draw.vertices.Get());key.indices=reinterpret_cast<uintptr_t>(draw.indices.Get());
        key.offset=draw.offset;key.stride=draw.stride;key.target=reinterpret_cast<uintptr_t>(target.Get());key.depth=reinterpret_cast<uintptr_t>(depth.Get());key.draw=geometry;key.pass={write,zfunc,cull,alpha,color};
        Scope scope=scopeDepth&&scopeDepth<=scopes.size()?scopes[scopeDepth-1]:Scope{};
        entry.sourceCount=scope.count;for(unsigned i=0;i<std::min(scope.count,2u);++i)entry.sourceEntities[i]=entities[scope.first+i];
        if(shader.kind==1||shader.kind==2){entry.rows=IdentifyInstances(draw.current,entities.data()+scope.first,scope.count);
            // The same native pool can issue several groups of 17 entities.
            // Distinguish their unordered ID sets without depending on sort
            // order. Exact sets are compared again when proving history.
            auto ids=IdSet(entry.rows);auto identity=Hash(ids.data(),unsigned(sizeof(ids)));key.context=uintptr_t(identity);key.visual=uintptr_t(identity>>32);
        }
        if(shader.kind==0){if(scope.count!=1||!entities[scope.first].id.Valid()){++stats[unsigned(eye)].unidentified;return;}key.id=entities[scope.first].id.value;}
        draw.previous=draw.current;
        for(auto const& prior:output.entries)if(prior.key==key){
            ++stats[unsigned(eye)].duplicates;
            // Repeated identical passes contribute once. Changing geometry or
            // transforms under the same key invalidates BOTH observations.
            bool same=SameGeometry(prior,entry)&&prior.draw.current.constants==draw.current.constants;
            if(same)return;
            for(auto& old:output.entries)if(old.key==key)old.ambiguous=true;
            entry.ambiguous=true;break;
        }
        if(draw.masked)++stats[unsigned(eye)].masked;
        output.entries.push_back(std::move(entry));++stats[unsigned(eye)].accepted;
    }
public:
    void SetSurveyEnabled(bool enabled)noexcept{surveyEnabled=enabled;}
    struct Counters {unsigned indexed=0,accepted=0,unsupported=0,rejected=0,unidentified=0,overflow=0,duplicates=0,history=0,masked=0;std::array<unsigned,unsigned(Refusal::Count)> refusal{};};
    std::array<Counters,2> stats;
    void Prepare(){for(auto& e:eyes){e.current.entries.reserve(D3D9NativeMotion::Limit);e.previous.entries.reserve(D3D9NativeMotion::Limit);}jobs.reserve(D3D9NativeMotion::Limit);if(!survey)survey=std::make_unique<D3D9ShaderSurvey>();}
    void Begin(unsigned index,uint64_t serial,uint64_t displayTime,unsigned w,unsigned h,std::array<float,2> jitter={}){
        Stop();if(index>1)return;if(!survey||!jobs.capacity())Prepare();
        if((width&&width!=w)||(height&&height!=h)||serial<lastFrame){Reset();Prepare();}
        width=w;height=h;lastFrame=serial;buffers.Begin(serial);eye=int(index);entityCount=scopeDepth=0;
        auto& e=eyes[index];std::swap(e.current,e.previous);e.current.entries.clear();e.current.closed=false;e.current.stamp={serial,epoch,displayTime,index};e.current.rasterJitter=jitter;e.captured=false;e.error.clear();e.raster={};stats[index]={};
        if(surveyEnabled)survey->Begin(index,serial,w,h);
    }
    void Stop()noexcept {eye=-1;scopeDepth=entityCount=0;}
    unsigned Push(Entity const* values,unsigned count)noexcept{
        if(eye<0)return UINT_MAX;unsigned token=scopeDepth++;
        if(token>=scopes.size())return token;
        if((count&&!values)||count>1024||count>entities.size()-entityCount){scopes[token]={};return token;}
        scopes[token]={entityCount,count};for(unsigned i=0;i<count;++i)entities[entityCount++]=values[i];return token;
    }
    void Pop(unsigned token)noexcept {if(token==UINT_MAX||token>=scopeDepth)return;if(token<scopes.size())entityCount=scopes[token].first;scopeDepth=token;}
    void SurveyDraw(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry,unsigned kind)noexcept{
        if(!surveyEnabled||eye<0||!d||D3D9NativeMotion::Replaying()||D3D9SkinnedMotion::Replaying())return;
        try{Scope scope=scopeDepth&&scopeDepth<=scopes.size()?scopes[scopeDepth-1]:Scope{};if(survey)survey->Observe(d,geometry,eyes[unsigned(eye)].captured,entities.data()+scope.first,scope.count,kind);}catch(...){}
    }
    void Observe(IDirect3DDevice9* d,diagnostic::temporal::Geometry const& geometry)noexcept{
        if(eye<0||!d||D3D9NativeMotion::Replaying()||D3D9SkinnedMotion::Replaying())return;
        SurveyDraw(d,geometry,1);
        if(eyes[unsigned(eye)].captured)return;
        ++stats[unsigned(eye)].indexed;try{ObserveImpl(d,geometry);}catch(...){++stats[unsigned(eye)].rejected;}
    }
    void Capture(IDirect3DDevice9* d,IDirect3DSurface9* opaqueDepth)noexcept{
        if(eye<0||!d||!opaqueDepth)return;auto index=unsigned(eye);auto& e=eyes[index];if(e.captured)return;
        try{
            jobs.clear();
            for(auto& current:e.current.entries){
                if(current.ambiguous||current.key.depth!=reinterpret_cast<uintptr_t>(opaqueDepth)||!Unchanged(current))continue;
                auto& draw=current.draw;draw.historyValid=false;draw.instanceHistory={};draw.previous=draw.current;
                Entry const* old=nullptr;
                if(e.previous.closed&&e.current.stamp.Follows(e.previous.stamp))for(auto const& before:e.previous.entries)if(!before.ambiguous&&SameGeometry(current,before)){
                    if(old){old=nullptr;break;}old=&before;
                }
                if(old){
                    if(draw.kind==Kind::InstancesA||draw.kind==Kind::InstancesB){
                        auto proof=diagnostic::temporal::RebaseInstances(draw.current,current.rows,old->draw.current,old->rows,current.version,old->version,e.current.stamp,e.previous.stamp);
                        draw.previous=proof.previous;draw.instanceHistory=proof.valid;
                        for(auto flag:proof.valid)draw.historyValid|=flag!=0;
                    }else{draw.previous=old->draw.current;draw.historyValid=true;
                        if(draw.kind==Kind::RigidWorld){double distance=0;for(unsigned c=0;c<3;++c){double delta=draw.current.constants[(4+c)*4+3]-draw.previous.constants[(4+c)*4+3];distance+=delta*delta;}draw.historyValid=distance<=1000.*1000.;}
                    }
                }
                if(draw.historyValid)++stats[index].history;jobs.push_back(draw);
            }
            if(!e.renderer){e.renderer=std::make_unique<D3D9NativeMotion>();e.renderer->Initialize(d,width,height);}
            e.raster=e.renderer->Render(opaqueDepth,jobs.data(),unsigned(jobs.size()),e.current.rasterJitter,e.previous.rasterJitter);e.captured=e.current.closed=true;jobs.clear();
        }catch(std::exception const& error){e.error=error.what();jobs.clear();}catch(...){e.error="Native motion capture failed";jobs.clear();}
    }
    D3D9NativeMotion* Renderer(unsigned index)const noexcept{return index<2&&eyes[index].captured?eyes[index].renderer.get():nullptr;}
    Stamp CurrentStamp(unsigned index)const noexcept{return index<2?eyes[index].current.stamp:Stamp{};}
    D3D9BufferObserver* Buffers()noexcept{return &buffers;}
    void Write(std::ostream& out,unsigned index,std::filesystem::path const& directory,std::string const& prefix,bool pixels=true){
        if(index>1){out<<"null";return;}auto& e=eyes[index];auto const& c=stats[index];bool saved=false;
        std::string name=prefix+"-native-motion-"+std::to_string(e.current.stamp.frame)+"-eye"+std::to_string(index)+".rgba16f";
        if(pixels&&e.captured&&e.renderer)try{
            Ptr<IDirect3DDevice9> d;Ptr<IDirect3DSurface9> staging;auto check=[](HRESULT hr){if(FAILED(hr))throw std::runtime_error("native motion readback");};
            check(e.renderer->Texture()->GetDevice(&d));check(d->CreateOffscreenPlainSurface(width,height,D3DFMT_A16B16G16R16F,D3DPOOL_SYSTEMMEM,&staging,nullptr));
            check(d->GetRenderTargetData(e.renderer->Surface(),staging.Get()));D3DLOCKED_RECT lock{};check(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY));
            struct Unlock{IDirect3DSurface9* p;~Unlock(){p->UnlockRect();}} unlock{staging.Get()};
            if(lock.Pitch<int(width*8))throw std::runtime_error("native motion pitch");std::ofstream file(directory/name,std::ios::binary);
            for(unsigned y=0;y<height;++y)file.write(static_cast<char const*>(lock.pBits)+y*lock.Pitch,width*8);file.close();saved=bool(file);
        }catch(std::exception const& error){e.error=error.what();}
        out<<"{\"source\":\"native_game_draw_observer\",\"frame\":"<<e.current.stamp.frame<<",\"eye\":"<<index<<",\"indexed_draws\":"<<c.indexed<<",\"accepted\":"<<c.accepted<<",\"masked_material_draws\":"<<c.masked<<",\"unsupported\":"<<c.unsupported<<",\"rejected\":"<<c.rejected<<",\"unidentified\":"<<c.unidentified<<",\"overflow\":"<<c.overflow<<",\"duplicates\":"<<c.duplicates<<",\"history_draws\":"<<c.history<<",\"raster_draws\":"<<e.raster.draws<<",\"file\":"<<std::quoted(name)<<",\"file_written\":"<<(saved?"true":"false")<<",\"width\":"<<width<<",\"height\":"<<height<<",\"scene_motion_coverage_complete\":false,\"game_dlaa_enabled\":false,\"error\":"<<std::quoted(e.error)<<",\"sample_draws\":[";
        auto array=[&](auto const& values){out<<'[';bool first=true;for(auto value:values){if(!first)out<<',';first=false;out<<+value;}out<<']';};
        for(unsigned i=0;i<std::min(size_t(8),e.current.entries.size());++i){if(i)out<<',';auto const& entry=e.current.entries[i];
            out<<"{\"kind\":"<<unsigned(entry.draw.kind)<<",\"ambiguous\":"<<(entry.ambiguous?"true":"false")<<",\"history_valid\":"<<(entry.draw.historyValid?"true":"false")<<",\"source_entity_count\":"<<entry.sourceCount<<",\"buffer_version\":["<<entry.version.vertexObject<<','<<entry.version.vertexContent<<','<<entry.version.indexObject<<','<<entry.version.indexContent<<"],\"identified_instance_rows\":";array(entry.rows.identified);
            out<<",\"instance_ids\":[";for(unsigned row=0;row<17;++row){if(row)out<<',';array(entry.rows.ids[row].value);}out<<"],\"current_constants_c0_c82\":";array(entry.draw.current.constants);
            out<<",\"sdk_world_samples\":[";for(unsigned j=0;j<std::min(2u,entry.sourceCount);++j){if(j)out<<',';out<<"{\"id\":";array(entry.sourceEntities[j].id.value);out<<",\"world\":";array(entry.sourceEntities[j].world);out<<'}';}out<<"]}";
        }out<<"],\"shader_cache_limit\":"<<shaders.size()<<",\"shader_cache_entries\":"<<shaderCount<<",\"rejection_reasons\":{";static constexpr char const* reasons[]={"viewport","state_query","depth_mode","colorless_pass","alpha_blend","alpha_test","stencil","clip_planes","partial_scissor"};unsigned classified=0;
        for(unsigned i=0;i<c.refusal.size();++i){if(i)out<<',';out<<std::quoted(reasons[i])<<':'<<c.refusal[i];classified+=c.refusal[i];}out<<",\"other\":"<<(c.rejected-classified)<<"},\"full_shader_survey\":";if(survey)survey->Write(out,index,directory,pixels);else out<<"null";out<<'}';
    }
    void Reset()noexcept {
        Stop();jobs.clear();for(auto& e:eyes){e.current.entries.clear();e.previous.entries.clear();e.current.stamp=e.previous.stamp={};e.renderer.reset();e.captured=false;}
        for(auto& s:shaders)s={};for(auto& p:pixels)p={};pixelCount=0;for(auto& d:declarations)d={};shaderCount=0;survey.reset();buffers.Reset();++epoch;width=height=0;lastFrame=0;
    }
    ~D3D9NativeMotionCapture(){Reset();}
};
}
