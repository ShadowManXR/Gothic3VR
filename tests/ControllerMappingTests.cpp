#include "ControllerMapping.h"
#include "ControllerMappingSettings.h"
#include "ControllerMappingIpc.h"
#include "ControllerControls.h"
#include "ControllerSneakToggle.h"
#include <cstdio>
using namespace diagnostic;namespace m=diagnostic::mapping;
unsigned failures=0;void Check(bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);if(!ok)++failures;}
struct Rig {
 m::Config config;m::Engine engine;ControllerFrame raw;uint64_t time=100;
 Rig(){raw.focused=true;raw.hands[0].active=raw.hands[1].active=true;Step();Step();}
 ControllerFrame Step(unsigned elapsed=16,bool suppress=false){time+=elapsed;raw.timeMs=time;return engine.Step(raw,config,time,suppress);}
};
int main(){
 {Rig r;r.raw.hands[0].stick[0]=.3f;r.raw.hands[0].stick[1]=.9f;r.raw.hands[0].stickClick=true;auto f=r.Step();
 Check(m::Active(f,m::Sprint)&&m::Pushed(f,m::Sprint),"default L3 is a normalized stick-click action");
 Check(f.hands[0].stick[0]==.3f&&f.hands[0].stick[1]==.9f,"mapping preserves locomotion axes exactly");
 Check(!m::Active(r.Step(),m::Sprint),"held single-press button never repeats");}
 {Rig r;r.raw.hands[1].trigger=.8f;r.raw.hands[0].squeeze=.9f;auto f=r.Step();
 Check(f.hands[1].trigger==1&&f.hands[0].squeeze==1,"default fire and support remain held independent actions");
 r.raw.hands[0].trigger=.9f;Check(r.Step().hands[0].trigger==1,"offhand trigger remains available for dual-wield power strikes");
 r.raw.hands[1].trigger=.5f;Check(m::Active(r.Step(),m::Interact),"analog mapping uses release hysteresis");
 r.raw.hands[1].trigger=.2f;Check(!m::Active(r.Step(),m::Interact),"trigger releases below threshold");}
 {Rig r;ControllerTranslator translator;auto step=[&](){auto f=r.Step();return translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);};
 step();r.raw.hands[1].stick[0]=.95f;r.raw.hands[1].stick[1]=.7f;auto c=step();
 Check(!c.held[Jump]&&c.turnX==.95f,"horizontal turn does not trigger jump");
 r.raw.hands[1].stick[0]=0;c=step();Check(c.held[Jump]&&!c.sneakToggleButton,"right stick up jumps");
 r.raw.hands[1].stick[1]=.5f;Check(!step().held[Jump],"held direction and threshold jitter do not repeat jump");
 r.raw.hands[1].stick[1]=-.9f;c=step();Check(c.sneakToggleButton&&!c.held[Jump],"right stick down emits a single sneak toggle");
 Check(!step().sneakToggleButton,"held down direction does not repeat sneak");
 r.raw.hands[1].stick[1]=0;step();r.raw.hands[1].primary=true;r.raw.hands[0].secondary=true;step();
 r.raw.hands[0].secondary=false;step();r.raw.hands[0].secondary=true;c=step();Check(!c.held[Jump]&&!c.sneakToggleButton,"old A and Y bindings no longer trigger jump or sneak");
 r.raw.hands[1].stickClick=true;step();r.raw.hands[1].stick[1]=.9f;c=step();
 Check(c.radial.visible&&!c.held[Jump]&&!c.sneakToggleButton,"radial keeps exclusive use of stick directions");}
 {Rig r;r.config.leftHanded=true;r.Step();r.Step();
 r.raw.hands[0].stick[1]=.9f;r.raw.hands[1].stick[0]=.5f;r.raw.hands[1].primary=true;r.raw.hands[0].secondary=true;
 r.raw.hands[0].grip.valid=true;r.raw.hands[0].menu=true;auto f=r.Step();
 Check(m::Pushed(f,m::Jump)&&m::Active(f,m::Inventory)&&m::Active(f,m::Weapon),"mirrored layout maps left stick up to jump, A to X and Y to B");
 Check(f.hands[0].stick[0]==.5f&&f.hands[1].stick[1]==.9f&&f.hands[0].grip.valid&&m::Active(f,m::Menu),"mirror swaps stick axes but preserves poses and application menu");
 r.raw.hands[0].stick[1]=-.9f;r.raw.hands[0].squeeze=.9f;f=r.Step();
 Check(m::Pushed(f,m::Sneak)&&m::Active(f,m::Block),"mirrored stick down and grip reach the expected actions");}
 for(bool mirrored:{false,true}){
  Rig r;r.config.leftHanded=mirrored;ControllerTranslator translator;
  auto step=[&](unsigned elapsed=16){auto f=r.Step(elapsed);return translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0,true,false,false,false,true);};
  step();step();auto& healHand=r.raw.hands[mirrored?1:0];healHand.squeeze=.9f;
  healHand.trigger=.9f;Check(!step().quickHeal,"first trigger press does not heal");healHand.trigger=0;step(40);
  healHand.trigger=.9f;Check(!step(80).quickHeal,"double trigger does not heal by default");healHand.trigger=0;step();
  healHand.secondary=true;Check(!step().quickHeal,"one secondary-button press does not heal");
  healHand.secondary=false;step(40);healHand.secondary=true;
  Check(step(80).quickHeal&&m::Active(r.Step(),m::Support),mirrored?"double B heals with mirrored controls":"double Y heals without releasing 2H support");
  Check(!step().quickHeal,"holding the second press does not repeat quick heal");
 }
 {Rig r;auto enabled=~(1u<<m::Locator);r.raw.timeMs=400;r.engine.Step(r.raw,r.config,400,false,enabled);r.raw.timeMs=416;r.engine.Step(r.raw,r.config,416,false,enabled);r.raw.hands[0].primary=true;r.raw.hands[0].trigger=.9f;r.raw.timeMs=432;
 auto f=r.engine.Step(r.raw,r.config,432,false,enabled);Check(m::Active(f,m::Inventory)&&!m::Pushed(f,m::Locator),"missing optional locator cannot consume inventory input");}
 {Rig r;r.raw.hands[0].primary=true;r.raw.hands[0].trigger=.9f;auto f=r.Step();
 Check(m::Pushed(f,m::Locator)&&!m::Active(f,m::Inventory)&&!m::Active(f,m::Heal),"locator chord consumes inventory and heal buttons");
 r.raw.hands[0].trigger=0;Check(!m::Active(r.Step(),m::Inventory),"modifier release does not leak a held primary into inventory");}
 {Rig r;r.config.bindings[m::Heal]={m::LeftTrigger,m::None,m::DoubleTap};r.Step();r.Step();
 r.raw.hands[0].trigger=.9f;r.Step();r.raw.hands[0].primary=true;r.Step();r.raw.hands[0].trigger=0;r.raw.hands[0].primary=false;r.Step();r.raw.hands[0].trigger=.9f;Check(!m::Pushed(r.Step(80),m::Heal),"locator chord cancels a pending first heal tap");}
 {Rig r;r.raw.hands[0].stickClick=r.raw.hands[1].stickClick=true;auto f=r.Step();
 Check(m::Chord(f,m::Recenter)&&!m::Pushed(f,m::Recenter)&&!m::Active(f,m::Sprint)&&!m::Active(f,m::RadialOpen),"recenter chord suppresses sprint and radial immediately");
 f=r.Step(499);Check(!m::Pushed(f,m::Recenter),"half-second chord never fires early");f=r.Step(1);Check(m::Pushed(f,m::Recenter),"half-second chord fires once at threshold");Check(!m::Pushed(r.Step(100),m::Recenter),"held recenter never repeats");}
 {Rig r;r.config.bindings[m::Menu]={m::LeftSecondary,m::LeftGrip,m::DoubleTap};r.Step();r.Step();r.raw.hands[0].squeeze=.9f;r.raw.hands[0].secondary=true;r.Step();r.raw.hands[0].secondary=false;r.Step(40);r.raw.hands[0].secondary=true;
 auto combined=r.Step(80);Check(m::Pushed(combined,m::Menu),"double taps support two-button combinations without Quest labels");
 Check(combined.hands[0].squeeze==1,"using grip as a modifier retains held weapon support");}
 {Rig r;r.raw.hands[1].trigger=.9f;Check(m::Active(r.Step(),m::Interact),"fire initially held");r.config.bindings[m::Interact]={m::RightGrip,m::None,m::Hold};auto f=r.Step();
 Check(!f.focused&&!m::Active(f,m::Interact)&&f.hands[1].trigger==0,"live remap releases old binding before applying new one");
 r.raw.hands[1].trigger=0;r.Step();r.Step();r.raw.hands[1].squeeze=.9f;Check(m::Active(r.Step(),m::Interact),"new binding works after neutral rearm");}
 {Rig r;r.raw.hands[0].squeeze=.9f;auto f=r.Step();auto old=r.raw;
 Check(!r.engine.Step(old,r.config,r.time+200).focused,"streaming stall suspends stale output");f=r.Step(300);
 Check(f.focused&&f.hands[0].squeeze==1&&!m::Pushed(f,m::Support),"held support resumes after zone stall without detach or fake press");
 r.raw.hands[0].active=false;r.raw.availableButtons=m::Bit(m::LeftGrip);Check(r.Step().hands[0].squeeze==1,"active XR button does not disappear with a temporarily untracked pose");}
 {Rig r;r.raw.hands[1].trigger=.9f;auto f=r.Step(16,true);Check(!f.focused&&f.hands[1].trigger==0,"VR capture suppresses game fire / input");
 Check(!r.Step().focused,"capture end requires physical release");r.raw.hands[1].trigger=0;r.Step();Check(r.Step().focused,"game input resumes after neutral");
 r.raw.focused=false;r.Step();r.raw.focused=true;r.raw.hands[1].trigger=.9f;Check(!r.Step().focused,"focus regain cannot fabricate a held press");}
 {Rig r;ControllerTranslator translator;auto f=r.Step();translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);r.raw.hands[0].stick[1]=1;r.raw.hands[0].stickClick=true;f=r.Step();auto c=translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);
 Check(c.held[Sprint],"mapped sprint activates on completed press while moving");r.raw.hands[0].stickClick=false;f=r.Step();c=translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);Check(c.held[Sprint],"sprint persists after mapped button release");r.raw.hands[0].stick[1]=0;f=r.Step();c=translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);Check(!c.held[Sprint],"stopping cancels mapped sprint");}
 {Rig r;ControllerTranslator translator;auto f=r.Step();translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);r.raw.hands[1].stickClick=true;f=r.Step();auto c=translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);
 Check(c.radial.visible,"mapped default radial opens through actual translator");r.raw.hands[1].stickClick=false;f=r.Step();c=translator.Translate(f,r.time,true,false,2.6f,1.4f,2,0);Check(c.radial.visible,"radial survives physical button release");}
 {wchar_t dir[MAX_PATH],file[MAX_PATH];GetTempPathW(MAX_PATH,dir);GetTempFileNameW(dir,L"g3m",0,file);m::Config config;config.leftHanded=true;config.bindings[m::Heal]={m::RightPrimary,m::LeftGrip,m::DoubleTap};
 WritePrivateProfileStringW(L"ControllerBindings",L"UnknownFutureOption",L"retain",file);
 Check(m::Save(file,config)&&m::Load(file)==config,"all bindings and gestures round-trip without changing unknown settings");wchar_t text[32];GetPrivateProfileStringW(L"ControllerBindings",L"UnknownFutureOption",L"",text,32,file);Check(wcscmp(text,L"retain")==0,"unknown mapping config remains intact");
 WritePrivateProfileStringW(L"ControllerBindings",L"Interact.Button",L"999",file);Check(m::Load(file).bindings[m::Interact]==m::Config{}.bindings[m::Interact],"invalid saved mapping falls back safely");
 m::Link game,launcher;Check(game.Open(file,true)&&launcher.Open(file),"launcher connects to this installation's live VR input");ControllerFrame raw;raw.timeMs=GetTickCount64();raw.focused=true;raw.availableButtons=7;game.Publish(raw,5);m::Telemetry info;
 Check(launcher.Read(info)&&info.buttons==5&&info.available==7&&info.focused,"bounded telemetry transfers raw buttons and availability");launcher.Capture(raw.timeMs+1000);Check(game.Capturing(raw.timeMs)&&!game.Capturing(raw.timeMs+1001),"capture lease expires automatically if launcher stops");launcher.ClearCaptured();game.Publish(raw,1);game.Publish(raw,0);launcher.Read(info);Check(info.buttons==0&&info.captured==1,"quick tap remains capturable after release between launcher polls");
 launcher.ClearCaptured();game.Publish(raw,1);game.Publish(raw,2);launcher.Read(info);Check(info.captured==1,"separate button taps are never invented into a chord");game.Publish(raw,3);launcher.Read(info);Check(info.captured==3,"simultaneous chord replaces a single captured button");
 launcher.Capture(0);Check(!game.Capturing(raw.timeMs),"capture cancellation immediately releases suppression");game.Close();launcher.Close();DeleteFileW(file);}
 return failures?1:0;
}
