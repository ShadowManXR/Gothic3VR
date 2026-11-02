#pragma once
// Private shared Vulkan imports for forward and experimental reverse transport.
// Windows DXVK 3.1.1 interop: import a native D3D11 NT texture into the
// existing DXVK Vulkan device. Keep the established D3D11 OpenXR session.
// All three sources are copied in one submission; no CPU pixel round trip.
#include "DxvkGpuBridge.h"
#include "D3D9Bridge.h"
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include <dxgi1_2.h>
#include <vulkan/vulkan.h>
#include "d3d9_interfaces.h"
#include <algorithm>
#include <array>
#include <memory>

namespace g3vr::dxvkinterop {
using stereo::ComPtr;
inline void Require(bool value,char const* operation){if(!value)throw std::runtime_error(operation);}
inline void VkCheck(VkResult value,char const* operation){if(value!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+": VkResult "+std::to_string(value));}
struct Module {
    HMODULE value{};
    explicit Module(wchar_t const* name,DWORD flags=LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS){
        value=LoadLibraryExW(name,nullptr,flags);Require(value!=nullptr,"LoadLibraryEx failed");}
    ~Module(){if(value)FreeLibrary(value);}
};
struct Handle {
    HANDLE value{};
    ~Handle(){if(value)CloseHandle(value);}
};

#define DEVICE_FUNCTIONS(X) \
 X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(AllocateMemory) X(FreeMemory) X(BindImageMemory) \
 X(GetMemoryWin32HandlePropertiesKHR) X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) \
 X(ResetCommandPool) X(BeginCommandBuffer) X(EndCommandBuffer) X(CmdPipelineBarrier) X(CmdCopyImage) \
 X(CreateFence) X(DestroyFence) X(ResetFences) X(WaitForFences) X(QueueSubmit)

