#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
namespace diagnostic::meta_bridge {
// One-shot, bounded CPU timestamps only. No GPU queries or pixel readback.
class FrameTiming {
 struct Values {
  std::array<double,256> samples{};size_t count=0;double sum=0,maximum=0;
  void Add(double ms){if(!std::isfinite(ms)||ms<0)return;if(count<samples.size())samples[count++]=ms;sum+=ms;maximum=std::max(maximum,ms);}
  double Mean()const{return count?sum/count:0;}
  double P95()const{if(!count)return 0;auto sorted=samples;std::sort(sorted.begin(),sorted.begin()+count);return sorted[(count*95+99)/100-1];}
 } wait,application,acquire,copy,end,interval,period,waitLead,submitLead,nativeWait,hiddenWait,beforeWait;
 bool enabled=false,frameValid=false,clockValid=false;double started=0,windowStart=0,waitReturned=0,frameWait=0,frameAcquire=0,displayAt=0,previousSubmit=0;
 int64_t predicted=0,previousPredicted=0,framePeriod=0;uint64_t frames=0,afterDisplay=0,skippedSlots=0;
public:
 void Enable(double now){enabled=true;started=now;}
 bool Enabled()const{return enabled;}
 void Waited(double before,double after,int64_t prediction,int64_t displayPeriod,bool converted,double displayMs){
  if(!enabled)return;if(after-started>120000){enabled=false;return;}
  frameValid=true;frameWait=std::max(0.,after-before);waitReturned=after;frameAcquire=0;predicted=prediction;framePeriod=displayPeriod;clockValid=converted;displayAt=displayMs;
  if(previousSubmit>0)beforeWait.Add(std::max(0.,before-previousSubmit));
 }
 void Prefetched(double started,double finished,double requested){if(!enabled)return;nativeWait.Add(std::max(0.,finished-started));hiddenWait.Add(std::max(0.,std::min(finished,requested)-started));}
 void Acquired(double ms){if(enabled&&frameValid)frameAcquire+=std::max(0.,ms);}
 void Submitted(double received,double copied,double returned,bool stereo,bool success){
  if(!enabled||!frameValid)return;frameValid=false;
  if(!stereo||!success){previousSubmit=0;previousPredicted=0;return;}
  if(!frames)windowStart=returned;
  ++frames;wait.Add(frameWait);application.Add(received-waitReturned);acquire.Add(frameAcquire);copy.Add(copied-received);end.Add(returned-copied);
  if(framePeriod>0)period.Add(double(framePeriod)/1e6);
  if(previousSubmit>0)interval.Add(returned-previousSubmit);
  if(previousPredicted>0&&predicted>previousPredicted&&framePeriod>0){auto slots=(predicted-previousPredicted+framePeriod/2)/framePeriod;if(slots>1)skippedSlots+=uint64_t(slots-1);}
  if(clockValid){waitLead.Add(std::max(0.,displayAt-waitReturned));submitLead.Add(std::max(0.,displayAt-copied));if(copied>displayAt)++afterDisplay;}
  previousSubmit=returned;previousPredicted=predicted;
 }
 bool ReportDue(double now)const{return frames&&(frames>=256||now-windowStart>=2000||!enabled);}
 uint64_t Count()const{return frames;}
 std::string Report(){
  if(!frames)return {};
  std::ostringstream s;s<<std::fixed<<std::setprecision(3);
  s<<"{\"type\":\"meta_frame_timing\",\"frames\":"<<frames
   <<",\"app_submit_fps\":"<<(interval.Mean()>0?1000./interval.Mean():0)
   <<",\"runtime_period_ms\":"<<period.Mean()
   <<",\"interval_ms_mean\":"<<interval.Mean()<<",\"interval_ms_p95\":"<<interval.P95()<<",\"interval_ms_max\":"<<interval.maximum
   <<",\"wait_frame_ms_mean\":"<<wait.Mean()<<",\"wait_frame_ms_p95\":"<<wait.P95()
   <<",\"native_wait_ms_mean\":"<<nativeWait.Mean()<<",\"overlapped_wait_ms_mean\":"<<hiddenWait.Mean()<<",\"before_wait_ms_mean\":"<<beforeWait.Mean()
   <<",\"application_and_ipc_ms_mean\":"<<application.Mean()<<",\"application_and_ipc_ms_p95\":"<<application.P95()
   <<",\"swapchain_acquire_wait_ms_mean\":"<<acquire.Mean()<<",\"swapchain_acquire_wait_ms_p95\":"<<acquire.P95()
   <<",\"copy_sync_ms_mean\":"<<copy.Mean()<<",\"copy_sync_ms_p95\":"<<copy.P95()
   <<",\"end_frame_ms_mean\":"<<end.Mean()<<",\"end_frame_ms_p95\":"<<end.P95()
   <<",\"clock_conversion\":"<<(clockValid?"true":"false")
   <<",\"nonnegative_lead_at_wait_ms_mean\":"<<waitLead.Mean()
   <<",\"nonnegative_lead_before_submit_ms_mean\":"<<submitLead.Mean()
   <<",\"submitted_after_predicted_display\":"<<afterDisplay
   <<",\"skipped_prediction_slots\":"<<skippedSlots
   <<",\"headset_display_fps_measured\":false,\"cpu_pixel_readback\":false}";
  wait={};application={};acquire={};copy={};end={};interval={};period={};waitLead={};submitLead={};nativeWait={};hiddenWait={};beforeWait={};frames=afterDisplay=skippedSlots=0;
  return s.str();
 }
};
}
