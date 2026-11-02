#include "DxvkFoveation.h"
#include "D3D9Bridge.h"
#include "FoveationPattern.h"
#include "g3vr_ffr.h"
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include "d3d9_interfaces.h"
#include <memory>
#include <array>
namespace g3vr {namespace {
using stereo::ComPtr;
void Need(bool ok,char const* m){if(!ok)throw std::runtime_error(m);}
void Check(VkResult r,char const* m){if(r!=VK_SUCCESS)throw std::runtime_error(std::string(m)+": "+std::to_string(r));}

struct Loader {HMODULE value{};~Loader(){if(value)FreeLibrary(value);}};
#define FFR_VK_FUNCTIONS(X) X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(AllocateMemory) X(FreeMemory) X(BindImageMemory) X(CreateImageView) X(DestroyImageView) X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(CreateCommandPool) X(DestroyCommandPool) X(ResetCommandPool) X(AllocateCommandBuffers) X(BeginCommandBuffer) X(EndCommandBuffer) X(CmdPipelineBarrier) X(CmdCopyBufferToImage) X(CreateFence) X(DestroyFence) X(ResetFences) X(QueueSubmit) X(WaitForFences)
struct State {
    ComPtr<IDirect3DDevice9Ex> native;ComPtr<ID3D9VkInteropDevice> interop;ComPtr<IDirect3DQuery9> event;
    Loader loader;ComPtr<IG3VRFoveation> control;G3VRFfrCaps caps{sizeof(caps),G3VR_FFR_ABI};
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};uint32_t family{},queueIndex{};
    VkPhysicalDeviceMemoryProperties memory{};
    VkImage images[2]{};VkDeviceMemory allocations[2]{};VkImageView views[2]{};
    VkCommandPool pool{};VkCommandBuffer commands{};VkFence fence{};VkBuffer staging{};VkDeviceMemory stagingMemory{};
    unsigned width=0,height=0;bool inFlight=false;VkImageView bound=VK_NULL_HANDLE;
    bool uploaded=false;int appliedMode=0,appliedDiameter=0;std::array<float,4> appliedCenters{};
#define DECL(n) PFN_vk##n n{};
    FFR_VK_FUNCTIONS(DECL)
#undef DECL
    State(IDirect3DDevice9Ex* d9,IG3VRFoveation* fn,G3VRFfrCaps c):native(d9),control(fn),caps(c){
        stereo::Hr(d9->QueryInterface(IID_PPV_ARGS(&interop)),"FFR DXVK interop");interop->GetVulkanHandles(&instance,&physical,&device);interop->GetSubmissionQueue(&queue,&queueIndex,&family);
        loader.value=LoadLibraryExW(L"vulkan-1.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);Need(loader.value!=nullptr,"FFR Vulkan loader");
        auto get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader.value,"vkGetInstanceProcAddr"));Need(get!=nullptr,"FFR Vulkan entry");auto gd=reinterpret_cast<PFN_vkGetDeviceProcAddr>(get(instance,"vkGetDeviceProcAddr"));Need(gd!=nullptr,"FFR device entry");
        reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(get(instance,"vkGetPhysicalDeviceMemoryProperties"))(physical,&memory);
#define LOAD(n) n=reinterpret_cast<PFN_vk##n>(gd(device,"vk" #n));Need(n!=nullptr,"FFR vk" #n);
        FFR_VK_FUNCTIONS(LOAD)
#undef LOAD
        stereo::Hr(native->CreateQuery(D3DQUERYTYPE_EVENT,&event),"FFR completion event");
    }
    uint32_t Memory(uint32_t bits,VkMemoryPropertyFlags flags){for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i))&&(memory.memoryTypes[i].propertyFlags&flags)==flags)return i;throw std::runtime_error("FFR memory type");}
    void Allocate(VkMemoryRequirements const& r,VkMemoryPropertyFlags flags,VkDeviceMemory& m){VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=Memory(r.memoryTypeBits,flags);Check(AllocateMemory(device,&a,nullptr,&m),"FFR allocate");}
    void Set(VkImageView view){if(view==bound)return;G3VRFfrState s{sizeof(s),G3VR_FFR_ABI,uint64_t(view),width,height};stereo::Hr(control->SetState(&s),"DXVK FFR scope");bound=view;}
    void Upload(unsigned w,unsigned h,int mode,int diameter,float const* center){
        diameter=std::clamp(diameter,40,95);
        std::array<float,4> centers{center[0],center[1],center[2],center[3]};
        Need(!width||(w==width&&h==height),"FFR resolution change requires a new session");
        if(uploaded&&mode==appliedMode&&diameter==appliedDiameter&&centers==appliedCenters)return;
        // Reuse both tiny rate maps. Drain their preceding D3D9 reads before
        // changing staging data or resetting the upload command buffer.
        if(uploaded){Set(VK_NULL_HANDLE);interop->FlushRenderingCommands();Wait9(event.Get());
            Check(ResetCommandPool(device,pool,0),"FFR reset upload commands");Check(ResetFences(device,1,&fence),"FFR reset upload fence");}
        width=w;height=h;
        unsigned mw=(w+caps.tileWidth-1)/caps.tileWidth,mh=(h+caps.tileHeight-1)/caps.tileHeight;VkDeviceSize bytes=VkDeviceSize(mw)*mh,stride=(bytes+3)&~VkDeviceSize(3);
        VkMemoryRequirements req{};
        if(!uploaded){VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=stride*2;b.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;Check(CreateBuffer(device,&b,nullptr,&staging),"FFR upload buffer");GetBufferMemoryRequirements(device,staging,&req);Allocate(req,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingMemory);Check(BindBufferMemory(device,staging,stagingMemory,0),"FFR bind upload");}
        void* mapped=nullptr;Check(MapMemory(device,stagingMemory,0,stride*2,0,&mapped),"FFR map");
        for(unsigned eye=0;eye<2;++eye){auto data=diagnostic::FfrPattern(w,h,caps.tileWidth,caps.tileHeight,center[eye*2],center[eye*2+1],mode,diameter);memcpy(static_cast<uint8_t*>(mapped)+eye*stride,data.data(),size_t(bytes));}
        UnmapMemory(device,stagingMemory);
        if(!uploaded){
            for(unsigned eye=0;eye<2;++eye){VkImageCreateInfo i{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};i.imageType=VK_IMAGE_TYPE_2D;i.format=VK_FORMAT_R8_UINT;i.extent={mw,mh,1};i.mipLevels=i.arrayLayers=1;i.samples=VK_SAMPLE_COUNT_1_BIT;i.tiling=VK_IMAGE_TILING_OPTIMAL;i.usage=VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV|VK_IMAGE_USAGE_TRANSFER_DST_BIT;Check(CreateImage(device,&i,nullptr,&images[eye]),"FFR rate image");GetImageMemoryRequirements(device,images[eye],&req);Allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,allocations[eye]);Check(BindImageMemory(device,images[eye],allocations[eye],0),"FFR bind image");VkImageViewCreateInfo v{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=images[eye];v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=VK_FORMAT_R8_UINT;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};Check(CreateImageView(device,&v,nullptr,&views[eye]),"FFR rate view");}
            VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;Check(CreateCommandPool(device,&pc,nullptr,&pool),"FFR command pool");VkCommandBufferAllocateInfo ac{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ac.commandPool=pool;ac.commandBufferCount=1;ac.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;Check(AllocateCommandBuffers(device,&ac,&commands),"FFR commands");
            VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};Check(CreateFence(device,&fc,nullptr,&fence),"FFR upload fence");
        }
        VkCommandBufferBeginInfo bc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};bc.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;Check(BeginCommandBuffer(commands,&bc),"FFR begin upload");
        for(unsigned eye=0;eye<2;++eye){VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=images[eye];barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};barrier.oldLayout=uploaded?VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:VK_IMAGE_LAYOUT_UNDEFINED;barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.srcAccessMask=uploaded?VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV:0;barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;CmdPipelineBarrier(commands,uploaded?VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV:VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);VkBufferImageCopy copy{};copy.bufferOffset=eye*stride;copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={mw,mh,1};CmdCopyBufferToImage(commands,staging,images[eye],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barrier.newLayout=VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;CmdPipelineBarrier(commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV,0,0,nullptr,0,nullptr,1,&barrier);}
        Check(EndCommandBuffer(commands),"FFR end upload");interop->FlushRenderingCommands();VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&commands;interop->LockSubmissionQueue();auto result=QueueSubmit(queue,1,&submit,fence);interop->ReleaseSubmissionQueue();Check(result,"FFR submit");inFlight=true;Check(WaitForFences(device,1,&fence,VK_TRUE,2000000000ull),"FFR upload completion");inFlight=false;
        uploaded=true;appliedMode=mode;appliedDiameter=diameter;appliedCenters=centers;
    }
    ~State(){if(fence)DestroyFence(device,fence,nullptr);if(pool)DestroyCommandPool(device,pool,nullptr);if(staging)DestroyBuffer(device,staging,nullptr);if(stagingMemory)FreeMemory(device,stagingMemory,nullptr);for(unsigned i=0;i<2;++i){if(views[i])DestroyImageView(device,views[i],nullptr);if(images[i])DestroyImage(device,images[i],nullptr);if(allocations[i])FreeMemory(device,allocations[i],nullptr);}}
};
}
bool DxvkFoveation::Initialize(IDirect3DDevice9Ex* native){
    Reset();ComPtr<IG3VRFoveation> control;
    auto result=native->QueryInterface(IID_PPV_ARGS(&control));
    if(result==E_NOINTERFACE){reason="Backend has no G3VR FFR interface";return false;}stereo::Hr(result,"FFR interface query");
    G3VRFfrCaps caps{sizeof(caps),G3VR_FFR_ABI};if(FAILED(control->GetCaps(&caps))){reason="FFR ABI capability query failed";return false;}if(!caps.supported){reason="Device did not enable NV shading-rate features";return false;}if(!caps.tileWidth||!caps.tileHeight||caps.paletteSize<3){reason="Invalid hardware shading-rate tile/palette limits";return false;}
    auto s=std::make_unique<State>(native,control.Get(),caps);state=s.release();reason="Ready";return true;
}
void DxvkFoveation::Configure(unsigned width,unsigned height,int mode,int diameter,float lx,float ly,float rx,float ry){auto* s=static_cast<State*>(state);Need(s&&width&&height&&width<=16384&&height<=16384&&diagnostic::FfrMode(mode),"FFR configure state");float centers[]={lx,ly,rx,ry};for(float c:centers)Need(std::isfinite(c)&&c>=.1f&&c<=.9f,"FFR projection center");s->Upload(width,height,mode,diameter,centers);}
void DxvkFoveation::BeginEye(unsigned eye){auto* s=static_cast<State*>(state);if(s){Need(eye<2&&s->views[eye],"FFR eye not configured");s->Set(s->views[eye]);}}
void DxvkFoveation::EndEye(){auto* s=static_cast<State*>(state);if(s)s->Set(VK_NULL_HANDLE);}
void DxvkFoveation::Reset() noexcept{auto* s=static_cast<State*>(state);state=nullptr;if(!s)return;try{s->Set(VK_NULL_HANDLE);s->interop->FlushRenderingCommands();Wait9(s->event.Get());if(s->inFlight&&s->WaitForFences(s->device,1,&s->fence,VK_TRUE,2000000000ull)!=VK_SUCCESS)return;delete s;}catch(...){/* Quarantine resources after a GPU timeout rather than freeing live images. */}}
DxvkFoveation::~DxvkFoveation(){Reset();}
}
