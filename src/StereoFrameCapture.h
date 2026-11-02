#pragma once
// Development-only, explicitly requested stereo video. No CPU readback or disk
// writes during the recorded interval. The production build excludes this file.
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cstdint>
namespace diagnostic {
class StereoFrameCapture {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    static void Check(HRESULT r){if(FAILED(r))throw std::runtime_error("Stereo video GPU operation failed");}
public:
    static constexpr unsigned Width=384,Height=216,Limit=256,LongLimit=4096,ArraySlices=2048;
    struct Frame {
        uint64_t serial=0,qpc=0;int64_t displayTime=0,displayPeriod=0;
        bool shouldRender=false,submitted=false;unsigned eyeMask=0,cameraMask=0;
        float pose[2][7]{},fov[2][4]{},view[2][16]{},projection[2][16]{};
    };
private:
    Ptr<ID3D11Device> device;
    std::vector<Ptr<ID3D11Texture2D>> history;
    Ptr<ID3D11Texture2D> scratch;
    Ptr<ID3D11RenderTargetView> target;
    Ptr<ID3D11VertexShader> vs;
    Ptr<ID3D11PixelShader> ps;
    Ptr<ID3D11SamplerState> sampler;
    Ptr<ID3D11RasterizerState> raster;
    Ptr<ID3D11DepthStencilState> depth;
    Ptr<ID3D11BlendState> blend;
    struct Source {Ptr<ID3D11Texture2D> image;Ptr<ID3D11ShaderResourceView> view;};
    std::vector<Source> sources;
    std::vector<Frame> frames;
    Frame current;
    uint64_t started=0;bool active=false;
    unsigned capacity=Limit;
    unsigned width=Width,height=Height,bytesPerPixel=4;
    uint64_t durationMs=8000;
public:
    bool Active()const noexcept{return active;}
    size_t Count()const noexcept{return frames.size();}
    void Start(ID3D11Device* d,unsigned maxPairs=Limit,bool longLuma=false){
        if(active||!d||!maxPairs||maxPairs>(longLuma?LongLimit:Limit))throw std::runtime_error("Invalid stereo video start");
        Reset();device=d;capacity=maxPairs;width=longLuma?192:Width;height=longLuma?108:Height;bytesPerPixel=longLuma?1:4;durationMs=longLuma?60000:8000;
        D3D11_TEXTURE2D_DESC t{};t.Width=width;t.Height=height;t.MipLevels=t.SampleDesc.Count=1;
        t.Format=longLuma?DXGI_FORMAT_R8_UNORM:DXGI_FORMAT_B8G8R8A8_UNORM;t.Usage=D3D11_USAGE_STAGING;t.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        for(unsigned remaining=capacity*2;remaining;){t.ArraySize=std::min(remaining,ArraySlices);Ptr<ID3D11Texture2D> part;Check(d->CreateTexture2D(&t,nullptr,&part));history.push_back(std::move(part));remaining-=t.ArraySize;}
        t.ArraySize=1;t.Usage=D3D11_USAGE_DEFAULT;t.CPUAccessFlags=0;t.BindFlags=D3D11_BIND_RENDER_TARGET;
        Check(d->CreateTexture2D(&t,nullptr,&scratch));Check(d->CreateRenderTargetView(scratch.Get(),nullptr,&target));
        char const* code=R"(
Texture2D image:register(t0);SamplerState linearClamp:register(s0);
struct Out{float4 p:SV_POSITION;float2 uv:TEXCOORD;};
Out VS(uint id:SV_VertexID){Out o;o.uv=float2((id<<1)&2,id&2);o.p=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float4 PS(Out i):SV_TARGET{float4 color=image.SampleLevel(linearClamp,i.uv,0);
#ifdef CAPTURE_LUMA
return float4(dot(color.rgb,float3(.299,.587,.114)),0,0,1);
#else
return color;
#endif
}
)";
        Ptr<ID3DBlob> b,e;
        Check(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"VS","vs_5_0",0,0,&b,&e));Check(d->CreateVertexShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&vs));
        D3D_SHADER_MACRO defines[]={{"CAPTURE_LUMA","1"},{nullptr,nullptr}};
        b.Reset();e.Reset();Check(D3DCompile(code,strlen(code),nullptr,longLuma?defines:nullptr,nullptr,"PS","ps_5_0",0,0,&b,&e));Check(d->CreatePixelShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&ps));
        D3D11_SAMPLER_DESC s{};s.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;s.AddressU=s.AddressV=s.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;s.MaxLOD=D3D11_FLOAT32_MAX;Check(d->CreateSamplerState(&s,&sampler));
        D3D11_RASTERIZER_DESC r{};r.FillMode=D3D11_FILL_SOLID;r.CullMode=D3D11_CULL_NONE;r.DepthClipEnable=TRUE;Check(d->CreateRasterizerState(&r,&raster));
        D3D11_DEPTH_STENCIL_DESC z{};Check(d->CreateDepthStencilState(&z,&depth));
        D3D11_BLEND_DESC blendDesc{};blendDesc.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;Check(d->CreateBlendState(&blendDesc,&blend));
        frames.reserve(capacity);started=GetTickCount64();active=true;
    }
    void Begin(Frame const& f){if(active)current=f;}
    void Camera(unsigned eye,float const* view,float const* projection){
        if(!active||eye>1)return;std::copy_n(view,16,current.view[eye]);std::copy_n(projection,16,current.projection[eye]);current.cameraMask|=1u<<eye;
    }
    void Pose(unsigned eye,float const* pose,float const* fov){if(active&&eye<2){std::copy_n(pose,7,current.pose[eye]);std::copy_n(fov,4,current.fov[eye]);}}
    void Eye(ID3D11DeviceContext* c,ID3D11Texture2D* image,unsigned eye,uint64_t serial){
        if(!active||eye>1||serial!=current.serial||(current.eyeMask&(1u<<eye)))return;
        Source* src=nullptr;for(auto& s:sources)if(s.image.Get()==image)src=&s;
        if(!src){if(sources.size()>=2)throw std::runtime_error("Unexpected stereo video source");Source s;s.image=image;Check(device->CreateShaderResourceView(image,nullptr,&s.view));sources.push_back(std::move(s));src=&sources.back();}
        // D3D11 is the private VR transfer context. Never touch native D3D9 state
        // or write into the source texture / the acquired XR destination.
        c->ClearState();auto* rt=target.Get();auto* srv=src->view.Get();auto* sample=sampler.Get();
        c->OMSetRenderTargets(1,&rt,nullptr);c->OMSetDepthStencilState(depth.Get(),0);c->OMSetBlendState(blend.Get(),nullptr,~0u);
        c->RSSetState(raster.Get());D3D11_VIEWPORT v{0,0,float(width),float(height),0,1};c->RSSetViewports(1,&v);
        c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);c->VSSetShader(vs.Get(),nullptr,0);c->PSSetShader(ps.Get(),nullptr,0);
        c->PSSetShaderResources(0,1,&srv);c->PSSetSamplers(0,1,&sample);c->Draw(3,0);c->ClearState();
        unsigned slice=unsigned(frames.size())*2+eye;
        c->CopySubresourceRegion(history[slice/ArraySlices].Get(),slice%ArraySlices,0,0,0,scratch.Get(),0,nullptr);
        current.eyeMask|=1u<<eye;
    }
    bool End(bool submitted){
        if(!active)return false;
        if(current.eyeMask==3){current.submitted=submitted;frames.push_back(current);}
        current={};
        if(frames.size()>=capacity||GetTickCount64()-started>=durationMs){active=false;return true;}return false;
    }
    void Stop()noexcept{active=false;}
    void Save(ID3D11DeviceContext* c,std::filesystem::path const& dir){
        if(active)throw std::runtime_error("Refusing video readback during capture");
        std::filesystem::create_directories(dir);
        std::ofstream pixels(dir/(bytesPerPixel==1?L"eyes.gray":L"eyes.bgra"),std::ios::binary),csv(dir/L"frames.csv"),meta(dir/L"capture.json");
        if(!pixels||!csv||!meta)throw std::runtime_error("Cannot write stereo video");
        LARGE_INTEGER hz{};QueryPerformanceFrequency(&hz);
        meta<<"{\"width\":"<<width<<",\"height\":"<<height<<",\"bytes_per_pixel\":"<<bytesPerPixel<<",\"pairs\":"<<frames.size()<<",\"qpc_frequency\":"<<hz.QuadPart<<",\"encoding\":\""<<(bytesPerPixel==1?"L8":"BGRA8")<<" top-left, left then right per pair\",\"stage\":\"native rendered eye, before VR SMAA/FFR and compositor\",\"cpu_readbacks_during_capture\":0,\"disk_writes_during_capture\":0}";
        csv.precision(9);
        csv<<"index,serial,qpc,predicted_display_time,predicted_period,should_render,submitted,eye_mask,camera_mask";
        for(unsigned eye=0;eye<2;++eye){for(unsigned i=0;i<7;++i)csv<<",eye"<<eye<<"_pose"<<i;for(unsigned i=0;i<4;++i)csv<<",eye"<<eye<<"_fov"<<i;for(unsigned i=0;i<16;++i)csv<<",eye"<<eye<<"_view"<<i;for(unsigned i=0;i<16;++i)csv<<",eye"<<eye<<"_projection"<<i;}csv<<'\n';
        for(unsigned i=0;i<frames.size();++i){auto const& f=frames[i];csv<<i<<','<<f.serial<<','<<f.qpc<<','<<f.displayTime<<','<<f.displayPeriod<<','<<f.shouldRender<<','<<f.submitted<<','<<f.eyeMask<<','<<f.cameraMask;
            for(unsigned eye=0;eye<2;++eye){for(float x:f.pose[eye])csv<<','<<x;for(float x:f.fov[eye])csv<<','<<x;for(float x:f.view[eye])csv<<','<<x;for(float x:f.projection[eye])csv<<','<<x;
                unsigned slice=i*2+eye;auto* texture=history[slice/ArraySlices].Get();unsigned subresource=slice%ArraySlices;
                D3D11_MAPPED_SUBRESOURCE m{};Check(c->Map(texture,subresource,D3D11_MAP_READ,0,&m));
                for(unsigned y=0;y<height;++y)pixels.write(static_cast<char const*>(m.pData)+y*m.RowPitch,width*bytesPerPixel);c->Unmap(texture,subresource);
            }csv<<'\n';
        }
        pixels.flush();csv.flush();meta.flush();if(!pixels||!csv||!meta)throw std::runtime_error("Stereo video write failed");
    }
    void Reset()noexcept{active=false;frames.clear();sources.clear();history.clear();scratch.Reset();target.Reset();vs.Reset();ps.Reset();sampler.Reset();raster.Reset();depth.Reset();blend.Reset();device.Reset();current={};}
};
}
