#include "DxvkTemporalBridge.h"
#include "DxvkTemporalInterop.h"
#include <vector>
namespace g3vr {
namespace {
using namespace temporal_interop;
struct Image {
    Vulkan& vk;ComPtr<IDirect3DSurface9> source;ComPtr<ID3D9VkInteropTexture> interop;
    ComPtr<ID3D11Texture2D> output;ComPtr<IDXGIKeyedMutex> mutex;Handle shared;
    VkImage native{},imported{};VkDeviceMemory memory{},bufferMemory{};VkBuffer buffer{};
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};VkImageLayout original{};
    VkImageAspectFlags aspects=VK_IMAGE_ASPECT_COLOR_BIT;bool held=false,prepared=false;
    TemporalEncoding encoding=TemporalEncoding::Color16F;
    PFN_vkCreateBuffer createBuffer{};PFN_vkDestroyBuffer destroyBuffer{};
    PFN_vkGetBufferMemoryRequirements bufferRequirements{};PFN_vkBindBufferMemory bindBuffer{};
    PFN_vkCmdCopyImageToBuffer imageToBuffer{};PFN_vkCmdCopyBufferToImage bufferToImage{};
    explicit Image(Vulkan& v):vk(v){}
    ~Image(){if(held)mutex->ReleaseSync(0);if(imported)vk.DestroyImage(vk.device,imported,nullptr);if(memory)vk.FreeMemory(vk.device,memory,nullptr);if(buffer)destroyBuffer(vk.device,buffer,nullptr);if(bufferMemory)vk.FreeMemory(vk.device,bufferMemory,nullptr);}
    void Create(IDirect3DSurface9* surface,ID3D11Device* device){
        source=surface;D3DSURFACE_DESC desc{};stereo::Hr(surface->GetDesc(&desc),"Temporal native surface");
        Require(desc.Width&&desc.Height&&desc.Width<=8192&&desc.Height<=8192&&uint64_t(desc.Width)*desc.Height<=33554432,"Temporal source dimensions");
        stereo::Hr(surface->QueryInterface(IID_PPV_ARGS(&interop)),"Temporal DXVK texture interface");
        stereo::Hr(interop->GetVulkanImageInfo(&native,&original,&info),"Temporal native Vulkan image");
        Require(native&&(info.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT)&&info.imageType==VK_IMAGE_TYPE_2D&&info.samples==VK_SAMPLE_COUNT_1_BIT&&info.extent.width==desc.Width&&info.extent.height==desc.Height&&info.extent.depth==1&&info.arrayLayers==1&&info.mipLevels==1,"Unsupported temporal backing image");
        DXGI_FORMAT format{};VkFormat vkFormat{};unsigned bytes=4;
        switch(info.format){
        case VK_FORMAT_R16G16B16A16_SFLOAT:format=DXGI_FORMAT_R16G16B16A16_FLOAT;vkFormat=info.format;break;
        case VK_FORMAT_D16_UNORM_S8_UINT:aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;[[fallthrough]];
        case VK_FORMAT_D16_UNORM:encoding=TemporalEncoding::Depth16Unorm;format=DXGI_FORMAT_R16_UNORM;vkFormat=VK_FORMAT_R16_UNORM;bytes=2;break;
        case VK_FORMAT_D24_UNORM_S8_UINT:aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;[[fallthrough]];
        case VK_FORMAT_X8_D24_UNORM_PACK32:encoding=TemporalEncoding::Depth24Uint;format=DXGI_FORMAT_R32_UINT;vkFormat=VK_FORMAT_R32_UINT;break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:aspects|=VK_IMAGE_ASPECT_STENCIL_BIT;[[fallthrough]];
        case VK_FORMAT_D32_SFLOAT:encoding=TemporalEncoding::Depth32Float;format=DXGI_FORMAT_R32_FLOAT;vkFormat=VK_FORMAT_R32_SFLOAT;break;
        default:throw std::runtime_error("Unsupported temporal source format");}
        if(encoding!=TemporalEncoding::Color16F){aspects=(aspects&VK_IMAGE_ASPECT_STENCIL_BIT)|VK_IMAGE_ASPECT_DEPTH_BIT;
            createBuffer=vk.Device<PFN_vkCreateBuffer>("vkCreateBuffer");destroyBuffer=vk.Device<PFN_vkDestroyBuffer>("vkDestroyBuffer");bufferRequirements=vk.Device<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements");bindBuffer=vk.Device<PFN_vkBindBufferMemory>("vkBindBufferMemory");
            imageToBuffer=vk.Device<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer");bufferToImage=vk.Device<PFN_vkCmdCopyBufferToImage>("vkCmdCopyBufferToImage");
            VkBufferCreateInfo b{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};b.size=VkDeviceSize(desc.Width)*desc.Height*bytes;b.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            VkCheck(createBuffer(vk.device,&b,nullptr,&buffer),"Temporal device-local buffer");VkMemoryRequirements req{};bufferRequirements(vk.device,buffer,&req);
            VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=req.size;a.memoryTypeIndex=vk.MemoryType(req.memoryTypeBits);
            VkCheck(vk.AllocateMemory(vk.device,&a,nullptr,&bufferMemory),"Temporal GPU buffer allocation");VkCheck(bindBuffer(vk.device,buffer,bufferMemory,0),"Temporal GPU buffer binding");}
        D3D11_TEXTURE2D_DESC t{};t.Width=desc.Width;t.Height=desc.Height;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=format;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        t.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        stereo::Hr(device->CreateTexture2D(&t,nullptr,&output),"Temporal D3D11 shared image");stereo::Hr(output.As(&mutex),"Temporal keyed mutex");
        ComPtr<IDXGIResource1> resource;stereo::Hr(output.As(&resource),"Temporal NT resource");stereo::Hr(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&shared.value),"Temporal NT handle");
        VkExternalMemoryImageCreateInfo external{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};external.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
        VkImageCreateInfo importedInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};importedInfo.pNext=&external;importedInfo.imageType=VK_IMAGE_TYPE_2D;importedInfo.format=vkFormat;importedInfo.extent=info.extent;importedInfo.mipLevels=importedInfo.arrayLayers=1;importedInfo.samples=VK_SAMPLE_COUNT_1_BIT;importedInfo.tiling=VK_IMAGE_TILING_OPTIMAL;importedInfo.usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
        VkCheck(vk.CreateImage(vk.device,&importedInfo,nullptr,&imported),"Temporal Vulkan import image");
        VkMemoryRequirements req{};vk.GetImageMemoryRequirements(vk.device,imported,&req);VkMemoryWin32HandlePropertiesKHR hp{VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR};
        VkCheck(vk.GetMemoryWin32HandlePropertiesKHR(vk.device,VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,shared.value,&hp),"Temporal NT import properties");
        VkMemoryDedicatedAllocateInfo dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};dedicated.image=imported;
        VkImportMemoryWin32HandleInfoKHR import{VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};import.pNext=&dedicated;import.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;import.handle=shared.value;
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};alloc.pNext=&import;alloc.allocationSize=req.size;alloc.memoryTypeIndex=vk.MemoryType(req.memoryTypeBits&hp.memoryTypeBits);
        VkCheck(vk.AllocateMemory(vk.device,&alloc,nullptr,&memory),"Temporal import memory");VkCheck(vk.BindImageMemory(vk.device,imported,memory,0),"Temporal imported binding");
    }
    void Prepare(){
        Require(!held,"Temporal image still read-owned");VkImage current{};VkImageCreateInfo now{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        stereo::Hr(interop->GetVulkanImageInfo(&current,&original,&now),"Temporal current image/layout");
        Require(current==native&&now.format==info.format&&now.extent.width==info.extent.width&&now.extent.height==info.extent.height&&original!=VK_IMAGE_LAYOUT_UNDEFINED&&original!=VK_IMAGE_LAYOUT_PREINITIALIZED,"Temporal backing changed/uninitialized");
        VkImageSubresourceRange range{aspects,0,1,0,1};vk.interop->TransitionTextureLayout(interop.Get(),&range,original,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);prepared=true;
    }
    void RestorePrepared(){if(!prepared||vk.inFlight)return;VkImageSubresourceRange range{aspects,0,1,0,1};vk.interop->TransitionTextureLayout(interop.Get(),&range,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,original);prepared=false;}
    void Record(){
        VkImageMemoryBarrier barriers[2]{};for(auto& b:barriers){b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
        barriers[0].image=native;barriers[0].subresourceRange.aspectMask=aspects;barriers[0].oldLayout=barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].srcAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barriers[0].srcQueueFamilyIndex=barriers[0].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barriers[1].image=imported;barriers[1].oldLayout=VK_IMAGE_LAYOUT_GENERAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].srcQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;barriers[1].dstQueueFamilyIndex=vk.family;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,barriers);
        if(buffer){
            // Depth and color aspects cannot be directly copied. The packed
            // depth bytes transit only a DEVICE_LOCAL buffer, never the CPU.
            VkBufferImageCopy region{};region.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};region.imageExtent=info.extent;
            imageToBuffer(vk.commands,native,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&region);
            VkBufferMemoryBarrier b{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b.buffer=buffer;b.size=VK_WHOLE_SIZE;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&b,0,nullptr);
            region.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;bufferToImage(vk.commands,buffer,imported,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        }else{VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent=info.extent;vk.CmdCopyImage(vk.commands,native,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,imported,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);}
        barriers[0].newLayout=original;barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        barriers[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_GENERAL;barriers[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barriers[1].dstAccessMask=0;barriers[1].srcQueueFamilyIndex=vk.family;barriers[1].dstQueueFamilyIndex=VK_QUEUE_FAMILY_EXTERNAL;
        vk.CmdPipelineBarrier(vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,2,barriers);
    }
};
struct State {
    ComPtr<IDirect3DDevice9Ex> native;Vulkan vk;ComPtr<IDirect3DQuery9> complete;
    std::vector<std::unique_ptr<Image>> images;std::array<Image*,6> queued{};unsigned count=0;bool failed=false;
    explicit State(IDirect3DDevice9Ex* n):native(n),vk(n){stereo::Hr(n->CreateQuery(D3DQUERYTYPE_EVENT,&complete),"Temporal native fence");}
};
}
bool DxvkTemporalBridge::Initialize(IDirect3DDevice9Ex* native){Reset();Require(native!=nullptr,"Temporal native device missing");ComPtr<ID3D9VkInteropDevice> candidate;auto hr=native->QueryInterface(IID_PPV_ARGS(&candidate));if(hr==E_NOINTERFACE)return false;stereo::Hr(hr,"Temporal DXVK interop");state=new State(native);return true;}
TemporalEncoding DxvkTemporalBridge::Register(IDirect3DSurface9* source,ID3D11Device* device,ComPtr<ID3D11Texture2D>& output){
    auto* s=static_cast<State*>(state);Require(s&&source&&device&&!s->count&&!s->failed&&s->images.size()<6,"Temporal registration state");
    ComPtr<IDirect3DDevice9> owner;stereo::Hr(source->GetDevice(&owner),"Temporal source device");Require(owner.Get()==s->native.Get(),"Temporal source device mismatch");
    // Imported NT handles must belong to exactly the DXVK rendering adapter.
    ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;stereo::Hr(device->QueryInterface(IID_PPV_ARGS(&dxgi)),"Temporal DXGI device");stereo::Hr(dxgi->GetAdapter(&adapter),"Temporal DXGI adapter");DXGI_ADAPTER_DESC desc{};stereo::Hr(adapter->GetDesc(&desc),"Temporal adapter description");
    VkPhysicalDeviceIDProperties id{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};props.pNext=&id;s->vk.properties(s->vk.physical,&props);Require(id.deviceLUIDValid&&!memcmp(id.deviceLUID,&desc.AdapterLuid,sizeof(LUID)),"Temporal adapters differ");
    auto image=std::make_unique<Image>(s->vk);image->Create(source,device);auto encoding=image->encoding;output=image->output;s->images.push_back(std::move(image));return encoding;
}
void DxvkTemporalBridge::Queue(ID3D11Texture2D* output){auto* s=static_cast<State*>(state);Require(s&&!s->failed&&s->count<s->queued.size(),"Temporal queue state/capacity");Image* found=nullptr;for(auto& image:s->images)if(image->output.Get()==output)found=image.get();Require(found&&!found->held,"Temporal output foreign/read-owned");for(unsigned i=0;i<s->count;++i)Require(s->queued[i]!=found&&s->queued[i]->native!=found->native,"Duplicate/aliased temporal source in one submission");s->queued[s->count++]=found;}
void DxvkTemporalBridge::Complete(){
    auto* s=static_cast<State*>(state);if(!s||!s->count)return;Require(!s->failed,"Temporal bridge must reconnect after transfer failure");
    Require(!s->vk.inFlight,"Temporal transfer still in flight");s->vk.submitted=false;
    try{for(unsigned i=0;i<s->count;++i)s->queued[i]->Prepare();Wait9(s->complete.Get());s->vk.Begin();for(unsigned i=0;i<s->count;++i)s->queued[i]->Record();
        VkDeviceMemory imported[6]{};for(unsigned i=0;i<s->count;++i)imported[i]=s->queued[i]->memory;s->failed=true;s->vk.Submit(imported,s->count);
        for(unsigned i=0;i<s->count;++i)s->queued[i]->prepared=false;
        for(unsigned i=0;i<s->count;++i){auto* image=s->queued[i];Require(image->mutex->AcquireSync(1,2000)==S_OK,"Temporal D3D11 acquire failed/timed out");image->held=true;}
        s->failed=false;s->queued={};s->count=0;
    }catch(...){if(!s->vk.submitted&&!s->vk.inFlight){for(unsigned i=0;i<s->count;++i)s->queued[i]->RestorePrepared();s->vk.interop->FlushRenderingCommands();s->queued={};s->count=0;s->failed=false;}throw;}
}
void DxvkTemporalBridge::ReleaseReads(){auto* s=static_cast<State*>(state);if(!s)return;for(auto& image:s->images)if(image->held){stereo::Hr(image->mutex->ReleaseSync(0),"Temporal return GPU ownership");image->held=false;}}
void DxvkTemporalBridge::Reset()noexcept{auto* s=static_cast<State*>(state);state=nullptr;if(!s)return;if(s->vk.ReadyToDestroy())delete s;}
DxvkTemporalBridge::~DxvkTemporalBridge(){Reset();}
}
