#include "RendererFeasibility.h"
#include <sstream>
#include <iostream>
#include <stdexcept>
using namespace diagnostic::renderer_trial;
void Need(bool ok){if(!ok)throw std::runtime_error("renderer feasibility test failed");}
int main(){try{
 Need(EligibleColor(true,true,true,0,1));
 Need(!EligibleColor(true,true,false,0,1)&&!EligibleColor(true,false,true,0,1)&&!EligibleColor(true,true,true,1,1)&&!EligibleColor(true,true,true,0,2)&&!EligibleColor(false,true,true,0,1));
 Experiment p;p.SetHooks(RequiredHooks);p.Arm(1000,true);uint64_t frame=0,clock=100;
 auto step=[&](uint64_t now,bool eligible=true){p.RuntimeReady(now-2,1);bool active=p.Begin(now,++frame,eligible);if(active){Need(p.Omitting()==(p.CurrentPhase()==Phase::B1||p.CurrentPhase()==Phase::B2));p.Eye(0);p.Eye(1);p.Target(0,p.Omitting());p.Target(1,p.Omitting());p.ScopeTime(Scope::Mesh,now,now+1);p.End(now+10,true);}return active;};
 Need(!step(clock,false)&&!p.Omitting());Need(step(clock));Need(p.CurrentPhase()==Phase::Warmup);
 clock+=12000;Need(step(clock));Need(p.CurrentPhase()==Phase::A1&&!p.Omitting());
 clock+=8000;Need(step(clock));Need(p.CurrentPhase()==Phase::B1&&!p.Omitting()); // End clears suppression.
 clock+=4000;Need(step(clock));Need(p.CurrentPhase()==Phase::A2);
 clock+=8000;Need(step(clock));Need(p.CurrentPhase()==Phase::B2);
 clock+=4000;Need(step(clock));Need(p.CurrentPhase()==Phase::A3);
 clock+=8000;Need(!step(clock)&&p.Finished()&&p.CurrentPhase()==Phase::Done);
 std::ostringstream csv,json;p.Csv(csv);p.Metadata(json);Need(csv.str().find("omit_static_color_B1")!=std::string::npos&&json.str().find("\"performance_gain_proven\":false")!=std::string::npos);
 Experiment absent;absent.Arm(1000,true);Need(!absent.Begin(1,1,true)&&absent.Finished());
 Experiment lost;lost.SetHooks(RequiredHooks);lost.Arm(1000,true);lost.RuntimeReady(1,0);Need(lost.Begin(1,1,true));lost.Eye(0);lost.End(2,true);Need(lost.Finished()&&!lost.Omitting());
 Experiment menu;menu.SetHooks(RequiredHooks);menu.Arm(1000,true);Need(menu.Begin(1,1,true));menu.Eye(0);menu.Eye(1);menu.End(2,true);Need(!menu.Begin(3,2,false)&&!menu.Finished());
 Need(menu.Begin(4,3,true));menu.Eye(0);menu.Eye(1);menu.End(5,true);Need(menu.Begin(12004,4,true));menu.Eye(0);menu.Eye(1);menu.End(12005,true);Need(!menu.Begin(12006,5,false)&&menu.Finished());
 Experiment disabled;disabled.Arm(1000,false);Need(!disabled.Begin(1,1,true)&&!disabled.Armed());
 Experiment reversed;reversed.SetHooks(RequiredHooks);reversed.Arm(1000,true);Need(reversed.Begin(10,1,true));Need(!reversed.Begin(9,2,true)&&reversed.Finished());
 Experiment bounded;bounded.SetHooks(RequiredHooks);bounded.Arm(1000000000,true);
 for(unsigned i=0;i<=Capacity;++i){Need(bounded.Begin(i+1,i+1,true));bounded.Eye(0);bounded.Eye(1);bounded.End(i+2,true);}
 Need(bounded.Finished()&&bounded.Size()==Capacity&&!bounded.Omitting());
 Experiment focus;focus.SetHooks(RequiredHooks);focus.Arm(1000,true);Need(focus.Begin(1,1,true));focus.Unavailable();Need(!focus.Finished()&&focus.CurrentPhase()==Phase::Waiting);
 Need(focus.Begin(2,2,true));focus.Eye(0);focus.Eye(1);focus.End(3,true);Need(focus.Begin(12002,3,true));focus.Unavailable();Need(focus.Finished()&&!focus.Omitting());
 std::cout<<"PASS bounded A/B/A screening, stereo gating, menu/focus abort, disabled mode and honest output labels\n";return 0;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
