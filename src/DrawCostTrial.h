#pragma once
#include "DrawCostCore.h"
#include "FramePerformance.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <vector>
#include <ostream>
#include <filesystem>
#include <fstream>

#pragma pack(push,8)
namespace diagnostic::draw_cost {
class Collector {
 using Query=Microsoft::WRL::ComPtr<IDirect3DQuery9>;
 struct Slot {Query disjoint,frequency;std::array<Query,MaxScopes*2> stamps;std::array<Span,MaxScopes> spans{};
  uint64_t frame=0;unsigned used=0;bool pending=false,valid=true;};
 struct Cpu {uint64_t frame=0;std::array<std::array<uint64_t,Count>,3> inclusive{},exclusive{},calls{},solid{},alpha{};};
 struct Gpu {uint64_t frame=0,frequency=0;Span span;uint64_t exclusive=0;};
 std::array<Slot,4> slots;std::vector<Cpu> cpu;std::vector<Gpu> gpu;
 Cpu current;uint64_t serial=0,cpuFrequency=performance::Frequency();
 bool sampled=false,tried=false,enabled=false,created=false;int active=-1,gpuParent=-1;
 unsigned depth=0;std::array<uint64_t,MaxDepth> childTicks{};
 uint64_t discarded=0,busy=0,unsupported=0,scopeOverflow=0,cpuOverflow=0;
 bool rendererHook=false,executeHook=false;
 bool Issue(IDirect3DQuery9* query,DWORD flags)noexcept{
  if(!query||FAILED(query->Issue(flags))){enabled=false;unsupported++;if(active>=0)slots[unsigned(active)].valid=false;return false;}return true;
 }
public:
 struct Token {uint64_t start=0,frame=0;unsigned kind=0,lane=0,depth=0;int gpu=-1,previousGpu=-1;};
 Collector(){cpu.reserve(2048);gpu.reserve(8192);}
 void Hooks(bool renderer,bool execute)noexcept{rendererHook=renderer;executeHook=execute;}
 bool Ready()const noexcept{return rendererHook&&executeHook;}
 void Operations(Kind kind,unsigned lane,unsigned solid,unsigned alpha)noexcept{if(sampled&&lane<3){current.solid[lane][unsigned(kind)]+=solid;current.alpha[lane][unsigned(kind)]+=alpha;}}
 void NextFrame(uint64_t frame,bool detailed){
  EndGpuFrame();
  if(serial&&sampled){if(depth){++cpuOverflow;}else if(cpu.size()<2048)cpu.push_back(current);else ++cpuOverflow;}
  current={};current.frame=frame;serial=frame;sampled=detailed&&Ready();depth=0;childTicks={};gpuParent=-1;
 }
 bool Initialize(IDirect3DDevice9* device){
  if(tried)return enabled;tried=true;if(!device||!Ready())return false;
  for(auto& s:slots){
   if(FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT,&s.disjoint))||FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ,&s.frequency))){++unsupported;ResetGpu();return false;}
   for(auto& q:s.stamps)if(FAILED(device->CreateQuery(D3DQUERYTYPE_TIMESTAMP,&q))){++unsupported;ResetGpu();return false;}
  }enabled=true;created=true;return true;
 }
 void Poll()noexcept{
  if(!enabled)return;
  for(auto& s:slots)if(s.pending){
   BOOL disjoint=TRUE;uint64_t frequency=0;auto result=s.disjoint->GetData(&disjoint,sizeof(disjoint),0);
   if(FAILED(result)){enabled=false;++unsupported;return;}if(result!=S_OK)continue;
   result=s.frequency->GetData(&frequency,sizeof(frequency),0);if(FAILED(result)){enabled=false;++unsupported;return;}if(result!=S_OK)continue;
   bool ready=true;
   for(unsigned i=0;i<s.used&&ready;++i)for(unsigned edge=0;edge<2&&ready;++edge){uint64_t value=0;result=s.stamps[i*2+edge]->GetData(&value,sizeof(value),0);
    if(FAILED(result)){enabled=false;++unsupported;return;}if(result!=S_OK){ready=false;break;}if(edge)s.spans[i].end=value;else s.spans[i].begin=value;}
   if(!ready)continue;s.pending=false;std::array<uint64_t,MaxScopes> exclusive{};
   if(!s.valid||disjoint||!frequency||!Exclusive(s.spans.data(),s.used,exclusive.data())){++discarded;continue;}
   if(gpu.size()+s.used>8192){++scopeOverflow;continue;}
   for(unsigned i=0;i<s.used;++i)gpu.push_back({s.frame,frequency,s.spans[i],exclusive[i]});
  }
 }
 void BeginGpuFrame(IDirect3DDevice9* device){
  if(!tried)Initialize(device);Poll();
  if(!enabled||!sampled||serial%16||active>=0)return;
  for(unsigned i=0;i<slots.size();++i)if(!slots[i].pending){active=int(i);break;}
  if(active<0){++busy;return;}auto& s=slots[unsigned(active)];s.used=0;s.valid=true;s.frame=serial;gpuParent=-1;
  Issue(s.disjoint.Get(),D3DISSUE_BEGIN);
 }
 Token Begin(Kind kind,unsigned lane)noexcept{
  Token t;if(!sampled||lane>2||depth>=MaxDepth){if(sampled&&depth>=MaxDepth){++cpuOverflow;if(active>=0)slots[unsigned(active)].valid=false;}return t;}
  t.start=performance::Now();t.frame=serial;t.kind=unsigned(kind);t.lane=lane;t.depth=depth;childTicks[depth++]=0;
  bool gpuKind=t.kind>=unsigned(Kind::HighExecute);
  if(enabled&&active>=0&&gpuKind&&lane){auto& s=slots[unsigned(active)];
   if(s.used>=MaxScopes){s.valid=false;++scopeOverflow;}
   else{t.gpu=int(s.used++);t.previousGpu=gpuParent;auto& span=s.spans[unsigned(t.gpu)];span={t.kind,lane,gpuParent,0,0};gpuParent=t.gpu;Issue(s.stamps[unsigned(t.gpu)*2].Get(),D3DISSUE_END);}
  }return t;
 }
 void End(Token const& t)noexcept{
  if(!t.start)return;
  if(enabled&&active>=0&&t.gpu>=0){auto& s=slots[unsigned(active)];Issue(s.stamps[unsigned(t.gpu)*2+1].Get(),D3DISSUE_END);gpuParent=t.previousGpu;}
  auto end=performance::Now();if(t.frame!=serial||!depth||t.depth+1!=depth||end<t.start){++cpuOverflow;return;}
  auto total=end-t.start;--depth;
  current.inclusive[t.lane][t.kind]+=total;current.exclusive[t.lane][t.kind]+=total-(std::min)(total,childTicks[t.depth]);++current.calls[t.lane][t.kind];
  if(depth)childTicks[depth-1]+=total;
 }
 void EndGpuFrame()noexcept{
  if(active<0)return;auto& s=slots[unsigned(active)];
  if(gpuParent>=0)s.valid=false;
  Issue(s.frequency.Get(),D3DISSUE_END);Issue(s.disjoint.Get(),D3DISSUE_END);s.pending=true;active=-1;gpuParent=-1;
 }
 void ResetGpu()noexcept{for(auto& s:slots)s=Slot{};active=-1;gpuParent=-1;enabled=false;}
 void Write(std::filesystem::path const& root){
  EndGpuFrame();Poll();
  std::ofstream c(root/L"draw-cpu.csv"),g(root/L"draw-gpu.csv"),m(root/L"draw.json");
  c<<"frame,kind,lane,inclusive_ms,exclusive_ms,calls,solid_operations,alpha_operations\n";
  double factor=cpuFrequency?1000./double(cpuFrequency):0;
  for(auto const& row:cpu)for(unsigned lane=0;lane<3;++lane)for(unsigned k=0;k<Count;++k)if(row.calls[lane][k])c<<row.frame<<','<<Names[k]<<','<<lane<<','<<row.inclusive[lane][k]*factor<<','<<row.exclusive[lane][k]*factor<<','<<row.calls[lane][k]<<','<<row.solid[lane][k]<<','<<row.alpha[lane][k]<<'\n';
  g<<"frame,kind,lane,parent,inclusive_ms,exclusive_ms\n";
  for(auto const& row:gpu){double ms=1000./double(row.frequency);g<<row.frame<<','<<Names[row.span.kind]<<','<<row.span.lane<<','<<row.span.parent<<','<<(row.span.end-row.span.begin)*ms<<','<<row.exclusive*ms<<'\n';}
  unsigned pending=0;for(auto const& s:slots)pending+=s.pending;
  m<<"{\"renderer_hook\":"<<(rendererHook?"true":"false")<<",\"execute_hook\":"<<(executeHook?"true":"false")<<",\"gpu_initialized\":"<<(created?"true":"false")<<",\"gpu_span_rows\":"<<gpu.size()<<",\"cpu_frames\":"<<cpu.size()<<",\"discarded_gpu_frames\":"<<discarded<<",\"pending_gpu_frames\":"<<pending<<",\"skipped_busy\":"<<busy<<",\"unsupported_errors\":"<<unsupported<<",\"scope_overflow\":"<<scopeOverflow<<",\"cpu_overflow\":"<<cpuOverflow<<",\"gpu_sample_period\":16,\"gpu_getdata_flags\":0,\"gpu_query_flushes\":0,\"distances_changed\":false,\"render_work_skipped\":false,\"gpu_elapsed_may_include_starvation\":true,\"high_low_classification\":\"native_camera_database_identity\",\"performance_gain_proven\":false}\n";
 }
};
}
#pragma pack(pop)
