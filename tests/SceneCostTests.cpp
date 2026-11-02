#include "SceneCostTrial.h"
#include "SceneCostEntries.h"
#ifdef G3VR_ENTITY_COST_TRIAL
#include "EntityCostEntries.h"
#endif
#include <sstream>
#include <iostream>
using namespace diagnostic::scene_cost;
#define CHECK(x) do {if(!(x)){std::cerr<<"failed line "<<__LINE__<<": "<<#x<<'\n';return 1;}}while(false)
int main(){
 Trial a;a.Arm(1000,RequiredHooks);a.Frame(100,true);a.Eye(0);a.Eye(1);
 a.Frame(8100,true);CHECK(a.CurrentPhase()==Phase::ControlBefore);CHECK(!a.Sampled());
 a.Frame(16100,true);CHECK(a.CurrentPhase()==Phase::Measure);
 for(uint64_t t=16110;t<16200;t+=10){a.Eye(0);a.Eye(1);a.Frame(t,true);if(a.Sampled()){a.Add(Scope::Entities,0,t,t+3);a.Add(Scope::VegetationGrid,1,t,t+2);}}
 a.Frame(40100,true);CHECK(a.CurrentPhase()==Phase::ControlAfter);CHECK(!a.Sampled());
 a.Frame(48100,true);CHECK(a.CurrentPhase()==Phase::Done);CHECK(!a.Sampled());
 auto size=a.Size();a.Frame(50000,true);CHECK(a.Size()==size);
 std::ostringstream csv,meta;a.Csv(csv);a.Metadata(meta);CHECK(csv.str().find("vegetation_grid_1_ms")!=std::string::npos);CHECK(meta.str().find("\"complete\":true")!=std::string::npos);
 Trial missing;missing.Arm(1000,0);missing.Frame(1,true);CHECK(missing.Finished());CHECK(missing.Size()==0);
 Trial reset;reset.Arm(1000,RequiredHooks);reset.Frame(1,true);reset.Frame(2,false);CHECK(reset.CurrentPhase()==Phase::Waiting);CHECK(reset.Size()==0);
 reset.Frame(3,true);reset.Frame(8003,true);reset.Frame(8004,false);
#ifdef G3VR_ENTITY_COST_TRIAL
 CHECK(reset.CurrentPhase()==Phase::Waiting&&reset.Size()==0);
 // A load/menu transition during controls or actual measurement must retry,
 // then permit a complete capture without restarting the game.
 reset.Frame(10000,true);reset.Frame(12000,true);CHECK(reset.CurrentPhase()==Phase::ControlBefore);
 reset.Frame(16000,true);CHECK(reset.CurrentPhase()==Phase::Measure);reset.Frame(16010,false);
 CHECK(reset.CurrentPhase()==Phase::Waiting&&reset.Size()==0);
 reset.Frame(17000,true);reset.Frame(19000,true);reset.Frame(23000,true);reset.Frame(43000,true);reset.Frame(47000,true);
 CHECK(reset.CurrentPhase()==Phase::Done);std::ostringstream retryMeta;reset.Metadata(retryMeta);
 CHECK(retryMeta.str().find("\"restart_count\":3")!=std::string::npos);
 CHECK(retryMeta.str().find("\"measure_ms\":20000")!=std::string::npos);
#else
 CHECK(reset.CurrentPhase()==Phase::Aborted);
#endif
 Trial cap;cap.Arm(1000000000,RequiredHooks);for(uint64_t t=1;t<Capacity+5;++t)cap.Frame(t,true);CHECK(cap.Finished());CHECK(cap.Size()==Capacity);
 Trial clock;clock.Arm(1000,RequiredHooks);clock.Frame(100,true);clock.Frame(99,true);CHECK(clock.CurrentPhase()==Phase::Aborted);
 unsigned char prefix[16]{};CHECK(Prefix(Entries[17],0x20000000,prefix));uint32_t operand=0;std::memcpy(&operand,prefix+4,4);CHECK(operand==0x20396388);CHECK(!Prefix(Entries[17],UINT32_MAX,prefix));
 CHECK(sizeof(Entries)/sizeof(Entries[0])==unsigned(Scope::Stereo));
#ifdef G3VR_ENTITY_COST_TRIAL
 CHECK(EntityEntryCount==8);
 CHECK(EntityPrefix(EntityEntries[3],0x20000000,prefix));std::memcpy(&operand,prefix+1,4);CHECK(operand==0x20602dcc);
 CHECK(EntityPrefix(EntityEntries[6],0x20000000,prefix));std::memcpy(&operand,prefix+4,4);CHECK(operand==0x20602a88);
 CHECK(!EntityPrefix(EntityEntries[3],UINT32_MAX,prefix));
 CHECK(csv.str().find("character_movement_0_ms")!=std::string::npos);
 CHECK(csv.str().find("navigation_routine_0_calls")!=std::string::npos);
 Trial detail;detail.Arm(1000,RequiredHooks);detail.Frame(100,true);detail.Frame(8100,true);detail.Frame(16100,true);detail.Frame(16110,true);
 CHECK(detail.Sampled());detail.Add(Scope::CharacterMovement,0,16111,16113);detail.Add(Scope::Particles,0,16111,16112);detail.Frame(16120,true);
 std::ostringstream output;detail.Csv(output);CHECK(output.str().find("particles_0_ms")!=std::string::npos);
#endif
 std::cout<<"PASS bounded observation, sampling, lane export, missing hooks, state change, capacity, clock and relocated operand\n";
}
