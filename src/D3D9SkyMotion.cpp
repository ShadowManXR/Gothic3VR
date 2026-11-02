#include "D3D9SkyMotion.h"
#include "TemporalSkyMotion.h"
#include "NativeSkyVS.h"
#include "NativeSkyMotionPS.h"
#include <wrl/client.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <fstream>
#include <iomanip>
namespace g3vr {
using Microsoft::WRL::ComPtr;
using namespace diagnostic::temporal;
namespace {
thread_local unsigned replayDepth=0;
void Check(HRESULT hr,char const* why){if(FAILED(hr))throw std::runtime_error(std::string(why)+": "+std::to_string(hr));}
void Require(bool value,char const* why){if(!value)throw std::runtime_error(why);}
struct Saved {
    IDirect3DDevice9* device;ComPtr<IDirect3DStateBlock9> block;ComPtr<IDirect3DSurface9> rt[4],depth;unsigned count=0;
    D3DVIEWPORT9 viewport{};RECT scissor{};bool armed=false;
    explicit Saved(IDirect3DDevice9* d):device(d){D3DCAPS9 caps{};Check(d->GetDeviceCaps(&caps),"Sky capabilities");count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
        Check(d->CreateStateBlock(D3DSBT_ALL,&block),"Sky state block");Check(block->Capture(),"Sky state capture");
        for(unsigned i=0;i<count;++i){auto hr=d->GetRenderTarget(i,&rt[i]);if(hr!=D3DERR_NOTFOUND)Check(hr,"Sky save MRT");}
        auto hr=d->GetDepthStencilSurface(&depth);if(hr!=D3DERR_NOTFOUND)Check(hr,"Sky save depth");Check(d->GetViewport(&viewport),"Sky save viewport");Check(d->GetScissorRect(&scissor),"Sky save scissor");armed=true;}
    HRESULT Apply()noexcept {HRESULT first=S_OK;auto save=[&](HRESULT hr){if(FAILED(hr)&&SUCCEEDED(first))first=hr;};save(device->SetDepthStencilSurface(nullptr));for(unsigned i=1;i<count;++i)save(device->SetRenderTarget(i,nullptr));save(device->SetRenderTarget(0,rt[0].Get()));for(unsigned i=1;i<count;++i)save(device->SetRenderTarget(i,rt[i].Get()));save(device->SetDepthStencilSurface(depth.Get()));save(block->Apply());save(device->SetViewport(&viewport));save(device->SetScissorRect(&scissor));return first;}
    void Finish(){auto hr=Apply();armed=false;Check(hr,"Sky restore caller state");}
    ~Saved(){if(armed)Apply();}
};
template<class T>uint64_t ShaderHash(T* shader){if(!shader)return 0;UINT size=0;Check(shader->GetFunction(nullptr,&size),"Sky shader size");if(!size||size>32768)return 0;std::array<unsigned char,32768> bytes{};UINT count=size;Check(shader->GetFunction(bytes.data(),&count),"Sky shader bytes");Require(count==size,"Sky shader byte count");uint64_t h=14695981039346656037ull;for(unsigned i=0;i<size;++i){h^=bytes[i];h*=1099511628211ull;}return h;}
}
struct D3D9SkyMotion::Impl {
    inline static constexpr D3DSAMPLERSTATETYPE samplerStates[]={D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW,D3DSAMP_BORDERCOLOR,D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_MAXMIPLEVEL,D3DSAMP_MAXANISOTROPY,D3DSAMP_SRGBTEXTURE};
    struct Material {
        std::array<ComPtr<IDirect3DTexture9>,4> textures;
        std::array<std::array<DWORD,11>,4> sampler{};
        std::array<float,8> pixel{};
        bool Same(Material const& b)const noexcept{if(pixel!=b.pixel||sampler!=b.sampler)return false;for(unsigned i=0;i<4;++i)if(textures[i].Get()!=b.textures[i].Get())return false;return true;}
    };
    struct Eye {
        ComPtr<IDirect3DTexture9> image;ComPtr<IDirect3DSurface9> surface,depth,target;
        GeometryFrameStamp stamp,previousStamp;SkyPose previous;
        std::array<float,2> currentJitter{},previousJitter{};
        Material previousMaterial;Report report;bool cleared=false,previousReady=false;
    };
    std::array<Eye,2> eyes;unsigned current=2,width=0,height=0;
    ComPtr<IDirect3DDevice9> device;ComPtr<IDirect3DVertexShader9> vertex;ComPtr<IDirect3DPixelShader9> pixel;
    struct Program {ComPtr<IDirect3DVertexShader9> shader;uint64_t hash=0;};std::array<Program,128> programs;unsigned count=0;
    ComPtr<IDirect3DPixelShader9> sourcePixel;uint64_t pixelHash=0;
    void Initialize(IDirect3DDevice9* d){
        if(device){Require(device.Get()==d,"Sky native device changed");return;}
        Check(d->CreateVertexShader(reinterpret_cast<DWORD const*>(NativeSkyVS),&vertex),"Sky exact native companion VS");Check(d->CreatePixelShader(reinterpret_cast<DWORD const*>(NativeSkyMotionPS),&pixel),"Sky motion PS");
        for(auto& eye:eyes){Check(d->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&eye.image,nullptr),"Sky private output");Check(eye.image->GetSurfaceLevel(0,&eye.surface),"Sky private surface");}device=d;
    }
    uint64_t VertexHash(IDirect3DVertexShader9* shader){for(unsigned i=0;i<count;++i)if(programs[i].shader.Get()==shader)return programs[i].hash;if(count==programs.size())return 0;auto& p=programs[count++];p.shader=shader;p.hash=ShaderHash(shader);return p.hash;}
    DWORD State(D3DRENDERSTATETYPE type){DWORD value=0;Check(device->GetRenderState(type,&value),"Sky source render state");return value;}
    void Boundary(IDirect3DDevice9* d,IDirect3DSurface9* depth){
        if(current>1)return;Initialize(d);auto& eye=eyes[current];Require(!eye.cleared&&depth,"Duplicate/missing sky opaque boundary");
        D3DSURFACE_DESC desc{};Check(depth->GetDesc(&desc),"Sky boundary depth");Require(desc.Width==width&&desc.Height==height&&desc.MultiSampleType==D3DMULTISAMPLE_NONE,"Sky boundary depth dimensions");
        Check(d->GetRenderTarget(0,&eye.target),"Sky boundary world target");eye.depth=depth;
        Saved saved(d);Check(d->SetDepthStencilSurface(nullptr),"Sky clear depth unbind");for(unsigned i=1;i<saved.count;++i)Check(d->SetRenderTarget(i,nullptr),"Sky clear MRT");Check(d->SetRenderTarget(0,eye.surface.Get()),"Sky clear private target");
        D3DVIEWPORT9 viewport{0,0,width,height,0,1};Check(d->SetViewport(&viewport),"Sky clear viewport");Check(d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE),"Sky clear scissor");Check(d->SetRenderState(D3DRS_COLORWRITEENABLE,15),"Sky clear channels");Check(d->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0),"Sky clear coverage");saved.Finish();eye.cleared=true;
    }
    void Replay(IDirect3DDevice9* d,Geometry const& geometry){
        if(current>1||!device||device.Get()!=d)return;auto& eye=eyes[current];if(!eye.cleared)return;
        ComPtr<IDirect3DVertexShader9> source;Check(d->GetVertexShader(&source),"Sky source VS");if(VertexHash(source.Get())!=0x9cd6b9e1f1627a6full)return;++eye.report.candidates;
        try {
            Require(!eye.report.draws,"Multiple sky contributions not combined");ComPtr<IDirect3DPixelShader9> ps;Check(d->GetPixelShader(&ps),"Sky source PS");if(sourcePixel.Get()!=ps.Get()){sourcePixel=ps;pixelHash=ShaderHash(ps.Get());}Require(pixelHash==0xe3a7e3dc1f1c8437ull,"Sky unsupported material PS");
            ComPtr<IDirect3DSurface9> target,depth;Check(d->GetRenderTarget(0,&target),"Sky current target");Check(d->GetDepthStencilSurface(&depth),"Sky current depth");Require(target==eye.target&&depth==eye.depth,"Sky source is not opaque world target/depth");
            D3DVIEWPORT9 viewport{};Check(d->GetViewport(&viewport),"Sky source viewport");Require(viewport.X==0&&viewport.Y==0&&viewport.Width==width&&viewport.Height==height&&viewport.MinZ==0&&viewport.MaxZ==1,"Sky viewport mismatch");
            Require(State(D3DRS_ZENABLE)==TRUE&&State(D3DRS_ZWRITEENABLE)==FALSE&&State(D3DRS_ALPHATESTENABLE)==FALSE&&(State(D3DRS_COLORWRITEENABLE)&7)==7,"Sky source depth/color/alpha mode");
            Require(State(D3DRS_TWOSIDEDSTENCILMODE)==FALSE,"Sky two-sided stencil unsupported");if(State(D3DRS_STENCILENABLE))Require(State(D3DRS_STENCILFAIL)==D3DSTENCILOP_KEEP&&State(D3DRS_STENCILZFAIL)==D3DSTENCILOP_KEEP&&State(D3DRS_STENCILPASS)==D3DSTENCILOP_KEEP,"Sky source writes stencil");
            bool alphaCoverage=false;if(State(D3DRS_ALPHABLENDENABLE)){
                Require(State(D3DRS_BLENDOP)==D3DBLENDOP_ADD,"Sky non-additive blend");auto src=State(D3DRS_SRCBLEND),dst=State(D3DRS_DESTBLEND);
                alphaCoverage=src==D3DBLEND_SRCALPHA&&dst==D3DBLEND_INVSRCALPHA;
                Require(alphaCoverage||(src==D3DBLEND_ONE&&(dst==D3DBLEND_ZERO||dst==D3DBLEND_INVSRCALPHA)),"Sky unsupported blend contribution");
            }
            Require((geometry.primitive==D3DPT_TRIANGLESTRIP||geometry.primitive==D3DPT_TRIANGLELIST)&&geometry.vertices&&geometry.count,"Sky source geometry");
            ComPtr<IDirect3DVertexDeclaration9> declaration;Check(d->GetVertexDeclaration(&declaration),"Sky source declaration");Require(declaration!=nullptr,"Sky missing declaration");D3DVERTEXELEMENT9 elements[2]{};UINT n=0;Check(declaration->GetDeclaration(nullptr,&n),"Sky source declaration size");Require(n==2,"Sky unsupported source element count");Check(declaration->GetDeclaration(elements,&n),"Sky source elements");Require(n==2&&elements[0].Stream==0&&elements[0].Offset==0&&elements[0].Type==D3DDECLTYPE_FLOAT3&&elements[0].Method==0&&elements[0].Usage==D3DDECLUSAGE_POSITION&&elements[0].UsageIndex==0,"Sky unsupported source declaration");
            ComPtr<IDirect3DVertexBuffer9> vb;ComPtr<IDirect3DIndexBuffer9> ib;UINT offset=0,stride=0,frequency=0;Check(d->GetStreamSource(0,&vb,&offset,&stride),"Sky source stream");Check(d->GetStreamSourceFreq(0,&frequency),"Sky source frequency");Check(d->GetIndices(&ib),"Sky source indices");Require(vb&&ib&&stride>=12&&frequency==1,"Sky missing/unexpected source buffers");
            D3DVERTEXBUFFER_DESC vd{};D3DINDEXBUFFER_DESC id{};Check(vb->GetDesc(&vd),"Sky VB size");Check(ib->GetDesc(&id),"Sky IB size");int64_t first=int64_t(geometry.base)+geometry.minimum,last=first+geometry.vertices;unsigned indexBytes=id.Format==D3DFMT_INDEX16?2:id.Format==D3DFMT_INDEX32?4:0;uint64_t indices=geometry.primitive==D3DPT_TRIANGLELIST?uint64_t(geometry.count)*3:uint64_t(geometry.count)+2;
            Require(first>=0&&last>0&&indexBytes&&uint64_t(offset)+uint64_t(last-1)*stride+12<=vd.Size&&(uint64_t(geometry.first)+indices)*indexBytes<=id.Size,"Sky draw exceeds buffers");
            SkyPose pose;pose.vertexHash=0x9cd6b9e1f1627a6full;pose.pixelHash=pixelHash;Check(d->GetVertexShaderConstantF(0,pose.constants.data(),15),"Sky source camera/material constants");Require(pose.Valid(),"Sky nonfinite pose");std::array<float,8> material{};Check(d->GetPixelShaderConstantF(0,material.data(),2),"Sky source pixel material");for(float f:material)Require(std::isfinite(f),"Sky nonfinite pixel material");
            Material currentMaterial;currentMaterial.pixel=material;
            for(unsigned slot=0;slot<4;++slot){ComPtr<IDirect3DBaseTexture9> texture;Check(d->GetTexture(slot,&texture),"Sky material texture");Require(texture&&texture->GetType()==D3DRTYPE_TEXTURE,"Sky missing 2D material texture");Check(texture.As(&currentMaterial.textures[slot]),"Sky material asset");D3DSURFACE_DESC desc{};Check(currentMaterial.textures[slot]->GetLevelDesc(0,&desc),"Sky asset description");Require(!desc.Usage,"Sky mutable/render-target asset unsupported");for(unsigned k=0;k<11;++k)Check(d->GetSamplerState(slot,samplerStates[k],&currentMaterial.sampler[slot][k]),"Sky material sampler state");}
            bool history=eye.previousReady&&eye.stamp.Follows(eye.previousStamp);std::array<float,16> matrix{};Require(SkyRotationTransform(pose,history?eye.previous:pose,matrix),"Sky reprojection singular");
            bool reactive=!history||SkyCloudParametersChanged(pose,eye.previous)||!currentMaterial.Same(eye.previousMaterial);
            Saved saved(d);struct Guard{Guard(){++replayDepth;}~Guard(){--replayDepth;}} guard;
            Check(d->SetDepthStencilSurface(nullptr),"Sky replay unbind depth");for(unsigned i=1;i<saved.count;++i)Check(d->SetRenderTarget(i,nullptr),"Sky replay remove MRT");Check(d->SetRenderTarget(0,eye.surface.Get()),"Sky replay target");Check(d->SetDepthStencilSurface(depth.Get()),"Sky replay preserve depth");Check(d->SetViewport(&viewport),"Sky replay viewport");
            for(auto key:{D3DRS_ALPHABLENDENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_FOGENABLE})Check(d->SetRenderState(key,FALSE),"Sky private output state");
            Check(d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE),"Sky read-only depth");Check(d->SetRenderState(D3DRS_STENCILWRITEMASK,0),"Sky read-only stencil");Check(d->SetRenderState(D3DRS_COLORWRITEENABLE,15),"Sky output channels");Check(d->SetRenderState(D3DRS_WRAP8,0),"Sky clip interpolator wrapping");
            Check(d->SetVertexShader(vertex.Get()),"Sky exact companion bind");Check(d->SetPixelShader(pixel.Get()),"Sky pixel companion bind");Check(d->SetPixelShaderConstantF(0,matrix.data(),4),"Sky rotation transform");float pixelParameters[]{width*.5f,-float(height)*.5f,eye.previousJitter[0]-eye.currentJitter[0],eye.previousJitter[1]-eye.currentJitter[1]};Check(d->SetPixelShaderConstantF(4,pixelParameters,1),"Sky pixel motion units");float flags[]{history?1.f:0.f,reactive?1.f:0.f,alphaCoverage?1.f:0.f,material[3]};Check(d->SetPixelShaderConstantF(5,flags,1),"Sky reactivity/coverage flags");
            Check(d->DrawIndexedPrimitive(D3DPRIMITIVETYPE(geometry.primitive),geometry.base,geometry.minimum,geometry.vertices,geometry.first,geometry.count),"Sky late replay draw");saved.Finish();
            ++eye.report.draws;eye.report.history=history;eye.report.materialReactive=reactive;eye.previous=pose;eye.previousMaterial=std::move(currentMaterial);eye.previousStamp=eye.stamp;eye.previousJitter=eye.currentJitter;eye.previousReady=true;
        } catch(std::exception const& error){++eye.report.rejected;eye.report.history=false;eye.previousReady=false;strncpy_s(eye.report.error,error.what(),_TRUNCATE);}
    }
};
D3D9SkyMotion::D3D9SkyMotion()=default;D3D9SkyMotion::~D3D9SkyMotion()=default;
bool D3D9SkyMotion::Replaying()noexcept{return replayDepth!=0;}
void D3D9SkyMotion::Begin(unsigned eye,GeometryFrameStamp const& stamp,unsigned width,unsigned height,std::array<float,2> jitter){Require(eye<2&&stamp.eye==eye&&stamp.frame&&stamp.epoch&&stamp.displayTimeNs&&width&&height&&width<=8192&&height<=8192&&uint64_t(width)*height<=16777216,"Sky frame contract");for(float v:jitter)Require(std::isfinite(v)&&std::abs(v)<=.5f,"Sky raster jitter contract");if(!impl)impl=std::make_unique<Impl>();auto& p=*impl;Require(!p.device||(p.width==width&&p.height==height),"Sky resolution requires reset");p.width=width;p.height=height;p.current=eye;auto& e=p.eyes[eye];e.stamp=stamp;e.currentJitter=jitter;e.report={};e.depth.Reset();e.target.Reset();e.cleared=false;}
void D3D9SkyMotion::OpaqueBoundary(IDirect3DDevice9* d,IDirect3DSurface9* depth){if(impl)impl->Boundary(d,depth);}
void D3D9SkyMotion::Replay(IDirect3DDevice9* d,Geometry const& g){if(impl&&!Replaying())impl->Replay(d,g);}
void D3D9SkyMotion::End()noexcept{if(impl)impl->current=2;}
void D3D9SkyMotion::ResetHistory()noexcept{if(impl)for(auto& e:impl->eyes)e.previousReady=false;}
void D3D9SkyMotion::Reset()noexcept{impl.reset();}
IDirect3DSurface9* D3D9SkyMotion::Surface(unsigned eye)const noexcept{return impl&&eye<2&&impl->eyes[eye].cleared&&!impl->eyes[eye].report.rejected?impl->eyes[eye].surface.Get():nullptr;}
D3D9SkyMotion::Report const& D3D9SkyMotion::Status(unsigned eye)const noexcept{static Report const empty{};return impl&&eye<2?impl->eyes[eye].report:empty;}
void D3D9SkyMotion::Write(std::ostream& out,unsigned index,std::filesystem::path const& directory,std::string const& prefix,bool pixels)const {
    if(!impl||index>1){out<<"null";return;}auto const& e=impl->eyes[index];auto const& r=e.report;std::string name,error=r.error;bool written=false;
    if(pixels&&r.draws==1&&r.rejected==0&&Surface(index))try{
        ComPtr<IDirect3DSurface9> staging;Check(impl->device->CreateOffscreenPlainSurface(impl->width,impl->height,D3DFMT_A16B16G16R16F,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Sky diagnostic staging");Check(impl->device->GetRenderTargetData(e.surface.Get(),staging.Get()),"Sky diagnostic readback");D3DLOCKED_RECT map{};Check(staging->LockRect(&map,nullptr,D3DLOCK_READONLY),"Sky diagnostic map");
        struct Unlock{IDirect3DSurface9* s;~Unlock(){s->UnlockRect();}} unlock{staging.Get()};Require(map.Pitch>=LONG(impl->width*8),"Sky diagnostic pitch");
        name=prefix+"-sky-motion-"+std::to_string(e.stamp.frame)+"-eye"+std::to_string(index)+".rgba16f";std::ofstream file(directory/name,std::ios::binary);for(unsigned y=0;y<impl->height;++y)file.write(static_cast<char const*>(map.pBits)+map.Pitch*y,impl->width*8);file.close();written=bool(file);if(!written)error="Sky diagnostic write failed";
    }catch(std::exception const& ex){error=ex.what();}
    out<<"{\"frame\":"<<e.stamp.frame<<",\"eye\":"<<index<<",\"width\":"<<impl->width<<",\"height\":"<<impl->height<<",\"source\":\"native_late_sky_exact_9cd6b9e1f1627a6f\",\"candidates\":"<<r.candidates<<",\"draws\":"<<r.draws<<",\"rejected\":"<<r.rejected<<",\"camera_history_valid\":"<<(r.history?"true":"false")<<",\"material_reactive\":"<<(r.materialReactive?"true":"false")<<",\"file\":"<<std::quoted(name)<<",\"file_written\":"<<(written?"true":"false")<<",\"encoding\":\"RGBA16F_little_endian_top_left\",\"rg_units\":\"previous_minus_current_pixels_unjittered\",\"b_channel\":\"actual_native_draw_contribution\",\"a_channel\":\"reactivity_NOT_geometry_history_valid\",\"scene_coverage_complete\":false,\"game_dlaa_enabled\":false,\"error\":"<<std::quoted(error)<<'}';
}
}
