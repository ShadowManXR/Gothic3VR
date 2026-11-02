#include "AsyncGpuTiming.h"
#include <cstring>
#include <cstdio>
using namespace diagnostic::performance;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    std::array<double,2> ms{};CHECK(GpuDurations(1000000,false,{1000,3000,4000,9000},ms)&&ms[0]==2&&ms[1]==5);
    CHECK(!GpuDurations(0,false,{0,1,2,3},ms)&&!GpuDurations(1000,true,{0,1,2,3},ms));
    CHECK(!GpuDurations(1000,false,{5,4,6,7},ms)&&!GpuDurations(1000,false,{1,4,3,7},ms)&&!GpuDurations(1000,false,{1,2,4,3},ms));
    uint64_t high=0xffffffffffff0000ull;CHECK(GpuDurations(1000,false,{high,high+2,high+3,high+9},ms)&&ms[0]==2&&ms[1]==6);
    AsyncGpuTiming timer;CHECK(!timer.Initialize(nullptr)&&!timer.Enabled());timer.BeginEye(0,1);timer.EndEye(1);timer.Cancel();timer.Poll();timer.Reset();
    GpuReport r;r.context=1;r.samples=2;r.sum={8,10};r.maximum={5,7};char line[1024];CHECK(r.Json(line,sizeof(line))&&std::strstr(line,"left_gpu_mean_ms\":4.0000")&&std::strstr(line,"query_flushes\":0"));
    char tiny[3];CHECK(!r.Json(tiny,sizeof(tiny)));puts("PASS GPU timestamp frequency/disjoint/order/precision validation; unavailable instrumentation remains inert");return 0;
}
