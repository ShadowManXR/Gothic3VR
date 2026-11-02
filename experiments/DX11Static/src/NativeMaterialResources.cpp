#include "NativeMaterialResources.h"
#include "DxvkInteropResources.h"
#include <d3dcompiler.h>

namespace g3vr::dx11 {
namespace {
using namespace dxvkinterop;
struct Shared {
    Vulkan& vk;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> view;
    ComPtr<IDXGIKeyedMutex> mutex;
    Handle handle;
    VkImage image{};
    VkDeviceMemory memory{};
    UINT width{}, height{},logicalWidth{},logicalHeight{};
    bool held = false;
    explicit Shared(Vulkan& v) : vk(v) {}
    ~Shared() {
        if (held) mutex->ReleaseSync(0);
        if (image) vk.DestroyImage(vk.device,image,nullptr);
        if (memory) vk.FreeMemory(vk.device,memory,nullptr);
    }
    void Create(ID3D11Device* d, UINT w, UINT h) {
        logicalWidth=w;logicalHeight=h;width=std::max(128u,w);height=std::max(128u,h);
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=width; desc.Height=height; desc.MipLevels=desc.ArraySize=1;
        // Carry depth's four raw bytes through the established BGRA8 interop
        // format; the conversion shader reconstructs the integer exactly.
        desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count=1; desc.Usage=D3D11_USAGE_DEFAULT;
        desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
        desc.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        stereo::Hr(d->CreateTexture2D(&desc,nullptr,&texture),"material shared texture");
        stereo::Hr(d->CreateShaderResourceView(texture.Get(),nullptr,&view),"material shared SRV");
        stereo::Hr(texture.As(&mutex),"material shared mutex");
        ComPtr<IDXGIResource1> resource;
        stereo::Hr(texture.As(&resource),"material shared resource");
        stereo::Hr(resource->CreateSharedHandle(nullptr,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&handle.value),"material NT handle");
        VkExternalMemoryImageCreateInfo external{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
        external.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
        VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; ci.pNext=&external;
        ci.imageType=VK_IMAGE_TYPE_2D; ci.format=VK_FORMAT_B8G8R8A8_UNORM;
        ci.extent={width,height,1}; ci.mipLevels=ci.arrayLayers=1; ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=VK_IMAGE_TILING_OPTIMAL; ci.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        VkCheck(vk.CreateImage(vk.device,&ci,nullptr,&image),"material import image");
        VkMemoryRequirements req{}; vk.GetImageMemoryRequirements(vk.device,image,&req);
        VkMemoryWin32HandlePropertiesKHR hp{VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR};
        VkCheck(vk.GetMemoryWin32HandlePropertiesKHR(vk.device,VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,handle.value,&hp),"material import properties");
        VkMemoryDedicatedAllocateInfo dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO}; dedicated.image=image;
        VkImportMemoryWin32HandleInfoKHR imp{VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR};
        imp.pNext=&dedicated; imp.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT; imp.handle=handle.value;
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; alloc.pNext=&imp;
        alloc.allocationSize=req.size; alloc.memoryTypeIndex=vk.MemoryType(req.memoryTypeBits&hp.memoryTypeBits);
        VkCheck(vk.AllocateMemory(vk.device,&alloc,nullptr,&memory),"material shared allocation");
        VkCheck(vk.BindImageMemory(vk.device,image,memory,0),"material shared bind");
    }
    VkImageMemoryBarrier Barrier(bool acquire) const {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.image=image; b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
        // Imported D3D11 memory is acquired from the external owner's GENERAL
        // layout, including the first use. Do not discard its driver metadata
        // through an UNDEFINED external-ownership acquisition.
        b.oldLayout=acquire?VK_IMAGE_LAYOUT_GENERAL:VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.newLayout=acquire?VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:VK_IMAGE_LAYOUT_GENERAL;
        b.srcAccessMask=acquire?0:VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask=acquire?VK_ACCESS_TRANSFER_WRITE_BIT:0;
        b.srcQueueFamilyIndex=acquire?VK_QUEUE_FAMILY_EXTERNAL:vk.family;
        b.dstQueueFamilyIndex=acquire?vk.family:VK_QUEUE_FAMILY_EXTERNAL;
        return b;
    }
};
struct Source {
    ComPtr<IDirect3DTexture9> texture;
    ComPtr<ID3D9VkInteropTexture> interop;
    VkImage image{}; VkImageLayout original{};
    VkImageSubresourceRange range{};
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    bool packed24=false;
    void Bind(IDirect3DDevice9Ex* owner, IDirect3DTexture9* t, bool depth) {
        Require(t!=nullptr,"Missing native material texture");
        ComPtr<IDirect3DDevice9> sourceDevice; stereo::Hr(t->GetDevice(&sourceDevice),"material source device");
        Require(sourceDevice.Get()==static_cast<IDirect3DDevice9*>(owner),"Foreign material source device");
        texture=t; stereo::Hr(t->QueryInterface(IID_PPV_ARGS(&interop)),"native material Vulkan texture");
        info={VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        stereo::Hr(interop->GetVulkanImageInfo(&image,&original,&info),"native material Vulkan properties");
        Require(image&&info.imageType==VK_IMAGE_TYPE_2D&&info.extent.depth==1&&info.mipLevels==1&&info.arrayLayers==1&&
            info.extent.width&&info.extent.height&&info.extent.width<=4096&&info.extent.height<=4096&&
            uint64_t(info.extent.width)*info.extent.height<=4194304&&info.samples==VK_SAMPLE_COUNT_1_BIT&&
            (info.usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT),"Unsupported native material dimensions/usage/samples");
        Require(original!=VK_IMAGE_LAYOUT_UNDEFINED&&original!=VK_IMAGE_LAYOUT_PREINITIALIZED,"Uninitialized native material contents");
        range={VkImageAspectFlags(depth?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT),0,1,0,1}; packed24=false;
        if (!depth) Require(info.format==VK_FORMAT_B8G8R8A8_UNORM,"Unsupported native lightmap format");
        else switch(info.format) {
        case VK_FORMAT_D24_UNORM_S8_UINT: range.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT; [[fallthrough]];
        case VK_FORMAT_X8_D24_UNORM_PACK32: packed24=true; break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT: range.aspectMask|=VK_IMAGE_ASPECT_STENCIL_BIT; [[fallthrough]];
        case VK_FORMAT_D32_SFLOAT: break;
        default: throw std::runtime_error("Unsupported material shadow depth format");
        }
    }
    void Prepare(Vulkan& vk) {
        // End the native pass/materialize deferred clears without changing its
        // tracked layout. DXVK 3.1 restores resource default layouts at submit;
        // an interop transition to TRANSFER_SRC does not survive that flush.
        vk.interop->TransitionTextureLayout(interop.Get(),&range,original,original);
    }
    VkImageMemoryBarrier Barrier(bool restore) const {
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.image=image; b.subresourceRange=range;
        b.oldLayout=restore?VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:original;
        b.newLayout=restore?original:VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcAccessMask=restore?VK_ACCESS_TRANSFER_READ_BIT:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
        b.dstAccessMask=restore?VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT:VK_ACCESS_TRANSFER_READ_BIT;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; return b;
    }
};
struct Depth {
    Vulkan& vk; Shared raw;
    VkBuffer buffer{}; VkDeviceMemory memory{};
    PFN_vkDestroyBuffer destroy{};
    ComPtr<ID3D11Texture2D> normalized;
    ComPtr<ID3D11ShaderResourceView> view;
    ComPtr<ID3D11UnorderedAccessView> uav;
    explicit Depth(Vulkan& v) : vk(v),raw(v) { destroy=vk.Device<PFN_vkDestroyBuffer>("vkDestroyBuffer"); }
    ~Depth() { if(buffer)destroy(vk.device,buffer,nullptr); if(memory)vk.FreeMemory(vk.device,memory,nullptr); }
    void Create(ID3D11Device* d,UINT w,UINT h) {
        raw.Create(d,w,h);
        VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; ci.size=VkDeviceSize(w)*h*4;
        ci.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT; ci.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
        VkCheck(vk.Device<PFN_vkCreateBuffer>("vkCreateBuffer")(vk.device,&ci,nullptr,&buffer),"GPU-only depth transfer buffer");
        VkMemoryRequirements req{}; vk.Device<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements")(vk.device,buffer,&req);
        VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; alloc.allocationSize=req.size; alloc.memoryTypeIndex=vk.MemoryType(req.memoryTypeBits);
        VkCheck(vk.AllocateMemory(vk.device,&alloc,nullptr,&memory),"GPU-only depth buffer memory");
        VkCheck(vk.Device<PFN_vkBindBufferMemory>("vkBindBufferMemory")(vk.device,buffer,memory,0),"GPU-only depth buffer bind");
        D3D11_TEXTURE2D_DESC desc{}; desc.Width=w;desc.Height=h;desc.MipLevels=desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R32_FLOAT;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;
        desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
        stereo::Hr(d->CreateTexture2D(&desc,nullptr,&normalized),"normalized shadow texture");
        stereo::Hr(d->CreateShaderResourceView(normalized.Get(),nullptr,&view),"normalized shadow SRV");
        stereo::Hr(d->CreateUnorderedAccessView(normalized.Get(),nullptr,&uav),"normalized shadow UAV");
    }
};
struct State {
    ComPtr<IDirect3DDevice9Ex> native;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    Vulkan vk;
    ComPtr<IDirect3DQuery9> complete9;
    ComPtr<ID3D11Query> complete11;
    ComPtr<ID3D11ComputeShader> convert;
    ComPtr<ID3D11Buffer> constants;
    std::unique_ptr<Shared> lightmap,noise;
    ComPtr<ID3D11Texture2D> smallLightmap;
    ComPtr<ID3D11ShaderResourceView> smallLightmapView;
    std::array<std::unique_ptr<Depth>,2> depths;
    std::array<Source,4> sources;unsigned sourceCount=3;
    uint64_t captures=0,allocations=0;
    bool failed=false, reads=false, ready=false;
    State(IDirect3DDevice9Ex* n,ID3D11Device* d,ID3D11DeviceContext* c) : native(n),device(d),context(c),vk(n) {
        ComPtr<ID3D11Device> owner; c->GetDevice(&owner);
        Require(owner.Get()==d&&c->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Material bridge requires owned immediate context");
        ComPtr<IDXGIDevice> dg; ComPtr<IDXGIAdapter> adapter; DXGI_ADAPTER_DESC desc{};
        stereo::Hr(d->QueryInterface(IID_PPV_ARGS(&dg)),"material DXGI device");stereo::Hr(dg->GetAdapter(&adapter),"material GPU");stereo::Hr(adapter->GetDesc(&desc),"material GPU description");
        VkPhysicalDeviceIDProperties id{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};VkPhysicalDeviceProperties2 props{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};props.pNext=&id;vk.properties(vk.physical,&props);
        Require(id.deviceLUIDValid&&!memcmp(id.deviceLUID,&desc.AdapterLuid,sizeof(LUID)),"Material bridge GPU mismatch");
        stereo::Hr(n->CreateQuery(D3DQUERYTYPE_EVENT,&complete9),"material native fence");
        D3D11_QUERY_DESC query{D3D11_QUERY_EVENT,0};stereo::Hr(d->CreateQuery(&query,&complete11),"material DX11 fence");
        char const* code="Texture2D<float4> rawDepth:register(t0); RWTexture2D<float> result:register(u0); cbuffer Params:register(b0){uint width;uint height;uint packed24;uint pad;} [numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID){if(p.x>=width||p.y>=height)return;uint4 bytes=(uint4)round(saturate(rawDepth.Load(int3(p.xy,0)))*255.0);uint raw=bytes.b|(bytes.g<<8)|(bytes.r<<16)|(bytes.a<<24);result[p.xy]=packed24?float(raw&0xffffff)/16777215.0:asfloat(raw);}";
        ComPtr<ID3DBlob> blob,error;stereo::Hr(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&blob,&error),"material GPU depth conversion compile");
        stereo::Hr(d->CreateComputeShader(blob->GetBufferPointer(),blob->GetBufferSize(),nullptr,&convert),"material GPU depth conversion shader");
        D3D11_BUFFER_DESC cb{};cb.ByteWidth=16;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        stereo::Hr(d->CreateBuffer(&cb,nullptr,&constants),"material conversion constants");
    }
    void Finish() {
        if(!reads)return;
        // These slots belong to the observed material. Do not leave an imported
        // SRV bound in the graphics context after returning its keyed ownership.
        ID3D11ShaderResourceView* empty[4]{};context->PSSetShaderResources(0,4,empty);
        Wait11(context.Get(),complete11.Get());
        Shared* images[]{lightmap.get(),&depths[0]->raw,&depths[1]->raw,noise.get()};
        for(unsigned i=0;i<sourceCount;++i)if(auto* image=images[i];image->held) { stereo::Hr(image->mutex->ReleaseSync(0),"return material GPU ownership");image->held=false; }
        // Publish the keyed releases before Vulkan can wait for key 0 again.
        // In particular the lightmap may have been consumed on the graphics
        // queue, whereas the raw shadow textures were read by compute.
        context->Flush();
        reads=false;
    }
};
State* Get(void* s) { Require(s!=nullptr,"Native material bridge is not initialized");return static_cast<State*>(s); }
}
bool NativeMaterialResources::Initialize(IDirect3DDevice9Ex* n,ID3D11Device* d,ID3D11DeviceContext* c) {
    Reset();Require(n&&d&&c,"Null material bridge device");ComPtr<ID3D9VkInteropDevice> interop;
    auto hr=n->QueryInterface(IID_PPV_ARGS(&interop));if(hr==E_NOINTERFACE)return false;
    stereo::Hr(hr,"material DXVK interface");state_=new State(n,d,c);return true;
}
void NativeMaterialResources::Capture(IDirect3DTexture9* l,IDirect3DTexture9* a,IDirect3DTexture9* b,IDirect3DTexture9* noise) {
    auto* s=Get(state_);Require(!s->failed,"Material bridge requires reconnect after failed transfer");s->Finish();s->ready=false;
    // Validate the complete batch before touching any layout or native state.
    s->sources[0].Bind(s->native.Get(),l,false);s->sources[1].Bind(s->native.Get(),a,true);s->sources[2].Bind(s->native.Get(),b,true);
    Require(l!=a&&l!=b&&a!=b,"Aliased material source images");
    s->sourceCount=noise?4:3;if(noise){Require(noise!=l&&noise!=a&&noise!=b,"Aliased material noise image");s->sources[3].Bind(s->native.Get(),noise,false);auto& e=s->sources[3].info.extent;Require(e.width==128&&e.height==128,"Unexpected native noise dimensions");if(!s->noise){s->noise=std::make_unique<Shared>(s->vk);s->noise->Create(s->device.Get(),128,128);++s->allocations;}}else s->sources[3]={};
    uint64_t pixels=0;for(unsigned i=0;i<s->sourceCount;++i)pixels+=uint64_t(s->sources[i].info.extent.width)*s->sources[i].info.extent.height;
    Require(pixels<=8388608,"Native material resource batch exceeds allocation budget");
    auto& li=s->sources[0].info.extent;
    if(!s->lightmap||s->lightmap->logicalWidth!=li.width||s->lightmap->logicalHeight!=li.height) {
        auto next=std::make_unique<Shared>(s->vk);next->Create(s->device.Get(),li.width,li.height);
        ComPtr<ID3D11Texture2D> logicalTexture;ComPtr<ID3D11ShaderResourceView> smallView;
        if(next->width!=li.width||next->height!=li.height) {
            D3D11_TEXTURE2D_DESC desc{};desc.Width=li.width;desc.Height=li.height;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
            desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
            stereo::Hr(s->device->CreateTexture2D(&desc,nullptr,&logicalTexture),"logical small lightmap");
            stereo::Hr(s->device->CreateShaderResourceView(logicalTexture.Get(),nullptr,&smallView),"logical small lightmap SRV");
        }
        s->lightmap=std::move(next);s->smallLightmap=logicalTexture;s->smallLightmapView=smallView;++s->allocations;
    }
    for(unsigned i=0;i<2;++i) { auto& e=s->sources[i+1].info.extent;
        if(!s->depths[i]||s->depths[i]->raw.logicalWidth!=e.width||s->depths[i]->raw.logicalHeight!=e.height) {
            auto next=std::make_unique<Depth>(s->vk);next->Create(s->device.Get(),e.width,e.height);s->depths[i]=std::move(next);++s->allocations;
        }
    }
    Shared* images[]{s->lightmap.get(),&s->depths[0]->raw,&s->depths[1]->raw,s->noise.get()};
    auto n=s->sourceCount;
    try {
        // The trial may already have queued target clears/previous rendering.
        // Publish and complete that work, including new resource initialization,
        // before the external queue tries to acquire the imported images.
        // Waiting for D3D9 alone cannot flush the independent D3D11 context.
        Wait11(s->context.Get(),s->complete11.Get());
        for(unsigned i=0;i<n;++i)s->sources[i].Prepare(s->vk);
        Wait9(s->complete9.Get());s->vk.Begin();
        VkImageMemoryBarrier begin[8];for(unsigned i=0;i<n;++i) { begin[i]=s->sources[i].Barrier(false);begin[i+n]=images[i]->Barrier(true); }
        s->vk.CmdPipelineBarrier(s->vk.commands,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2*n,begin);
        VkImageCopy region{};region.srcSubresource=region.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.extent=li;
        s->vk.CmdCopyImage(s->vk.commands,s->sources[0].image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,images[0]->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
        if(noise){region.extent=s->sources[3].info.extent;s->vk.CmdCopyImage(s->vk.commands,s->sources[3].image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,images[3]->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);}
        auto imageToBuffer=s->vk.Device<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer");
        auto bufferToImage=s->vk.Device<PFN_vkCmdCopyBufferToImage>("vkCmdCopyBufferToImage");
        for(unsigned i=0;i<2;++i) {
            auto& source=s->sources[i+1];auto& depth=*s->depths[i];
            VkBufferImageCopy r{};r.imageSubresource={VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};r.imageExtent=source.info.extent;
            imageToBuffer(s->vk.commands,source.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,depth.buffer,1,&r);
            VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};barrier.buffer=depth.buffer;barrier.size=VK_WHOLE_SIZE;
            barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
            barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
            s->vk.CmdPipelineBarrier(s->vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,1,&barrier,0,nullptr);
            r.imageSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            bufferToImage(s->vk.commands,depth.buffer,images[i+1]->image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&r);
        }
        VkImageMemoryBarrier end[8];for(unsigned i=0;i<n;++i){end[i]=s->sources[i].Barrier(true);end[i+n]=images[i]->Barrier(false);}
        s->vk.CmdPipelineBarrier(s->vk.commands,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,2*n,end);
        VkDeviceMemory imported[4]{};for(unsigned i=0;i<n;++i)imported[i]=images[i]->memory;
        s->failed=true;s->vk.Submit(imported,n);
        s->reads=true;
        for(unsigned i=0;i<n;++i) { auto* image=images[i];Require(image->mutex->AcquireSync(1,2000)==S_OK,"Material keyed acquire failed/timed out");image->held=true; }
        if(s->smallLightmap) {
            D3D11_BOX box{0,0,0,li.width,li.height,1};
            s->context->CopySubresourceRegion(s->smallLightmap.Get(),0,0,0,0,s->lightmap->texture.Get(),0,&box);
        }
        s->context->CSSetShader(s->convert.Get(),nullptr,0);ID3D11Buffer* cb=s->constants.Get();s->context->CSSetConstantBuffers(0,1,&cb);
        for(unsigned i=0;i<2;++i) {
            auto& depth=*s->depths[i];unsigned params[]{depth.raw.logicalWidth,depth.raw.logicalHeight,unsigned(s->sources[i+1].packed24),0};
            s->context->UpdateSubresource(cb,0,nullptr,params,0,0);ID3D11ShaderResourceView* srv=depth.raw.view.Get();auto* uav=depth.uav.Get();
            s->context->CSSetShaderResources(0,1,&srv);s->context->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
            s->context->Dispatch((params[0]+7)/8,(params[1]+7)/8,1);
            srv=nullptr;uav=nullptr;s->context->CSSetShaderResources(0,1,&srv);s->context->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
        }
        cb=nullptr;s->context->CSSetConstantBuffers(0,1,&cb);s->context->CSSetShader(nullptr,nullptr,0);
        ++s->captures;s->failed=false;s->ready=true;
    } catch(...) { s->failed=true;throw; }
}
ID3D11ShaderResourceView* NativeMaterialResources::Lightmap() const { auto* s=Get(state_);Require(s->ready&&s->reads&&!s->failed,"Missing/acquire-released material capture");return s->smallLightmapView?s->smallLightmapView.Get():s->lightmap->view.Get(); }
ID3D11ShaderResourceView* NativeMaterialResources::Noise() const {auto* s=Get(state_);Require(s->ready&&s->reads&&!s->failed&&s->sourceCount==4,"Missing/acquire-released noise capture");return s->noise->view.Get();}
ID3D11ShaderResourceView* NativeMaterialResources::Shadow(unsigned i) const { auto* s=Get(state_);Require(i<2&&s->ready&&!s->failed,"Missing shadow capture");return s->depths[i]->view.Get(); }
void NativeMaterialResources::FinishReads() { Get(state_)->Finish(); }
uint64_t NativeMaterialResources::Captures() const { return Get(state_)->captures; }
uint64_t NativeMaterialResources::Allocations() const { return Get(state_)->allocations; }
unsigned NativeMaterialResources::ShadowBackingFormat(unsigned i) const { auto* s=Get(state_);Require(i<2&&s->ready,"Missing shadow format");return unsigned(s->sources[i+1].info.format); }
void NativeMaterialResources::Reset() noexcept {
    auto* s=static_cast<State*>(state_);state_=nullptr;if(!s)return;
    // Preserve referenced GPU resources on timeout instead of freeing in-flight
    // memory. A failed bridge is never reused; the native renderer continues.
    if(!s->vk.ReadyToDestroy())return;
    try { s->Finish(); } catch(...) { return; }
    delete s;
}
NativeMaterialResources::~NativeMaterialResources() { Reset(); }
}
