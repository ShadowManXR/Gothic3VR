#pragma once
#include <windows.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace diagnostic::performance {
enum class Stage : unsigned {XrWait,Tracking,Arms,LeftRender,RightRender,UiDraw,D3D9Wait,SwapchainWait,D3D11Wait,EndFrame,Work,Count};
inline constexpr char const* Names[]={"xr_wait","tracking","arms_prepare","left_render","right_render","ui_draw","d3d9_wait","swapchain_wait","d3d11_wait","xr_end","frame_work"};
inline constexpr unsigned StageCount=unsigned(Stage::Count);
static_assert(sizeof(Names)/sizeof(Names[0])==StageCount);
enum class Counter : unsigned {D3D9Polls,D3D9Flushes,D3D11Polls,D3D11Flushes,Copies,UiDraws,Count};
inline constexpr unsigned CounterCount=unsigned(Counter::Count);
inline uint64_t Now() noexcept {LARGE_INTEGER n{};QueryPerformanceCounter(&n);return uint64_t(n.QuadPart);}
inline uint64_t Frequency() noexcept {LARGE_INTEGER n{};QueryPerformanceFrequency(&n);return uint64_t(n.QuadPart);}
struct Sample {
    std::array<uint64_t,StageCount> ticks{};
    std::array<uint64_t,CounterCount> counters{};
};
struct Report {
    uint64_t frames=0,intervals=0;
    double intervalMeanMs=0,intervalP95Ms=0,intervalMaxMs=0;
    std::array<double,StageCount> meanMs{},maxMs{};
    std::array<uint64_t,CounterCount> counters{};
    bool Json(char* out,std::size_t capacity,bool batched) const noexcept {
        if(!out||!capacity)return false;
        int n=std::snprintf(out,capacity,"{\"type\":\"vr_frame_performance\",\"batched_copies\":%s,\"frames\":%llu,\"interval_samples\":%llu,\"interval_mean_ms\":%.3f,\"interval_p95_recent_ms\":%.3f,\"interval_max_ms\":%.3f,\"cpu_wall_mean_ms\":{",batched?"true":"false",static_cast<unsigned long long>(frames),static_cast<unsigned long long>(intervals),intervalMeanMs,intervalP95Ms,intervalMaxMs);
        if(n<0||std::size_t(n)>=capacity)return false;std::size_t used=std::size_t(n);
        for(unsigned i=0;i<StageCount;++i){n=std::snprintf(out+used,capacity-used,"%s\"%s\":%.3f",i?",":"",Names[i],meanMs[i]);if(n<0||std::size_t(n)>=capacity-used)return false;used+=std::size_t(n);}
        n=std::snprintf(out+used,capacity-used,"},\"cpu_wall_max_ms\":{");if(n<0||std::size_t(n)>=capacity-used)return false;used+=std::size_t(n);
        for(unsigned i=0;i<StageCount;++i){n=std::snprintf(out+used,capacity-used,"%s\"%s\":%.3f",i?",":"",Names[i],maxMs[i]);if(n<0||std::size_t(n)>=capacity-used)return false;used+=std::size_t(n);}
        n=std::snprintf(out+used,capacity-used,"},\"d3d9_polls\":%llu,\"d3d9_flush_requests\":%llu,\"d3d11_polls\":%llu,\"d3d11_flushes\":%llu,\"copies\":%llu,\"ui_draws\":%llu}",static_cast<unsigned long long>(counters[0]),static_cast<unsigned long long>(counters[1]),static_cast<unsigned long long>(counters[2]),static_cast<unsigned long long>(counters[3]),static_cast<unsigned long long>(counters[4]),static_cast<unsigned long long>(counters[5]));
        return n>=0&&std::size_t(n)<capacity-used;
    }
};
// Fixed storage: no allocation, formatting, file I/O or extra GPU queries in
// per-frame collection. Report CPU wall times, never pretend these are GPU
// timestamps. In particular xr_wait includes intentional runtime pacing.
class Collector {
    uint64_t frequency=0,windowStart=0,previousReady=0,ready=0;
    bool active=false,started=false;
    Sample current,total,maximum;
    uint64_t frames=0,intervals=0,intervalSum=0,intervalMax=0;
    std::array<uint64_t,256> recent{};unsigned recentCount=0,recentNext=0;
    uint64_t currentInterval=0;
public:
    explicit Collector(uint64_t hz=Frequency()) noexcept:frequency(hz){}
    void Begin() noexcept {current={};ready=currentInterval=0;active=true;}
    void Ready(uint64_t now) noexcept {
        ready=now;if(previousReady&&now>=previousReady)currentInterval=now-previousReady;
        previousReady=now;if(!started){windowStart=now;started=true;}
    }
    void Add(Stage stage,uint64_t ticks) noexcept {if(active&&unsigned(stage)<StageCount)current.ticks[unsigned(stage)]+=ticks;}
    void Count(Counter counter,uint64_t value=1) noexcept {if(active&&unsigned(counter)<CounterCount)current.counters[unsigned(counter)]+=value;}
    void Cancel() noexcept {active=false;previousReady=ready=currentInterval=0;}
    void Pause() noexcept {Cancel();started=false;total={};maximum={};frames=intervals=intervalSum=intervalMax=0;recentCount=recentNext=0;}
    bool Finish(uint64_t now,Report& out) noexcept {
        if(!active||!ready||!frequency||now<ready){Cancel();return false;}
        Add(Stage::Work,now-ready);active=false;++frames;
        for(unsigned i=0;i<StageCount;++i){total.ticks[i]+=current.ticks[i];maximum.ticks[i]=(std::max)(maximum.ticks[i],current.ticks[i]);}
        for(unsigned i=0;i<CounterCount;++i)total.counters[i]+=current.counters[i];
        if(currentInterval){++intervals;intervalSum+=currentInterval;intervalMax=(std::max)(intervalMax,currentInterval);recent[recentNext]=currentInterval;recentNext=(recentNext+1)%unsigned(recent.size());recentCount=(std::min)(recentCount+1,unsigned(recent.size()));}
        if(now<windowStart||now-windowStart<frequency*2)return false;
        out={};out.frames=frames;out.intervals=intervals;double ms=1000.0/double(frequency);
        out.intervalMeanMs=intervals?double(intervalSum)*ms/double(intervals):0;out.intervalMaxMs=double(intervalMax)*ms;
        if(recentCount){auto ordered=recent;std::sort(ordered.begin(),ordered.begin()+recentCount);unsigned index=(recentCount*95+99)/100-1;out.intervalP95Ms=double(ordered[index])*ms;}
        for(unsigned i=0;i<StageCount;++i){out.meanMs[i]=double(total.ticks[i])*ms/double(frames);out.maxMs[i]=double(maximum.ticks[i])*ms;}
        out.counters=total.counters;windowStart=now;total={};maximum={};frames=intervals=intervalSum=intervalMax=0;recentCount=recentNext=0;
        return true;
    }
};
}
