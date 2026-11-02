#pragma once
#include <windows.h>
#include <openxr/openxr.h>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <algorithm>
namespace diagnostic::meta_bridge {
struct WaitedFrame {
 XrResult result=XR_ERROR_RUNTIME_FAILURE;XrFrameState state{XR_TYPE_FRAME_STATE};double startedMs=0,finishedMs=0;
};
inline bool ExpiredWait(WaitedFrame const& f,double now,bool converted,double display){return XR_SUCCEEDED(f.result)&&(converted?display<now:now-f.finishedMs>std::max(25.,double(f.state.predictedDisplayPeriod)/5e5));}
// One native xrWaitFrame caller, at most one prefetched result. Only timing is
// prefetched; poses, input and swapchain images are obtained by the render loop.
class FrameWaiter {
 std::thread worker;std::mutex mutex;std::condition_variable wake;
 bool pending=false,requested=false,ready=false,stopping=false;WaitedFrame result;
 std::function<WaitedFrame()> wait;HANDLE exited=nullptr;
public:
 FrameWaiter()=default;FrameWaiter(FrameWaiter const&)=delete;
 ~FrameWaiter(){Stop(INFINITE);}
 bool Started()const{return worker.joinable();}
 void Start(std::function<WaitedFrame()> call){
  if(Started())throw std::runtime_error("Meta frame waiter already running");
  wait=std::move(call);pending=requested=ready=stopping=false;
  exited=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!exited)throw std::runtime_error("Cannot create Meta frame waiter completion event");
  try{worker=std::thread([this]{
   for(;;){std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&]{return stopping||requested;});if(stopping)break;requested=false;lock.unlock();
    WaitedFrame next;try{next=wait();}catch(...){next.result=XR_ERROR_RUNTIME_FAILURE;}
    lock.lock();result=next;ready=true;wake.notify_all();
   }
   SetEvent(exited);
  });}catch(...){CloseHandle(exited);exited=nullptr;throw;}
 }
 void Schedule(){std::lock_guard<std::mutex> lock(mutex);if(!Started()||stopping)throw std::runtime_error("Meta frame waiter stopped");if(!pending){pending=requested=true;ready=false;wake.notify_all();}}
 WaitedFrame Take(){Schedule();std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&]{return ready||stopping;});if(!ready)throw std::runtime_error("Meta frame wait cancelled");auto value=result;pending=ready=false;wake.notify_all();return value;}
 bool Stop(DWORD timeout){
  if(!Started())return true;
  {std::lock_guard<std::mutex> lock(mutex);stopping=true;wake.notify_all();}
  if(WaitForSingleObject(exited,timeout)!=WAIT_OBJECT_0)return false;
  worker.join();CloseHandle(exited);exited=nullptr;wait={};return true;
 }
};
}
