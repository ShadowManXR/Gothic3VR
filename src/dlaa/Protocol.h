#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
namespace dlaa {
constexpr uint32_t Magic=0x41414c44,ProtocolVersion=2;
enum class Message:uint32_t {Initialize=1,Frame=2,Stop=3,Reply=4};
enum InputBits:uint32_t {ColorValid=1,DepthValid=2,MotionValid=4,JitterValid=8,ObjectMotionValid=16,ReactiveValid=32,AllInputs=63};
enum class Source:uint32_t {Unknown=0,SyntheticValidationScene=1,GameRenderer=2};
#pragma pack(push,8)
struct Header {uint32_t magic=Magic,version=ProtocolVersion;Message type=Message::Stop;uint32_t bytes=0;};
struct Initialize {
    uint32_t width=0,height=0,adapterLow=0;int32_t adapterHigh=0;
    Source source=Source::Unknown;uint32_t reserved=0;
    uint64_t textures[2][5]{}; // Color RGBA16F, depth R32F, motion RG16F, output RGBA16F, reactive R8; remote NT handles.
};
struct Frame {
    uint64_t serial=0;uint32_t validInputs=0,resetMask=0;
    float jitterX=0,jitterY=0,deltaMs=0,reserved=0;
};
struct Reply {
    uint64_t serial=0;uint32_t success=0,writtenEyes=0;
    double gpuMs[2]{},cpuMs=0;char detail[256]{};
};
#pragma pack(pop)
static_assert(sizeof(Header)==16&&sizeof(Initialize)==104&&sizeof(Frame)==32&&sizeof(Reply)==296,"Stable x86/x64 protocol layout");
inline bool Valid(Header const& h,Message expected,uint32_t bytes){return h.magic==Magic&&h.version==ProtocolVersion&&h.type==expected&&h.bytes==bytes;}
inline bool Valid(Initialize const& value){
    if(value.width<64||value.height<64||value.width>8192||value.height>8192||uint64_t(value.width)*value.height>16777216||value.reserved||
       (value.source!=Source::SyntheticValidationScene&&value.source!=Source::GameRenderer))return false;
    uint64_t seen[10]{};unsigned count=0;
    for(auto const& eye:value.textures)for(uint64_t h:eye){if(!h||h==UINT64_MAX)return false;for(unsigned i=0;i<count;++i)if(h==seen[i])return false;seen[count++]=h;}
    return true;
}
inline bool Valid(Frame const& value,uint64_t previous){
    return value.serial>previous&&(value.validInputs&AllInputs)==AllInputs&&!(value.validInputs&~AllInputs)&&!(value.resetMask&~3u)&&
        std::isfinite(value.jitterX)&&std::isfinite(value.jitterY)&&std::fabs(value.jitterX)<=.5f&&std::fabs(value.jitterY)<=.5f&&
        std::isfinite(value.deltaMs)&&value.deltaMs>0&&value.deltaMs<=1000&&value.reserved==0;
}
inline float Halton(uint32_t i,uint32_t base){float value=0,f=1;while(i){f/=float(base);value+=f*float(i%base);i/=base;}return value;}
inline void Jitter(uint64_t serial,float& x,float& y){auto i=uint32_t((serial-1)%16)+1;x=Halton(i,2)-.5f;y=Halton(i,3)-.5f;}
}