struct Vulkan {
    Module loader{L"vulkan-1.dll",LOAD_LIBRARY_SEARCH_SYSTEM32};
    ComPtr<ID3D9VkInteropDevice> interop;
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    uint32_t family{},queueIndex{};VkCommandPool pool{};VkCommandBuffer commands{};VkFence fence{};
    VkPhysicalDeviceMemoryProperties memory{};bool inFlight=false;
    PFN_vkGetInstanceProcAddr get{};PFN_vkGetDeviceProcAddr getDevice{};
    PFN_vkGetPhysicalDeviceProperties2 properties{};
#define DECLARE(n) PFN_vk##n n{};
    DEVICE_FUNCTIONS(DECLARE)
#undef DECLARE
    template<class T>T Instance(char const* name){auto p=get(instance,name);Require(p!=nullptr,name);return reinterpret_cast<T>(p);}
    template<class T>T Device(char const* name){auto p=getDevice(device,name);Require(p!=nullptr,name);return reinterpret_cast<T>(p);}
    explicit Vulkan(IDirect3DDevice9Ex* d9){
        stereo::Hr(d9->QueryInterface(IID_PPV_ARGS(&interop)),"DXVK ID3D9VkInteropDevice");
        interop->GetVulkanHandles(&instance,&physical,&device);
        interop->GetSubmissionQueue(&queue,&queueIndex,&family);
        Require(instance&&physical&&device&&queue,"DXVK returned null Vulkan handles");
        get=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(loader.value,"vkGetInstanceProcAddr"));Require(get!=nullptr,"Vulkan loader entry point");
        getDevice=Instance<PFN_vkGetDeviceProcAddr>("vkGetDeviceProcAddr");
        properties=Instance<PFN_vkGetPhysicalDeviceProperties2>("vkGetPhysicalDeviceProperties2");
        Instance<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties")(physical,&memory);
#define LOAD(n) n=Device<PFN_vk##n>("vk" #n);
        DEVICE_FUNCTIONS(LOAD)
#undef LOAD
        try{
            VkCommandPoolCreateInfo pc{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pc.queueFamilyIndex=family;
            VkCheck(CreateCommandPool(device,&pc,nullptr,&pool),"command pool");
            VkCommandBufferAllocateInfo ac{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};ac.commandPool=pool;ac.commandBufferCount=1;ac.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VkCheck(AllocateCommandBuffers(device,&ac,&commands),"command buffer");
            VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};VkCheck(CreateFence(device,&fc,nullptr,&fence),"Vulkan fence");
        }catch(...){if(pool)DestroyCommandPool(device,pool,nullptr);throw;}
    }
    bool ReadyToDestroy() noexcept {
        if(!inFlight)return true;
        auto result=WaitForFences(device,1,&fence,VK_TRUE,2000000000ull);
        return result==VK_SUCCESS||result==VK_ERROR_DEVICE_LOST;
    }
    ~Vulkan(){if(fence)DestroyFence(device,fence,nullptr);if(pool)DestroyCommandPool(device,pool,nullptr);}
    uint32_t MemoryType(uint32_t bits){
        for(uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i))&&(memory.memoryTypes[i].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))return i;
        throw std::runtime_error("No compatible DEVICE_LOCAL memory for imported D3D11 image");
    }
    void Begin(){
        Require(!inFlight,"Previous DXVK copy is still in flight");
        // Flush BEFORE locking the DXVK submission queue. Calling D3D methods
        // with that lock held can deadlock the DXVK worker.
        interop->FlushRenderingCommands();
        VkCheck(ResetCommandPool(device,pool,0),"reset command pool");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkCheck(BeginCommandBuffer(commands,&begin),"begin commands");
    }
    void Submit(VkDeviceMemory const* imported,uint32_t count){
        VkCheck(EndCommandBuffer(commands),"end commands");VkCheck(ResetFences(device,1,&fence),"reset fence");
        Require(count<=4, "Too many shared resources in one submission");
        uint64_t acquireKeys[4]{},releaseKeys[4]{1,1,1,1};uint32_t timeouts[4]{2000,2000,2000,2000};
        VkWin32KeyedMutexAcquireReleaseInfoKHR mutex{VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR};
        mutex.acquireCount=mutex.releaseCount=count;mutex.pAcquireSyncs=mutex.pReleaseSyncs=imported;
        mutex.pAcquireKeys=acquireKeys;mutex.pReleaseKeys=releaseKeys;mutex.pAcquireTimeouts=timeouts;
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.pNext=count?&mutex:nullptr;si.commandBufferCount=1;si.pCommandBuffers=&commands;
        interop->LockSubmissionQueue();auto result=QueueSubmit(queue,1,&si,fence);interop->ReleaseSubmissionQueue();
        VkCheck(result,"queue submit");inFlight=true;
        // A bounded fence wait, not vkQueueWaitIdle/device-wide draining.
        VkCheck(WaitForFences(device,1,&fence,VK_TRUE,2000000000ull),"Vulkan copy completion");inFlight=false;
    }
};

