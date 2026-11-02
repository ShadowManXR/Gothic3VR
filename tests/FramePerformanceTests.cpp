#include "FramePerformance.h"
#include <cmath>
#include <cstring>
#define CHECK(x) do{if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
using namespace diagnostic::performance;
bool Near(double a,double b){return std::abs(a-b)<.001;}
int main(){
    Collector p(1000);Report r;bool emitted=false;
    p.Begin();p.Ready(500);p.Add(Stage::LeftRender,99999);p.Pause(); // standby/cancel must not pollute results
    for(unsigned frame=0;frame<3;++frame){
        p.Begin();p.Add(Stage::XrWait,50);p.Ready(1000+frame*1000);
        p.Add(Stage::LeftRender,20*(frame+1));p.Add(Stage::RightRender,10);p.Add(Stage::RightRender,5);
        p.Count(Counter::Copies,3);p.Count(Counter::D3D11Flushes);
        emitted=p.Finish(1010+frame*1000,r);CHECK(emitted==(frame==2));
    }
    CHECK(r.frames==3&&r.intervals==2&&Near(r.intervalMeanMs,1000)&&Near(r.intervalP95Ms,1000));
    CHECK(Near(r.meanMs[unsigned(Stage::LeftRender)],40)&&Near(r.maxMs[unsigned(Stage::LeftRender)],60));
    CHECK(Near(r.meanMs[unsigned(Stage::RightRender)],15)&&Near(r.meanMs[unsigned(Stage::Work)],10));
    CHECK(Near(r.meanMs[unsigned(Stage::XrWait)],50)&&r.counters[unsigned(Counter::Copies)]==9&&r.counters[unsigned(Counter::D3D11Flushes)]==3);
    char json[4096];CHECK(r.Json(json,sizeof(json),true)&&std::strstr(json,"\"batched_copies\":true")&&std::strstr(json,"\"left_render\":40.000"));
    char tiny[8];CHECK(!r.Json(tiny,sizeof(tiny),true));CHECK(!r.Json(nullptr,0,true));
    p.Pause();p.Begin();p.Ready(50000);CHECK(!p.Finish(50001,r));
    p.Begin();p.Ready(52000);CHECK(p.Finish(52001,r)&&r.frames==2&&r.intervals==1&&Near(r.intervalMeanMs,2000));
    p.Pause();uint64_t clock=1000;bool bounded=false;
    for(unsigned i=0;i<700&&!bounded;++i){p.Begin();clock+=i==1?40:4;p.Ready(clock);bounded=p.Finish(clock+1,r);}
    CHECK(bounded&&r.intervals>256&&Near(r.intervalP95Ms,4)&&Near(r.intervalMaxMs,40));
    // Fixed-size reporting and collection remain inexpensive; this measures
    // the CPU collector only and makes no claim about headset FPS.
    Collector benchmark;auto start=Now();unsigned reports=0;
    for(unsigned i=0;i<100000;++i){benchmark.Begin();auto now=Now();benchmark.Ready(now);benchmark.Add(Stage::LeftRender,3);benchmark.Count(Counter::Copies,3);reports+=benchmark.Finish(Now(),r);}
    double elapsed=double(Now()-start)*1000/double(Frequency());
    std::printf("PASS frame mean/max/p95, separate XR pacing, canceled/standby resets, bounded storage and JSON; 100000 CPU collections %.3f ms (%u reports)\n",elapsed,reports);return 0;
}
