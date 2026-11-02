#include "NativeMovementTrace.h"
#include <cstdio>
using diagnostic::NativeMovementTraceWindow;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    NativeMovementTraceWindow trace;
    CHECK(!trace.Armed()&&!trace.Select(1,1000)&&!trace.Selected(1));
    trace.Arm();CHECK(trace.Armed()&&!trace.Selected(0));
    // Menu/loading time doesn't consume the gameplay window before Select.
    CHECK(trace.Select(123,500000)&&trace.Selected(123));
    for(int phase=0;phase<4;++phase)CHECK(trace.Selected(123));
    CHECK(trace.Select(123,500001)); // additional phase doesn't consume quota
    CHECK(!trace.Select(124,500249)&&!trace.Selected(124));
    CHECK(trace.Select(125,500250)&&!trace.Selected(123)&&trace.Selected(125));
    CHECK(!trace.Select(126,620000)&&!trace.Armed()&&!trace.Selected(125));
    // Complete 120s/480-update bound, including a four-phase consumer.
    trace.Arm();
    for(unsigned frame=0;frame<480;++frame){
        auto now=1000+uint64_t(frame)*250;
        CHECK(trace.Select(frame,now));
        for(int phase=0;phase<4;++phase)CHECK(trace.Selected(frame));
        CHECK(!trace.Select(frame+1000,now+1));
    }
    CHECK(!trace.Select(480,121000)&&!trace.Armed());
    // Clock rollback and explicit reset cannot leak observations to later play.
    trace.Arm();CHECK(trace.Select(1,1000));
    CHECK(!trace.Select(2,999)&&!trace.Armed());
    trace.Arm();CHECK(trace.Select(3,9000));trace.Reset();
    CHECK(!trace.Armed()&&!trace.Selected(3)&&!trace.Select(4,9250));
    puts("PASS one-shot native movement observation: disabled by default, four phases, 4Hz, 120s/480 sample bound and clock/reset ownership");
}
