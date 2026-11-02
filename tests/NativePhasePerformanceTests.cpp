#include "NativePhasePerformance.h"
#include "NativeSceneExecutionProfile.h"
#include "DeferredProfileLines.h"
#include <cstdio>
#include <cstring>
using namespace diagnostic::performance;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    NativePhaseCollector collector(1000);NativePhaseReport report;
    CHECK(!collector.Add(NativePhase::Focus,3,1000,1,report));
    CHECK(!collector.Add(NativePhase::SessionProcess,10,1100,1,report));
    CHECK(!collector.Add(NativePhase::Focus,5,1200,1,report));
    CHECK(collector.Add(NativePhase::SessionProcess,20,3000,1,report));
    CHECK(report.calls[0]==2&&report.meanMs[0]==15&&report.maxMs[0]==20&&report.calls[1]==2&&report.meanMs[1]==4&&report.maxMs[1]==5);
    CHECK(!collector.Add(NativePhase::Focus,9999,3100,1,report));
    CHECK(!collector.Add(NativePhase::SessionProcess,4,3200,2,report));
    CHECK(collector.Add(NativePhase::SessionProcess,8,5200,2,report)&&report.context==2&&report.calls[1]==0&&report.meanMs[0]==6);
    CHECK(!collector.Add(NativePhase::Focus,2,10,2,report));
    CHECK(collector.Add(NativePhase::Focus,4,2010,2,report)&&report.calls[0]==0&&report.meanMs[1]==3);
    CHECK(!collector.Add(NativePhase::ProjectileShoot,7,3000,1,report));
    CHECK(collector.Add(NativePhase::ProjectileShoot,11,5000,1,report)&&report.meanMs[unsigned(NativePhase::ProjectileShoot)]==9&&report.calls[unsigned(NativePhase::ProjectileShoot)]==2);
    char line[DeferredProfileLines::LineCapacity];CHECK(report.Json(line,sizeof(line))&&std::strstr(line,"not the entire engine loop")&&std::strstr(line,"native_projectile_shoot\":9.0000"));char tiny[3];CHECK(!report.Json(tiny,sizeof(tiny)));
    CHECK(!collector.Add(NativePhase::FrustumRefresh,2,5100,1,report));
    CHECK(!collector.Add(NativePhase::PvsProcess,230,5200,1,report));
    CHECK(!collector.Add(NativePhase::PvsExecute,229,5300,1,report));
    CHECK(collector.Add(NativePhase::PvsWait,200,7000,1,report));
    CHECK(report.calls[unsigned(NativePhase::FrustumRefresh)]==1&&report.maxMs[unsigned(NativePhase::PvsProcess)]==230&&report.maxMs[unsigned(NativePhase::PvsExecute)]==229&&report.maxMs[unsigned(NativePhase::PvsWait)]==200);
    CHECK(report.Json(line,sizeof(line))&&std::strstr(line,"pvs_prefetch_wait_inclusive\":200.0000"));
    // A slow child and its inclusive parent both appear; never subtract a
    // separate window's maximum or treat these nested scopes as additive.
    for(unsigned i=0;i<NativePhaseCount;++i){report.calls[i]=9999999;report.meanMs[i]=12345.6789;report.maxMs[i]=123456.789;}
    CHECK(report.Json(line,sizeof(line)));
    DeferredProfileLines pending;CHECK(pending.Push(line));bool delivered=false;
    pending.Drain([&](char const* text){delivered=!strcmp(text,line)&&strstr(text,"ai_script_routine_inclusive")&&strstr(text,"physics_simulate_inclusive");});CHECK(delivered&&!pending.TakeDropped());
    CHECK(unsigned(NativePhase::SampledDecals)==38&&unsigned(NativePhase::SampledInstanceItem)==39);
    CHECK(unsigned(NativePhase::Navigation)==29); // All historical IDs remain stable.
    CHECK(unsigned(NativePhase::SampledShadowMap)==40&&unsigned(NativePhase::SampledSolidEnd)==48);
    CHECK(unsigned(NativePhase::SampledLowPolyRenderer)==49&&unsigned(NativePhase::SampledNonLowPolyRenderer)==50);
    CHECK(ExecutionEntryCount==17&&ExecutionSamplePeriod==32);
    unsigned char adjusted[16]{};
    for(auto const& entry:ExecutionEntries){
        CHECK(ExecutionPrefixAtBase(entry,ExecutionPreferredBase,adjusted)&&!memcmp(adjusted,entry.prefix,entry.bytes));
        CHECK(ExecutionPrefixAtBase(entry,0x028c0000,adjusted));
        if(entry.phase==NativePhase::SampledInstanceBatches){
            unsigned char captured[]={0x51,0x56,0x57,0x8b,0x3d,0xb8,0xa5,0xec,0x02};
            CHECK(!memcmp(adjusted,captured,sizeof(captured)));
        }else CHECK(!memcmp(adjusted,entry.prefix,entry.bytes));
    }
    CHECK(!ExecutionPrefixAtBase(ExecutionEntries[1],0,adjusted));
    CHECK(!ExecutionPrefixAtBase(ExecutionEntries[1],0xfff00000,adjusted));
    auto invalidBatch=ExecutionEntries[1];invalidBatch.prefix[5]^=1;
    CHECK(!ExecutionPrefixAtBase(invalidBatch,0x028c0000,adjusted));
    unsigned admitted=0;
    for(uint64_t epoch=1;epoch<=1024;++epoch){
        if(SampleSceneExecution(epoch,1,false))++admitted;
        CHECK(!SampleSceneExecution(epoch,0,false)&&!SampleSceneExecution(epoch,2,false)&&!SampleSceneExecution(epoch,1,true));
    }
    CHECK(admitted==32&&!SampleSceneExecution(0,1,false));
    for(auto const& entry:ExecutionEntries)CHECK(unsigned(entry.phase)>=30&&unsigned(entry.phase)<NativePhaseCount);
    NativePhaseCollector sampled(1000);
    CHECK(!sampled.Add(NativePhase::SampledRenderOperation,100,100,1,report));
    CHECK(!sampled.Add(NativePhase::SampledMaterialBegin,20,200,1,report));
    CHECK(sampled.Add(NativePhase::SampledRenderOperation,300,2100,1,report));
    CHECK(report.calls[unsigned(NativePhase::SampledRenderOperation)]==2&&report.meanMs[unsigned(NativePhase::SampledRenderOperation)]==200);
    CHECK(report.calls[unsigned(NativePhase::SampledMaterialBegin)]==1&&report.meanMs[unsigned(NativePhase::SampledMaterialBegin)]==20);
    CHECK(report.Json(line,sizeof(line))&&strstr(line,"sampled_render_operation_inclusive")&&strstr(line,"one of 32"));
    puts("PASS native session/focus collection independent of callback windows, nested phases, context/clock resets and bounded output");return 0;
}
