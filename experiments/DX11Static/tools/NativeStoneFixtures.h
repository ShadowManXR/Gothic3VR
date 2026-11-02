#pragma once
#include <d3d9.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <windows.h>
#include <array>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include "NativeStageInterface.h"
#include "NativeMaterialResources.h"

using Microsoft::WRL::ComPtr;
namespace {
unsigned W=128,H=96;constexpr unsigned T=8;
void Need(bool value,char const* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,char const* message){if(FAILED(result))throw std::runtime_error(std::string(message)+" HRESULT="+std::to_string(unsigned(result)));}
std::vector<char> Read(std::filesystem::path const& path,size_t limit){std::ifstream f(path,std::ios::binary|std::ios::ate);Need(bool(f),"Missing local material fixture");auto n=f.tellg();Need(n>0&&uint64_t(n)<=limit,"Material fixture size");std::vector<char> result(size_t(n),0);f.seekg(0);f.read(result.data(),n);Need(bool(f),"Material fixture read");return result;}
struct Window {HWND value=nullptr;Window(){value=CreateWindowExW(0,L"STATIC",L"Gothic native material reference",WS_POPUP,0,0,W,H,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);Need(value!=nullptr,"Owned hidden reference window");}~Window(){DestroyWindow(value);}};
struct Library {HMODULE value=nullptr;~Library(){if(value)FreeLibrary(value);}};
ComPtr<ID3DBlob> Compile(std::string const& source,char const* entry,char const* profile){ComPtr<ID3DBlob> code,error;auto hr=D3DCompile(source.data(),source.size(),nullptr,nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error);if(FAILED(hr))throw std::runtime_error(error?std::string(static_cast<char*>(error->GetBufferPointer()),error->GetBufferSize()):"Material compile");return code;}
char const* VertexShader=R"(
cbuffer Frame:register(b0){float4 values[9];float4 rasterOffset;}
struct Out{float4 position:POSITION;float4 v0:COLOR0;float4 v1:COLOR1;float4 v2:TEXCOORD0;float4 unused:TEXCOORD1;float4 v3:TEXCOORD2;float4 v4:TEXCOORD3;float4 v5:TEXCOORD4;float4 v6:TEXCOORD5;float4 v7:TEXCOORD6;float4 v8:TEXCOORD7;};
Out VS(float4 p:POSITION){Out o;o.position=p+rasterOffset;float2 uv=p.xy*float2(.5,-.5)+.5;
o.v0=values[0];o.v1=values[1];o.v2=values[2];o.v3=values[3];o.v4=values[4];o.v5=values[5];o.v6=values[6];o.v7=values[7];o.v8=values[8];
o.unused=0;o.v2.xy+=uv*2.5;o.v3.xy+=uv*.7;o.v7.xy+=uv*.8;o.v8.xy+=uv*.6;return o;})";
struct Target9 {
    std::array<ComPtr<IDirect3DTexture9>,2> textures;std::array<ComPtr<IDirect3DSurface9>,2> surfaces,readback;
    void Create(IDirect3DDevice9Ex* d){for(unsigned i=0;i<2;++i){Check(d->CreateTexture(W,H,1,D3DUSAGE_RENDERTARGET,D3DFMT_A32B32G32R32F,D3DPOOL_DEFAULT,&textures[i],nullptr),"Native HDR reference target");Check(textures[i]->GetSurfaceLevel(0,&surfaces[i]),"Native HDR surface");Check(d->CreateOffscreenPlainSurface(W,H,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,&readback[i],nullptr),"Native HDR readback");}}
    void Bind(IDirect3DDevice9Ex* d){Check(d->SetDepthStencilSurface(nullptr),"Reference native depth off");for(unsigned i=0;i<2;++i)Check(d->SetRenderTarget(i,surfaces[i].Get()),"Reference native MRT");D3DVIEWPORT9 v{0,0,W,H,0,1};Check(d->SetViewport(&v),"Native reference viewport");}
    std::vector<float> Pixels(IDirect3DDevice9Ex* d,unsigned index){Check(d->GetRenderTargetData(surfaces[index].Get(),readback[index].Get()),"Read native material reference");D3DLOCKED_RECT r{};Check(readback[index]->LockRect(&r,nullptr,D3DLOCK_READONLY),"Map native material reference");std::vector<float> result(W*H*4);for(unsigned y=0;y<H;++y)memcpy(result.data()+y*W*4,static_cast<char*>(r.pBits)+y*r.Pitch,W*16);Check(readback[index]->UnlockRect(),"Unmap native material reference");return result;}
};
struct Target11 {
    std::array<ComPtr<ID3D11Texture2D>,2> textures,readback;std::array<ComPtr<ID3D11RenderTargetView>,2> views;
    void Create(ID3D11Device* d){D3D11_TEXTURE2D_DESC desc{};desc.Width=W;desc.Height=H;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
        for(unsigned i=0;i<2;++i){Check(d->CreateTexture2D(&desc,nullptr,&textures[i]),"DX11 HDR reference target");Check(d->CreateRenderTargetView(textures[i].Get(),nullptr,&views[i]),"DX11 HDR reference view");auto staging=desc;staging.BindFlags=0;staging.Usage=D3D11_USAGE_STAGING;staging.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Check(d->CreateTexture2D(&staging,nullptr,&readback[i]),"DX11 HDR readback");}}
    std::vector<float> Pixels(ID3D11DeviceContext* c,unsigned index){c->CopyResource(readback[index].Get(),textures[index].Get());D3D11_MAPPED_SUBRESOURCE r{};Check(c->Map(readback[index].Get(),0,D3D11_MAP_READ,0,&r),"Map DX11 material reference");std::vector<float> result(W*H*4);for(unsigned y=0;y<H;++y)memcpy(result.data()+y*W*4,static_cast<char*>(r.pData)+y*r.RowPitch,W*16);c->Unmap(readback[index].Get(),0);return result;}
};
struct TexturePair {ComPtr<IDirect3DTexture9> native;ComPtr<ID3D11ShaderResourceView> view;};
TexturePair OriginalDds(IDirect3DDevice9Ex* d9,ID3D11Device* d11,std::filesystem::path const& path){
    auto file=Read(path,32*1024*1024);Need(file.size()>=128&&!memcmp(file.data(),"DDS ",4),"Original DDS header");auto u32=[&](unsigned at){uint32_t value;memcpy(&value,file.data()+at,4);return value;};
    unsigned w=u32(16),h=u32(12),mips=u32(28),fourcc=u32(84);Need(w&&h&&w<=4096&&h<=4096&&mips&&mips<=13,"Original DDS dimensions");Need(fourcc==0x31545844||fourcc==0x35545844,"Original DDS compression");unsigned block=fourcc==0x31545844?8:16;
    TexturePair result;ComPtr<IDirect3DTexture9> staging;auto format=D3DFORMAT(fourcc);Check(d9->CreateTexture(w,h,mips,0,format,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Original DDS staging");Check(d9->CreateTexture(w,h,mips,0,format,D3DPOOL_DEFAULT,&result.native,nullptr),"Original DDS native texture");std::vector<D3D11_SUBRESOURCE_DATA> levels;size_t at=128;
    for(unsigned level=0;level<mips;++level){unsigned pitch=std::max(1u,(std::max(1u,w>>level)+3)/4)*block,rows=std::max(1u,(std::max(1u,h>>level)+3)/4);size_t bytes=size_t(pitch)*rows;Need(at<=file.size()&&bytes<=file.size()-at,"Original DDS mip bounds");D3DLOCKED_RECT map{};Check(staging->LockRect(level,&map,nullptr,0),"Original DDS map");for(unsigned y=0;y<rows;++y)memcpy(static_cast<char*>(map.pBits)+y*map.Pitch,file.data()+at+y*pitch,pitch);Check(staging->UnlockRect(level),"Original DDS unmap");levels.push_back({file.data()+at,pitch,UINT(bytes)});at+=bytes;}
    Need(at==file.size(),"Original DDS trailing payload");Check(d9->UpdateTexture(staging.Get(),result.native.Get()),"Original DDS native upload");D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=mips;desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=block==8?DXGI_FORMAT_BC1_UNORM:DXGI_FORMAT_BC3_UNORM;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;ComPtr<ID3D11Texture2D> texture;Check(d11->CreateTexture2D(&desc,levels.data(),&texture),"Original DDS DX11 texture");Check(d11->CreateShaderResourceView(texture.Get(),nullptr,&result.view),"Original DDS view");return result;
}
TexturePair OriginalLightmap(IDirect3DDevice9Ex* d9,ID3D11Device* d11,std::filesystem::path const& path){
    auto file=Read(path,32*1024*1024);Need(file.size()>=16&&!memcmp(file.data(),"G3LMAP01",8),"Native lightmap atlas format");uint32_t w,h;memcpy(&w,file.data()+8,4);memcpy(&h,file.data()+12,4);Need(w&&h&&w<=2048&&h<=2048&&file.size()==16+size_t(w)*h*4,"Native atlas bounds");
    TexturePair result;ComPtr<IDirect3DTexture9> staging;Check(d9->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Original atlas staging");Check(d9->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&result.native,nullptr),"Original atlas texture");D3DLOCKED_RECT map{};Check(staging->LockRect(0,&map,nullptr,0),"Original atlas map");for(unsigned y=0;y<h;++y)memcpy(static_cast<char*>(map.pBits)+y*map.Pitch,file.data()+16+y*w*4,w*4);Check(staging->UnlockRect(0),"Original atlas unmap");Check(d9->UpdateTexture(staging.Get(),result.native.Get()),"Original atlas upload");
    D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{file.data()+16,w*4,0};ComPtr<ID3D11Texture2D> texture;Check(d11->CreateTexture2D(&desc,&data,&texture),"Original atlas DX11 texture");Check(d11->CreateShaderResourceView(texture.Get(),nullptr,&result.view),"Original atlas view");return result;
}
struct NativeScene {
    unsigned vertices=0,indices=0;std::array<unsigned,3> strides{32,16,24};
    std::array<ComPtr<IDirect3DVertexBuffer9>,3> buffers9;ComPtr<IDirect3DIndexBuffer9> index9;
    std::array<ComPtr<ID3D11Buffer>,3> buffers11;ComPtr<ID3D11Buffer> index11;
    void Create(IDirect3DDevice9Ex* d9,ID3D11Device* d11,std::filesystem::path const& path){
        auto file=Read(path,16*1024*1024);Need(file.size()>=32&&!memcmp(file.data(),"G3NATIV1",8),"Native scene fixture format");uint32_t header[6];memcpy(header,file.data()+8,24);Need(header[0]==1&&header[1]&&header[1]<=100000&&header[2]&&header[2]<=1000000&&header[2]%3==0&&header[3]==32&&header[4]==16&&header[5]==24,"Native scene fixture bounds");vertices=header[1];indices=header[2];Need(file.size()==32+size_t(vertices)*72+size_t(indices)*4,"Native scene data size");size_t at=32;
        for(unsigned i=0;i<3;++i){unsigned size=vertices*strides[i];Check(d9->CreateVertexBuffer(size,D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&buffers9[i],nullptr),"Native scene VB");void* map=nullptr;Check(buffers9[i]->Lock(0,size,&map,0),"Native scene VB map");memcpy(map,file.data()+at,size);Check(buffers9[i]->Unlock(),"Native scene VB unmap");D3D11_BUFFER_DESC desc{};desc.ByteWidth=size;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA data{file.data()+at,0,0};Check(d11->CreateBuffer(&desc,&data,&buffers11[i]),"DX11 scene VB");at+=size;}
        for(unsigned i=0;i<indices;++i){uint32_t index;memcpy(&index,file.data()+at+i*4,4);Need(index<vertices,"Native scene index bounds");}
        Check(d9->CreateIndexBuffer(indices*4,D3DUSAGE_WRITEONLY,D3DFMT_INDEX32,D3DPOOL_DEFAULT,&index9,nullptr),"Native scene IB");void* map=nullptr;Check(index9->Lock(0,indices*4,&map,0),"Native scene IB map");memcpy(map,file.data()+at,indices*4);Check(index9->Unlock(),"Native scene IB unmap");D3D11_BUFFER_DESC desc{};desc.ByteWidth=indices*4;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_INDEX_BUFFER;D3D11_SUBRESOURCE_DATA data{file.data()+at,0,0};Check(d11->CreateBuffer(&desc,&data,&index11),"DX11 scene IB");
    }
    void Bind(IDirect3DDevice9Ex* d9,ID3D11DeviceContext* context){for(unsigned i=0;i<3;++i){Check(d9->SetStreamSource(i,buffers9[i].Get(),0,strides[i]),"Native scene stream bind");auto buffer=buffers11[i].Get();unsigned offset=0;context->IASetVertexBuffers(i,1,&buffer,&strides[i],&offset);}Check(d9->SetIndices(index9.Get()),"Native scene index bind");context->IASetIndexBuffer(index11.Get(),DXGI_FORMAT_R32_UINT,0);context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);}
};
TexturePair Color(IDirect3DDevice9Ex* d9,ID3D11Device* d11,unsigned slot,unsigned side=T){
    std::vector<uint32_t> pixels(size_t(side)*side);for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned r=40+(x*17+y*11+slot*23)%160,g=45+(x*13+y*7+slot*29)%150,b=30+(x*3+y*21+slot*13)%180,a=100+(x*7+y*5)%40;
        if(slot==4){r=128;b=255;g=115+(x+y)%20;a=115+(x*3+y)%20;}if(slot==5)a=180+(x+y)%50;pixels[y*side+x]=a<<24|r<<16|g<<8|b;}
    TexturePair result;ComPtr<IDirect3DTexture9> staging;Check(d9->CreateTexture(side,side,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr),"Native material texture staging");Check(d9->CreateTexture(side,side,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&result.native,nullptr),"Native material texture");D3DLOCKED_RECT map{};Check(staging->LockRect(0,&map,nullptr,0),"Native material texture map");for(unsigned y=0;y<side;++y)memcpy(static_cast<char*>(map.pBits)+y*map.Pitch,pixels.data()+y*side,side*4);staging->UnlockRect(0);Check(d9->UpdateTexture(staging.Get(),result.native.Get()),"Native material texture upload");
    D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=side;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{pixels.data(),side*4,0};ComPtr<ID3D11Texture2D> texture;Check(d11->CreateTexture2D(&desc,&data,&texture),"DX11 material texture");Check(d11->CreateShaderResourceView(texture.Get(),nullptr,&result.view),"DX11 material texture view");return result;
}
TexturePair Shadow(IDirect3DDevice9Ex* d9,ID3D11Device* d11,unsigned slot,unsigned side=T){
    TexturePair result;Check(d9->CreateTexture(side,side,1,D3DUSAGE_DEPTHSTENCIL,D3DFMT_D24X8,D3DPOOL_DEFAULT,&result.native,nullptr),"Native comparison-depth fixture");ComPtr<IDirect3DSurface9> depth;Check(result.native->GetSurfaceLevel(0,&depth),"Native comparison-depth surface");
    ComPtr<IDirect3DTexture9> color;ComPtr<IDirect3DSurface9> surface;Check(d9->CreateTexture(side,side,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&color,nullptr),"Depth fixture color");Check(color->GetSurfaceLevel(0,&surface),"Depth fixture surface");Check(d9->SetDepthStencilSurface(nullptr),"Depth fixture unbind");Check(d9->SetRenderTarget(1,nullptr),"Depth fixture MRT off");Check(d9->SetRenderTarget(0,surface.Get()),"Depth fixture color bind");Check(d9->SetDepthStencilSurface(depth.Get()),"Depth fixture bind");D3DVIEWPORT9 viewport{0,0,side,side,0,1};Check(d9->SetViewport(&viewport),"Depth fixture viewport");
    std::vector<float> values(size_t(side)*side);for(unsigned q=0;q<4;++q){float z=float(1+((q+slot)%4))*.2f;z=float(std::round(double(z)*16777215.)/16777215.);D3DRECT rect{LONG((q%2)*side/2),LONG((q/2)*side/2),LONG((q%2+1)*side/2),LONG((q/2+1)*side/2)};Check(d9->Clear(1,&rect,D3DCLEAR_ZBUFFER,0,z,0),"Depth fixture quadrant clear");for(unsigned y=unsigned(rect.y1);y<unsigned(rect.y2);++y)for(unsigned x=unsigned(rect.x1);x<unsigned(rect.x2);++x)values[y*side+x]=z;}
    Check(d9->SetDepthStencilSurface(nullptr),"Depth fixture release bind");D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=side;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R32_FLOAT;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{values.data(),side*4,0};ComPtr<ID3D11Texture2D> texture;Check(d11->CreateTexture2D(&desc,&data,&texture),"DX11 depth fixture");Check(d11->CreateShaderResourceView(texture.Get(),nullptr,&result.view),"DX11 comparison-depth view");return result;
}
struct Error {double sum=0;float maximum=0,nativeAtMax=0,dx11AtMax=0;size_t maximumIndex=0;uint64_t values=0,aboveMilli=0;void Compare(std::vector<float> const& a,std::vector<float> const& b){Need(a.size()==b.size(),"Material comparison dimensions");for(size_t i=0;i<a.size();++i){if(!std::isfinite(a[i])||!std::isfinite(b[i]))throw std::runtime_error("Nonfinite component "+std::to_string(i)+" native="+std::to_string(a[i])+" dx11="+std::to_string(b[i]));float error=std::abs(a[i]-b[i]);if(error>maximum){maximum=error;maximumIndex=i;nativeAtMax=a[i];dx11AtMax=b[i];}aboveMilli+=error>.001f;sum+=error;++values;}}};
}
