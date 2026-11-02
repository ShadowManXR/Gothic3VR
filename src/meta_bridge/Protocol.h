#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>
namespace diagnostic::meta_bridge {
inline constexpr uint32_t Version=2,MaximumPacket=1024*1024;
enum class Command:uint32_t {
 Hello=1,DestroyInstance,InstanceProperties,GetSystem,SystemProperties,GraphicsRequirements,
 CreateSession,DestroySession,BeginSession,EndSession,RequestExitSession,PollEvent,
 CreateReferenceSpace,DestroySpace,ViewConfigurationViews,BlendModes,SwapchainFormats,
 CreateSwapchain,DestroySwapchain,SwapchainImages,AcquireImage,WaitImage,ReleaseImage,
 WaitFrame,BeginFrame,EndFrame,LocateViews,ResultString,Haptic,StringToPath,
 CreateActionSet,DestroyActionSet,CreateAction,SuggestBindings,AttachActionSets,
 CreateActionSpace,SyncActions,PoseState,BooleanState,FloatState,VectorState,LocateSpace
};
struct Writer {
 std::vector<uint8_t> bytes;
 template<class T> void Pod(T const& value){static_assert(std::is_trivially_copyable_v<T>);auto p=reinterpret_cast<uint8_t const*>(&value);bytes.insert(bytes.end(),p,p+sizeof(T));}
 void Data(void const* value,size_t size){if(!size)return;if(size>MaximumPacket||bytes.size()>MaximumPacket-size)throw std::runtime_error("Meta bridge data too large");auto p=static_cast<uint8_t const*>(value);bytes.insert(bytes.end(),p,p+size);}
 void Text(std::string const& text){if(text.size()>32768)throw std::runtime_error("Meta bridge string too long");Pod(uint32_t(text.size()));bytes.insert(bytes.end(),text.begin(),text.end());}
 void Wide(std::wstring const& text){if(text.size()>32768)throw std::runtime_error("Meta bridge path too long");Pod(uint32_t(text.size()));auto p=reinterpret_cast<uint8_t const*>(text.data());bytes.insert(bytes.end(),p,p+text.size()*2);}
};
struct Reader {
 std::vector<uint8_t> bytes;size_t position=0;
 explicit Reader(std::vector<uint8_t> data):bytes(std::move(data)){}
 template<class T>T Pod(){static_assert(std::is_trivially_copyable_v<T>);if(position>bytes.size()||bytes.size()-position<sizeof(T))throw std::runtime_error("Truncated Meta bridge packet");T value;memcpy(&value,bytes.data()+position,sizeof(T));position+=sizeof(T);return value;}
 void Data(void* value,size_t size){if(size>bytes.size()-position)throw std::runtime_error("Truncated Meta bridge data");memcpy(value,bytes.data()+position,size);position+=size;}
 std::string Text(){auto n=Pod<uint32_t>();if(n>32768||n>bytes.size()-position)throw std::runtime_error("Invalid Meta bridge string");std::string s(reinterpret_cast<char const*>(bytes.data()+position),n);position+=n;return s;}
 std::wstring Wide(){auto n=Pod<uint32_t>();if(n>32768||size_t(n)*2>bytes.size()-position)throw std::runtime_error("Invalid Meta bridge path");std::wstring s(n,L'\0');memcpy(s.data(),bytes.data()+position,n*2);position+=n*2;return s;}
 void Complete()const{if(position!=bytes.size())throw std::runtime_error("Unexpected Meta bridge packet data");}
};
template<class T>uint64_t HandleBits(T value){static_assert(sizeof(T)==8);uint64_t bits;memcpy(&bits,&value,8);return bits;}
template<class T>T FromHandle(uint64_t bits){static_assert(sizeof(T)==8);T value;memcpy(&value,&bits,8);return value;}
struct PacketHeader {uint32_t version=Version,command=0,bytes=0,serial=0;};
static_assert(sizeof(PacketHeader)==16);
// Pointer-free records, with identical packing in both executables.
#pragma pack(push,8)
struct ActionState {uint64_t action=0,path=0;uint32_t active=0,changed=0;int64_t time=0;float x=0,y=0;};
struct LocatedSpace {uint64_t space=0,base=0,flags=0;float orientation[4]{0,0,0,1},position[3]{};uint32_t reserved=0;};
#pragma pack(pop)
static_assert(sizeof(ActionState)==40&&sizeof(LocatedSpace)==56);
}
