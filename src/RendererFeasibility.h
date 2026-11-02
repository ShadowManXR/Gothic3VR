#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <ostream>
#include <algorithm>

#pragma pack(push,8)
namespace diagnostic::renderer_trial {
enum class Scope : unsigned {Update,Extract,OcclusionExtract,Execute,RenderSystem,Pvs,Operation,Mesh,TargetMesh,Instances,Shadow,Wait9,Wait11,Count};
inline constexpr unsigned ScopeCount=unsigned(Scope::Count);
inline constexpr char const* ScopeNames[]={"frustum_update","frustum_extract","frustum_occlusion_extract","frustum_execute","render_system","pvs_process","render_operation","mesh_render","target_mesh_render","instance_flush","shadow_render","gpu_wait9","gpu_wait11"};
enum class Phase : unsigned {Waiting,Warmup,A1,B1,A2,B2,A3,Done,Aborted};
inline constexpr char const* PhaseNames[]={"waiting","warmup","native_A1","omit_static_color_B1","native_A2","omit_static_color_B2","native_A3","done","aborted"};
inline constexpr uint32_t RequiredHooks=(1u<<3)|(1u<<5)|(1u<<6)|(1u<<7)|(1u<<10)|(1u<<11);
inline bool EligibleColor(bool active,bool rootDatabase,bool solidPhase,unsigned shadowDepth,unsigned executeDepth)noexcept {
 return active&&rootDatabase&&solidPhase&&!shadowDepth&&executeDepth==1;
}
inline constexpr unsigned Capacity=8192;
struct Sample {
 uint64_t frame=0,work=0,interval=0,xrWait=0,phaseAge=0;
 Phase phase=Phase::Waiting;unsigned eyes=0;
 std::array<uint64_t,ScopeCount> ticks{},calls{};
 std::array<uint64_t,2> targets{},omitted{};
 std::array<float,3> nativePosition{},nativeRotation{},headPosition{};
 std::array<float,4> headQuaternion{};
};
// Controller shared by the two translation units of the opt-in test DLL.
// No world pointers or native resources survive a frame.
class Experiment {
 uint64_t frequency=0,phaseStart=0,ready=0,lastEnd=0,pendingWait=0;
 Phase phase=Phase::Waiting;bool armed=false,active=false,finished=false;
 unsigned hooks=0;char const* reason="not_started";Sample current;
 std::vector<Sample> samples;
 static unsigned Duration(Phase p)noexcept {return p==Phase::Warmup?12000:(p==Phase::B1||p==Phase::B2?4000:8000);}
public:
 bool written=false;
 void SetHooks(unsigned mask)noexcept {hooks=mask;}
 unsigned Hooks()const noexcept{return hooks;}
 void Arm(uint64_t hz,bool enabled){
  if(armed||!enabled||!hz)return;
  frequency=hz;armed=true;reason="waiting_for_world";samples.reserve(Capacity);
 }
 bool Armed()const noexcept{return armed;}
 void RuntimeReady(uint64_t now,uint64_t waited)noexcept {ready=now;pendingWait=waited;}
 void Unavailable()noexcept {
  if(!armed||finished)return;
  if(phase==Phase::Waiting||phase==Phase::Warmup){active=false;phase=Phase::Waiting;phaseStart=lastEnd=0;}
  else Abort("headset_unfocused_or_not_rendering");
 }
 bool Begin(uint64_t now,uint64_t frame,bool eligible){
  active=false;current={};
  if(!armed||finished)return false;
  if((hooks&RequiredHooks)!=RequiredHooks){Abort("required_native_hook_unavailable");return false;}
  if(!eligible){
   if(phase==Phase::Waiting||phase==Phase::Warmup){phase=Phase::Waiting;phaseStart=lastEnd=0;return false;}
   Abort("world_focus_menu_or_player_state_changed");return false;
  }
  if(phase==Phase::Waiting){phase=Phase::Warmup;phaseStart=now;reason="running";}
  if(now<phaseStart){Abort("clock_reversed");return false;}
  if((now-phaseStart)*1000/frequency>=Duration(phase)){
   phase=Phase(unsigned(phase)+1);phaseStart=now;lastEnd=0;
   if(phase==Phase::Done){finished=true;reason="completed_screening_not_renderer_gain";return false;}
  }
  current.frame=frame;current.phase=phase;current.xrWait=pendingWait;current.phaseAge=now-phaseStart;
  current.work=ready&&ready<=now?ready:now;
  active=true;return true;
 }
 void Camera(float const* p,float units,float const* r,float const* hp,float const* hq)noexcept {
  if(!active||units<=0)return;
  for(unsigned i=0;i<3;++i){current.nativePosition[i]=p[i]/units;current.nativeRotation[i]=r[i];current.headPosition[i]=hp[i];}
  for(unsigned i=0;i<4;++i)current.headQuaternion[i]=hq[i];
 }
 bool Active()const noexcept{return active;}
 bool Omitting()const noexcept{return active&&(phase==Phase::B1||phase==Phase::B2);}
 Phase CurrentPhase()const noexcept{return phase;}
 bool Finished()const noexcept{return finished;}
 void Eye(unsigned eye)noexcept {if(active&&eye<2)current.eyes|=1u<<eye;}
 void ScopeTime(Scope scope,uint64_t start,uint64_t end)noexcept {
  auto i=unsigned(scope);if(active&&i<ScopeCount&&start&&end>=start){current.ticks[i]+=end-start;++current.calls[i];}
 }
 void Target(unsigned eye,bool omitted)noexcept {if(active&&eye<2){++current.targets[eye];if(omitted)++current.omitted[eye];}}
 void End(uint64_t now,bool submitted){
  if(!active)return;active=false;
  if(!submitted||current.eyes!=3||now<current.work){Abort("incomplete_stereo_frame");return;}
  current.work=now-current.work;current.interval=lastEnd&&now>=lastEnd?now-lastEnd:0;lastEnd=now;
  if(samples.size()>=Capacity){Abort("bounded_sample_capacity_reached");return;}
  samples.push_back(current);
 }
 void Abort(char const* why)noexcept {if(!armed||finished)return;active=false;finished=true;phase=Phase::Aborted;reason=why;}
 void CancelFrame()noexcept {if(active)Abort("cancelled_or_failed_vr_frame");}
 size_t Size()const noexcept{return samples.size();}
 void Csv(std::ostream& out)const {
  out<<"frame,phase,phase_age_ms,work_ms,interval_ms,xr_wait_ms,eyes,left_targets,right_targets,left_omitted,right_omitted";
  for(auto name:ScopeNames)out<<','<<name<<"_ms,"<<name<<"_calls";
  out<<",native_x_m,native_y_m,native_z_m,native_rx,native_ry,native_rz,head_x_m,head_y_m,head_z_m,head_qw,head_qx,head_qy,head_qz\n";
  double ms=frequency?1000./double(frequency):0;
  for(auto const& s:samples){
   out<<s.frame<<','<<PhaseNames[unsigned(s.phase)]<<','<<s.phaseAge*ms<<','<<s.work*ms<<','<<s.interval*ms<<','<<s.xrWait*ms<<','<<s.eyes<<','<<s.targets[0]<<','<<s.targets[1]<<','<<s.omitted[0]<<','<<s.omitted[1];
   for(unsigned i=0;i<ScopeCount;++i)out<<','<<s.ticks[i]*ms<<','<<s.calls[i];
   for(auto const& a:{s.nativePosition,s.nativeRotation,s.headPosition})for(auto x:a)out<<','<<x;
   for(auto x:s.headQuaternion)out<<','<<x;out<<'\n';
  }
 }
 void Metadata(std::ostream& out)const {
  out<<"{\"kind\":\"static_color_omission_screening\",\"complete\":"<<(phase==Phase::Done?"true":"false")<<",\"reason\":\""<<reason<<"\",\"phase\":\""<<PhaseNames[unsigned(phase)]<<"\",\"frames\":"<<samples.size()<<",\"hook_mask\":"<<hooks
     <<",\"replacement_renderer\":false,\"image_equivalent\":false,\"performance_gain_proven\":false,\"strict_upper_bound\":false,\"simulation_unchanged\":true,\"depth_zpass_unchanged\":true,\"native_shadow_calls_forwarded\":true,\"visibility_preparation_unchanged\":true,\"timers_inclusive_not_additive\":true,\"work_excludes_xr_wait_and_outside_simulation\":true,\"write_only_after_test\":true}\n";
 }
};
#ifdef G3VR_RENDERER_FEASIBILITY
inline Experiment experiment;
#endif
}
#pragma pack(pop)
