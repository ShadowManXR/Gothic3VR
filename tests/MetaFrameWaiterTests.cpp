#include "meta_bridge/FrameWaiter.h"
#include <atomic>
#include <iostream>
using namespace diagnostic::meta_bridge;
void Check(bool b,char const* why){if(!b)throw std::runtime_error(why);}
int main(){try{
 FrameWaiter w;std::atomic<int> calls{0};HANDLE entered=CreateEventW(nullptr,TRUE,FALSE,nullptr),release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 Check(entered&&release,"events unavailable");
 w.Start([&]{++calls;SetEvent(entered);WaitForSingleObject(release,INFINITE);WaitedFrame f;f.result=XR_SUCCESS;f.state.predictedDisplayTime=1234567890123;return f;});
 w.Schedule();Check(WaitForSingleObject(entered,2000)==WAIT_OBJECT_0,"prefetch did not start independently");w.Schedule();Check(calls==1,"concurrent duplicate native waits");
 SetEvent(release);auto f=w.Take();Check(f.result==XR_SUCCESS&&f.state.predictedDisplayTime==1234567890123,"prefetched frame changed");Check(w.Stop(2000),"normal stop failed");
 ResetEvent(entered);ResetEvent(release);
 w.Start([&]{++calls;SetEvent(entered);WaitForSingleObject(release,INFINITE);return WaitedFrame{};});w.Schedule();Check(WaitForSingleObject(entered,2000)==WAIT_OBJECT_0,"restarted wait did not begin");
 Check(!w.Stop(0),"busy runtime wait was not detected");SetEvent(release);Check(w.Stop(2000),"pending runtime wait did not drain");
 w.Start([&]{return WaitedFrame{};});Check(w.Stop(2000),"idle waiter did not stop");
 calls=0;w.Start([&]{WaitedFrame f;f.result=XR_SUCCESS;f.state.predictedDisplayTime=++calls;return f;});
 for(int n=1;n<=1000;++n){w.Schedule();auto next=w.Take();Check(next.state.predictedDisplayTime==n,"lost wakeup or reused prefetched frame");}
 Check(w.Stop(2000)&&calls==1000,"native wait count mismatch");CloseHandle(entered);CloseHandle(release);
 WaitedFrame cached;cached.result=XR_SUCCESS;cached.finishedMs=100;cached.state.predictedDisplayPeriod=12500000;
 Check(!ExpiredWait(cached,110,true,150)&&ExpiredWait(cached,160,true,150),"clock-based stale detection failed");
 Check(!ExpiredWait(cached,110,false,0)&&ExpiredWait(cached,130,false,0),"fallback stale detection failed");
 cached.result=XR_ERROR_SESSION_NOT_RUNNING;Check(!ExpiredWait(cached,1000,false,0),"runtime error treated as a valid cached frame");
 std::cout<<"PASS Meta frame prefetch: one wait, overlap, original timestamp, bounded drain and restart\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
