#include "StaticScene.h"
#include "MaterialPreview.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <unordered_set>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <limits>
namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;using namespace DirectX;
namespace {
void Need(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Check(HRESULT r,char const* why){if(FAILED(r))throw std::runtime_error(std::string(why)+" HRESULT="+std::to_string(uint32_t(r)));}
constexpr uint64_t Budget=128ull*1024*1024;
uint64_t Hash(void const* data,size_t bytes,uint64_t h=14695981039346656037ull){auto* p=static_cast<uint8_t const*>(data);for(size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;}
struct Reader {
    std::vector<uint8_t> data;size_t position=0;
    void Read(void* out,size_t n){Need(position<=data.size()&&n<=data.size()-position,"Truncated mesh");std::memcpy(out,data.data()+position,n);position+=n;}
    uint32_t U32(){uint32_t v;Read(&v,4);return v;}
};
bool Finite(XMFLOAT4X4 const& m){for(float f:std::array<float,16>{m._11,m._12,m._13,m._14,m._21,m._22,m._23,m._24,m._31,m._32,m._33,m._34,m._41,m._42,m._43,m._44})if(!std::isfinite(f))return false;return true;}
struct Bounds {XMFLOAT3 low{FLT_MAX,FLT_MAX,FLT_MAX},high{-FLT_MAX,-FLT_MAX,-FLT_MAX};};
void Add(Bounds& b,float const* p){b.low.x=std::min(b.low.x,p[0]);b.low.y=std::min(b.low.y,p[1]);b.low.z=std::min(b.low.z,p[2]);b.high.x=std::max(b.high.x,p[0]);b.high.y=std::max(b.high.y,p[1]);b.high.z=std::max(b.high.z,p[2]);}
// Reject only if all eight corners are outside one D3D clip half-space.
// Use the union of the actual two asymmetric eye frusta, not a center camera.
bool Visible(Bounds const& b,FXMMATRIX world,Camera const& c){
    XMMATRIX m=world*XMLoadFloat4x4(&c.viewProjection);unsigned planes[6]{};
    for(unsigned i=0;i<8;++i){XMFLOAT4 v;XMStoreFloat4(&v,XMVector4Transform(XMVectorSet(i&1?b.high.x:b.low.x,i&2?b.high.y:b.low.y,i&4?b.high.z:b.low.z,1),m));
        planes[0]+=v.x < -v.w;planes[1]+=v.x > v.w;planes[2]+=v.y < -v.w;planes[3]+=v.y > v.w;planes[4]+=v.z < 0;planes[5]+=v.z > v.w;}
    return std::none_of(std::begin(planes),std::end(planes),[](unsigned n){return n==8;});
}
char const* Shader=R"(
cbuffer Camera:register(b0){row_major float4x4 viewProjection;float4 tint;float4 material;float4 uvTransform;}
Texture2D<float4> diffuseTexture:register(t0);SamplerState diffuseSampler:register(s0);
struct Input{float3 p:POSITION;float3 n:NORMAL;float2 uv:TEXCOORD0;float4 color:COLOR0;
 float4 w0:INSTANCE0;float4 w1:INSTANCE1;float4 w2:INSTANCE2;float4 w3:INSTANCE3;
 float4 n0:NORMALMAT0;float4 n1:NORMALMAT1;float4 n2:NORMALMAT2;float4 n3:NORMALMAT3;};
struct Output{float4 p:SV_POSITION;float3 n:NORMAL;float2 uv:TEXCOORD0;float4 color:COLOR0;};
Output VS(Input i){Output o;float4x4 world=float4x4(i.w0,i.w1,i.w2,i.w3);
 float4x4 normals=float4x4(i.n0,i.n1,i.n2,i.n3);o.p=mul(mul(float4(i.p,1),world),viewProjection);
 o.n=mul(float4(i.n,0),normals).xyz;o.uv=i.uv;o.color=i.color;return o;}
float4 PS(Output i):SV_TARGET{float lighting=.28+.72*abs(dot(normalize(i.n),normalize(float3(.4,.8,-.3))));
 if(material.x>.5){float4 color=diffuseTexture.Sample(diffuseSampler,i.uv*uvTransform.xy);
   if(material.y>.5 && color.a<=material.z)discard;
   return float4(color.rgb*lighting,1);}
 // Fallback retains unknown materials explicitly. Original lighting, normal
 // maps, parallax and shadowing are not reproduced by the diffuse preview.
 float2 check=floor(i.uv*8);float grid=.88+.12*fmod(abs(check.x+check.y),2);
 return float4(tint.rgb*lighting*grid,1);}
)";
}
Mesh ReadMesh(std::filesystem::path const& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);Need(bool(f),"Cannot open imported mesh");auto n=f.tellg();Need(n>=20&&uint64_t(n)<=Budget,"Invalid mesh size");Reader r;r.data.resize(size_t(n));f.seekg(0);f.read(reinterpret_cast<char*>(r.data.data()),n);Need(bool(f),"Mesh read failed");
    char magic[8];r.Read(magic,8);Need(std::memcmp(magic,"G3DX11M1",8)==0&&r.U32()==1,"Mesh format/version");auto sections=r.U32();Need(sections&&sections<=4096&&r.U32()==sizeof(Vertex),"Mesh section/stride");
    Mesh mesh;mesh.resource=path.filename().string();mesh.content=Hash(r.data.data(),r.data.size());mesh.sections.reserve(sections);
    for(unsigned i=0;i<sections;++i){Section s;auto name=r.U32(),nv=r.U32(),ni=r.U32();Need(r.U32()==0&&name<=32768&&nv&&nv<=4000000&&ni&&ni<=12000000&&ni%3==0,"Mesh section sizes");
        Need(uint64_t(nv)*sizeof(Vertex)+uint64_t(ni)*4+name<=r.data.size()-r.position,"Mesh payload sizes");s.material.resize(name);r.Read(s.material.data(),name);s.vertices.resize(nv);s.indices.resize(ni);r.Read(s.vertices.data(),nv*sizeof(Vertex));r.Read(s.indices.data(),ni*4);
        for(auto const& v:s.vertices){std::array<float,12> values;std::memcpy(values.data(),&v,sizeof(v));for(float value:values)Need(std::isfinite(value)&&std::abs(value)<1.e10f,"Invalid mesh vertex");}
        for(auto index:s.indices)Need(index<nv,"Mesh index out of range");mesh.sections.push_back(std::move(s));}
    Need(r.position==r.data.size(),"Unexpected trailing mesh bytes");return mesh;
}
struct StaticScene::Impl {
    struct Part {ComPtr<ID3D11Buffer> vertices,indices;unsigned indexCount=0;std::string material;XMFLOAT4 tint;};
    struct Cached {uint64_t content=0,bytes=0;Bounds bounds;std::vector<Part> parts;};
    struct Transforms {XMFLOAT4X4 world,normal;};
    struct Group {Cached const* mesh=nullptr;unsigned start=0,count=0;};
    struct Constant {XMFLOAT4X4 vp;XMFLOAT4 tint,material,uvTransform;};
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11VertexShader> vs;ComPtr<ID3D11PixelShader> ps;ComPtr<ID3D11InputLayout> layout;
    ComPtr<ID3D11Buffer> constants,instances;ComPtr<ID3D11RasterizerState> raster;ComPtr<ID3D11DepthStencilState> depth;
    std::map<std::string,Cached> meshes;std::vector<Group> groups;std::array<Camera,2> cameras;
    MaterialPreview materials;
    std::vector<Transforms> previous;Statistics stats;size_t capacity=0;bool prepared=false;
};
StaticScene::StaticScene()=default;StaticScene::~StaticScene()=default;
void StaticScene::Initialize(ID3D11Device* device,ID3D11DeviceContext* context){
    Need(device&&context,"DX11 device/context missing");ComPtr<ID3D11Device> owner;context->GetDevice(&owner);Need(owner.Get()==device&&context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"DX11 context ownership");
    auto p=std::make_unique<Impl>();p->device=device;p->context=context;ComPtr<ID3DBlob> vs,ps,errors;
    auto compile=[&](char const* entry,char const* target,ComPtr<ID3DBlob>& code){auto r=D3DCompile(Shader,std::strlen(Shader),nullptr,nullptr,nullptr,entry,target,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&errors);if(FAILED(r))throw std::runtime_error(errors?static_cast<char*>(errors->GetBufferPointer()):"DX11 shader compile");};
    compile("VS","vs_5_0",vs);compile("PS","ps_5_0",ps);Check(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&p->vs),"Static VS");Check(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&p->ps),"Static PS");
    std::vector<D3D11_INPUT_ELEMENT_DESC> elements{{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0}};
    for(unsigned i=0;i<8;++i)elements.push_back({i<4?"INSTANCE":"NORMALMAT",i%4,DXGI_FORMAT_R32G32B32A32_FLOAT,1,i*16,D3D11_INPUT_PER_INSTANCE_DATA,1});
    Check(device->CreateInputLayout(elements.data(),UINT(elements.size()),vs->GetBufferPointer(),vs->GetBufferSize(),&p->layout),"Static input layout");
    D3D11_BUFFER_DESC cb{};cb.ByteWidth=sizeof(Impl::Constant);cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Check(device->CreateBuffer(&cb,nullptr,&p->constants),"Static constants");
    D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;Check(device->CreateRasterizerState(&rs,&p->raster),"Static raster");
    D3D11_DEPTH_STENCIL_DESC ds{};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS;Check(device->CreateDepthStencilState(&ds,&p->depth),"Static depth");impl=std::move(p);
}
void StaticScene::Upload(Mesh const& mesh){
    Need(impl&&!mesh.resource.empty()&&mesh.content&&!mesh.sections.empty()&&mesh.sections.size()<=4096,"Invalid static mesh upload");auto& p=*impl;
    auto found=p.meshes.find(mesh.resource);if(found!=p.meshes.end()&&found->second.content==mesh.content)return;
    Impl::Cached cached;cached.content=mesh.content;
    for(auto const& s:mesh.sections){Need(!s.vertices.empty()&&!s.indices.empty()&&s.indices.size()%3==0,"Empty/nontriangle mesh section");
        uint64_t vb=s.vertices.size()*sizeof(Vertex),ib=s.indices.size()*sizeof(uint32_t);Need(vb+ib<=Budget&&cached.bytes+vb+ib<=Budget,"Static mesh memory budget");cached.bytes+=vb+ib;}
    uint64_t old=found==p.meshes.end()?0:found->second.bytes;Need(p.stats.geometryBytes-old+cached.bytes<=Budget,"Static scene memory budget");
    for(auto const& s:mesh.sections){Impl::Part part;part.indexCount=unsigned(s.indices.size());part.material=s.material;auto h=Hash(s.material.data(),s.material.size());part.tint={.35f+.5f*float(h&255)/255,.35f+.5f*float((h>>8)&255)/255,.35f+.5f*float((h>>16)&255)/255,1};
        for(auto const& v:s.vertices){std::array<float,12> values;std::memcpy(values.data(),&v,sizeof(v));for(float f:values)Need(std::isfinite(f)&&std::abs(f)<1.e10f,"Nonfinite/excessive mesh vertex");Add(cached.bounds,v.position);}
        for(auto index:s.indices)Need(index<s.vertices.size(),"Static index out of bounds");
        auto buffer=[&](void const* data,UINT bytes,UINT flags,ComPtr<ID3D11Buffer>& out){D3D11_BUFFER_DESC d{};d.ByteWidth=bytes;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=flags;D3D11_SUBRESOURCE_DATA init{data,0,0};Check(p.device->CreateBuffer(&d,&init,&out),"Immutable static geometry");};
        buffer(s.vertices.data(),UINT(s.vertices.size()*sizeof(Vertex)),D3D11_BIND_VERTEX_BUFFER,part.vertices);buffer(s.indices.data(),UINT(s.indices.size()*4),D3D11_BIND_INDEX_BUFFER,part.indices);cached.parts.push_back(std::move(part));}
    // Publish only after every part succeeds. A bad replacement keeps old data.
    auto bytes=cached.bytes;p.meshes.insert_or_assign(mesh.resource,std::move(cached));p.stats.geometryBytes=p.stats.geometryBytes-old+bytes;++p.stats.geometryUploads;p.prepared=false;p.groups.clear();
}
void StaticScene::Evict(std::string const& resource){Need(bool(impl),"Static renderer not initialized");auto& p=*impl;auto i=p.meshes.find(resource);if(i!=p.meshes.end()){p.stats.geometryBytes-=i->second.bytes;p.meshes.erase(i);p.prepared=false;p.groups.clear();}}
void StaticScene::LoadMaterialPreview(std::filesystem::path const& path){Need(bool(impl),"Static renderer not initialized");auto& p=*impl;if(p.materials.Load(p.device.Get(),path)){p.prepared=false;p.groups.clear();p.stats.textureUploads=p.materials.Uploads();p.stats.textureBytes=p.materials.Bytes();p.stats.materials=p.materials.Count();}}
void StaticScene::Prepare(std::vector<Instance> const& instances,std::array<Camera,2> const& cameras){
    Need(impl&&instances.size()<=65536,"Static preparation state/count");auto& p=*impl;p.prepared=false;p.groups.clear();for(auto const& c:cameras)Need(Finite(c.viewProjection),"Nonfinite stereo camera");
    std::map<std::string,std::vector<Impl::Transforms>> groups;std::unordered_set<uint64_t> ids;
    unsigned visible=0;for(auto const& i:instances){Need(i.id&&ids.insert(i.id).second&&Finite(i.world)&&i.world._14==0&&i.world._24==0&&i.world._34==0&&i.world._44==1,"Invalid/duplicate static instance");
        auto mesh=p.meshes.find(i.resource);Need(mesh!=p.meshes.end()&&i.content==mesh->second.content,"Missing/stale static mesh content");auto world=XMLoadFloat4x4(&i.world);XMVECTOR determinant;auto inverse=XMMatrixInverse(&determinant,world);Need(std::abs(XMVectorGetX(determinant))>1.e-8f,"Singular instance transform");
        if(!Visible(mesh->second.bounds,world,cameras[0])&&!Visible(mesh->second.bounds,world,cameras[1]))continue;
        Impl::Transforms t;t.world=i.world;XMStoreFloat4x4(&t.normal,XMMatrixTranspose(inverse));groups[i.resource].push_back(t);++visible;}
    std::vector<Impl::Transforms> transforms;transforms.reserve(visible);
    for(auto const& entry:groups){p.groups.push_back({&p.meshes.at(entry.first),unsigned(transforms.size()),unsigned(entry.second.size())});transforms.insert(transforms.end(),entry.second.begin(),entry.second.end());}
    bool changed=transforms.size()!=p.previous.size()||(!transforms.empty()&&std::memcmp(transforms.data(),p.previous.data(),transforms.size()*sizeof(transforms[0]))!=0);
    if(changed&&!transforms.empty()){
        if(transforms.size()>p.capacity){size_t capacity=1;while(capacity<transforms.size())capacity*=2;D3D11_BUFFER_DESC d{};d.ByteWidth=UINT(capacity*sizeof(Impl::Transforms));d.Usage=D3D11_USAGE_DYNAMIC;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;d.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;ComPtr<ID3D11Buffer> buffer;Check(p.device->CreateBuffer(&d,nullptr,&buffer),"Static instance buffer");p.instances=buffer;p.capacity=capacity;}
        D3D11_MAPPED_SUBRESOURCE mapped{};Check(p.context->Map(p.instances.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped),"Static instance upload");std::memcpy(mapped.pData,transforms.data(),transforms.size()*sizeof(transforms[0]));p.context->Unmap(p.instances.Get(),0);++p.stats.instanceUploads;p.stats.instanceBytes+=transforms.size()*sizeof(transforms[0]);}
    p.previous=std::move(transforms);p.cameras=cameras;++p.stats.preparations;p.stats.submitted=unsigned(instances.size());p.stats.visible=visible;p.stats.culled=unsigned(instances.size())-visible;p.stats.groups=unsigned(p.groups.size());p.stats.draws[0]=p.stats.draws[1]=p.stats.texturedDraws[0]=p.stats.texturedDraws[1]=0;p.prepared=true;
}
void StaticScene::Render(unsigned eye,EyeTarget const& target,bool instanced){
    Need(impl&&impl->prepared&&eye<2&&target.color&&target.depth&&target.width&&target.height&&target.width<=16384&&target.height<=16384,"Static eye state/target");auto& p=*impl;auto* c=p.context.Get();
    // Verify same-device, matching single-sample color/depth resources before binding.
    for(unsigned index=0;index<2;++index){ComPtr<ID3D11Resource> resource;if(index)target.depth->GetResource(&resource);else target.color->GetResource(&resource);ComPtr<ID3D11Texture2D> texture;Check(resource.As(&texture),"Static eye must be Texture2D");ComPtr<ID3D11Device> owner;resource->GetDevice(&owner);D3D11_TEXTURE2D_DESC d{};texture->GetDesc(&d);Need(owner.Get()==p.device.Get()&&d.Width==target.width&&d.Height==target.height&&d.SampleDesc.Count==1,"Static eye device/dimensions/samples");}
    c->OMSetRenderTargets(1,&target.color,target.depth);c->OMSetBlendState(nullptr,nullptr,~0u);c->OMSetDepthStencilState(p.depth.Get(),0);c->RSSetState(p.raster.Get());D3D11_VIEWPORT viewport{0,0,float(target.width),float(target.height),0,1};c->RSSetViewports(1,&viewport);
    c->IASetInputLayout(p.layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->VSSetShader(p.vs.Get(),nullptr,0);c->PSSetShader(p.ps.Get(),nullptr,0);c->GSSetShader(nullptr,nullptr,0);c->HSSetShader(nullptr,nullptr,0);c->DSSetShader(nullptr,nullptr,0);ID3D11Buffer* cb=p.constants.Get();c->VSSetConstantBuffers(0,1,&cb);c->PSSetConstantBuffers(0,1,&cb);
    unsigned draws=0,textured=0;for(auto const& group:p.groups)for(auto const& part:group.mesh->parts){
        auto const* material=p.materials.Find(part.material);Impl::Constant data{p.cameras[eye].viewProjection,part.tint,{0,0,0,0},{1,1,0,0}};
        ID3D11ShaderResourceView* diffuse=nullptr;ID3D11SamplerState* sampler=nullptr;
        if(material){data.material={1,material->masked?1.f:0.f,material->alphaReference,0};data.uvTransform={material->scale[0],material->scale[1],0,0};diffuse=material->diffuse.Get();sampler=material->sampler.Get();textured+=instanced?1:group.count;}
        c->PSSetShaderResources(0,1,&diffuse);c->PSSetSamplers(0,1,&sampler);c->UpdateSubresource(p.constants.Get(),0,nullptr,&data,0,0);c->IASetIndexBuffer(part.indices.Get(),DXGI_FORMAT_R32_UINT,0);
        ID3D11Buffer* buffers[]{part.vertices.Get(),p.instances.Get()};UINT strides[]{sizeof(Vertex),sizeof(Impl::Transforms)},offsets[]{0,0};c->IASetVertexBuffers(0,2,buffers,strides,offsets);
        if(instanced){c->DrawIndexedInstanced(part.indexCount,group.count,0,0,group.start);++draws;}
        else for(unsigned i=0;i<group.count;++i){c->DrawIndexedInstanced(part.indexCount,1,0,0,group.start+i);++draws;}
    }p.stats.draws[eye]=draws;p.stats.texturedDraws[eye]=textured;
}
Statistics StaticScene::Stats()const{Need(bool(impl),"Static renderer not initialized");return impl->stats;}
void StaticScene::Reset()noexcept{impl.reset();}
bool ReplacementProof::CanOmitNative()const noexcept {
    if(!frame||!epoch||!geometryContent||!materialsExact||!depthCompatible||!liveEntity)return false;
    for(unsigned eye=0;eye<2;++eye)if(eyeFrame[eye]!=frame||eyeEpoch[eye]!=epoch||eyeContent[eye]!=geometryContent)return false;return true;
}
}
