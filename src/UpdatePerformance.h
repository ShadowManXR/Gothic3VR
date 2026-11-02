#pragma once
#include "FramePerformance.h"
namespace diagnostic::performance {
enum class UpdateStage : unsigned {Native,Input,Melee,Ranged,Finisher,Focus,Count};
inline constexpr unsigned UpdateStageCount=unsigned(UpdateStage::Count);
inline constexpr char const* UpdateNames[]={"application_process_callback","vr_input","physical_melee","physical_ranged","finisher_search","focus_during_application_callback"};
struct UpdateReport {
    uint64_t updates=0;int context=0;
    std::array<double,UpdateStageCount> meanMs{},maxMs{};
    std::array<uint64_t,UpdateStageCount> calls{};
    bool Json(char* out,std::size_t capacity)const noexcept{
        if(!out||!capacity)return false;std::size_t used=0;
        auto append=[&](char const* format,auto... args){int n=std::snprintf(out+used,capacity-used,format,args...);if(n<0||std::size_t(n)>=capacity-used)return false;used+=std::size_t(n);return true;};
        if(!append("{\"type\":\"vr_update_performance\",\"context\":%d,\"updates\":%llu,\"cpu_wall_mean_per_update_ms\":{",context,static_cast<unsigned long long>(updates)))return false;
        for(unsigned i=0;i<UpdateStageCount;++i)if(!append("%s\"%s\":%.4f",i?",":"",UpdateNames[i],meanMs[i]))return false;
        if(!append("},\"cpu_wall_max_per_update_ms\":{"))return false;
        for(unsigned i=0;i<UpdateStageCount;++i)if(!append("%s\"%s\":%.4f",i?",":"",UpdateNames[i],maxMs[i]))return false;
        if(!append("},\"calls\":{"))return false;
        for(unsigned i=0;i<UpdateStageCount;++i)if(!append("%s\"%s\":%llu",i?",":"",UpdateNames[i],static_cast<unsigned long long>(calls[i])))return false;
        return append("},\"note\":\"application callback is not the native simulation; input includes finisher search; stages overlap\"}");
    }
};
class UpdateCollector {
    uint64_t hz=0,start=0,updates=0;int context=-1;bool active=false;
    std::array<uint64_t,UpdateStageCount> current{},currentCalls{},total{},maximum{},calls{};
public:
    explicit UpdateCollector(uint64_t frequency=Frequency()):hz(frequency){}
    void Reset(){start=updates=0;context=-1;active=false;current={};currentCalls={};total={};maximum={};calls={};}
    void Begin(int next,uint64_t now){if(context!=next||now<start){Reset();context=next;start=now;}current={};currentCalls={};active=true;}
    void Add(UpdateStage stage,uint64_t ticks){unsigned i=unsigned(stage);if(active&&i<UpdateStageCount){current[i]+=ticks;++currentCalls[i];}}
    bool End(uint64_t now,UpdateReport& report){
        if(!active||!hz||now<start){Reset();return false;}active=false;++updates;
        for(unsigned i=0;i<UpdateStageCount;++i){total[i]+=current[i];maximum[i]=(std::max)(maximum[i],current[i]);calls[i]+=currentCalls[i];}
        if(now-start<2*hz)return false;
        report={};report.context=context;report.updates=updates;report.calls=calls;
        for(unsigned i=0;i<UpdateStageCount;++i){report.meanMs[i]=double(total[i])*1000/double(hz)/double(updates);report.maxMs[i]=double(maximum[i])*1000/double(hz);}
        start=now;updates=0;total={};maximum={};calls={};return true;
    }
};
}
