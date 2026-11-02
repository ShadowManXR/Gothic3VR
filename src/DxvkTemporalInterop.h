#pragma once
// Private copy of the proven bounded DXVK queue/import helper, kept separate
// from the installed color bridge until temporal transport passes live tests.
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_WIN32_KHR
#include "D3D9Bridge.h"
#include <dxgi1_2.h>
#include <vulkan/vulkan.h>
#include "d3d9_interfaces.h"
#include <memory>
#include <array>
namespace g3vr::temporal_interop {
using stereo::ComPtr;
inline void Require(bool value,char const* why){if(!value)throw std::runtime_error(why);}
inline void VkCheck(VkResult value,char const* why){if(value!=VK_SUCCESS)throw std::runtime_error(std::string(why)+": VkResult "+std::to_string(value));}
#define DEVICE_FUNCTIONS(X) \
 X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(AllocateMemory) X(FreeMemory) X(BindImageMemory) \
 X(GetMemoryWin32HandlePropertiesKHR) X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) \
 X(ResetCommandPool) X(BeginCommandBuffer) X(EndCommandBuffer) X(CmdPipelineBarrier) X(CmdCopyImage) \
 X(CreateFence) X(DestroyFence) X(ResetFences) X(WaitForFences) X(QueueSubmit)
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
    VkPhysicalDeviceMemoryProperties memory{};bool inFlight=false,submitted=false;
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
        submitted=false;
        // Flush BEFORE locking the DXVK submission queue. Calling D3D methods
        // with that lock held can deadlock the DXVK worker.
        interop->FlushRenderingCommands();
        VkCheck(ResetCommandPool(device,pool,0),"reset command pool");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VkCheck(BeginCommandBuffer(commands,&begin),"begin commands");
    }
    void Submit(VkDeviceMemory const* imported,uint32_t count){
        VkCheck(EndCommandBuffer(commands),"end commands");VkCheck(ResetFences(device,1,&fence),"reset fence");
        Require(count<=6,"Temporal batch overflow"); uint64_t acquireKeys[6]{},releaseKeys[6]{1,1,1,1,1,1};uint32_t timeouts[6]{2000,2000,2000,2000,2000,2000};
        VkWin32KeyedMutexAcquireReleaseInfoKHR mutex{VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHR};
        mutex.acquireCount=mutex.releaseCount=count;mutex.pAcquireSyncs=mutex.pReleaseSyncs=imported;
        mutex.pAcquireKeys=acquireKeys;mutex.pReleaseKeys=releaseKeys;mutex.pAcquireTimeouts=timeouts;
        VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};si.pNext=count?&mutex:nullptr;si.commandBufferCount=1;si.pCommandBuffers=&commands;
        interop->LockSubmissionQueue();auto result=QueueSubmit(queue,1,&si,fence);interop->ReleaseSubmissionQueue();
        VkCheck(result,"queue submit");submitted=true;inFlight=true;
        // A bounded fence wait, not vkQueueWaitIdle/device-wide draining.
        VkCheck(WaitForFences(device,1,&fence,VK_TRUE,2000000000ull),"Vulkan copy completion");inFlight=false;
    }
};


#undef DEVICE_FUNCTIONS
}
