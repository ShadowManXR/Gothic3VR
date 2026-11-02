#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <ostream>

#pragma pack(push,8)
namespace diagnostic::scene_cost {
enum class Scope : unsigned {Application,Modules,Entities,Physics,Ai,Navigation,Pvs,Extract,LowPoly,VegetationGrid,VegetationCache,VegetationGeometry,VegetationNode,Hierarchy,PvsGrid,GeometryRead,QueueWait,ResourceWait,Stereo,
#ifdef G3VR_ENTITY_COST_TRIAL
 DynamicPre,DynamicProcess,DynamicPost,PhysicObject,CharacterMovement,CharacterAnimation,AudioEmitter,NavigationRoutine,Particles,
#endif
 Count};
inline constexpr unsigned Count=unsigned(Scope::Count),Capacity=8192;
inline constexpr unsigned RequiredHooks=(1u<<0)|(1u<<2)|(1u<<6)|(1u<<7)|(1u<<9);
inline constexpr char const* Names[]={"application","modules","entities","physics","ai","navigation","pvs","extract","low_poly","vegetation_grid","vegetation_cache","vegetation_geometry","vegetation_node","hierarchy","pvs_grid","geometry_read","queue_wait","resource_wait","stereo_camera",
#ifdef G3VR_ENTITY_COST_TRIAL
 "dynamic_pre","dynamic_process","dynamic_post","physic_object","character_movement","character_animation","audio_emitter","navigation_routine","particles",
#endif
};
static_assert(sizeof(Names)/sizeof(Names[0])==Count);
enum class Phase : unsigned {Waiting,Warmup,ControlBefore,Measure,ControlAfter,Done,Aborted};
inline constexpr char const* Phases[]={"waiting","warmup","control_before","measure","control_after","done","aborted"};
struct Record {
 uint64_t frame=0,start=0,interval=0,age=0;Phase phase=Phase::Waiting;unsigned eyes=0;bool sampled=false;
 unsigned rangeEntities=0,processableEntities=0;bool countsValid=false;
 std::array<std::array<uint64_t,Count>,3> ticks{},calls{}; // outside eyes / left / right
 std::array<float,3> position{},rotation{};
};
class Trial {
 uint64_t hz=0,phaseStart=0,serial=0;Phase phase=Phase::Waiting;unsigned hooks=0,restarts=0;
 bool currentOpen=false;Record current;std::vector<Record> records;
 char const* reason="not_armed";
 static unsigned Duration(Phase p)noexcept{
#ifdef G3VR_ENTITY_COST_TRIAL
  return p==Phase::Measure?20000:p==Phase::Warmup?2000:4000;
#else
  return p==Phase::Warmup?8000:p==Phase::Measure?24000:8000;
#endif
 }
 void FinishFrame(uint64_t now){
  if(!currentOpen)return;currentOpen=false;
  if(now<current.start){Abort("clock_reversed");return;}
  current.interval=now-current.start;
  if(records.size()>=Capacity){Abort("capacity_reached");return;}
  records.push_back(current);
 }
public:
 bool written=false;
 void Arm(uint64_t frequency,unsigned mask){if(hz||!frequency)return;hz=frequency;hooks=mask;records.reserve(Capacity);reason="waiting_for_first_person_world";}
 bool Armed()const noexcept{return hz!=0;}
 bool Finished()const noexcept{return phase==Phase::Done||phase==Phase::Aborted;}
 bool Sampled()const noexcept{return currentOpen&&current.sampled&&!Finished();}
 uint64_t FrameId()const noexcept{return currentOpen?current.frame:0;}
 Phase CurrentPhase()const noexcept{return phase;}
 size_t Size()const noexcept{return records.size();}
 void Frame(uint64_t now,bool eligible){
  if(!hz||Finished())return;FinishFrame(now);if(Finished())return;
  if(!eligible){
#ifdef G3VR_ENTITY_COST_TRIAL
   // Save loading, tutorial pages and temporary third-person states can arrive
   // after the first eligible frame. Wait for a new uninterrupted segment;
   // never permanently consume this one-shot capture before timers start.
   if(phase!=Phase::Waiting)++restarts;
   phase=Phase::Waiting;phaseStart=0;records.clear();reason="waiting_after_world_state_change";return;
#else
   if(phase==Phase::Waiting||phase==Phase::Warmup){phase=Phase::Waiting;phaseStart=0;records.clear();return;}
   Abort("world_menu_or_player_state_changed");return;
#endif
  }
  if((hooks&RequiredHooks)!=RequiredHooks){Abort("native_hook_coverage_incomplete");return;}
  if(phase==Phase::Waiting){phase=Phase::Warmup;phaseStart=now;reason="running";}
  if(now<phaseStart){Abort("clock_reversed");return;}
  if((now-phaseStart)*1000/hz>=Duration(phase)){
   phase=Phase(unsigned(phase)+1);phaseStart=now;
   if(phase==Phase::Done){reason="completed_observation_only";return;}
  }
  current={};current.frame=++serial;current.start=now;current.phase=phase;current.age=now-phaseStart;
  current.sampled=phase==Phase::Measure&&serial%4==0;currentOpen=true;
 }
 void Eye(unsigned eye)noexcept{if(currentOpen&&eye<2)current.eyes|=1u<<eye;}
 void Counts(unsigned range,unsigned processable)noexcept{if(Sampled()){current.rangeEntities=range;current.processableEntities=processable;current.countsValid=true;}}
 void Pose(float const* p,float const* r,float units)noexcept{if(currentOpen&&units>0)for(unsigned i=0;i<3;++i){current.position[i]=p[i]/units;current.rotation[i]=r[i];}}
 void Add(Scope scope,unsigned lane,uint64_t start,uint64_t end)noexcept{
  if(!Sampled()||lane>2||!start||end<start)return;
  current.ticks[lane][unsigned(scope)]+=end-start;++current.calls[lane][unsigned(scope)];
 }
 void Abort(char const* why)noexcept{if(Finished())return;phase=Phase::Aborted;reason=why;currentOpen=false;}
 void Csv(std::ostream& out)const{
  out<<"frame,phase,phase_age_ms,interval_ms,sampled,eyes,x_m,y_m,z_m,rx,ry,rz,counts_valid,range_entities,processable_entities";
  for(unsigned lane=0;lane<3;++lane)for(auto name:Names)out<<','<<name<<'_'<<lane<<"_ms,"<<name<<'_'<<lane<<"_calls";out<<'\n';
  double ms=hz?1000./double(hz):0;
  for(auto const& s:records){out<<s.frame<<','<<Phases[unsigned(s.phase)]<<','<<s.age*ms<<','<<s.interval*ms<<','<<s.sampled<<','<<s.eyes;
   for(auto v:s.position)out<<','<<v;for(auto v:s.rotation)out<<','<<v;out<<','<<s.countsValid<<','<<s.rangeEntities<<','<<s.processableEntities;
   for(unsigned lane=0;lane<3;++lane)for(unsigned i=0;i<Count;++i)out<<','<<s.ticks[lane][i]*ms<<','<<s.calls[lane][i];out<<'\n';}
 }
 void Metadata(std::ostream& out)const{
  out<<"{\"kind\":\"scene_cost_observation\",\"complete\":"<<(phase==Phase::Done?"true":"false")<<",\"reason\":\""<<reason<<"\",\"frames\":"<<records.size()<<",\"hook_mask\":"<<hooks
     <<",\"restart_count\":"<<restarts<<",\"warmup_ms\":"<<Duration(Phase::Warmup)<<",\"control_ms\":"<<Duration(Phase::ControlBefore)<<",\"measure_ms\":"<<Duration(Phase::Measure)
     <<",\"sample_period\":4,\"distances_changed\":false,\"simulation_skipped\":false,\"render_work_skipped\":false,\"timers_inclusive_not_additive\":true,\"main_thread_cpu_wall_only\":true,\"lanes\":[\"outside_eyes\",\"left\",\"right\"],\"interval_boundary\":\"native_application_process_entry\",\"gpu_time_measured\":false,\"performance_gain_proven\":false}\n";
 }
};
#ifdef G3VR_SCENE_COST_TRIAL
inline Trial trial;
#endif
}
#pragma pack(pop)
