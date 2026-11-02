#include "MaterialPreview.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cmath>
namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
namespace {
void Need(bool b,char const* why){if(!b)throw std::runtime_error(why);}
void Check(HRESULT r,char const* why){Need(SUCCEEDED(r),why);}
uint64_t Hash(void const* data,size_t bytes){uint64_t h=14695981039346656037ull;auto* p=static_cast<uint8_t const*>(data);for(size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;}
struct Reader {
    std::vector<uint8_t> data;size_t pos=0;
    explicit Reader(std::filesystem::path const& path,size_t limit){std::ifstream f(path,std::ios::binary|std::ios::ate);Need(bool(f),"Material/texture file missing");auto n=f.tellg();Need(n>0&&uint64_t(n)<=limit,"Excessive material/texture file");data.resize(size_t(n));f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),n);Need(bool(f),"Material/texture file read failed");}
    void Read(void* value,size_t bytes){Need(pos<=data.size()&&bytes<=data.size()-pos,"Truncated material/texture");std::memcpy(value,data.data()+pos,bytes);pos+=bytes;}
    uint32_t U32(){uint32_t v;Read(&v,4);return v;}
    uint64_t U64(){uint64_t v;Read(&v,8);return v;}
    float Float(){float v;Read(&v,4);Need(std::isfinite(v),"Nonfinite material field");return v;}
    std::string String(){auto n=U32();Need(n&&n<=4096,"Invalid material name length");std::string s(n,'\0');Read(s.data(),n);Need(s.find('\0')==std::string::npos,"Embedded material name terminator");return s;}
};
struct Loaded {ComPtr<ID3D11ShaderResourceView> view;uint64_t bytes=0;};
Loaded LoadDds(ID3D11Device* device,std::filesystem::path const& path,uint64_t content){
    Reader r(path,128*1024*1024);Need(Hash(r.data.data(),r.data.size())==content,"Texture content hash mismatch");
    Need(r.U32()==0x20534444&&r.U32()==124,"DDS magic/header");uint32_t flags=r.U32(),height=r.U32(),width=r.U32();r.U32();auto depth=r.U32(),mips=r.U32();
    Need((flags&0x1007)==0x1007&&width&&height&&width<=16384&&height<=16384&&depth<=1&&mips&&mips<=15,"DDS dimensions/flags");
    uint32_t maxMip=1;for(auto n=std::max(width,height);n>1;n>>=1)++maxMip;Need(mips<=maxMip,"Excessive DDS mip chain");
    r.pos=76;Need(r.U32()==32&&r.U32()==4,"DDS pixel format");auto fourcc=r.U32();DXGI_FORMAT format;
    unsigned block=16;if(fourcc==0x31545844){format=DXGI_FORMAT_BC1_UNORM_SRGB;block=8;}else if(fourcc==0x33545844)format=DXGI_FORMAT_BC2_UNORM_SRGB;else if(fourcc==0x35545844)format=DXGI_FORMAT_BC3_UNORM_SRGB;else throw std::runtime_error("Unsupported DDS BC format");
    r.pos=112;Need(r.U32()==0,"DDS array/cube/volume not supported");size_t offset=128;std::vector<D3D11_SUBRESOURCE_DATA> inputs;Loaded loaded;
    for(unsigned level=0;level<mips;++level){unsigned w=std::max(1u,width>>level),h=std::max(1u,height>>level),pitch=std::max(1u,(w+3)/4)*block;uint64_t size=uint64_t(pitch)*std::max(1u,(h+3)/4);
        Need(offset<=r.data.size()&&size<=r.data.size()-offset,"DDS mip exceeds payload");inputs.push_back({r.data.data()+offset,pitch,UINT(size)});offset+=size_t(size);loaded.bytes+=size;}
    Need(offset==r.data.size(),"DDS trailing data or missing mip levels");D3D11_TEXTURE2D_DESC d{};d.Width=width;d.Height=height;d.MipLevels=mips;d.ArraySize=d.SampleDesc.Count=1;d.Format=format;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> texture;Check(device->CreateTexture2D(&d,inputs.data(),&texture),"DX11 native diffuse texture creation");Check(device->CreateShaderResourceView(texture.Get(),nullptr,&loaded.view),"DX11 diffuse SRV");return loaded;
}
}
bool MaterialPreview::Load(ID3D11Device* device,std::filesystem::path const& path){
    Need(device!=nullptr,"Material library device missing");Reader r(path,16*1024*1024);auto hash=Hash(r.data.data(),r.data.size());if(hash==content)return false;
    char magic[8];r.Read(magic,8);Need(std::memcmp(magic,"G3DX11A1",8)==0&&r.U32()==1,"Material library format/version");auto count=r.U32();Need(count&&count<=4096,"Material library count");
    std::map<std::string,Entry> next;std::map<uint64_t,Texture> nextTextures;uint64_t nextBytes=0,newUploads=0;
    for(unsigned i=0;i<count;++i){auto name=r.String(),filename=r.String();uint64_t textureContent=r.U64();auto blend=r.U32(),mask=r.U32(),u=r.U32(),v=r.U32();Entry e;e.scale[0]=r.Float();e.scale[1]=r.Float();
        Need(!next.count(name)&&textureContent&&blend<=1&&mask<=255&&u<=2&&v<=2&&std::abs(e.scale[0])<10000&&std::abs(e.scale[1])<10000,"Invalid material/sampler fields");
        std::filesystem::path file(filename);Need(file==file.filename()&&file.extension()==L".dds"&&filename.find(':')==std::string::npos,"Material texture must be a local DDS basename");
        if(!nextTextures.count(textureContent)){
            auto previous=textures.find(textureContent);Texture t;
            if(previous!=textures.end())t=previous->second;else{auto loaded=LoadDds(device,path.parent_path()/L"textures"/file,textureContent);t.view=loaded.view;t.bytes=loaded.bytes;++newUploads;}
            nextBytes+=t.bytes;Need(nextBytes<=256ull*1024*1024,"Diffuse texture memory budget exceeded");nextTextures.emplace(textureContent,std::move(t));}
        e.diffuse=nextTextures.at(textureContent).view;e.masked=blend==1;e.alphaReference=float(mask)/255;
        constexpr D3D11_TEXTURE_ADDRESS_MODE address[]={D3D11_TEXTURE_ADDRESS_WRAP,D3D11_TEXTURE_ADDRESS_CLAMP,D3D11_TEXTURE_ADDRESS_MIRROR};D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_ANISOTROPIC;sampler.AddressU=address[u];sampler.AddressV=address[v];sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sampler.MaxAnisotropy=8;sampler.ComparisonFunc=D3D11_COMPARISON_NEVER;sampler.MinLOD=0;sampler.MaxLOD=D3D11_FLOAT32_MAX;
        Check(device->CreateSamplerState(&sampler,&e.sampler),"Native diffuse sampler");next.emplace(std::move(name),std::move(e));}
    Need(r.pos==r.data.size(),"Trailing material library bytes");entries=std::move(next);textures=std::move(nextTextures);content=hash;bytes=nextBytes;uploads+=newUploads;return true;
}
MaterialPreview::Entry const* MaterialPreview::Find(std::string const& name)const noexcept{auto i=entries.find(name);return i==entries.end()?nullptr:&i->second;}
}
