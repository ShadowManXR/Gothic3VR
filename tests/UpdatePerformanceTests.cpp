#include "UpdatePerformance.h"
#include <cstring>
#include <cstdio>
using namespace diagnostic::performance;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    UpdateCollector collector(1000);UpdateReport report;bool emitted=false;
    for(unsigned i=0;i<3;++i){collector.Begin(1,1000+i*1000);collector.Add(UpdateStage::Native,10+i*10);collector.Add(UpdateStage::Finisher,1);collector.Add(UpdateStage::Finisher,2);
        emitted=collector.End(1010+i*1000,report);CHECK(emitted==(i==2));}
    CHECK(report.updates==3&&report.context==1&&report.meanMs[0]==20&&report.maxMs[0]==30);
    CHECK(report.meanMs[unsigned(UpdateStage::Finisher)]==3&&report.calls[unsigned(UpdateStage::Finisher)]==6);
    char out[2048];CHECK(report.Json(out,sizeof(out))&&std::strstr(out,"application_process_callback")&&std::strstr(out,"not the native simulation")&&std::strstr(out,"stages overlap"));
    char tiny[5];CHECK(!report.Json(tiny,sizeof(tiny))&&!report.Json(nullptr,0));
    collector.Begin(1,4000);collector.Add(UpdateStage::Native,99999);collector.Begin(2,5000);CHECK(!collector.End(5010,report));
    collector.Begin(2,7000);collector.Add(UpdateStage::Native,4);CHECK(collector.End(7010,report)&&report.updates==2&&report.context==2&&report.maxMs[0]==4);
    collector.Begin(2,100);CHECK(!collector.End(110,report));collector.Reset();
    collector.Begin(1,1000);collector.Add(UpdateStage::Native,99999);
    collector.Begin(1,1100);collector.Add(UpdateStage::Native,3);CHECK(!collector.End(1110,report));
    collector.Begin(1,3000);CHECK(collector.End(3010,report));
    CHECK(report.updates==2&&report.calls[0]==1&&report.maxMs[0]==3&&report.meanMs[0]==1.5);
    collector.Reset();
    auto began=Now();for(unsigned i=0;i<100000;++i){auto t=Now();collector.Begin(1,t);collector.Add(UpdateStage::Native,2);collector.End(t,report);}
    printf("PASS CPU update context/lifecycle separation and nested stages;100000 fixed-storage samples %.3f ms\n",double(Now()-began)*1000/double(Frequency()));return 0;
}
