#pragma once
#define XR_NO_PROTOTYPES
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include "StereoRenderer.h"
#include <openxr/openxr_platform.h>
#include <filesystem>
#include <sstream>
#include <functional>
#include "OpenXrRuntimeSelection.h"
#include "ControllerProfiles.h"
#include <memory>
#ifndef _WIN64
#include "meta_bridge/Client.h"
#endif
namespace g3vr
{
namespace fs=std::filesystem;
using stereo::ComPtr;
std::string Quote(std::string const& value)
{
    std::string out="\"";for(unsigned char c:value){if(c=='\\'||c=='\"'){out+='\\';out+=c;}else if(c<32)out+=' ';else out+=c;}return out+'\"';
}
std::string Utf8(wchar_t const* text)
{
    int n=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);std::string s(n,'\0');
    WideCharToMultiByte(CP_UTF8,0,text,-1,s.data(),n,nullptr,nullptr);if(n)s.pop_back();return s;
}
struct Log
{
    fs::path path;std::ofstream stream;
    explicit Log(fs::path p):path(std::move(p)){fs::create_directories(path.parent_path());stream.open(path);if(!stream)throw std::runtime_error("Cannot open log");}
    void Line(std::string const& value){stream<<value<<'\n';stream.flush();}
};
struct Scope {std::function<void()> finish;~Scope(){if(finish)finish();}};
#define XR_FUNCTIONS(X) \
 X(DestroyInstance) X(GetInstanceProperties) X(GetSystem) X(GetSystemProperties) \
 X(GetD3D11GraphicsRequirementsKHR) X(CreateSession) X(DestroySession) X(BeginSession) X(EndSession) X(RequestExitSession) X(PollEvent) \
 X(CreateReferenceSpace) X(DestroySpace) X(EnumerateViewConfigurationViews) X(EnumerateEnvironmentBlendModes) \
 X(EnumerateSwapchainFormats) X(CreateSwapchain) X(DestroySwapchain) X(EnumerateSwapchainImages) \
 X(AcquireSwapchainImage) X(WaitSwapchainImage) X(ReleaseSwapchainImage) X(WaitFrame) X(BeginFrame) X(EndFrame) X(LocateViews) X(ResultToString) \
 X(ApplyHapticFeedback) X(StringToPath) X(CreateActionSet) X(DestroyActionSet) X(CreateAction) X(SuggestInteractionProfileBindings) X(AttachSessionActionSets) \
 X(CreateActionSpace) X(SyncActions) X(GetActionStatePose) X(GetActionStateBoolean) X(GetActionStateFloat) X(GetActionStateVector2f) X(LocateSpace)
