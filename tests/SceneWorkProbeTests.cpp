#include "SceneWorkProbe.h"
#include <cstring>
#include <cstdio>
using namespace diagnostic::performance;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    SceneWorkSnapshot before{1000000,10000,2,true,true},after{1200000,26000,5,true,true};
    auto sample=SceneWorkSample::Between(897,1000,before,after);
    CHECK(sample.wallMs==897&&sample.cpuValid&&sample.cpuMs==20&&sample.ioValid&&sample.readBytes==16000&&sample.readOperations==3);
    char line[1536];CHECK(sample.Json(line,sizeof(line),"pvs_prefetch_process_inclusive",1,0,6092,123456,1000,true,false,12,0,4,0));
    CHECK(strstr(line,"\"thread_cpu_ms\":20.0000")&&strstr(line,"\"process_read_bytes\":16000")&&strstr(line,"\"native_threading_after\":false"));
    char tiny[8];CHECK(!sample.Json(tiny,sizeof(tiny),"scene",1,0,1,1,1000,true,true,0,0,0,0));
    after.cpu100ns=0;after.readOperations=0;sample=SceneWorkSample::Between(100,1000,before,after);CHECK(!sample.cpuValid&&!sample.ioValid&&sample.cpuMs==0&&sample.readBytes==0);
    after={};sample=SceneWorkSample::Between(100,1000,before,after);CHECK(!sample.cpuValid&&!sample.ioValid);
    sample=SceneWorkSample::Between(100,0,before,before);CHECK(sample.wallMs==0&&!sample.cpuValid);
    auto start=Now();auto first=CaptureSceneWork();unsigned valid=0;
    for(unsigned i=0;i<10000;++i){auto current=CaptureSceneWork();if(current.cpuValid&&current.ioValid)++valid;}
    auto last=CaptureSceneWork();CHECK(valid==10000&&first.cpuValid&&last.cpuValid&&last.cpu100ns>=first.cpu100ns);
    printf("PASS long-scene CPU vs wait evidence, process-wide read counters, invalid/reset handling and bounded JSON; 10000 snapshot pairs of APIs %.3f ms (not game FPS)\n",double(Now()-start)*1000/Frequency());return 0;
}
