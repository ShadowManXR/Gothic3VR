#include "OpenXrRuntimeSelection.h"
#include "StereoOpenXrApi.h"
#include <iostream>
namespace xr=diagnostic::xr_runtime;
using diagnostic::OpenXrRuntimeChoice;
void Check(bool value,char const* message){if(!value)throw std::runtime_error(message);std::cout<<"PASS "<<message<<'\n';}
template<class F> bool Fails(F f){try{f();return false;}catch(std::exception const&){return true;}}
void Pe(xr::fs::path const& path,WORD machine){IMAGE_DOS_HEADER dos{};dos.e_magic=IMAGE_DOS_SIGNATURE;dos.e_lfanew=sizeof(dos);DWORD sig=IMAGE_NT_SIGNATURE;IMAGE_FILE_HEADER header{};header.Machine=machine;header.Characteristics=IMAGE_FILE_DLL;std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&dos),sizeof(dos));out.write(reinterpret_cast<char*>(&sig),sizeof(sig));out.write(reinterpret_cast<char*>(&header),sizeof(header));}
int main(int argc,char** argv){try{
    if(argc==4&&(std::string(argv[1])=="--meta-loader-check"||std::string(argv[1])=="--meta-session-check"||std::string(argv[1])=="--meta-bridge-session-check")){
        SetEnvironmentVariableW(L"XR_LOADER_DEBUG",L"all");
        bool bridge=std::string(argv[1])=="--meta-bridge-session-check";
        g3vr::Log log{xr::fs::path(argv[3])};g3vr::Api api;api.Initialize(xr::fs::path(argv[2]),log,OpenXrRuntimeChoice::MetaLink,bridge?xr::fs::path{}:xr::Select(OpenXrRuntimeChoice::MetaLink));
        if(std::string(argv[1])!="--meta-loader-check"){
            XrSystemId system=XR_NULL_SYSTEM_ID;XrSystemGetInfo si{XR_TYPE_SYSTEM_GET_INFO};si.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;api.Check(api.GetSystem(api.instance,&si,&system),"xrGetSystem");
            XrGraphicsRequirementsD3D11KHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};api.Check(api.GetD3D11GraphicsRequirementsKHR(api.instance,system,&req),"graphics requirements");
            stereo::ComPtr<IDXGIFactory1> factory;stereo::Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"DXGI factory");stereo::ComPtr<IDXGIAdapter1> adapter;
            for(UINT i=0;;++i){stereo::ComPtr<IDXGIAdapter1> a;if(factory->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 d{};a->GetDesc1(&d);if(!memcmp(&d.AdapterLuid,&req.adapterLuid,sizeof(LUID))){adapter=a;break;}}
            Check(bool(adapter),"Meta graphics adapter available");stereo::ComPtr<ID3D11Device> device;stereo::ComPtr<ID3D11DeviceContext> context;
            stereo::Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,&req.minFeatureLevel,1,D3D11_SDK_VERSION,&device,nullptr,&context),"D3D11 device");
            XrGraphicsBindingD3D11KHR binding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};binding.device=device.Get();XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};ci.next=&binding;ci.systemId=system;XrSession session=XR_NULL_HANDLE;
            log.Line("{\"type\":\"meta_standalone_create_session_begin\"}");api.Check(api.CreateSession(api.instance,&ci,&session),"xrCreateSession");
            log.Line("{\"type\":\"meta_standalone_create_session_succeeded\"}");api.DestroySession(session);
        }
        return 0;
    }
    if(argc==2&&std::string(argv[1])=="--installed"){
        for(auto choice:{OpenXrRuntimeChoice::Vdxr,OpenXrRuntimeChoice::MetaLink,OpenXrRuntimeChoice::SteamVR})std::wcout<<diagnostic::OpenXrRuntimeLabel(choice)<<L": "<<xr::Select(choice).wstring()<<L'\n';return 0;
    }
    wchar_t temp[MAX_PATH];GetTempPathW(MAX_PATH,temp);auto root=xr::fs::path(temp)/(L"G3VR-runtime-"+std::to_wstring(GetCurrentProcessId()));xr::fs::create_directories(root);
    auto cleanup=[&]{std::error_code error;xr::fs::remove_all(root,error);};
    try{
        auto manifest=root/L"runtime.json";Pe(root/L"runtime.dll",IMAGE_FILE_MACHINE_I386);
        {std::ofstream out(manifest);out<<R"({"file_format_version":"1.0.0","runtime":{"library_path":".\\runtime.dll"}})";}
        xr::ValidateManifest(manifest);Check(true,"relative x86 runtime DLL is validated without loading it");
        Pe(root/L"runtime.dll",IMAGE_FILE_MACHINE_AMD64);Check(Fails([&]{xr::ValidateManifest(manifest);}),"64-bit runtime rejected before loader initialization");
        Pe(root/L"runtime.dll",IMAGE_FILE_MACHINE_I386);
        Check(Fails([&]{xr::ValidateManifest(root/L"missing.json");}),"missing manifest gives a clear initialization failure");
        Check(Fails([&]{xr::LibraryPath(R"({"runtime":{"library_path":"unterminated})");}),"truncated manifest string rejected");
        Check(xr::LibraryPath(R"({"runtime":{"library_path":"C:\\Runtime\\plugin.dll"}})")==L"C:\\Runtime\\plugin.dll","Windows manifest path escapes decoded");
        Check(xr::Manifest32(L"C:/SteamVR/steamxr_win64.json").filename()==L"steamxr_win32.json","SteamVR x86 manifest discovered beside registered x64 manifest");
        Check(xr::Manifest32(L"D:/Oculus/oculus_openxr_64.json").filename()==L"oculus_openxr_32.json","Meta installation drive is preserved when selecting x86");
        Check(diagnostic::ValidOpenXrRuntime(-1)==OpenXrRuntimeChoice::Vdxr&&diagnostic::ValidOpenXrRuntime(99)==OpenXrRuntimeChoice::Vdxr,"invalid selection retains VDXR default");
        DWORD size=GetEnvironmentVariableW(L"XR_RUNTIME_JSON",nullptr,0);auto original=xr::Environment(L"XR_RUNTIME_JSON");
        {xr::ScopedManifest restore(manifest);Check(xr::Environment(L"XR_RUNTIME_JSON")==manifest.wstring(),"selected manifest applies to this process");Check(xr::Select(OpenXrRuntimeChoice::System)==manifest,"system option honors an explicit x86 XR_RUNTIME_JSON");
            try{xr::ScopedManifest inner(root/L"other.json");throw std::runtime_error("test");}catch(...){}
            Check(xr::Environment(L"XR_RUNTIME_JSON")==manifest.wstring(),"manifest environment restored after an exception");
        }
        Check(xr::Environment(L"XR_RUNTIME_JSON")==original&&GetEnvironmentVariableW(L"XR_RUNTIME_JSON",nullptr,0)==size,"original runtime environment restored after scope");
        cleanup();return 0;
    }catch(...){cleanup();throw;}
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