struct Api
{
#ifndef _WIN64
    std::unique_ptr<diagnostic::meta_bridge::Client> meta;
#endif
    HMODULE loader=nullptr;XrInstance instance=XR_NULL_HANDLE;PFN_xrGetInstanceProcAddr get=nullptr;
    std::vector<std::string> controllerExtensions;
#define DECLARE(name) PFN_xr##name name=nullptr;
    XR_FUNCTIONS(DECLARE)
#undef DECLARE
    void Check(XrResult result,char const* operation)
    {
        if(XR_FAILED(result)){char text[XR_MAX_RESULT_STRING_SIZE];sprintf_s(text,"%d",int(result));if(ResultToString&&instance)ResultToString(instance,result,text);throw std::runtime_error(std::string(operation)+": "+text);}
    }
    template<class T> T Proc(char const* name){PFN_xrVoidFunction f=nullptr;Check(get(instance,name,&f),name);return reinterpret_cast<T>(f);}
    void Initialize(fs::path const& path,Log& log,diagnostic::OpenXrRuntimeChoice choice=diagnostic::OpenXrRuntimeChoice::Vdxr,fs::path const& explicitManifest={},bool frameClock=false)
    {
        controllerExtensions.clear();
        auto manifest=explicitManifest.empty()?diagnostic::xr_runtime::Select(choice):explicitManifest;
#ifndef _WIN64
        if(explicitManifest.empty()&&(choice==diagnostic::OpenXrRuntimeChoice::MetaLink||(choice==diagnostic::OpenXrRuntimeChoice::System&&!_wcsicmp(manifest.filename().c_str(),L"oculus_openxr_32.json")))){
            meta=std::make_unique<diagnostic::meta_bridge::Client>(path,log.path,manifest);instance=meta->Instance();meta->Install(*this);
            log.Line("{\"type\":\"runtime\",\"name\":\"Meta OpenXR x64 host\",\"architecture\":\"x86_game_x64_compositor\",\"graphics\":\"D3D11_NT_shared_GPU\",\"headless\":false,\"cpu_pixel_readback\":false}");return;
        }
#endif
        if(!explicitManifest.empty())diagnostic::xr_runtime::ValidateManifest(manifest,sizeof(void*)==8?IMAGE_FILE_MACHINE_AMD64:IMAGE_FILE_MACHINE_I386);
        diagnostic::xr_runtime::ScopedManifest runtime(manifest);
        log.Line("{\"type\":\"runtime_selection\",\"choice\":"+std::to_string(int(choice))+",\"manifest\":"+Quote(Utf8(manifest.c_str()))+",\"architecture\":\""+(sizeof(void*)==8?"x64":"x86")+"\",\"global_runtime_changed\":false}");
        loader=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if(!loader)throw std::runtime_error("Cannot load OpenXR loader ("+std::string(sizeof(void*)==8?"x64":"x86")+"): "+std::to_string(GetLastError()));
        get=reinterpret_cast<PFN_xrGetInstanceProcAddr>(GetProcAddress(loader,"xrGetInstanceProcAddr"));if(!get)throw std::runtime_error("Missing xrGetInstanceProcAddr");
        auto enumerate=Proc<PFN_xrEnumerateInstanceExtensionProperties>("xrEnumerateInstanceExtensionProperties");uint32_t count=0;
        Check(enumerate(nullptr,0,&count,nullptr),"extensions count");std::vector<XrExtensionProperties> extensions(count,{XR_TYPE_EXTENSION_PROPERTIES});
        Check(enumerate(nullptr,count,&count,extensions.data()),"extensions");bool supported=false;
        for(auto const& e:extensions)if(!strcmp(e.extensionName,XR_KHR_D3D11_ENABLE_EXTENSION_NAME))supported=true;
        if(!supported)throw std::runtime_error("Selected OpenXR runtime does not expose XR_KHR_D3D11_enable");
        XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};strcpy_s(info.applicationInfo.applicationName,"Gothic 3 VR");
        info.applicationInfo.applicationVersion=1;strcpy_s(info.applicationInfo.engineName,"Gothic 3 VR");info.applicationInfo.apiVersion=XR_API_VERSION_1_0;
        std::vector<std::string> available;for(auto const& e:extensions)available.emplace_back(e.extensionName);
        controllerExtensions=diagnostic::SupportedControllerExtensions(available);
        std::vector<char const*> names{XR_KHR_D3D11_ENABLE_EXTENSION_NAME};
        if(frameClock&&std::find(available.begin(),available.end(),XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME)!=available.end())names.push_back(XR_KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME_EXTENSION_NAME);
        for(auto const& name:controllerExtensions)names.push_back(name.c_str());
        info.enabledExtensionCount=uint32_t(names.size());info.enabledExtensionNames=names.data();
        // Keep Meta's module alive long enough to read its thread-local native
        // error if the OpenXR loader unloads it after a failed CreateInstance.
        HMODULE metaModule=nullptr;
        if(choice==diagnostic::OpenXrRuntimeChoice::MetaLink||choice==diagnostic::OpenXrRuntimeChoice::System)
            GetModuleHandleExW(0,sizeof(void*)==8?L"LibOVRRTImpl64_1.dll":L"LibOVRRTImpl32_1.dll",&metaModule);
        Scope metaRelease{[&]{if(metaModule)FreeLibrary(metaModule);}};
        auto created=Proc<PFN_xrCreateInstance>("xrCreateInstance")(&info,&instance);
        if(XR_FAILED(created)&&metaModule){
            struct MetaError {int32_t result;char message[512];} detail{};
            using ReadError=void(__cdecl*)(MetaError*);
            auto read=reinterpret_cast<ReadError>(GetProcAddress(metaModule,"ovr_GetLastErrorInfo"));if(read)read(&detail);detail.message[511]=0;
            OSVERSIONINFOW os{};os.dwOSVersionInfoSize=sizeof(os);GetVersionExW(&os);
            log.Line("{\"type\":\"meta_instance_failure\",\"xr_result\":"+std::to_string(int(created))+",\"native_result\":"+std::to_string(detail.result)+",\"native_message\":"+Quote(detail.message)+",\"reported_windows_major\":"+std::to_string(os.dwMajorVersion)+",\"reported_windows_minor\":"+std::to_string(os.dwMinorVersion)+"}");
        }
        Check(created,"xrCreateInstance");
#define LOAD(name) name=Proc<PFN_xr##name>("xr" #name);
        XR_FUNCTIONS(LOAD)
#undef LOAD
        XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};Check(GetInstanceProperties(instance,&props),"runtime properties");
        log.Line("{\"type\":\"runtime\",\"name\":"+Quote(props.runtimeName)+",\"architecture\":\""+(sizeof(void*)==8?"x64":"x86")+"\",\"graphics\":\"D3D11\",\"headless\":false}");
    }
    ~Api(){if(instance&&DestroyInstance)DestroyInstance(instance);if(loader)FreeLibrary(loader);}
};

}
