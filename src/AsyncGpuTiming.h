#pragma once
#include <d3d9.h>
#include <wrl/client.h>
#include "FramePerformance.h"
namespace diagnostic::performance {
inline bool GpuDurations(uint64_t frequency,bool disjoint,std::array<uint64_t,4> const& stamps,std::array<double,2>& ms){
    if(!frequency||disjoint||stamps[1]<stamps[0]||stamps[2]<stamps[1]||stamps[3]<stamps[2])return false;
    ms={double(stamps[1]-stamps[0])*1000/double(frequency),double(stamps[3]-stamps[2])*1000/double(frequency)};return true;
}
struct GpuReport {
    int context=0;unsigned samplePeriod=16;uint64_t samples=0,discarded=0,skipped=0;
    std::array<double,2> sum{},maximum{};
    bool Json(char* line,std::size_t size)const{
        if(!line||!size)return false;
        int n=std::snprintf(line,size,"{\"type\":\"vr_gpu_performance\",\"context\":%d,\"render_samples\":%llu,\"sample_period_frames\":%u,\"discarded\":%llu,\"skipped_busy\":%llu,\"left_gpu_mean_ms\":%.4f,\"right_gpu_mean_ms\":%.4f,\"left_gpu_max_ms\":%.4f,\"right_gpu_max_ms\":%.4f,\"readback_waits\":0,\"query_flushes\":0}",
            context,static_cast<unsigned long long>(samples),samplePeriod,static_cast<unsigned long long>(discarded),static_cast<unsigned long long>(skipped),
            samples?sum[0]/double(samples):0,samples?sum[1]/double(samples):0,maximum[0],maximum[1]);
        return n>=0&&std::size_t(n)<size;
    }
};
// Sparse GPU timestamps. Pending query slots are metadata, not queued images.
// Poll each outstanding slot once per render with flags0; never wait/flush.
// Full ring or unsupported queries skip instrumentation, never game rendering.
class AsyncGpuTiming {
    using Query=Microsoft::WRL::ComPtr<IDirect3DQuery9>;
    struct Slot {Query disjoint,frequency;std::array<Query,4> timestamp;bool pending=false,complete=false;int context=0;unsigned mask=0;};
    std::array<Slot,4> slots;int active=-1,context=-1;bool enabled=false;unsigned period=16;uint64_t frames=0,window=0;
    GpuReport totals;
    bool Issue(IDirect3DQuery9* query,DWORD flags){if(SUCCEEDED(query->Issue(flags)))return true;enabled=false;active=-1;return false;}
    void Finish(bool complete){
        if(active<0)return;auto& s=slots[unsigned(active)];
        // Complete even a cancelled partial sample so slots can be reused.
        for(unsigned i=0;i<4;++i)if(!(s.mask&(1u<<i))&&!Issue(s.timestamp[i].Get(),D3DISSUE_END))return;
        if(!Issue(s.frequency.Get(),D3DISSUE_END)||!Issue(s.disjoint.Get(),D3DISSUE_END))return;
        s.complete=complete;s.pending=true;active=-1;
    }
public:
    bool Initialize(IDirect3DDevice9* device,unsigned samplePeriod=16){
        Reset();if(!device)return false;period=(std::max)(1u,samplePeriod);
        for(auto& s:slots){
            if(FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT,&s.disjoint))||FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ,&s.frequency))){Reset();return false;}
            for(auto& q:s.timestamp)if(FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&q))){Reset();return false;}
        }enabled=true;return true;
    }
    bool Enabled()const{return enabled;}
    void Reset(){for(auto& s:slots)s=Slot{};active=-1;context=-1;enabled=false;frames=window=0;totals={};}
    void Cancel(){Finish(false);}
    void Poll(){
        if(!enabled)return;
        for(auto& s:slots)if(s.pending){
            BOOL disjoint=TRUE;uint64_t frequency=0;std::array<uint64_t,4> stamps{};bool ready=true;
            HRESULT result=s.disjoint->GetData(&disjoint,sizeof(disjoint),0);
            if(FAILED(result)){enabled=false;return;}if(result!=S_OK)continue;
            result=s.frequency->GetData(&frequency,sizeof(frequency),0);
            if(FAILED(result)){enabled=false;return;}ready=result==S_OK;
            for(unsigned i=0;i<4&&ready;++i){result=s.timestamp[i]->GetData(&stamps[i],sizeof(stamps[i]),0);if(FAILED(result)){enabled=false;return;}ready=result==S_OK;}
            if(!ready)continue;s.pending=false;
            std::array<double,2> ms{};if(s.context!=context)continue;
            if(!s.complete||!GpuDurations(frequency,disjoint!=FALSE,stamps,ms)){++totals.discarded;continue;}
            ++totals.samples;for(unsigned i=0;i<2;++i){totals.sum[i]+=ms[i];totals.maximum[i]=(std::max)(totals.maximum[i],ms[i]);}
        }
    }
    void BeginEye(unsigned eye,int nextContext){
        if(!enabled||eye>1)return;
        if(!eye){
            Cancel();if(nextContext!=context){context=nextContext;totals={};totals.context=context;totals.samplePeriod=period;window=Now();}
            Poll();if(!enabled||(frames++%period)!=0)return;
            for(unsigned i=0;i<slots.size();++i)if(!slots[i].pending){active=int(i);break;}
            if(active<0){++totals.skipped;return;}
            auto& s=slots[unsigned(active)];s.mask=0;s.context=context;
            if(!Issue(s.disjoint.Get(),D3DISSUE_BEGIN))return;
        }
        if(active<0)return;auto& s=slots[unsigned(active)];
        if(eye&&s.mask!=3){Finish(false);return;}
        if(Issue(s.timestamp[eye*2].Get(),D3DISSUE_END))s.mask|=1u<<(eye*2);
    }
    void EndEye(unsigned eye){
        if(!enabled||active<0||eye>1)return;auto& s=slots[unsigned(active)];
        if(Issue(s.timestamp[eye*2+1].Get(),D3DISSUE_END))s.mask|=1u<<(eye*2+1);
        if(eye)Finish(s.mask==15);
    }
    bool Report(uint64_t now,GpuReport& out){
        if(!window){window=now;return false;}
        if(now<window||now-window<Frequency()*2)return false;
        out=totals;totals={};totals.context=context;totals.samplePeriod=period;window=now;return out.samples||out.discarded||out.skipped;
    }
    GpuReport Current()const{return totals;}
};
}
