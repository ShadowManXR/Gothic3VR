#include "NativeMaterialCapture.h"
#include "../../../src/NativeShaderProfiles.h"
#include <wrl/client.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
namespace {
constexpr D3DRENDERSTATETYPE States[]={D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,D3DRS_COLORWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_ALPHAREF,D3DRS_CULLMODE,D3DRS_SRGBWRITEENABLE,D3DRS_DEPTHBIAS,D3DRS_SLOPESCALEDEPTHBIAS};
constexpr char const* StateNames[]={"z_enable","z_write","z_func","color_write","alpha_blend","alpha_test","alpha_ref","cull","srgb_write","depth_bias_bits","slope_bias_bits"};
constexpr D3DSAMPLERSTATETYPE Samplers[]={D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW,D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER,D3DSAMP_SRGBTEXTURE,D3DSAMP_MAXANISOTROPY,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_MAXMIPLEVEL};
template<size_t N>bool Copy(char (&out)[N],char const* text)noexcept{out[0]=0;if(!text)return false;size_t n=0;while(n<N&&text[n])++n;if(!n||n==N)return false;memcpy(out,text,n+1);return true;}
template<class T,size_t N>void Values(std::ostream& out,std::array<T,N> const& data){out<<'[';for(size_t i=0;i<N;++i){if(i)out<<',';if(std::isfinite(double(data[i])))out<<data[i];else out<<"null";}out<<']';}
template<class Shader>std::string SaveShader(Shader* shader,std::filesystem::path const& directory,char const* stage){
    if(!shader)throw std::runtime_error("No captured shader");UINT bytes=0;if(FAILED(shader->GetFunction(nullptr,&bytes))||bytes<8||bytes>32768||bytes%4)throw std::runtime_error("Shader evidence size");std::array<unsigned char,32768> data{};UINT n=bytes;if(FAILED(shader->GetFunction(data.data(),&n))||n!=bytes)throw std::runtime_error("Shader evidence read");
    uint64_t hash=14695981039346656037ull;for(unsigned i=0;i<bytes;++i){hash^=data[i];hash*=1099511628211ull;}std::ostringstream name;name<<stage<<'-'<<std::hex<<hash<<".cso";auto path=directory/name.str();std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<char const*>(data.data()),bytes);if(!file)throw std::runtime_error("Shader evidence write");return name.str();
}
}
struct NativeMaterialCapture::Impl {
    struct Scope {unsigned eye=2;uint64_t frame=0;char mesh[1024]{},material[1024]{};bool deferred=false;std::array<float,16> worldCm{};NativeLightmapSource lightmap;};
    struct Entry : NativeMaterialDraw {Scope scope;std::array<BOOL,16> vb{},pb{};std::array<int,64> vi{},pi{};bool extraConstantsValid=false;ComPtr<IDirect3DVertexShader9> vs;ComPtr<IDirect3DPixelShader9> ps;bool written=false;};
    struct Program {ComPtr<IUnknown> shader;uint64_t hash=0;};
    std::array<Program,64> programs;unsigned programCount=0;std::array<unsigned char,32768> code{};
    uint64_t scopes=0,invalidNames=0,deferredScopes=0,observed=0,scopedDraws=0,wrongViewport=0,depthOnly=0,shaderQueryFailed=0,unknownProgram=0,matrixMismatch=0;
    Entry current;bool currentValid=false,continuous=false;std::array<Entry,16> entries;unsigned count=0,width=0,height=0,depth=0;std::array<Scope,8> stack;std::filesystem::path directory;std::ofstream log;
    bool Has(Scope const& scope)const noexcept{for(unsigned i=0;i<count;++i)if(entries[i].scope.eye==scope.eye&&!strcmp(entries[i].scope.mesh,scope.mesh)&&!strcmp(entries[i].scope.material,scope.material))return true;return false;}
    template<class Shader>uint64_t Hash(Shader* shader)noexcept{
        for(unsigned i=0;i<programCount;++i)if(programs[i].shader.Get()==shader)return programs[i].hash;
        if(!shader||programCount==programs.size())return 0;UINT size=0;uint64_t hash=0;
        if(SUCCEEDED(shader->GetFunction(nullptr,&size))&&size>=8&&size<=code.size()&&size%4==0){UINT bytes=size;if(SUCCEEDED(shader->GetFunction(code.data(),&bytes))&&bytes==size){hash=14695981039346656037ull;for(unsigned i=0;i<size;++i){hash^=code[i];hash*=1099511628211ull;}}}
        programs[programCount++]={shader,hash};return hash;
    }
    bool MatchDeferred(IDirect3DDevice9* d,Entry& entry)noexcept{
        using namespace diagnostic::temporal;
        entry.vertexHash=Hash(entry.vs.Get());auto* profile=FindNativeShaderProfile(entry.vertexHash);
        if(!profile||profile->kind==NativeGeometryKind::Wind){++unknownProgram;return false;}
        NativeGeometryPose pose;if(!NormalizeNativePose(*profile,entry.vertex.data(),256,pose)){++unknownProgram;return false;}
        bool rigid=profile->kind==NativeGeometryKind::RigidWorld;unsigned rows=1;
        if(!rigid){UINT frequency=0;if(FAILED(d->GetStreamSourceFreq(0,&frequency))||!(frequency&D3DSTREAMSOURCE_INDEXEDDATA)||!(frequency&0x3fffffffu)||(frequency&0x3fffffffu)>17){++matrixMismatch;return false;}rows=frequency&0x3fffffffu;}
        int found=-1;
        for(unsigned row=0;row<rows;++row){bool equal=true;unsigned first=rigid?4:row*3;
            for(unsigned r=0;r<3&&equal;++r)for(unsigned c=0;c<4;++c)if(entry.scope.worldCm[c*4+r]!=pose.constants[(first+r)*4+c]){equal=false;break;}
            if(equal){if(found!=-1){++matrixMismatch;return false;}found=int(row);}}
        if(found<0){++matrixMismatch;return false;}entry.matchedRow=found;return true;
    }
};
NativeMaterialCapture::NativeMaterialCapture()=default;NativeMaterialCapture::~NativeMaterialCapture()=default;
void NativeMaterialCapture::Initialize(std::filesystem::path const& directory,unsigned w,unsigned h,bool continuous){
    auto p=std::make_unique<Impl>();p->continuous=continuous;p->directory=directory;p->width=w;p->height=h;std::filesystem::create_directories(directory);p->log.open(directory/L"materials.jsonl",std::ios::trunc);if(!p->log)throw std::runtime_error("Cannot open native material evidence");p->log<<std::setprecision(9);impl=std::move(p);
}
unsigned NativeMaterialCapture::Begin(unsigned eye,uint64_t frame,char const* mesh,char const* material,float const* worldCm,NativeLightmapSource lightmap)noexcept{
    if(!impl)return UINT_MAX;auto& p=*impl;unsigned token=p.depth++;if(token>=p.stack.size())return token;auto& s=p.stack[token];s={};
    ++p.scopes;if(eye>1||!frame||(!p.continuous&&p.count==p.entries.size()))return token;if(!Copy(s.mesh,mesh)||!Copy(s.material,material)){++p.invalidNames;return token;}
    if(worldCm){++p.deferredScopes;s.deferred=true;std::copy_n(worldCm,16,s.worldCm.begin());for(float f:s.worldCm)if(!std::isfinite(f))return token;if(s.worldCm[3]!=0||s.worldCm[7]!=0||s.worldCm[11]!=0||s.worldCm[15]!=1)return token;}
    s.eye=eye;s.frame=frame;s.lightmap=lightmap;if(!p.continuous&&p.Has(s))s.eye=2;return token;
}
void NativeMaterialCapture::End(unsigned token)noexcept{if(impl&&token!=UINT_MAX&&token<impl->depth){impl->depth=token;impl->currentValid=false;impl->current.lightmap={};impl->current.scope.lightmap={};for(unsigned i=token;i<impl->stack.size();++i)impl->stack[i].lightmap={};}}
unsigned NativeMaterialCapture::Count()const noexcept{return impl?impl->count:0;}
bool NativeMaterialCapture::InScope()const noexcept{return impl&&impl->depth!=0;}
void NativeMaterialCapture::Observe(IDirect3DDevice9* d,unsigned kind)noexcept{
    if(!impl||!d)return;auto& p=*impl;p.currentValid=false;p.current.lightmap={};p.current.scope.lightmap={};++p.observed;if(!p.depth||p.depth>p.stack.size()||(!p.continuous&&p.count==p.entries.size()))return;auto const& scope=p.stack[p.depth-1];if(scope.eye>1||(!p.continuous&&p.Has(scope)))return;++p.scopedDraws;
    D3DVIEWPORT9 viewport{};DWORD color=0;if(FAILED(d->GetViewport(&viewport))||viewport.X||viewport.Y||viewport.Width!=p.width||viewport.Height!=p.height){++p.wrongViewport;return;}
    if(FAILED(d->GetRenderState(D3DRS_COLORWRITEENABLE,&color))||(color&7)!=7){++p.depthOnly;return;}
    auto& e=p.current;e={};e.scope=scope;e.drawKind=kind;e.eye=scope.eye;e.frame=scope.frame;e.lightmap=scope.lightmap;
    if(FAILED(d->GetVertexShader(&e.vs))||!e.vs||FAILED(d->GetPixelShader(&e.ps))||!e.ps||FAILED(d->GetVertexShaderConstantF(0,e.vertex.data(),256))||FAILED(d->GetPixelShaderConstantF(0,e.pixel.data(),224))){++p.shaderQueryFailed;e={};return;}
    e.vertexHash=p.Hash(e.vs.Get());e.pixelHash=p.Hash(e.ps.Get());
    if(scope.deferred&&!p.MatchDeferred(d,e)){e={};return;}
    for(unsigned i=0;i<e.states.size();++i)if(FAILED(d->GetRenderState(States[i],&e.states[i]))){e={};return;}
    e.extraConstantsValid=SUCCEEDED(d->GetVertexShaderConstantB(0,e.vb.data(),16))&&SUCCEEDED(d->GetPixelShaderConstantB(0,e.pb.data(),16))&&SUCCEEDED(d->GetVertexShaderConstantI(0,e.vi.data(),16))&&SUCCEEDED(d->GetPixelShaderConstantI(0,e.pi.data(),16));
    ComPtr<IDirect3DVertexDeclaration9> declaration;if(SUCCEEDED(d->GetVertexDeclaration(&declaration))&&declaration){e.elements=UINT(e.declaration.size());if(FAILED(declaration->GetDeclaration(e.declaration.data(),&e.elements))||e.elements>e.declaration.size())e.elements=0;}
    for(unsigned i=0;i<16;++i){auto& s=e.samplers[i];ComPtr<IDirect3DBaseTexture9> texture;s.valid=SUCCEEDED(d->GetTexture(i,&texture));if(texture){s.identity=reinterpret_cast<uintptr_t>(texture.Get());s.levels=texture->GetLevelCount();s.type=unsigned(texture->GetType());ComPtr<IDirect3DTexture9> image;if(SUCCEEDED(texture.As(&image)))s.valid=s.valid&&SUCCEEDED(image->GetLevelDesc(0,&s.image));}for(unsigned j=0;j<std::size(Samplers);++j)if(FAILED(d->GetSamplerState(i,Samplers[j],&s.state[j])))s.valid=false;}
    p.currentValid=true;if(!p.Has(scope)&&p.count<p.entries.size()){p.entries[p.count]=e;p.entries[p.count].scope.lightmap={};p.entries[p.count++].lightmap={};}
}
NativeMaterialDraw const* NativeMaterialCapture::Latest(unsigned eye,uint64_t frame)const noexcept{
    return impl&&impl->currentValid&&impl->current.eye==eye&&impl->current.frame==frame?&impl->current:nullptr;
}
void NativeMaterialCapture::WriteDiagnostics(std::ostream& out)const{
    if(!impl)return;auto const& p=*impl;
    out<<"{\"type\":\"native_material_observation_summary\",\"scopes\":"<<p.scopes<<",\"invalid_names\":"<<p.invalidNames<<",\"deferred_scopes\":"<<p.deferredScopes<<",\"observed_draws\":"<<p.observed<<",\"scoped_draws\":"<<p.scopedDraws<<",\"wrong_viewport\":"<<p.wrongViewport<<",\"depth_only\":"<<p.depthOnly<<",\"shader_query_failed\":"<<p.shaderQueryFailed<<",\"unknown_position_program\":"<<p.unknownProgram<<",\"selected_matrix_mismatch\":"<<p.matrixMismatch<<",\"captured\":"<<p.count<<"}\n";
}
void NativeMaterialCapture::Flush()noexcept{
    if(!impl)return;auto& p=*impl;for(unsigned i=0;i<p.count;++i){auto& e=p.entries[i];if(e.written)continue;e.written=true;
        try{auto vs=SaveShader(e.vs.Get(),p.directory,"vs"),ps=SaveShader(e.ps.Get(),p.directory,"ps");auto& out=p.log;out<<"{\"type\":\"native_material_draw\",\"frame\":"<<e.scope.frame<<",\"eye\":"<<e.scope.eye<<",\"mesh\":"<<std::quoted(e.scope.mesh)<<",\"material\":"<<std::quoted(e.scope.material)<<",\"vertex_shader\":"<<std::quoted(vs)<<",\"pixel_shader\":"<<std::quoted(ps)<<",\"draw_kind\":"<<e.drawKind<<",\"deferred_pool\":"<<(e.scope.deferred?"true":"false")<<",\"matched_instance_row\":"<<e.matchedRow<<",\"selected_world_cm\":";Values(out,e.scope.worldCm);out<<",\"states\":{";
            for(unsigned j=0;j<e.states.size();++j){if(j)out<<',';out<<std::quoted(StateNames[j])<<':'<<e.states[j];}out<<"},\"vertex_constants_c0_c255\":";Values(out,e.vertex);out<<",\"pixel_constants_c0_c223\":";Values(out,e.pixel);out<<",\"integer_boolean_constants_valid\":"<<(e.extraConstantsValid?"true":"false")<<",\"vertex_bool_b0_b15\":";Values(out,e.vb);out<<",\"pixel_bool_b0_b15\":";Values(out,e.pb);out<<",\"vertex_int_i0_i15\":";Values(out,e.vi);out<<",\"pixel_int_i0_i15\":";Values(out,e.pi);out<<",\"declaration\":[";
            for(unsigned j=0;j<e.elements;++j){if(j)out<<',';auto const& v=e.declaration[j];out<<'['<<v.Stream<<','<<v.Offset<<','<<unsigned(v.Type)<<','<<unsigned(v.Method)<<','<<unsigned(v.Usage)<<','<<unsigned(v.UsageIndex)<<']';}out<<"],\"samplers\":[";
            for(unsigned j=0;j<16;++j){if(j)out<<',';auto const& s=e.samplers[j];out<<"{\"slot\":"<<j<<",\"valid\":"<<(s.valid?"true":"false")<<",\"identity\":"<<s.identity<<",\"type\":"<<s.type<<",\"levels\":"<<s.levels<<",\"width\":"<<s.image.Width<<",\"height\":"<<s.image.Height<<",\"format\":"<<unsigned(s.image.Format)<<",\"addressUVW_mag_min_mip_srgb\":";out<<'[';for(unsigned k=0;k<7;++k){if(k)out<<',';out<<s.state[k];}out<<"],\"max_anisotropy\":"<<s.state[7]<<",\"mip_lod_bias_bits\":"<<s.state[8]<<",\"max_mip_level\":"<<s.state[9]<<'}';}out<<"],\"gpu_readback\":false,\"native_draw_suppressed\":false}\n";out.flush();
        }catch(...){try{p.log<<"{\"type\":\"native_material_capture_failed\",\"entry\":"<<i<<"}\n";p.log.flush();}catch(...){}}e.vs.Reset();e.ps.Reset();
    }
}
}