// One-shot diagnostic depth access. All data interpretation is based on the
// actual Vulkan backing format, not the D3D9 enum (DXVK may promote D24 to D32).
struct DepthReadback {
    Vulkan& vk;VkBuffer buffer{};VkDeviceMemory allocation{};void* mapped=nullptr;VkDeviceSize capacity=0;bool coherent=false;
    PFN_vkCreateBuffer create{};PFN_vkDestroyBuffer destroy{};PFN_vkGetBufferMemoryRequirements requirements{};
    PFN_vkBindBufferMemory bind{};PFN_vkMapMemory map{};PFN_vkUnmapMemory unmap{};
    PFN_vkInvalidateMappedMemoryRanges invalidate{};PFN_vkCmdCopyImageToBuffer copy{};
    explicit DepthReadback(Vulkan& v):vk(v){
        create=vk.Device<PFN_vkCreateBuffer>("vkCreateBuffer");destroy=vk.Device<PFN_vkDestroyBuffer>("vkDestroyBuffer");requirements=vk.Device<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements");
        bind=vk.Device<PFN_vkBindBufferMemory>("vkBindBufferMemory");map=vk.Device<PFN_vkMapMemory>("vkMapMemory");unmap=vk.Device<PFN_vkUnmapMemory>("vkUnmapMemory");
        invalidate=vk.Device<PFN_vkInvalidateMappedMemoryRanges>("vkInvalidateMappedMemoryRanges");copy=vk.Device<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer");
    }
    void Free()noexcept {if(mapped)unmap(vk.device,allocation);mapped=nullptr;if(buffer)destroy(vk.device,buffer,nullptr);buffer={};if(allocation)vk.FreeMemory(vk.device,allocation,nullptr);allocation={};capacity=0;}
    ~DepthReadback(){Free();}
    void Ensure(VkDeviceSize bytes){
        if(bytes<=capacity)return;Require(!vk.inFlight,"Depth readback buffer is in flight");Free();
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=bytes;info.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT;info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        VkCheck(create(vk.device,&info,nullptr,&buffer),"depth transfer buffer");VkMemoryRequirements req{};requirements(vk.device,buffer,&req);
        uint32_t type=UINT32_MAX;
        for(unsigned pass=0;pass<2&&type==UINT32_MAX;++pass)for(uint32_t i=0;i<vk.memory.memoryTypeCount;++i){auto flags=vk.memory.memoryTypes[i].propertyFlags;
            if((req.memoryTypeBits&(1u<<i))&&(flags&VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)&&(pass||(flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))){type=i;coherent=(flags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;break;}}
        Require(type!=UINT32_MAX,"No host-visible depth transfer memory");VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};alloc.allocationSize=req.size;alloc.memoryTypeIndex=type;
        VkCheck(vk.AllocateMemory(vk.device,&alloc,nullptr,&allocation),"depth transfer memory");VkCheck(bind(vk.device,buffer,allocation,0),"depth transfer buffer binding");
        VkCheck(map(vk.device,allocation,0,VK_WHOLE_SIZE,0,&mapped),"depth transfer map");capacity=bytes;
    }
    void Read(IDirect3DSurface9* surface,DepthSnapshot& result,IDirect3DQuery9* fence){
        D3DSURFACE_DESC desc{};stereo::Hr(surface->GetDesc(&desc),"native depth description");Require(desc.Width&&desc.Height&&uint64_t(desc.Width)*desc.Height<=16777216,"Invalid depth dimensions");
        ComPtr<ID3D9VkInteropTexture> texture;stereo::Hr(surface->QueryInterface(IID_PPV_ARGS(&texture)),"DXVK depth surface interop");
        VkImage image{};VkImageLayout layout{};VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};stereo::Hr(texture->GetVulkanImageInfo(&image,&layout,&info),"DXVK depth backing image");
        Require(image&&(info.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)&&info.samples==VK_SAMPLE_COUNT_1_BIT&&info.extent.width==desc.Width&&info.extent.height==desc.Height,"Unsupported depth backing image / samples");
        unsigned element=4;bool d16=false,d24=false;VkImageAspectFlags aspects=VK_IMAGE_ASPECT_DEPTH_BIT;
        switch(info.format){
        case VK_FORMAT_D16_UNORM:d16=true;element=2;break;
        case VK_FORMAT_D16_UNORM_S8_UINT:d16=true;element=2;aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;break;
        case VK_FORMAT_X8_D24_UNORM_PACK32:d24=true;break;
        case VK_FORMAT_D24_UNORM_S8_UINT:d24=true;aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;break;
        case VK_FORMAT_D32_SFLOAT:break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;break;
        default:throw std::runtime_error("Unknown Vulkan depth format");}
        size_t pixels=size_t(desc.Width)*desc.Height;Ensure(VkDeviceSize(pixels)*element);
        result.width=desc.Width;result.height=desc.Height;result.sourceFormat=unsigned(desc.Format);result.vulkanFormat=unsigned(info.format);result.depth.resize(pixels);
        Require(layout!=VK_IMAGE_LAYOUT_UNDEFINED&&layout!=VK_IMAGE_LAYOUT_PREINITIALIZED,"Depth contents have not been initialized");
        VkImageSubresourceRange range{aspects,0,1,0,1};
        struct RestoreLayout {
            Vulkan& vk;ID3D9VkInteropTexture* texture;VkImageSubresourceRange range;VkImageLayout original;bool pending=false;
            ~RestoreLayout(){if(pending&&!vk.inFlight){vk.interop->TransitionTextureLayout(texture,&range,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,original);vk.interop->FlushRenderingCommands();}}
        } restore{vk,texture.Get(),range,layout};
        // Materialize lazy clears and drain the DXVK submission before external
        // queue access. The image returns to its original layout before reuse.
        vk.interop->TransitionTextureLayout(texture.Get(),&range,layout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);restore.pending=true;Wait9(fence);vk.Begin();
        VkImageMemoryBarrier imageBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};imageBarrier.image=image;imageBarrier.subresourceRange=range;
        imageBarrier.oldLayout=imageBarrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;imageBarrier.srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT|VK_ACCESS_MEMORY_READ_BIT;imageBarrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        imageBarrier.srcQueueFamilyIndex=imageBarrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&imageBarrier);
        VkBufferImageCopy region{};region.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};region.imageExtent={desc.Width,desc.Height,1};copy(vk.commands,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&region);
        imageBarrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;imageBarrier.newLayout=layout;imageBarrier.srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;imageBarrier.dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};host.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;host.dstAccessMask=VK_ACCESS_HOST_READ_BIT;host.buffer=buffer;host.size=VK_WHOLE_SIZE;host.srcQueueFamilyIndex=host.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT|VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&host,1,&imageBarrier);vk.Submit(nullptr,0);restore.pending=false;
        if(!coherent){VkMappedMemoryRange mappedRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};mappedRange.memory=allocation;mappedRange.size=VK_WHOLE_SIZE;VkCheck(invalidate(vk.device,1,&mappedRange),"depth transfer invalidate");}
        for(size_t i=0;i<pixels;++i){float value;
            if(d16){uint16_t raw;memcpy(&raw,static_cast<unsigned char const*>(mapped)+i*2,2);value=float(raw)/65535.f;}
            else if(d24){uint32_t raw;memcpy(&raw,static_cast<unsigned char const*>(mapped)+i*4,4);value=float(raw&0xffffff)/16777215.f;}
            else memcpy(&value,static_cast<unsigned char const*>(mapped)+i*4,4);
            Require(std::isfinite(value)&&value>=0&&value<=1,"Invalid captured depth value");result.depth[i]=value;
        }
    }
};

