// Included after scene-cost timing helpers. Classify by native camera database
// identity, without intercepting the PUP-modified RenderLowPoly entry.
diagnostic::FocusHook gDrawSystemHook,gDrawExecuteHook;
diagnostic::draw_cost::Collector gDrawCollector;
diagnostic::draw_cost::Kind gDrawKind=diagnostic::draw_cost::Kind::OtherRender;
bool gDrawEnabled=false,gDrawWritten=false;
bool DrawCostActive()noexcept{
 return GetCurrentThreadId()==gApplicationThread.load()&&gDrawEnabled&&!gDrawWritten&&diagnostic::scene_cost::trial.Sampled()&&gNativeRenderScope&&gStereo.rendering&&gStereo.eyeActive;
}
diagnostic::draw_cost::Kind DrawClass(eCFrustumDatabase const& database)noexcept{
 using Kind=diagnostic::draw_cost::Kind;
 __try{auto* camera=gStereo.camera;if(!camera||!diagnostic::Readable(camera,sizeof(*camera)))return Kind::OtherRender;
  if(&database==&camera->GetLowPolyFrustumDatabase())return Kind::LowRender;
  if(&database==&camera->GetFrustumDatabase())return Kind::HighRender;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
 return Kind::OtherRender;
}
void __fastcall DrawCostSystem(eCRenderSystemAdmin* self,void*,eCFrustumDatabase& database){
 bool active=DrawCostActive();auto previous=active?gDrawKind:diagnostic::draw_cost::Kind::OtherRender;
 if(active)gDrawKind=DrawClass(database);
 auto token=active?gDrawCollector.Begin(gDrawKind,gCostLane):diagnostic::draw_cost::Collector::Token{};
 __try{reinterpret_cast<void(__thiscall*)(eCRenderSystemAdmin*,eCFrustumDatabase&)>(gDrawSystemHook.original)(self,database);}
 __finally{if(active){gDrawCollector.End(token);gDrawKind=previous;}}
}
void __fastcall DrawCostExecute(eCFrustumDatabase* self,void*){
 bool active=DrawCostActive();auto kind=active?diagnostic::draw_cost::ExecuteKind(DrawClass(*self)):diagnostic::draw_cost::Kind::OtherExecute;
 auto token=active?gDrawCollector.Begin(kind,gCostLane):diagnostic::draw_cost::Collector::Token{};
 if(active){__try{auto solid=self->GetSolidElementCount(),alpha=self->GetAlphaElementCount();if(solid>=0&&alpha>=0)gDrawCollector.Operations(kind,gCostLane,unsigned(solid),unsigned(alpha));}__except(EXCEPTION_EXECUTE_HANDLER){}}
 __try{reinterpret_cast<void(__thiscall*)(eCFrustumDatabase*)>(gDrawExecuteHook.original)(self);}
 __finally{if(active)gDrawCollector.End(token);}
}
diagnostic::draw_cost::Collector::Token DrawVegetationBegin()noexcept{
 if(!DrawCostActive())return {};
 return gDrawCollector.Begin(diagnostic::draw_cost::VegetationKind(gDrawKind),gCostLane);
}
void DrawCostNextFrame(){
 if(!gDrawEnabled||gDrawWritten)return;
 gDrawCollector.NextFrame(diagnostic::scene_cost::trial.FrameId(),diagnostic::scene_cost::trial.Sampled());
}
void DrawCostGpuFrame()noexcept{
 if(!gDrawEnabled||gDrawWritten||diagnostic::scene_cost::trial.Finished())return;
 __try{auto* wrapper=DeviceAccess::sGetAPIDevice();if(diagnostic::Readable(wrapper,sizeof(eCDX9Device)))
  gDrawCollector.BeginGpuFrame(reinterpret_cast<eCDX9Device*>(wrapper)->m_pDevice);
 }__except(EXCEPTION_EXECUTE_HANDLER){}
}
void WriteDrawCost(std::filesystem::path const& output){
 if(!gDrawEnabled||gDrawWritten)return;
 try{gDrawCollector.Write(output);}catch(...){gDrawCollector.ResetGpu();gDrawWritten=true;throw;}
 gDrawCollector.ResetGpu();gDrawWritten=true;
}
void InstallDrawCost(HMODULE engine){
 wchar_t mode[2]{};if(GetEnvironmentVariableW(L"G3VR_DRAW_COST",mode,2)!=1||mode[0]!=L'1')return;
 auto const& system=diagnostic::performance::RenderEntries[4];auto const& execute=diagnostic::performance::RenderEntries[3];
 auto install=[&](diagnostic::FocusHook& hook,diagnostic::performance::RenderEntry const& entry,void* replacement){
  auto* target=reinterpret_cast<unsigned char*>(GetProcAddress(engine,entry.symbol));
  return target==reinterpret_cast<unsigned char*>(engine)+entry.rva&&hook.Install(target,replacement,entry.prefix,entry.bytes);
 };
 bool systemReady=install(gDrawSystemHook,system,reinterpret_cast<void*>(&DrawCostSystem));
 bool executeReady=install(gDrawExecuteHook,execute,reinterpret_cast<void*>(&DrawCostExecute));
 gDrawCollector.Hooks(systemReady,executeReady);gDrawEnabled=true;
 Message("draw_cost_trial_ready; native_database_identity; sparse_gpu_timestamps_no_flush_no_wait",(systemReady?1:0)|(executeReady?2:0),true);
}
