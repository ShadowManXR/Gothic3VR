// Separate CPU-only observation. No callbacks, simulation or rendering skipped.
diagnostic::FocusHook gEntityCostHooks[diagnostic::scene_cost::EntityEntryCount];
unsigned gEntityCostMask=0;bool gEntityCostInstalled=false;
uint64_t EntityCostParticleStarted()noexcept{return gEntityCostInstalled?CostStart():0;}
void EntityCostParticleFinished(uint64_t started)noexcept{CostEnd(unsigned(diagnostic::scene_cost::Scope::Particles),started?gCostLane:0,started);}
template<unsigned Index> void __fastcall EntityCostVoid(void* self,void*){
 auto started=CostStart();unsigned lane=started?gCostLane:0;
 __try{reinterpret_cast<void(__thiscall*)(void*)>(gEntityCostHooks[Index].original)(self);}
 __finally{CostEnd(unsigned(diagnostic::scene_cost::EntityEntries[Index].scope),lane,started);}
}
void WriteEntityCost(std::filesystem::path const& root){
 if(!gEntityCostInstalled)return;std::ofstream out(root/L"entity.json");
 out<<"{\"hook_mask\":"<<gEntityCostMask<<",\"required_mask\":255,\"particle_hook\":"<<(gFlameHooksReady?"true":"false")
    <<",\"sample_period\":4,\"native_callbacks_skipped\":false,\"main_thread_wall_only\":true,\"inclusive_not_additive\":true}\n";
}
void InstallEntityCost(HMODULE engine){
 wchar_t enabled[2]{};if(gCostOutput.empty()||GetEnvironmentVariableW(L"G3VR_ENTITY_COST",enabled,2)!=1||enabled[0]!=L'1')return;
 auto game=GetModuleHandleW(L"Game.dll");if(!game)return;
 // Engine and Game whole-file hashes were checked by InstallSceneCost.
 void* replacements[]={reinterpret_cast<void*>(&EntityCostVoid<0>),reinterpret_cast<void*>(&EntityCostVoid<1>),reinterpret_cast<void*>(&EntityCostVoid<2>),reinterpret_cast<void*>(&EntityCostVoid<3>),
  reinterpret_cast<void*>(&EntityCostVoid<4>),reinterpret_cast<void*>(&EntityCostVoid<5>),reinterpret_cast<void*>(&EntityCostVoid<6>),reinterpret_cast<void*>(&EntityCostVoid<7>)};
 static_assert(sizeof(replacements)/sizeof(replacements[0])==diagnostic::scene_cost::EntityEntryCount);
 for(unsigned i=0;i<diagnostic::scene_cost::EntityEntryCount;++i){auto const& profile=diagnostic::scene_cost::EntityEntries[i];auto const& entry=profile.entry;auto module=entry.game?game:engine;
  auto* target=reinterpret_cast<unsigned char*>(GetProcAddress(module,entry.symbol));unsigned char prefix[16]{};
  if(diagnostic::scene_cost::EntityPrefix(profile,uint32_t(reinterpret_cast<uintptr_t>(module)),prefix)&&target==reinterpret_cast<unsigned char*>(module)+entry.rva&&
     gEntityCostHooks[i].Install(target,replacements[i],prefix,entry.bytes))gEntityCostMask|=1u<<i;
 }
 gEntityCostInstalled=true;
 Message("entity_cost_trial_ready; bounded_native_callback_observation; no_simulation_skipped",gEntityCostMask,true);
}