struct Image {
    Vulkan& vk;ComPtr<IDirect3DTexture9> source;ComPtr<IDirect3DSurface9> surface;
    ComPtr<ID3D9VkInteropTexture> interop;
    ComPtr<ID3D11Texture2D> texture11;Handle shared;
    ComPtr<IDXGIKeyedMutex> nativeMutex;bool readHeld=false;
    VkImage sourceImage{},image{};VkDeviceMemory allocation{};VkExtent3D extent{};
    VkImageLayout originalLayout{};
    explicit Image(Vulkan& v):vk(v){}
    ~Image(){if(readHeld)nativeMutex->ReleaseSync(0);if(image)vk.DestroyImage(vk.device,image,nullptr);if(allocation)vk.FreeMemory(vk.device,allocation,nullptr);}
    void Create(IDirect3DDevice9Ex* d9,ID3D11Device* d11,UINT w,UINT h){
        extent={w,h,1};
        stereo::Hr(d9->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&source,nullptr),"unshared DXVK source");
        stereo::Hr(source->GetSurfaceLevel(0,&surface),"DXVK source surface");
        stereo::Hr(source.As(&interop),"ID3D9VkInteropTexture");
        VkImageCreateInfo sourceInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};VkImageLayout sourceLayout{};
        stereo::Hr(interop->GetVulkanImageInfo(&sourceImage,&sourceLayout,&sourceInfo),"DXVK backing image");
        Require(sourceInfo.format==VK_FORMAT_B8G8R8A8_UNORM&&(sourceInfo.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)&&sourceInfo.samples==VK_SAMPLE_COUNT_1_BIT,"Unexpected DXVK source image properties");
        D3D11_TEXTURE2D_DESC desc{};desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;
        desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        stereo::Hr(d11->CreateTexture2D(&desc,nullptr,&texture11),"native D3D11 export texture");
        stereo::Hr(texture11.As(&nativeMutex),"D3D11 import synchronization mutex");
        ComPtr<IDXGIResource1> resource;stereo::Hr(texture11.As(&resource),"IDXGIResource1");
        stereo::Hr(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&shared.value),"D3D11 NT handle");
        VkExternalMemoryImageCreateInfo external{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};external.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
        VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};ci.pNext=&external;ci.imageType=VK_IMAGE_TYPE_2D;ci.format=VK_FORMAT_B8G8R8A8_UNORM;
        ci.extent=extent;ci.mipLevels=ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;
        ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;ci.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        VkCheck(vk.CreateImage(vk.device,&ci,nullptr,&image),"import image");
        VkMemoryRequirements req{};vk.GetImageMemoryRequirements(vk.device,image,&req);
        VkMemoryWin32HandlePropertiesKHR hp{VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR};
        VkCheck(vk.GetMemoryWin32HandlePropertiesKHR(vk.device,VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,shared.value,&hp),"Win32 import properties");
        VkMemoryDedicatedAllocateInfo dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};dedicated.image=image;
        VkImportMemoryWin32HandleInfoKHR import{VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};import.pNext=&dedicated;
        import.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;import.handle=shared.value;
        VkMemoryAllocateInfo ma{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};ma.pNext=&import;ma.allocationSize=req.size;ma.memoryTypeIndex=vk.MemoryType(req.memoryTypeBits&hp.memoryTypeBits);
        VkCheck(vk.AllocateMemory(vk.device,&ma,nullptr,&allocation),"import D3D11 memory");
        VkCheck(vk.BindImageMemory(vk.device,image,allocation,0),"bind imported memory");
    }
    void Prepare(){
        stereo::Hr(interop->GetVulkanImageInfo(&sourceImage,&originalLayout,nullptr),"current source image/layout");
        VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        // Route this through DXVK's context: it must materialize lazy clears
        // and pending framebuffer operations before external Vulkan can read.
        vk.interop->TransitionTextureLayout(interop.Get(),&range,originalLayout,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    }
    void Record(){
        VkImageMemoryBarrier barriers[2]{};
        for(auto& b:barriers){b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
        barriers[0].image=sourceImage;barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barriers[0].srcAccessMask=VK_ACCESS_MEMORY_WRITE_BIT|VK_ACCESS_MEMORY_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        barriers[0].srcQueueFamilyIndex=barriers[0].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barriers[1].image=image;barriers[1].oldLayout=VK_IMAGE_LAYOUT_GENERAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].srcAccessMask=0;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].srcQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;barriers[1].dstQueueFamilyIndex=vk.family;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers);
        VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent=extent;
        vk.CmdCopyImage(vk.commands,sourceImage,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
        barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].newLayout=originalLayout;
        barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        barriers[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_GENERAL;
        barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].dstAccessMask=0;
        barriers[1].srcQueueFamilyIndex=vk.family;barriers[1].dstQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,2,barriers);
    }
};



#undef DEVICE_FUNCTIONS
}
