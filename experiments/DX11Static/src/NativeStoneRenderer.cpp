#include "NativeStoneRenderer.h"
#include "OwnedStoneAtlas.h"
#include "NativeMaterialResources.h"
#include "NativeStageInterface.h"
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
namespace {
void Need(bool v,char const* m){if(!v)throw std::runtime_error(m);}
void Check(HRESULT hr,char const* m){if(FAILED(hr))throw std::runtime_error(std::string(m)+" HRESULT="+std::to_string(unsigned(hr)));}
std::vector<char> Read(std::filesystem::path const& p,size_t limit){std::ifstream f(p,std::ios::binary|std::ios::ate);Need(bool(f),"Missing local stone profile");auto n=f.tellg();Need(n>0&&uint64_t(n)<=limit,"Stone profile file bounds");std::vector<char> v(size_t(n),0);f.seekg(0);f.read(v.data(),n);Need(bool(f),"Stone profile read");return v;}
uint64_t Hash(std::vector<char> const& bytes){uint64_t h=14695981039346656037ull;for(unsigned char v:bytes){h^=v;h*=1099511628211ull;}return h;}
ComPtr<ID3DBlob> Compile(std::vector<char> const& s,char const* entry,char const* profile){ComPtr<ID3DBlob> b,e;auto hr=D3DCompile(s.data(),s.size(),nullptr,nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&e);if(FAILED(hr))throw std::runtime_error(e?std::string(static_cast<char*>(e->GetBufferPointer()),e->GetBufferSize()):"Stone shader compile");return b;}
ComPtr<ID3D11ShaderResourceView> OriginalDds(ID3D11Device* d11,std::filesystem::path const& path){
    auto file=Read(path,32*1024*1024);Need(file.size()>=128&&!memcmp(file.data(),"DDS ",4),"Original DDS header");auto u32=[&](unsigned at){uint32_t value;memcpy(&value,file.data()+at,4);return value;};
    unsigned w=u32(16),h=u32(12),mips=u32(28),fourcc=u32(84);Need(w&&h&&w<=4096&&h<=4096&&mips&&mips<=13,"Original DDS dimensions");Need(fourcc==0x31545844||fourcc==0x35545844,"Original DDS compression");unsigned block=fourcc==0x31545844?8:16;
    ComPtr<ID3D11ShaderResourceView> result;std::vector<D3D11_SUBRESOURCE_DATA> levels;size_t at=128;
    for(unsigned level=0;level<mips;++level){unsigned pitch=std::max(1u,(std::max(1u,w>>level)+3)/4)*block,rows=std::max(1u,(std::max(1u,h>>level)+3)/4);size_t bytes=size_t(pitch)*rows;Need(at<=file.size()&&bytes<=file.size()-at,"Original DDS mip bounds");levels.push_back({file.data()+at,pitch,UINT(bytes)});at+=bytes;}
    Need(at==file.size(),"Original DDS trailing payload");D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=mips;desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=block==8?DXGI_FORMAT_BC1_UNORM:DXGI_FORMAT_BC3_UNORM;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;ComPtr<ID3D11Texture2D> texture;Check(d11->CreateTexture2D(&desc,levels.data(),&texture),"Original DDS DX11 texture");Check(d11->CreateShaderResourceView(texture.Get(),nullptr,&result),"Original DDS view");return result;
}
constexpr D3DVERTEXELEMENT9 Declaration[]={{0,0,2,0,0,0},{0,12,2,0,3,0},{0,24,1,0,5,0},{1,0,4,0,10,0},{1,4,2,0,6,0},{2,0,4,0,10,1},{2,4,2,0,5,7},{2,16,1,0,5,6},D3DDECL_END()};
}
struct NativeStoneRenderer::Impl {
    ComPtr<IDirect3DDevice9Ex> native;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    NativeMaterialResources resources;char const* refusal="not attempted";
    ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11VertexShader> instancedVS;ComPtr<ID3D11Buffer> instanceWorlds;
    unsigned instanceCount=0,submittedDraws=0;
    std::array<ComPtr<ID3D11Buffer>,3> streams;ComPtr<ID3D11Buffer> indices,vc,pc;
    std::array<ComPtr<ID3D11ShaderResourceView>,2> material;
    std::array<ComPtr<ID3D11SamplerState>,6> samplers;
    std::array<std::array<DWORD,10>,6> samplerKeys{};
    ComPtr<ID3D11RasterizerState> raster;DWORD cull=0;
    ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11BlendState> blend;
    std::vector<unsigned char> additional;unsigned vertices=0,indexCount=0;
    bool batching=false;unsigned batchDraws=0;
    bool bridgeReady=false;ResidentStoneInputs const* resident=nullptr;float const* residentClear=nullptr;
};
NativeStoneRenderer::NativeStoneRenderer()=default;NativeStoneRenderer::~NativeStoneRenderer()=default;
void NativeStoneRenderer::Initialize(IDirect3DDevice9Ex* n,ID3D11Device* d,ID3D11DeviceContext* c,std::filesystem::path const& profile){
    Need(d&&c,"Invalid stone renderer devices");ComPtr<ID3D11Device> owner;c->GetDevice(&owner);Need(owner.Get()==d&&c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Stone renderer requires owned immediate context");auto s=std::make_unique<Impl>();s->native=n;s->device=d;s->context=c;
    Need(Hash(Read(profile/L"native.cso",32768))==0x26cb84265f99bd7dull&&Hash(Read(profile/L"native-vertex.cso",32768))==0x7a13cd383a26eb4bull,"Stone source programs changed");
    auto vs=Compile(Read(profile/L"NativeStoneVertex.hlsl",32768),"VS","vs_5_0"),ps=Compile(Read(profile/L"NativeStoneMaterial.hlsl",131072),"PS","ps_5_0");
    Need(NativeStageInterfaceCompatible(vs.Get(),ps.Get()),"Stone stage interface mismatch");
    Check(d->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&s->vs),"Stone VS");Check(d->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&s->ps),"Stone PS");
    auto vertexSource=Read(profile/L"NativeStoneVertex.hlsl",32768);std::string instanceSource(vertexSource.begin(),vertexSource.end());
    auto replace=[&](std::string const& from,std::string const& to){size_t at=0;bool found=false;while((at=instanceSource.find(from,at))!=std::string::npos){instanceSource.replace(at,from.size(),to);at+=to.size();found=true;}Need(found,"Unsupported instanced stone vertex source");};
    replace("Output VS(Input input)","cbuffer Instances:register(b1){float4 iw[384];}\nOutput VS(Input input,uint instance:SV_InstanceID)");
    for(unsigned i=0;i<3;++i)replace("vc["+std::to_string(12+i)+"]","iw[instance*3+"+std::to_string(i)+"]");
    auto instanced=Compile(std::vector<char>(instanceSource.begin(),instanceSource.end()),"VS","vs_5_0");Need(NativeStageInterfaceCompatible(instanced.Get(),ps.Get()),"Instanced stone stage interface mismatch");Check(d->CreateVertexShader(instanced->GetBufferPointer(),instanced->GetBufferSize(),nullptr,&s->instancedVS),"Instanced stone VS");
    D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,1,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,1,4,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",1,DXGI_FORMAT_R8G8B8A8_UNORM,2,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",7,DXGI_FORMAT_R32G32B32_FLOAT,2,4,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",6,DXGI_FORMAT_R32G32_FLOAT,2,16,D3D11_INPUT_PER_VERTEX_DATA,0}};
    Check(d->CreateInputLayout(elements,UINT(std::size(elements)),vs->GetBufferPointer(),vs->GetBufferSize(),&s->layout),"Stone declaration");
    auto mesh=Read(profile/L"native-scene.bin",16*1024*1024);Need(mesh.size()>=32&&!memcmp(mesh.data(),"G3NATIV1",8),"Stone native streams header");uint32_t h[6];memcpy(h,mesh.data()+8,24);
    Need(h[0]==1&&h[1]==2097&&h[2]==3390&&h[3]==32&&h[4]==16&&h[5]==24&&mesh.size()==32+size_t(h[1])*72+size_t(h[2])*4,"Unexpected stone geometry");s->vertices=h[1];s->indexCount=h[2];s->additional.resize(s->vertices*24);
    size_t at=32;for(unsigned i=0;i<3;++i){D3D11_BUFFER_DESC b{};b.ByteWidth=s->vertices*h[i+3];b.BindFlags=D3D11_BIND_VERTEX_BUFFER;b.Usage=i==2?D3D11_USAGE_DEFAULT:D3D11_USAGE_IMMUTABLE;D3D11_SUBRESOURCE_DATA init{mesh.data()+at,0,0};Check(d->CreateBuffer(&b,i==2?nullptr:&init,&s->streams[i]),"Stone vertex buffer");at+=b.ByteWidth;}
    for(unsigned i=0;i<s->indexCount;++i){uint32_t index;memcpy(&index,mesh.data()+at+i*4,4);Need(index<s->vertices,"Stone index bounds");}
    D3D11_BUFFER_DESC b{};b.Usage=D3D11_USAGE_IMMUTABLE;b.ByteWidth=s->indexCount*4;b.BindFlags=D3D11_BIND_INDEX_BUFFER;D3D11_SUBRESOURCE_DATA init{mesh.data()+at,0,0};Check(d->CreateBuffer(&b,&init,&s->indices),"Stone index buffer");
    b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;b.ByteWidth=1028*4;Check(d->CreateBuffer(&b,nullptr,&s->vc),"Stone VS registers");b.ByteWidth=896*4;Check(d->CreateBuffer(&b,nullptr,&s->pc),"Stone PS registers");
    b.ByteWidth=128*3*16;Check(d->CreateBuffer(&b,nullptr,&s->instanceWorlds),"Stone instance worlds");
    s->material[0]=OriginalDds(d,profile/L"normal.dds");s->material[1]=OriginalDds(d,profile/L"diffuse.dds");
    D3D11_DEPTH_STENCIL_DESC depth{};depth.DepthEnable=TRUE;depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;depth.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;Check(d->CreateDepthStencilState(&depth,&s->depth),"Stone depth state");D3D11_BLEND_DESC blend{};blend.RenderTarget[0].RenderTargetWriteMask=7;Check(d->CreateBlendState(&blend,&s->blend),"Stone RGB state");
    if(n){Need(s->resources.Initialize(n,d,c),"Stone material requires DXVK GPU interop");s->bridgeReady=true;}impl=std::move(s);
}
bool NativeStoneRenderer::Render(NativeMaterialDraw const& draw,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,unsigned width,unsigned height){
    Need(bool(impl),"Stone renderer not initialized");auto& s=*impl;s.refusal="program, eye, frame or live lightmap source mismatch";
    if(!s.resident&&!s.bridgeReady)return false;
    if(draw.eye>1||!draw.frame||draw.vertexHash!=0x7a13cd383a26eb4bull||draw.pixelHash!=0x26cb84265f99bd7dull||draw.matchedRow!=0||!draw.lightmap.context||!draw.lightmap.copy)return false;
    s.refusal="native vertex declaration mismatch";if(draw.elements!=std::size(Declaration)||memcmp(draw.declaration.data(),Declaration,sizeof(Declaration)))return false;
    s.refusal="unsupported native depth, blend, cull or raster state";auto const& st=draw.states;
    if(st[0]!=TRUE||st[1]!=TRUE||st[2]!=D3DCMP_LESSEQUAL||st[3]!=7||st[4]||st[5]||st[8]||st[9]||st[10]||st[7]<D3DCULL_NONE||st[7]>D3DCULL_CCW)return false;
    if(!s.resident){for(auto state:{D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_CLIPPLANEENABLE}){DWORD v=0;if(FAILED(s.native->GetRenderState(state,&v))||v)return false;}
        DWORD fill=0;if(FAILED(s.native->GetRenderState(D3DRS_FILLMODE,&fill))||fill!=D3DFILL_SOLID)return false;}
    s.refusal="non-finite native constants";for(float f:draw.vertex)if(!std::isfinite(f))return false;for(float f:draw.pixel)if(!std::isfinite(f))return false;
    s.refusal="native texture or sampler mismatch";std::array<ComPtr<IDirect3DTexture9>,6> textures;
    for(unsigned i=0;i<6;++i){auto const& p=draw.samplers[i];if(!p.valid||!p.identity||p.type!=D3DRTYPE_TEXTURE||p.state[6])return false;
        if(!s.resident){ComPtr<IDirect3DBaseTexture9> t;Check(s.native->GetTexture(i,&t),"Current stone texture");if(reinterpret_cast<uintptr_t>(t.Get())!=p.identity)return false;Check(t.As(&textures[i]),"Stone 2D texture");}
        if(i>=4&&(p.image.Width!=2048||p.image.Height!=2048||p.levels!=12||p.image.Format!=(i==4?D3DFMT_DXT5:D3DFMT_DXT1)))return false;
        if(p.state[0]<1||p.state[0]>3||p.state[1]<1||p.state[1]>3||p.state[2]<1||p.state[2]>3||p.state[7]<1||p.state[7]>16)return false;
        if(i==0&&(p.state[3]!=D3DTEXF_POINT||p.state[4]!=D3DTEXF_POINT||p.state[5]!=D3DTEXF_POINT))return false;
        if(i>0&&i<4&&(p.state[3]!=D3DTEXF_LINEAR||p.state[4]!=D3DTEXF_LINEAR||p.state[5]!=(i==3?D3DTEXF_LINEAR:D3DTEXF_NONE)))return false;
        if(i>=4&&((p.state[3]!=D3DTEXF_ANISOTROPIC&&p.state[3]!=D3DTEXF_LINEAR&&p.state[3]!=D3DTEXF_POINT)||p.state[4]!=p.state[3]||p.state[5]!=D3DTEXF_LINEAR))return false;
    }
    s.refusal="SDK additional stream missing, dirty, type or count mismatch";if(!draw.lightmap.copy(draw.lightmap.context,s.additional.data(),s.vertices,unsigned(s.additional.size())))return false;
    for(unsigned i=0;i<6;++i){auto const& k=draw.samplers[i].state;if(s.samplers[i]&&s.samplerKeys[i]==k)continue;
        D3D11_SAMPLER_DESC d{};d.AddressU=D3D11_TEXTURE_ADDRESS_MODE(k[0]);d.AddressV=D3D11_TEXTURE_ADDRESS_MODE(k[1]);d.AddressW=D3D11_TEXTURE_ADDRESS_MODE(k[2]);d.MaxAnisotropy=k[7];memcpy(&d.MipLODBias,&k[8],4);if(!std::isfinite(d.MipLODBias))return false;d.MinLOD=float(k[9]);d.MaxLOD=D3D11_FLOAT32_MAX;d.ComparisonFunc=D3D11_COMPARISON_LESS_EQUAL;
        d.Filter=i==1||i==2?D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:k[3]==D3DTEXF_ANISOTROPIC?D3D11_FILTER_ANISOTROPIC:k[3]==D3DTEXF_POINT?(k[5]==D3DTEXF_LINEAR?D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT):D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Check(s.device->CreateSamplerState(&d,&s.samplers[i]),"Live stone sampler");s.samplerKeys[i]=k;
    }
    if(!s.raster||s.cull!=st[7]){D3D11_RASTERIZER_DESC r{};r.FillMode=D3D11_FILL_SOLID;r.DepthClipEnable=TRUE;r.CullMode=st[7]==D3DCULL_NONE?D3D11_CULL_NONE:st[7]==D3DCULL_CW?D3D11_CULL_FRONT:D3D11_CULL_BACK;Check(s.device->CreateRasterizerState(&r,&s.raster),"Live stone cull");s.cull=st[7];}
    s.context->OMSetRenderTargets(0,nullptr,nullptr);if(!s.resident&&(!s.batching||!s.batchDraws))s.resources.Capture(textures[3].Get(),textures[1].Get(),textures[2].Get(),textures[0].Get());
    // Native D3D9 pixel centers correspond to a half-pixel viewport offset.
    // Apply it at viewport transform time, not by perturbing clip coordinates:
    // the latter can change fixed-point edge coverage at high resolutions.
    std::array<float,1028> vc{};std::copy(draw.vertex.begin(),draw.vertex.end(),vc.begin());
    s.context->UpdateSubresource(s.vc.Get(),0,nullptr,vc.data(),0,0);s.context->UpdateSubresource(s.pc.Get(),0,nullptr,draw.pixel.data(),0,0);s.context->UpdateSubresource(s.streams[2].Get(),0,nullptr,s.additional.data(),0,0);
    float clear[4]{};if(!s.batching||!s.batchDraws){s.context->ClearRenderTargetView(target,s.residentClear?s.residentClear:clear);s.context->ClearDepthStencilView(depth,D3D11_CLEAR_DEPTH,1,0);}s.context->OMSetRenderTargets(1,&target,depth);s.context->OMSetDepthStencilState(s.depth.Get(),0);s.context->OMSetBlendState(s.blend.Get(),nullptr,UINT_MAX);s.context->RSSetState(s.raster.Get());D3D11_VIEWPORT vp{.5f,.5f,float(width),float(height),0,1};s.context->RSSetViewports(1,&vp);
    s.context->IASetInputLayout(s.layout.Get());s.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);unsigned strides[]{32,16,24},offsets[3]{};ID3D11Buffer* buffers[]{s.streams[0].Get(),s.streams[1].Get(),s.streams[2].Get()};s.context->IASetVertexBuffers(0,3,buffers,strides,offsets);s.context->IASetIndexBuffer(s.indices.Get(),DXGI_FORMAT_R32_UINT,0);s.context->VSSetShader(s.instanceCount?s.instancedVS.Get():s.vs.Get(),nullptr,0);s.context->PSSetShader(s.ps.Get(),nullptr,0);auto* vertex=s.vc.Get();auto* pixel=s.pc.Get();s.context->VSSetConstantBuffers(0,1,&vertex);auto* instances=s.instanceCount?s.instanceWorlds.Get():nullptr;s.context->VSSetConstantBuffers(1,1,&instances);s.context->PSSetConstantBuffers(0,1,&pixel);
    ID3D11ShaderResourceView* views[]{s.resident?s.resident->noise:s.resources.Noise(),s.resident?s.resident->shadows[0]:s.resources.Shadow(0),s.resident?s.resident->shadows[1]:s.resources.Shadow(1),s.resident?s.resident->lightmap:s.resources.Lightmap(),s.material[0].Get(),s.material[1].Get()};ID3D11SamplerState* samplers[6];for(unsigned i=0;i<6;++i)samplers[i]=s.samplers[i].Get();s.context->PSSetShaderResources(0,6,views);s.context->PSSetSamplers(0,6,samplers);if(s.instanceCount)s.context->DrawIndexedInstanced(s.indexCount,s.instanceCount,0,0,0);else s.context->DrawIndexed(s.indexCount,0,0);++s.submittedDraws;s.context->OMSetRenderTargets(0,nullptr,nullptr);if(s.batching)++s.batchDraws;else if(!s.resident)s.resources.FinishReads();s.refusal="none";return true;
}
bool NativeStoneRenderer::RenderBatch(NativeMaterialDraw const* draws,unsigned count,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,unsigned width,unsigned height){
    Need(bool(impl),"Stone renderer not initialized");auto& s=*impl;s.refusal="invalid or mixed material batch";
    if((!s.resident&&!s.bridgeReady)||s.batching||!draws||!count||count>128||!target||!depth||!width||!height)return false;
    for(unsigned i=0;i<count;++i){auto const& d=draws[i];if(d.eye!=draws[0].eye||d.frame!=draws[0].frame)return false;
        for(unsigned j=0;j<6;++j)if(d.samplers[j].identity!=draws[0].samplers[j].identity)return false;}
    s.batching=true;s.batchDraws=0;
    try{bool ok=true;for(unsigned i=0;i<count;++i)if(!Render(draws[i],target,depth,width,height)){ok=false;break;}
        if(!s.resident)s.resources.FinishReads();s.batching=false;s.batchDraws=0;return ok;
    }catch(...){s.batching=false;s.batchDraws=0;try{if(!s.resident)s.resources.FinishReads();}catch(...){}throw;}
}
bool NativeStoneRenderer::RenderResidentBatch(NativeMaterialDraw const* draws,unsigned count,ResidentStoneInputs const& inputs,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,unsigned width,unsigned height,float const* clearColor,bool allowInstancing){
    Need(bool(impl),"Stone renderer not initialized");auto& s=*impl;s.refusal="invalid resident pass inputs";s.submittedDraws=0;
    if(s.resident||s.batching||!target||!depth||!draws||!count||count>128||!width||!height||draws[0].elements!=std::size(Declaration))return false;
    if(clearColor)for(unsigned i=0;i<4;++i)if(!std::isfinite(clearColor[i]))return false;
    ComPtr<ID3D11Resource> rt,z;target->GetResource(&rt);depth->GetResource(&z);
    for(auto* resource:{rt.Get(),z.Get()}){ComPtr<ID3D11Device> owner;resource->GetDevice(&owner);if(owner.Get()!=s.device.Get())return false;
        ComPtr<ID3D11Texture2D> texture;if(FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture))))return false;D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);if(d.Width!=width||d.Height!=height||d.SampleDesc.Count!=1||d.ArraySize!=1)return false;}
    D3D11_RENDER_TARGET_VIEW_DESC colorDesc{};target->GetDesc(&colorDesc);D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc{};depth->GetDesc(&depthDesc);
    if(colorDesc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM||depthDesc.Format!=DXGI_FORMAT_D32_FLOAT||colorDesc.ViewDimension!=D3D11_RTV_DIMENSION_TEXTURE2D||depthDesc.ViewDimension!=D3D11_DSV_DIMENSION_TEXTURE2D||colorDesc.Texture2D.MipSlice||depthDesc.Texture2D.MipSlice||depthDesc.Flags)return false;
    ID3D11ShaderResourceView* views[]{inputs.noise,inputs.shadows[0],inputs.shadows[1],inputs.lightmap};
    for(unsigned i=0;i<4;++i){if(!views[i])return false;ComPtr<ID3D11Device> owner;views[i]->GetDevice(&owner);if(owner.Get()!=s.device.Get())return false;
        D3D11_SHADER_RESOURCE_VIEW_DESC view{};views[i]->GetDesc(&view);if(view.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||view.Texture2D.MostDetailedMip||view.Texture2D.MipLevels!=1)return false;
        if(view.Format!=((i==1||i==2)?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_B8G8R8A8_UNORM))return false;
        ComPtr<ID3D11Resource> resource;views[i]->GetResource(&resource);if(resource.Get()==rt.Get()||resource.Get()==z.Get())return false;
        ComPtr<ID3D11Texture2D> texture;if(FAILED(resource.As(&texture)))return false;D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);
        unsigned slot=i==3?3:i;if(d.SampleDesc.Count!=1||d.ArraySize!=1||d.Width!=draws[0].samplers[slot].image.Width||d.Height!=draws[0].samplers[slot].image.Height)return false;
    }
    bool instancing=allowInstancing&&count>1;
    // Instancing is legal only for identical material/atlas/camera state. A
    // per-instance lighting or atlas difference takes the ordinary batch path.
    for(unsigned i=1;i<count&&instancing;++i){auto const& a=draws[0];auto const& b=draws[i];
        if(a.eye!=b.eye||a.frame!=b.frame||a.vertexHash!=b.vertexHash||a.pixelHash!=b.pixelHash||a.matchedRow!=b.matchedRow||a.states!=b.states||a.elements!=b.elements||memcmp(a.declaration.data(),b.declaration.data(),a.elements*sizeof(D3DVERTEXELEMENT9))||a.pixel!=b.pixel||memcmp(a.vertex.data(),b.vertex.data(),48*sizeof(float))||memcmp(a.vertex.data()+60,b.vertex.data()+60,(1024-60)*sizeof(float)))instancing=false;
        for(unsigned j=0;j<6;++j)if(a.samplers[j].identity!=b.samplers[j].identity||a.samplers[j].state!=b.samplers[j].state)instancing=false;
        for(unsigned j=48;j<60;++j)if(!std::isfinite(b.vertex[j]))instancing=false;
    }
    std::vector<unsigned char> common,other;if(instancing){
        unsigned char const* first=nullptr;
        for(unsigned i=0;i<count;++i){auto const& source=draws[i].lightmap;auto* owned=OwnedStoneAtlas::Resolve(source);unsigned char const* bytes=nullptr;
            if(owned){if(owned->Bytes()!=s.additional.size()){s.refusal="resident owned atlas size mismatch";return false;}bytes=owned->Data();}
            else {auto& storage=i?other:common;storage.resize(s.additional.size());if(!source.copy||!source.context||!source.copy(source.context,storage.data(),s.vertices,unsigned(storage.size()))){s.refusal="resident instance atlas unavailable";return false;}bytes=storage.data();}
            if(!i)first=bytes;else if(first!=bytes&&std::memcmp(first,bytes,s.additional.size())){instancing=false;break;}
        }
    }
    if(instancing){std::array<float,128*12> worlds{};for(unsigned i=0;i<count;++i)std::copy_n(draws[i].vertex.data()+48,12,worlds.data()+i*12);s.context->UpdateSubresource(s.instanceWorlds.Get(),0,nullptr,worlds.data(),0,0);}
    s.submittedDraws=0;s.instanceCount=instancing?count:0;
    s.resident=&inputs;s.residentClear=clearColor;
    try{bool ok=RenderBatch(draws,instancing?1:count,target,depth,width,height);s.instanceCount=0;s.resident=nullptr;s.residentClear=nullptr;return ok;}
    catch(...){s.instanceCount=0;s.resident=nullptr;s.residentClear=nullptr;throw;}
}
unsigned NativeStoneRenderer::LastSubmittedDraws()const noexcept{return impl?impl->submittedDraws:0;}
uint64_t NativeStoneRenderer::ResourceCaptures()const noexcept{return impl&&impl->bridgeReady?impl->resources.Captures():0;}
char const* NativeStoneRenderer::LastRefusal()const noexcept{return impl?impl->refusal:"not initialized";}
}
