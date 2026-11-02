#include "DxvkInteropResources.h"
namespace g3vr {
namespace {
using namespace dxvkinterop;
struct BridgeState {
    ComPtr<IDirect3DDevice9Ex> native;
    Vulkan vk;
    ComPtr<IDirect3DQuery9> renderComplete;
    std::vector<std::unique_ptr<Image>> images;
    std::unique_ptr<DepthReadback> depthReadback;
    std::array<Image*,3> queued{};size_t count=0;bool failed=false;
    explicit BridgeState(IDirect3DDevice9Ex* d9):native(d9),vk(d9){stereo::Hr(d9->CreateQuery(D3DQUERYTYPE_EVENT,&renderComplete),"DXVK rendering completion query");}
};
}
bool DxvkGpuBridge::Initialize(IDirect3DDevice9Ex* native){
    Reset();ComPtr<ID3D9VkInteropDevice> candidate;
    auto result=native->QueryInterface(IID_PPV_ARGS(&candidate));
    if(result==E_NOINTERFACE)return false;
    stereo::Hr(result,"query DXVK Vulkan interop");
    auto next=std::make_unique<BridgeState>(native);state=next.release();return true;
}
void DxvkGpuBridge::Create(IDirect3DDevice9Ex* native,ID3D11Device* device,UINT width,UINT height,
    ComPtr<IDirect3DTexture9>& texture9,ComPtr<IDirect3DSurface9>& surface9,ComPtr<ID3D11Texture2D>& texture11){
    auto* s=static_cast<BridgeState*>(state);Require(s&&s->native.Get()==native,"DXVK bridge/device mismatch");
    Require(!s->count&&s->images.size()<3&&width&&height,"Invalid DXVK bridge image creation");
    auto image=std::make_unique<Image>(s->vk);image->Create(native,device,width,height);
    texture9=image->source;surface9=image->surface;texture11=image->texture11;s->images.push_back(std::move(image));
}
void DxvkGpuBridge::Queue(ID3D11Texture2D* source){
    auto* s=static_cast<BridgeState*>(state);Require(s&&s->count<s->queued.size(),"DXVK pending image overflow");
    Image* selected=nullptr;for(auto& image:s->images)if(image->texture11.Get()==source)selected=image.get();
    Require(selected!=nullptr,"DXVK copy source is not owned by this bridge");
    for(size_t i=0;i<s->count;++i)Require(s->queued[i]!=selected,"Duplicate DXVK copy source");
    s->queued[s->count++]=selected;
}
void DxvkGpuBridge::Complete(){
    auto* s=static_cast<BridgeState*>(state);if(!s||!s->count)return;
    Require(!s->failed,"DXVK bridge must reconnect after failed ownership transfer");
    // Preserve one bounded D3D9 completion event for this source batch.
    // Prepare must first materialize DXVK's deferred framebuffer operations;
    // a completion query alone does not materialize a pending lazy clear.
    for(size_t i=0;i<s->count;++i)s->queued[i]->Prepare();
    Wait9(s->renderComplete.Get());
    s->failed=true;
    VkDeviceMemory imported[3]{};s->vk.Begin();for(size_t i=0;i<s->count;++i){s->queued[i]->Record();imported[i]=s->queued[i]->allocation;}s->vk.Submit(imported,uint32_t(s->count));
    for(size_t i=0;i<s->count;++i){auto* image=s->queued[i];auto result=image->nativeMutex->AcquireSync(1,2000);Require(result==S_OK,"D3D11 keyed acquire after DXVK copy failed/timed out");image->readHeld=true;}
    s->failed=false;
    s->queued={};s->count=0;
}
void DxvkGpuBridge::ReleaseReads(){
    auto* s=static_cast<BridgeState*>(state);if(!s)return;
    for(auto& image:s->images)if(image->readHeld){stereo::Hr(image->nativeMutex->ReleaseSync(0),"return imported texture ownership to DXVK");image->readHeld=false;}
}
bool DxvkGpuBridge::CaptureDepthSnapshot(IDirect3DSurface9* surface,DepthSnapshot& result){
    auto* s=static_cast<BridgeState*>(state);if(!s||!surface)return false;Require(!s->failed&&!s->vk.inFlight,"DXVK depth capture requires a healthy idle bridge");
    if(!s->depthReadback)s->depthReadback=std::make_unique<DepthReadback>(s->vk);
    try{s->depthReadback->Read(surface,result,s->renderComplete.Get());return true;}
    catch(...){if(s->vk.inFlight)s->failed=true;throw;}
}
void DxvkGpuBridge::ReleaseDepthSnapshotStorage()noexcept {auto* s=static_cast<BridgeState*>(state);if(s&&!s->vk.inFlight)s->depthReadback.reset();}
void DxvkGpuBridge::Reset() noexcept {
    auto* s=static_cast<BridgeState*>(state);state=nullptr;if(!s)return;
    // A GPU timeout must not free command buffers, imported memory or native
    // device references that may still be in use. Quarantine only that failed
    // bridge until process exit; ordinary completion releases everything.
    if(s->vk.ReadyToDestroy())delete s;
}
DxvkGpuBridge::~DxvkGpuBridge(){Reset();}
}
