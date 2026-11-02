// Included inside Diagnostics.cpp's anonymous namespace. Native calls always
// forward; only bounded, sampled, main-thread CPU wall timings are collected.
diagnostic::FocusHook gCostHooks[18];
unsigned gCostLane=0;
std::filesystem::path gCostOutput;
#ifdef G3VR_ENTITY_COST_TRIAL
void WriteEntityCost(std::filesystem::path const& root);
#endif
#ifdef G3VR_DRAW_COST_TRIAL
void WriteDrawCost(std::filesystem::path const& output);
#endif
void FlushSceneCost(bool exiting)noexcept{
 try{auto& trial=diagnostic::scene_cost::trial;if(gCostOutput.empty()||trial.written||!trial.Armed())return;
  if(exiting)trial.Abort("session_ended_early");if(!trial.Finished())return;
  std::ofstream csv(gCostOutput/L"frames.csv"),meta(gCostOutput/L"capture.json");if(!csv||!meta)return;
  trial.Csv(csv);trial.Metadata(meta);csv.flush();meta.flush();
#ifdef G3VR_ENTITY_COST_TRIAL
  WriteEntityCost(gCostOutput);
#endif
#ifdef G3VR_DRAW_COST_TRIAL
  WriteDrawCost(gCostOutput);
#endif
  if(csv&&meta)trial.written=true;
 }catch(...){}
}
uint64_t CostStart()noexcept{
 if(GetCurrentThreadId()!=gApplicationThread.load())return 0;
 return diagnostic::scene_cost::trial.Sampled()?diagnostic::performance::Now():0;
}
void CostEnd(unsigned index,unsigned lane,uint64_t started)noexcept{
 if(started)diagnostic::scene_cost::trial.Add(static_cast<diagnostic::scene_cost::Scope>(index),lane,started,diagnostic::performance::Now());
}
void CostEntityCounts(void* self)noexcept{
 __try{auto* admin=static_cast<eCEntityAdmin*>(self);diagnostic::scene_cost::trial.Counts(admin->GetEntitiesInProcessingRange().GetCount(),admin->GetProcessableEntities().GetCount());}
 __except(EXCEPTION_EXECUTE_HANDLER){}
}
#ifdef G3VR_DRAW_COST_TRIAL
#include "DrawCostTrialHooks.inl"
#endif
void __fastcall CostApplication(void* self,void*){
 bool main=GetCurrentThreadId()==gApplicationThread.load();
 if(main){
  bool eligible=!gStereo.shuttingDown&&!gWorldLifecycle.Blocked()&&gStereoRuntime.Connected()&&gApplication&&gApplication->IsGameRunning()&&gStereo.firstPerson&&!MenuPanelActive()&&!gRadialOpen&&!gCSession::GetInstance().IsPaused();
  diagnostic::scene_cost::trial.Frame(diagnostic::performance::Now(),eligible);
#ifdef G3VR_DRAW_COST_TRIAL
  DrawCostNextFrame();
#endif
  FlushSceneCost();
 }
 auto started=CostStart();
 __try{reinterpret_cast<void(__thiscall*)(void*)>(gCostHooks[0].original)(self);}
 __finally{CostEnd(0,0,started);}
}
template<unsigned Index> void __fastcall CostVoid(void* self,void*){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{reinterpret_cast<void(__thiscall*)(void*)>(gCostHooks[Index].original)(self);}
 __finally{CostEnd(Index,lane,started);if constexpr(Index==2)if(started)CostEntityCounts(self);}
}
template<unsigned Index> void __fastcall CostArgument(void* self,void*,void const* argument){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
#ifdef G3VR_DRAW_COST_TRIAL
 auto drawToken=Index==9?DrawVegetationBegin():diagnostic::draw_cost::Collector::Token{};
#endif
 __try{reinterpret_cast<void(__thiscall*)(void*,void const*)>(gCostHooks[Index].original)(self,argument);}
 __finally{CostEnd(Index,lane,started);
#ifdef G3VR_DRAW_COST_TRIAL
  if constexpr(Index==9)gDrawCollector.End(drawToken);
#endif
 }
}
template<unsigned Index,class Result> Result __fastcall CostResult(void* self,void*){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{return reinterpret_cast<Result(__thiscall*)(void*)>(gCostHooks[Index].original)(self);}
 __finally{CostEnd(Index,lane,started);}
}
GEBool __fastcall CostPvs(void* self,void*,void* database){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{return reinterpret_cast<GEBool(__thiscall*)(void*,void*)>(gCostHooks[6].original)(self,database);}
 __finally{CostEnd(6,lane,started);}
}
GEBool __fastcall CostPvsGrid(void* self,void*,void* database,void const* sphere,void* cells){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{return reinterpret_cast<GEBool(__thiscall*)(void*,void*,void const*,void*)>(gCostHooks[14].original)(self,database,sphere,cells);}
 __finally{CostEnd(14,lane,started);}
}
void __fastcall CostResourceWait(void* self,void*,int timeout){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{reinterpret_cast<void(__thiscall*)(void*,int)>(gCostHooks[17].original)(self,timeout);}
 __finally{CostEnd(17,lane,started);}
}
#ifdef G3VR_ENTITY_COST_TRIAL
#include "EntityCostTrialHooks.inl"
#endif
void InstallSceneCost(HMODULE engine){
 wchar_t enabled[2]{},output[32768]{};
 if(GetEnvironmentVariableW(L"G3VR_SCENE_COST",enabled,2)!=1||enabled[0]!=L'1')return;
 auto length=GetEnvironmentVariableW(L"G3VR_SCENE_COST_OUTPUT",output,32768);if(!length||length>=32768)return;
 using namespace diagnostic::scene_cost;
 HMODULE game=GetModuleHandleW(L"Game.dll");wchar_t path[MAX_PATH]{};unsigned char digest[32]{};
 if(!game||!GetModuleFileNameW(game,path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,diagnostic::performance::GameSceneHash,32))return;
 // The caller validated the exact entire Engine.dll. The second module is
 // validated above. Each entry also requires export, RVA and complete prologue.
 void* replacements[]={reinterpret_cast<void*>(&CostApplication),reinterpret_cast<void*>(&CostVoid<1>),reinterpret_cast<void*>(&CostVoid<2>),reinterpret_cast<void*>(&CostVoid<3>),
  reinterpret_cast<void*>(&CostVoid<4>),reinterpret_cast<void*>(&CostVoid<5>),reinterpret_cast<void*>(&CostPvs),reinterpret_cast<void*>(&CostVoid<7>),reinterpret_cast<void*>(&CostVoid<8>),
  reinterpret_cast<void*>(&CostArgument<9>),reinterpret_cast<void*>(&CostArgument<10>),reinterpret_cast<void*>(&CostVoid<11>),reinterpret_cast<void*>(&CostVoid<12>),reinterpret_cast<void*>(&CostVoid<13>),
  reinterpret_cast<void*>(&CostPvsGrid),reinterpret_cast<void*>(&CostResult<15,bEResult>),reinterpret_cast<void*>(&CostResult<16,GEBool>),reinterpret_cast<void*>(&CostResourceWait)};
 static_assert(sizeof(replacements)/sizeof(replacements[0])==18);
 unsigned mask=0;
 for(unsigned i=0;i<18;++i){auto const& entry=Entries[i];auto module=entry.game?game:engine;
  auto* target=reinterpret_cast<unsigned char*>(GetProcAddress(module,entry.symbol));unsigned char prefix[16]{};
  if(!Prefix(entry,uint32_t(reinterpret_cast<uintptr_t>(module)),prefix))continue;
  if(target==reinterpret_cast<unsigned char*>(module)+entry.rva&&gCostHooks[i].Install(target,replacements[i],prefix,entry.bytes))mask|=1u<<i;
 }
 gCostOutput=output;trial.Arm(diagnostic::performance::Frequency(),mask);
#ifdef G3VR_DRAW_COST_TRIAL
 InstallDrawCost(engine);
#endif
 Message("scene_cost_trial_ready; observation_only; settings_unchanged; sample_period=4",mask,true);
}
