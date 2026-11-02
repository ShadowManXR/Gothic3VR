#include "EyeDrawTiming.h"
#include "DeferredProfileLines.h"
#include <cstdio>
#include <cstring>
using namespace diagnostic::performance;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    DeferredProfileLines pending;unsigned writes=0;for(unsigned i=0;i<16;++i)CHECK(pending.Push("event"));
    CHECK(!pending.Push("overflow")&&pending.TakeDropped()==1&&!pending.TakeDropped());
    pending.Drain([&](char const* line){if(!strcmp(line,"event"))++writes;});CHECK(writes==16);
    pending.Drain([&](char const*){++writes;});CHECK(writes==16);
    CHECK(pending.Push("aborted"));try{pending.Drain([](char const*){throw 1;});}catch(int){}
    pending.Drain([&](char const*){++writes;});CHECK(writes==16);
    EyeDrawTiming timer;EyeDrawSample sample;
    CHECK(!timer.BeginCall(0,1).tracked&&!timer.Finish(2,sample));
    timer.Begin(100);auto a=timer.BeginCall(0,110);auto b=timer.BeginCall(1,120);
    timer.EndCall(b,130);timer.EndCall(a,140);auto c=timer.BeginCall(3,150);timer.EndCall(c,170);
    CHECK(timer.Finish(200,sample)&&sample.elapsed==100&&sample.drawTicks==50&&sample.longestDraw==30&&sample.calls[0]==1&&sample.calls[1]==1&&sample.calls[3]==1);
    char json[1024];CHECK(sample.Json(json,sizeof(json),1000,1,1,99,true)&&strstr(json,"outside_draw_api_cpu_ms\":50.0000")&&strstr(json,"is_spike\":true"));
    timer.Begin(210);auto stale=timer.BeginCall(0,220);timer.Cancel();timer.Begin(300);timer.EndCall(stale,500);
    CHECK(timer.Finish(310,sample)&&!sample.drawTicks&&!sample.calls[0]);
    timer.Begin(1000);timer.BeginCall(0,1001);CHECK(!timer.Finish(1010,sample)); // failed native call cancels whole profile
    timer.Begin(2000);CHECK(!timer.Finish(1999,sample));char tiny[3];CHECK(!sample.Json(tiny,sizeof(tiny),1000,0,1,1,false));
    auto start=Now();timer.Begin(start);for(unsigned i=0;i<100000;++i){auto call=timer.BeginCall(i%4,Now());timer.EndCall(call,Now());}CHECK(timer.Finish(Now(),sample));
    printf("PASS original draw timing, nested dispatch, cancel epochs and clock bounds; 100000 instrumented calls %.3f ms including clocks\n",double(Now()-start)*1000/Frequency());return 0;
}
