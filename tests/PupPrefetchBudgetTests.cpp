#include "PupPrefetchBudget.h"
#include <windows.h>
#include <cstdio>
#include <thread>
using namespace diagnostic::pup;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    PrefetchBudget b(1000000);
    CHECK(b.Admit(10,10000)&&b.RemainingMs()==2);
    CHECK(b.Start(10000,100)==97&&!b.Admit(10,10001));
    b.Finish(11000);CHECK(b.RemainingMs()==1&&b.Admit(10,11000));
    CHECK(b.Start(11000,101)==97);b.Finish(12000);
    CHECK(!b.Admit(10,12001)); // right eye/shadow must not replenish the allowance
    CHECK(b.Admit(11,13000)&&b.RemainingMs()==2);
    CHECK(b.Start(13000,1)==uint32_t(-2));b.Finish(440000);
    CHECK(!b.Admit(11,440001)); // a long indivisible item is retained, not retried/interrupted
    CHECK(b.Admit(12,441000));b.Start(441000,200);b.Finish(440000);
    CHECK(b.Admit(12,440100)&&b.RemainingMs()==2);b.Reset();
    CHECK(b.Admit(12,500000)&&b.RemainingMs()==2);
    PrefetchBudget unavailable(0);CHECK(!unavailable.Admit(1,1));
    // A completed left image and its right partner must not see different
    // publications merely because optional budget remains in the same pair.
    PrefetchBudget pair(1000000);unsigned pending=3,published=0;
    auto service=[&](uint64_t epoch,uint64_t time,bool right){if(pair.Admit(epoch,time,right)&&pending){pair.Start(time,100);--pending;++published;pair.Finish(time+200);}};
    service(1,1000,false);auto leftScene=published;
    service(1,1300,true);service(1,1500,true);CHECK(published==leftScene&&pending==2&&pair.RemainingMs()==1);
    service(2,2000,false);CHECK(published==leftScene+1&&pending==1);
    service(2,2300,true);CHECK(pending==1);
    service(3,3000,false);CHECK(published==3&&pending==0);
    // A right-only first callback cannot consume/reset the next pair's quota.
    pair.Reset();CHECK(!pair.Admit(4,4000,true)&&pair.Admit(4,4100,false)&&pair.RemainingMs()==2);
    // Exercise the actual Windows recursive lock semantics used by the SDK.
    // A busy worker must leave optional work untouched, with no waiting.
    CRITICAL_SECTION cs;InitializeCriticalSection(&cs);HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    CHECK(ready&&release);int queueItems=7;bool workerOk=false;
    std::thread worker([&]{EnterCriticalSection(&cs);SetEvent(ready);workerOk=WaitForSingleObject(release,2000)==WAIT_OBJECT_0;LeaveCriticalSection(&cs);});
    bool signaled=WaitForSingleObject(ready,2000)==WAIT_OBJECT_0;
    bool acquired=TryEnterCriticalSection(&cs)!=0;if(acquired)LeaveCriticalSection(&cs);
    SetEvent(release);worker.join();CHECK(signaled&&workerOk&&!acquired&&queueItems==7);
    CHECK(TryEnterCriticalSection(&cs));EnterCriticalSection(&cs);--queueItems;LeaveCriticalSection(&cs);LeaveCriticalSection(&cs);
    bool freeOnOtherThread=false;std::thread observer([&]{freeOnOtherThread=TryEnterCriticalSection(&cs)!=0;if(freeOnOtherThread)LeaveCriticalSection(&cs);});observer.join();
    CHECK(freeOnOtherThread&&queueItems==6);CloseHandle(ready);CloseHandle(release);DeleteCriticalSection(&cs);
    puts("PASS paired-eye optional publication consistency, queue completion on later frames, shared stereo frame budget, native timer preservation/wrap, complete-item overruns, resets and nonblocking recursive queue locking");return 0;
}
