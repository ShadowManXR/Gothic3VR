#pragma once
#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdio>
namespace dlaa {
using Microsoft::WRL::ComPtr;
inline std::string Hex(uint32_t value){std::ostringstream s;s<<"0x"<<std::hex<<std::setw(8)<<std::setfill('0')<<value;return s.str();}
inline void Require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
inline void Hr(HRESULT value,char const* message){if(FAILED(value))throw std::runtime_error(std::string(message)+": "+Hex(uint32_t(value)));}
inline std::string Utf8(std::wstring const& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,0,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;}
inline std::string Quote(std::string const& s){std::string out="\"";for(unsigned char c:s){if(c=='\\'||c=='\"'){out+='\\';out+=char(c);}else if(c=='\n')out+="\\n";else if(c>=32)out+=char(c);}return out+'\"';}
inline std::filesystem::path ExeFolder(){std::vector<wchar_t> p(32768);auto n=GetModuleFileNameW(nullptr,p.data(),DWORD(p.size()));Require(n&&n<p.size(),"Executable path");return std::filesystem::path(std::wstring(p.data(),n)).parent_path();}
inline double Seconds(){LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return double(t.QuadPart)/double(f.QuadPart);}
struct Handle {
    HANDLE value=nullptr;
    Handle()=default;explicit Handle(HANDLE h):value(h){}
    Handle(Handle const&)=delete;Handle& operator=(Handle const&)=delete;
    ~Handle(){Close();}
    void Close(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);value=nullptr;}
};
struct Gpu {
    ComPtr<IDXGIAdapter1> adapter;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;DXGI_ADAPTER_DESC1 description{};
    // Wrap the renderer's actual immediate context. The game client must not
    // create a second D3D11 device to consume its native bridge textures.
    Gpu(ID3D11Device* existing,ID3D11DeviceContext* immediate):device(existing),context(immediate){
        Require(existing&&immediate&&immediate->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"DLAA requires renderer immediate context");
        ComPtr<ID3D11Device> owner;immediate->GetDevice(&owner);Require(owner.Get()==existing,"DLAA renderer context belongs to another device");
        ComPtr<IDXGIDevice> dxgi;Hr(device.As(&dxgi),"Renderer DXGI device");ComPtr<IDXGIAdapter> base;Hr(dxgi->GetAdapter(&base),"Renderer DXGI adapter");
        Hr(base.As(&adapter),"Renderer adapter identity");Hr(adapter->GetDesc1(&description),"Renderer GPU description");
        Require(description.VendorId==0x10de&&!(description.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&device->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_0,"DLAA renderer requires NVIDIA feature level 11");
    }
    explicit Gpu(LUID const* requested=nullptr){
        ComPtr<IDXGIFactory1> factory;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");
        for(UINT i=0;;++i){ComPtr<IDXGIAdapter1> candidate;auto hr=factory->EnumAdapters1(i,&candidate);if(hr==DXGI_ERROR_NOT_FOUND)break;Hr(hr,"Enumerate GPU");
            DXGI_ADAPTER_DESC1 d{};Hr(candidate->GetDesc1(&d),"GPU description");
            if(requested?(d.AdapterLuid.HighPart==requested->HighPart&&d.AdapterLuid.LowPart==requested->LowPart):(d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE))){adapter=candidate;description=d;break;}}
        Require(adapter!=nullptr,"NVIDIA GPU / requested adapter unavailable");D3D_FEATURE_LEVEL level=D3D_FEATURE_LEVEL_11_0;
        Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,&level,1,D3D11_SDK_VERSION,&device,nullptr,&context),"Native D3D11 device");
    }
    ComPtr<ID3D11Texture2D> Texture(UINT w,UINT h,DXGI_FORMAT format,UINT bind,UINT misc=0){
        D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=d.SampleDesc.Count=1;d.Format=format;d.BindFlags=bind;d.MiscFlags=misc;
        ComPtr<ID3D11Texture2D> t;Hr(device->CreateTexture2D(&d,nullptr,&t),"Create GPU texture");return t;
    }
    void Wait(unsigned timeout=5000){D3D11_QUERY_DESC d{D3D11_QUERY_EVENT,0};ComPtr<ID3D11Query> q;Hr(device->CreateQuery(&d,&q),"GPU event");context->End(q.Get());context->Flush();auto began=GetTickCount64();
        for(;;){BOOL done=FALSE;auto hr=context->GetData(q.Get(),&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH);Hr(hr,"GPU wait");if(hr==S_OK&&done)return;Require(GetTickCount64()-began<timeout,"GPU completion timeout");Sleep(0);}}
};
}
