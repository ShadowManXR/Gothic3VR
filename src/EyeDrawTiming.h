#pragma once
#include "FramePerformance.h"
namespace diagnostic::performance {
struct DrawCallStamp {uint64_t started=0,generation=0;bool outer=false,tracked=false;};
struct EyeDrawSample {
    uint64_t started=0,elapsed=0,drawTicks=0,longestDraw=0;std::array<uint64_t,4> calls{};
    bool Json(char* line,std::size_t size,uint64_t hz,unsigned eye,int context,uint64_t frame,bool spike)const{
        if(!line||!size||!hz)return false;double ms=1000/double(hz);
        int n=std::snprintf(line,size,"{\"type\":\"vr_eye_draw_profile\",\"eye\":%u,\"context\":%d,\"xr_frame_serial\":%llu,\"is_spike\":%s,\"qpc_started\":%llu,\"qpc_frequency\":%llu,\"total_cpu_ms\":%.4f,\"draw_api_cpu_ms\":%.4f,\"outside_draw_api_cpu_ms\":%.4f,\"longest_draw_api_ms\":%.4f,\"draw_calls\":[%llu,%llu,%llu,%llu]}",
            eye,context,static_cast<unsigned long long>(frame),spike?"true":"false",static_cast<unsigned long long>(started),static_cast<unsigned long long>(hz),elapsed*ms,drawTicks*ms,(elapsed-(std::min)(drawTicks,elapsed))*ms,longestDraw*ms,
            static_cast<unsigned long long>(calls[0]),static_cast<unsigned long long>(calls[1]),static_cast<unsigned long long>(calls[2]),static_cast<unsigned long long>(calls[3]));
        return n>=0&&std::size_t(n)<size;
    }
};
// CPU wall time around the original D3D9 draw methods only. No GPU query,
// state change, wait, allocation or log write in a draw hook. Nested dispatch
// contributes once to elapsed time. Epochs reject stale returns after cancel.
class EyeDrawTiming {
    EyeDrawSample sample;uint64_t generation=0;unsigned depth=0;bool active=false;
public:
    void Begin(uint64_t now){Cancel();sample={};sample.started=now;active=true;}
    void Cancel(){active=false;depth=0;++generation;}
    DrawCallStamp BeginCall(unsigned kind,uint64_t now){
        if(!active||kind>=4)return {}; ++sample.calls[kind];
        return {now,generation,depth++==0,true};
    }
    void EndCall(DrawCallStamp stamp,uint64_t now){
        if(!active||!stamp.tracked||stamp.generation!=generation||!depth)return;
        --depth;if(stamp.outer&&now>=stamp.started){auto elapsed=now-stamp.started;sample.drawTicks+=elapsed;sample.longestDraw=(std::max)(sample.longestDraw,elapsed);}
    }
    bool Finish(uint64_t now,EyeDrawSample& out){
        bool valid=active&&!depth&&now>=sample.started;Cancel();if(!valid)return false;
        sample.elapsed=now-sample.started;if(sample.drawTicks>sample.elapsed)return false;out=sample;return true;
    }
};
}
