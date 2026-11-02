#pragma once
#include "FramePerformance.h"
#include <cstdio>

namespace diagnostic::performance {
// Thread CPU time distinguishes a long executing scope from one which mostly
// waits. Process read counters are supporting evidence only: other game threads
// contribute, buffered reads count, and mapped-file page faults may not count.
struct SceneWorkSnapshot {
    uint64_t cpu100ns=0,readBytes=0,readOperations=0;
    bool cpuValid=false,ioValid=false;
};
inline SceneWorkSnapshot CaptureSceneWork() noexcept {
    SceneWorkSnapshot value;FILETIME created{},exited{},kernel{},user{};
    auto ticks=[](FILETIME const& t){return (uint64_t(t.dwHighDateTime)<<32)|t.dwLowDateTime;};
    if(GetThreadTimes(GetCurrentThread(),&created,&exited,&kernel,&user)){
        value.cpu100ns=ticks(kernel)+ticks(user);value.cpuValid=true;
    }
    IO_COUNTERS io{};
    if(GetProcessIoCounters(GetCurrentProcess(),&io)){
        value.readBytes=io.ReadTransferCount;value.readOperations=io.ReadOperationCount;value.ioValid=true;
    }
    return value;
}
struct SceneWorkSample {
    double wallMs=0,cpuMs=0;uint64_t readBytes=0,readOperations=0;
    bool cpuValid=false,ioValid=false;
    static SceneWorkSample Between(uint64_t elapsed,uint64_t frequency,SceneWorkSnapshot const& before,SceneWorkSnapshot const& after) noexcept {
        SceneWorkSample value;if(!frequency)return value;
        value.wallMs=double(elapsed)*1000/double(frequency);
        value.cpuValid=before.cpuValid&&after.cpuValid&&after.cpu100ns>=before.cpu100ns;
        if(value.cpuValid)value.cpuMs=double(after.cpu100ns-before.cpu100ns)/10000.;
        value.ioValid=before.ioValid&&after.ioValid&&after.readBytes>=before.readBytes&&after.readOperations>=before.readOperations;
        if(value.ioValid){value.readBytes=after.readBytes-before.readBytes;value.readOperations=after.readOperations-before.readOperations;}
        return value;
    }
    bool Json(char* output,size_t capacity,char const* scope,int context,int eye,uint64_t frame,uint64_t started,uint64_t frequency,bool threadingBefore,bool threadingAfter,unsigned jobsBefore,unsigned jobsAfter,unsigned workersBefore,unsigned workersAfter)const noexcept {
        if(!output||!capacity||!scope||!frequency)return false;
        // scope is a compile-time phase name supplied by the profiler.
        int n=std::snprintf(output,capacity,
            "{\"type\":\"vr_scene_stall\",\"scope\":\"%s\",\"context\":%d,\"eye\":%d,\"xr_frame_serial\":%llu,\"qpc_started\":%llu,\"qpc_frequency\":%llu,\"wall_ms\":%.4f,\"thread_cpu_ms\":%.4f,\"thread_cpu_valid\":%s,\"process_read_bytes\":%llu,\"process_read_operations\":%llu,\"process_io_valid\":%s,\"native_threading_before\":%s,\"native_threading_after\":%s,\"queued_jobs_before\":%u,\"queued_jobs_after\":%u,\"working_threads_before\":%u,\"working_threads_after\":%u}",
            scope,context,eye,static_cast<unsigned long long>(frame),static_cast<unsigned long long>(started),static_cast<unsigned long long>(frequency),wallMs,cpuMs,cpuValid?"true":"false",static_cast<unsigned long long>(readBytes),static_cast<unsigned long long>(readOperations),ioValid?"true":"false",threadingBefore?"true":"false",threadingAfter?"true":"false",jobsBefore,jobsAfter,workersBefore,workersAfter);
        return n>=0&&size_t(n)<capacity;
    }
};
struct SceneWorkState {
    SceneWorkSnapshot work;
    bool threading=false;unsigned jobs=0,workers=0;
};
}
