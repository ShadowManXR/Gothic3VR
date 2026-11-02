#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>
namespace diagnostic::particles {
inline int Quality(int value){return std::clamp(value,0,2);}
inline unsigned char Lower(unsigned char c){return c>='A'&&c<='Z'?c+('a'-'A'):c;}
inline bool Starts(char const* name,char const* prefix){
    if(!name)return false;for(unsigned i=0;prefix[i];++i)if(Lower(name[i])!=Lower(prefix[i]))return false;return true;
}
inline bool Contains(char const* name,char const* word){
    if(!name)return false;for(unsigned i=0;i<128&&name[i];++i)if(Starts(name+i,word))return true;return false;
}
inline bool Effect(char const* name){
    return Starts(name,"VFX_Magic_")||Starts(name,"VFX_Env_Fire")||Starts(name,"VFX_Env_Flame")||
        Starts(name,"VFX_Env_Torch")||Starts(name,"VFX_Env_Smoke")||Starts(name,"VFX_Env_Candle");
}
inline bool HeatOnly(char const* name){
    return Effect(name)&&(Contains(name,"HeatHaze")||Contains(name,"Heat_Haze"));
}
inline int DrawCount(int count,int quality){
    quality=Quality(quality);if(!quality||count<=12)return count;
    return std::min(count,std::clamp(int(int64_t(count)*(quality==1?65:40)/100),12,quality==1?768:384));
}
inline constexpr uint32_t VerticesRva=0x153630,RefractionRva=0x14eb60;
inline constexpr unsigned char VerticesPrefix[]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
inline constexpr unsigned char RefractionPrefix[]{0x8b,0x81,0xa8,0x03,0,0};
inline constexpr size_t DataOffset=0x318,CountOffset=0x31c,CapacityOffset=0x320,FrameOffset=0x348,RecordSize=0xb4;
inline constexpr size_t ApplicationFrameOffset=0x5e8;
// A single application-thread scratch buffer. Only the CPU vertex builder sees
// a sampled view; native simulation, particle ages/collisions/emission, lifetime,
// instancing capacity and the original array are never modified or truncated.
// Native UpdateVertices updates its GPU draw count from the sampled count.
class RenderLease {
    struct Record {unsigned char bytes[RecordSize];};
    alignas(16) Record scratch[768]{};
    unsigned char* owner=nullptr;void* previous=nullptr;int before=0;
public:
    bool Active()const{return owner!=nullptr;}
    bool Begin(void* object,size_t size,int quality){
        if(sizeof(void*)!=4||owner||!object||size<CapacityOffset+4||!Quality(quality))return false;
        auto* p=static_cast<unsigned char*>(object);void* data=nullptr;int count=0,capacity=0;
        std::memcpy(&data,p+DataOffset,sizeof(data));std::memcpy(&count,p+CountOffset,4);std::memcpy(&capacity,p+CapacityOffset,4);
        if(!data||count<=12||count>capacity||capacity>16383||capacity<0)return false;
        int wanted=DrawCount(count,quality);if(wanted>=count)return false;
        auto* source=static_cast<unsigned char const*>(data);
        // Evenly cover the complete emitter, rather than dropping its newest
        // half or changing the subset between eyes. Native sorting uses scratch.
        for(int i=0;i<wanted;++i){size_t index=size_t(int64_t(i)*count/wanted);std::memcpy(scratch[i].bytes,source+index*RecordSize,RecordSize);}
        owner=p;previous=data;before=count;void* sampled=scratch;
        std::memcpy(p+DataOffset,&sampled,sizeof(sampled));std::memcpy(p+CountOffset,&wanted,4);return true;
    }
    void End(){if(!owner)return;std::memcpy(owner+DataOffset,&previous,sizeof(previous));std::memcpy(owner+CountOffset,&before,4);owner=nullptr;previous=nullptr;before=0;}
};
}
