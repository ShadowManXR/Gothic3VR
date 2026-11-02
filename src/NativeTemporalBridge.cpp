#include "NativeTemporalBridge.h"
#include <dxgi.h>
#include <array>
#include <memory>
#include <vector>
#include <stdexcept>
#include <string>
using Microsoft::WRL::ComPtr;
namespace g3vr { namespace {
void Require(bool value,char const* why){if(!value)throw std::runtime_error(why);}
void Check(HRESULT hr,char const* why){Require(SUCCEEDED(hr),why);}
using Query=void*(__cdecl*)(unsigned);
using InitializeNv=int(__cdecl*)();
using RegisterNv=int(__cdecl*)(IDirect3DResource9*);
using StretchNv=int(__cdecl*)(IDirect3DDevice9*,IDirect3DResource9*,RECT const*,IDirect3DResource9*,RECT const*,D3DTEXTUREFILTERTYPE);
struct Driver {
    HMODULE module=nullptr;RegisterNv reg=nullptr,unreg=nullptr;StretchNv stretch=nullptr;
    bool Open(){
        wchar_t folder[MAX_PATH]{};auto count=GetSystemDirectoryW(folder,MAX_PATH);
        if(!count||count>=MAX_PATH)return false;
        // Explicit system path avoids loading a game-directory DLL.
        module=LoadLibraryW((std::wstring(folder)+L"\\nvapi.dll").c_str());if(!module)return false;
        auto query=reinterpret_cast<Query>(GetProcAddress(module,"nvapi_QueryInterface"));if(!query)return false;
        // Public NVIDIA nvapi_interface.h identifiers; no private driver ABI.
        auto init=reinterpret_cast<InitializeNv>(query(0x0150e828));
        reg=reinterpret_cast<RegisterNv>(query(0xa064bdfc));unreg=reinterpret_cast<RegisterNv>(query(0xbb2b17aa));
        stretch=reinterpret_cast<StretchNv>(query(0x22de03aa));
        return init&&reg&&unreg&&stretch&&init()==0;
    }
    ~Driver(){if(module)FreeLibrary(module);}
};
struct Registration {
    Driver& driver;ComPtr<IDirect3DResource9> resource;
    Registration(Driver& d,IDirect3DResource9* r):driver(d){auto status=driver.reg(r);if(status)throw std::runtime_error("Native temporal NVIDIA resource registration status="+std::to_string(status));resource=r;}
    ~Registration(){if(resource)driver.unreg(resource.Get());}
};
struct Image {
    ComPtr<IDirect3DSurface9> source;ComPtr<IDirect3DTexture9> texture;
    ComPtr<ID3D11Texture2D> output;
    std::shared_ptr<Registration> sourceRegistration;
    std::unique_ptr<Registration> destinationRegistration;
    TemporalEncoding encoding{};bool held=false;
};
struct State {
    Driver driver;ComPtr<IDirect3DDevice9Ex> native;ComPtr<ID3D11Device> consumer;
    ComPtr<IDirect3DQuery9> fence;LUID adapter{};
    std::vector<std::unique_ptr<Image>> images;
    std::array<Image*,6> queued{};unsigned count=0;
    bool inFlight=false,failed=false;
    void Wait(){
        Check(fence->Issue(D3DISSUE_END),"Native temporal completion fence");
        auto start=GetTickCount64();bool flush=true;
        for(;;){BOOL done=FALSE;auto hr=fence->GetData(&done,sizeof(done),flush?D3DGETDATA_FLUSH:0);flush=false;
            Check(hr,"Native temporal fence result");if(hr==S_OK&&done)break;
            Require(GetTickCount64()-start<2000,"Native temporal transfer timeout");SwitchToThread();}
    }
    bool SafeToDestroy()const{if(inFlight)return false;for(auto const& i:images)if(i->held)return false;return true;}
};
}
bool NativeTemporalBridge::Initialize(IDirect3DDevice9Ex* native){
    Reset();Require(native!=nullptr,"Native temporal device missing");auto p=std::make_unique<State>();
    if(!p->driver.Open())return false;
    ComPtr<IDirect3D9> api;ComPtr<IDirect3D9Ex> ex;D3DDEVICE_CREATION_PARAMETERS creation{};
    Check(native->GetDirect3D(&api),"Native temporal D3D9 owner");Check(api.As(&ex),"Native temporal D3D9Ex owner");
    Check(native->GetCreationParameters(&creation),"Native temporal device adapter");
    Check(ex->GetAdapterLUID(creation.AdapterOrdinal,&p->adapter),"Native temporal adapter LUID");
    D3DADAPTER_IDENTIFIER9 id{};Check(api->GetAdapterIdentifier(creation.AdapterOrdinal,0,&id),"Native temporal adapter vendor");if(id.VendorId!=0x10de)return false;
    Check(native->CreateQuery(D3DQUERYTYPE_EVENT,&p->fence),"Native temporal fence creation");p->native=native;state=p.release();return true;
}
TemporalEncoding NativeTemporalBridge::Register(IDirect3DSurface9* source,ID3D11Device* device,ComPtr<ID3D11Texture2D>& output){
    auto* s=static_cast<State*>(state);Require(s&&source&&device&&!s->failed&&!s->count&&!s->inFlight&&s->images.size()<6,"Native temporal registration state/capacity");
    for(auto const& image:s->images)Require(!image->held,"Native temporal registration during GPU reads");
    ComPtr<IDirect3DDevice9> owner;Check(source->GetDevice(&owner),"Native temporal source owner");Require(owner.Get()==s->native.Get(),"Native temporal foreign source device");
    ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC desc{};
    Check(device->QueryInterface(IID_PPV_ARGS(&dxgi)),"Native temporal DXGI device");Check(dxgi->GetAdapter(&adapter),"Native temporal DXGI adapter");Check(adapter->GetDesc(&desc),"Native temporal adapter description");
    Require(!memcmp(&s->adapter,&desc.AdapterLuid,sizeof(LUID)),"Native temporal adapters differ");Require(!s->consumer||s->consumer.Get()==device,"Native temporal consumer changed");
    D3DSURFACE_DESC input{};Check(source->GetDesc(&input),"Native temporal source description");
    Require(input.Pool==D3DPOOL_DEFAULT&&input.MultiSampleType==D3DMULTISAMPLE_NONE&&input.Width&&input.Height&&input.Width<=8192&&input.Height<=8192&&uint64_t(input.Width)*input.Height<=33554432,"Native temporal source layout");
    bool depth=input.Format==D3DFMT_D16||input.Format==D3DFMT_D24S8||input.Format==D3DFMT_D24X8||input.Format==D3DFMT_D32F_LOCKABLE;
    Require((depth&&(input.Usage&D3DUSAGE_DEPTHSTENCIL))||(!depth&&input.Format==D3DFMT_A16B16G16R16F&&(input.Usage&D3DUSAGE_RENDERTARGET)),"Native temporal source format/usage");
    auto image=std::make_unique<Image>();image->source=source;image->encoding=depth?TemporalEncoding::Depth32Float:TemporalEncoding::Color16F;
    for(auto const& prior:s->images)if(prior->source.Get()==source){image->sourceRegistration=prior->sourceRegistration;break;}
    if(!image->sourceRegistration){
        // NVIDIA registers color render-target TEXTURES, not the level-zero
        // surface returned by GetSurfaceLevel. Standalone depth is a surface.
        ComPtr<IDirect3DTexture9> container;
        if(!depth){
            Check(source->GetContainer(IID_PPV_ARGS(&container)),"Native temporal motion texture container");
            ComPtr<IDirect3DSurface9> level;Check(container->GetSurfaceLevel(0,&level),"Native temporal motion level zero");
            Require(container->GetLevelCount()==1&&level.Get()==source,"Native temporal motion must be level-zero single-mip texture");
        }
        image->sourceRegistration=std::make_shared<Registration>(s->driver,depth?static_cast<IDirect3DResource9*>(source):static_cast<IDirect3DResource9*>(container.Get()));
    }
    HANDLE shared=nullptr;
    Check(s->native->CreateTexture(input.Width,input.Height,1,D3DUSAGE_RENDERTARGET,depth?D3DFMT_R32F:D3DFMT_A16B16G16R16F,D3DPOOL_DEFAULT,&image->texture,&shared),"Native temporal shared texture");
    Require(shared!=nullptr,"Native temporal shared handle missing");
    Check(device->OpenSharedResource(shared,IID_PPV_ARGS(&image->output)),"Native temporal D3D11 import");
    // D3D9 legacy shared handles are owned by the resource; not NT handles.
    D3D11_TEXTURE2D_DESC target{};image->output->GetDesc(&target);
    Require(target.Width==input.Width&&target.Height==input.Height&&target.ArraySize==1&&target.MipLevels==1&&target.SampleDesc.Count==1&&target.Format==(depth?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R16G16B16A16_FLOAT)&&(target.BindFlags&D3D11_BIND_SHADER_RESOURCE),"Native temporal imported texture layout");
    image->destinationRegistration=std::make_unique<Registration>(s->driver,image->texture.Get());
    auto encoding=image->encoding;output=image->output;s->consumer=device;s->images.push_back(std::move(image));return encoding;
}
void NativeTemporalBridge::Queue(ID3D11Texture2D* output){
    auto* s=static_cast<State*>(state);Require(s&&!s->failed&&!s->inFlight&&s->count<s->queued.size(),"Native temporal queue state/capacity");
    Image* found=nullptr;for(auto const& image:s->images)if(image->output.Get()==output)found=image.get();Require(found&&!found->held,"Native temporal foreign/read-owned output");
    for(unsigned i=0;i<s->count;++i)Require(s->queued[i]!=found&&s->queued[i]->source.Get()!=found->source.Get(),"Native temporal duplicate/aliased source batch");
    s->queued[s->count++]=found;
}
void NativeTemporalBridge::Complete(){
    auto* s=static_cast<State*>(state);if(!s||!s->count)return;Require(!s->failed&&!s->inFlight,"Native temporal transfer failure requires reset");
    // No state-setting calls: copy during Gothic's scene without disturbing
    // its render targets, shaders, viewport or final opaque depth binding.
    s->inFlight=true;s->failed=true;
    for(unsigned i=0;i<s->count;++i){auto* image=s->queued[i];Require(s->driver.stretch(s->native.Get(),image->sourceRegistration->resource.Get(),nullptr,image->texture.Get(),nullptr,D3DTEXF_NONE)==0,"Native temporal NVIDIA GPU copy");}
    s->Wait();s->inFlight=false;
    for(unsigned i=0;i<s->count;++i)s->queued[i]->held=true;
    s->count=0;s->queued={};s->failed=false;
}
void NativeTemporalBridge::ReleaseReads(){auto* s=static_cast<State*>(state);if(!s)return;Require(!s->inFlight&&!s->failed,"Native temporal release after failed transfer");for(auto& image:s->images)image->held=false;}
void NativeTemporalBridge::Reset()noexcept{
    auto* s=static_cast<State*>(state);state=nullptr;if(!s)return;
    // A device/driver timeout leaves ownership unknown. Keep registrations,
    // COM references and driver module alive; no use-after-free at shutdown.
    if(s->SafeToDestroy())delete s;
}
NativeTemporalBridge::~NativeTemporalBridge(){Reset();}
}
