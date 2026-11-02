#include "LocatorIntegration.h"
#include "ControllerControls.h"
#include "ControllerInput.h"
#include "LocatorNativeInput.h"
#include "LocatorProjection.h"
#include <cstdio>
#include <limits>
using namespace diagnostic;
int wmain(int argc,wchar_t** argv){
 unsigned failures=0;auto check=[&](bool ok,char const* text){printf("%s %s\n",ok?"PASS":"FAIL",text);failures+=!ok;};
 unsigned char unionHash[]={0xbf,0x92,0xc2,0x7b,0xad,0xe1,0x92,0x8d,0x30,0x05,0x4c,0xfe,0xf4,0x0d,0x95,0xe8,0x36,0xa6,0x76,0x42,0xa8,0xa9,0xa2,0x97,0x87,0x00,0xf5,0xab,0x8c,0xf8,0x5b,0x52};
 auto profile=LocatorProfile(unionHash);check(profile.object==0x6cc30&&profile.key==0x46010&&profile.projection==0x460b0,"Union+ 1.0.8 exact binary layout");unionHash[0]^=1;check(!LocatorProfile(unionHash).object,"unknown Locator build skipped");
 LocatorNativePulse configuredPulse;configuredPulse.Queue(1000,21);check(!configuredPulse.Read(100,1001)&&configuredPulse.Read(21,1002),"Union+ native Y hotkey accepted without rewriting user INI");
 LocatorNativePulse nativePulse;nativePulse.Queue(1000);check(!nativePulse.Read(20,1001)&&nativePulse.Read(100,1010),"native adapter delivers only F13 to original mod's buffered-key query");
 check(!nativePulse.Read(100,1020)&&!nativePulse.Read(100,1030),"native query consumes one press and observes a release");nativePulse.Queue(1040);check(nativePulse.Read(100,1050),"second controller press can toggle again");nativePulse.Queue(1060);check(!nativePulse.Read(100,1070)&&nativePulse.Read(100,1080),"rapid presses include a native release edge");nativePulse.Cancel();check(!nativePulse.Read(100,1090),"context loss cancels pending native input");nativePulse.Queue(1100);check(!nativePulse.Read(100,3000),"delayed queries cannot toggle after expiry");
 LocatorNativeInput bridge;check(!bridge.Install(GetModuleHandleW(nullptr),reinterpret_cast<void*>(&wmain)),"native adapter refuses every module except hash-verified original locator");
 {
  LocatorProjection projection;float left[16]{},right[16]{},matrix[16]{};left[0]=left[5]=left[10]=left[15]=right[0]=right[5]=right[10]=right[15]=1;left[12]=3.2f;right[12]=-3.2f;
  bool raysMatch=true;
  for(float width:{1.3f,2.6f,5.2f})for(float distance:{1.f,2.f,4.f})for(float vertical:{-.4f,0.f,.5f}){
   raysMatch&=projection.Publish(left,right,3840,2160,width,distance,vertical,1000)&&projection.Read(1010,matrix);
   for(float x:{-150.f,0.f,120.f})for(float y:{-60.f,0.f,80.f})for(float z:{200.f,1000.f}){
    float p[]={x,y,z,1},clip[4]{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)clip[c]+=p[r]*matrix[r*4+c];
    float u=clip[0]/clip[3]/3840.f,v=clip[1]/clip[3]/2160.f,height=width*2160.f/3840.f;
    float quadX=(u-.5f)*width,quadY=(.5f-v)*height+vertical;
    raysMatch&=std::fabs(quadX/distance-x/z)<.00001f&&std::fabs(quadY/distance-y/z)<.00001f;
   }
  }
  check(raysMatch,"locator pixels reconstruct the correct centered head ray across HUD scales, distances, heights and object depths");
  left[0]=right[0]=.8660254f;left[2]=right[2]=-.5f;left[8]=right[8]=.5f;left[10]=right[10]=.8660254f;
  left[12]=8.2f;right[12]=1.8f;left[13]=right[13]=20;left[14]=right[14]=-10;
  projection.Publish(left,right,1920,1080,2.6f,2.f,-.3f,1000);projection.Read(1001,matrix);
  float point[]={0,0,1000,1},projected[4]{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)projected[c]+=point[r]*matrix[r*4+c];
  float u=projected[0]/projected[3]/1920.f,v=projected[1]/projected[3]/1080.f;
  check(std::fabs((u-.5f)*2.6f/2.f-505.f/856.0254f)<.00001f&&std::fabs(((.5f-v)*1.4625f-.3f)/2.f-20.f/856.0254f)<.00001f,"rendered head yaw and translation are retained instead of desktop camera orientation");
  check(!projection.Read(2000,matrix),"stale VR projection falls back to the native camera");
  right[0]=.9f;check(!projection.Publish(left,right,3840,2160,2.6f,2,0,2000)&&!projection.Read(2000,matrix),"unmatched eye rotations cannot publish a skewed center matrix");
  right[0]=1;left[0]=std::numeric_limits<float>::quiet_NaN();check(!projection.Publish(left,right,3840,2160,2.6f,2,0,2000),"invalid eye data is rejected");
  left[0]=1;check(!projection.Publish(left,right,0,2160,2.6f,2,0,2000),"unknown native viewport does not change locator placement");
  projection.Publish(left,right,3840,2160,2.6f,2,0,2000);projection.Reset();check(!projection.Read(2001,matrix),"world stop / flat mode restores native projection immediately");
 }
 ControllerFrame f{};f.focused=true;f.timeMs=1000;for(auto& h:f.hands)h.active=true;ControllerTranslator translator;
 auto step=[&](bool enabled=true,bool menu=false,bool fresh=true){f.timeMs+=10;return translator.Translate(f,f.timeMs+(fresh?0:200),true,menu,2,1,2,0,true,false,false,false,enabled);};
 step();step();f.hands[0].trigger=1;step();f.hands[0].primary=true;auto out=step();
 check(out.pulse==LocatorToggle&&!out.held[Inventory]&&!out.quickHeal,"trigger + X emits one locator pulse and consumes inventory / heal");
 bool held=true;for(int i=0;i<20;++i){out=step();held&=out.pulse<0&&!out.held[Inventory]&&!out.quickHeal;}check(held,"held chord never repeats or heals");
 f.hands[0].trigger=0;out=step();check(!out.held[Inventory],"releasing modifier first cannot leak inventory");f.hands[0].primary=false;for(int i=0;i<5;++i)step();
 f.hands[0].primary=true;check(step().held[Inventory],"plain X keeps inventory");f.hands[0].primary=false;step();f.hands[0].trigger=1;step();f.hands[0].primary=true;
 check(step(false).pulse<0,"uninstalled locator emits no shortcut");check(step(true,true).pulse<0,"menus cannot toggle locator");check(step(true,false,false).pulse<0,"stale tracking emits nothing");check(step().pulse<0,"held X after tracking recovery needs a fresh press");
 f.hands[0].primary=false;for(int i=0;i<5;++i)step();f.hands[0].primary=true;check(step().pulse==LocatorToggle,"fresh X rearms after recovery");
 f.hands[0].primary=false;f.hands[0].trigger=0;for(int i=0;i<50;++i)step();f.hands[0].trigger=1;check(!step().quickHeal,"locator does not leave a pending heal tap");for(int i=0;i<4;++i)step();f.hands[0].trigger=0;for(int i=0;i<4;++i)step();f.hands[0].trigger=1;check(!step().quickHeal,"heal second press waits for release so a late X can cancel it");f.hands[0].trigger=0;check(step().quickHeal,"ordinary double left trigger still heals on second release");
 for(int i=0;i<50;++i)step();f.hands[0].trigger=1;step();for(int i=0;i<4;++i)step();f.hands[0].trigger=0;for(int i=0;i<4;++i)step();f.hands[0].trigger=1;step();f.hands[0].primary=true;out=step();check(out.pulse==LocatorToggle&&!out.quickHeal,"late X on a second trigger press cancels the pending potion");f.hands[0].trigger=0;check(!step().quickHeal,"modifier release cannot revive a canceled heal");
 ControllerInput input;input.Initialize(L"missing.ini");out={};out.pulse=LocatorToggle;std::vector<bool> edges;auto emit=[&](unsigned type,unsigned key,bool down){check(type==0&&key==0x64,"locator uses isolated F13 scancode");edges.push_back(down);return true;};
 input.Update(out,1000,true,emit);input.Update({},1010,true,emit);input.Update({},1080,true,emit);check(edges==std::vector<bool>{true,false},"F13 pulse releases after 80 ms");
 if(argc<2)return 1;namespace fs=std::filesystem;auto fixture=fs::absolute(argv[1])/(L"run-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));fs::create_directories(fixture);locator::testRoot=fixture/L"state";auto game=fixture/L"game";fs::create_directories(game);
 auto rejected=[&](auto action){try{action();return false;}catch(std::exception const&){return true;}};
 auto write=[&](fs::path p,char const* text){fs::create_directories(p.parent_path());std::ofstream(p,std::ios::binary)<<text;};
 check(locator::Allowed(locator::Dll)&&locator::Allowed(L"Data/_compiledImage/eCDELocator/ICON_TEST.png")&&!locator::Allowed(L"Data/anything.dll"),"import/recovery paths are restricted to locator files");
 check(rejected([&]{locator::Allowed(L"../outside.dll");}),"traversal paths rejected");write(fixture/L"bad"/locator::Dll,"not the original DLL");check(rejected([&]{locator::Import(game,fixture/L"bad");})&&!fs::exists(game/locator::Dll),"invalid download cannot modify game");
 auto session=locator::Root(game)/L"interrupted";auto journal=session/L"transaction.ini";write(game/locator::Ini,"changed");write(session/L"before"/locator::Ini,"original");
 optional_mods::Put(journal,L"Transaction",L"Game",optional_mods::Full(game));optional_mods::Put(journal,L"Transaction",L"Count",L"1");optional_mods::Put(journal,L"File0",L"Path",locator::Ini);optional_mods::Put(journal,L"File0",L"Existed",L"1");optional_mods::Put(journal,L"File0",L"SHA256",optional_mods::Hash(session/L"before"/locator::Ini));optional_mods::Put(locator::Root(game)/L"pending.ini",L"Pending",L"Session",L"interrupted");
 locator::Recover(game);check(!locator::Pending(game)&&optional_mods::Hash(game/locator::Ini)==optional_mods::Hash(session/L"before"/locator::Ini),"interrupted transaction restores verified original config");
 if(argc>2){auto download=fs::path(argv[2]);try{auto sourceHash=optional_mods::Hash(download/locator::Dll);locator::Import(game,download);check(locator::Imported(game)&&!fs::exists(game/locator::Dll),"real download imports disabled into external cache");
  locator::Settings s;s.enabled=true;s.distance=2250;for(auto group:{locator::Plants,locator::Npcs,locator::Creatures,locator::QuestTargets})s.groups[group]=false;locator::Apply(game,s);
  check(optional_mods::Hash(game/locator::Dll)==sourceHash&&optional_mods::Get(game/locator::Ini,L"Main",L"eCDELocatorToggleHotkey")==L"F13","original DLL installs unchanged with F13 config");
  auto loaded=locator::Load(game);bool same=loaded.enabled&&loaded.distance==2250;for(unsigned group=0;group<locator::GroupCount;++group)same&=loaded.groups[group]==s.groups[group];check(same,"all nine independent category filters round-trip");
  bool hidden=true;for(auto key:{L"ShowItemPlant",L"ShowItemPlantPerm",L"ShowSmallAndFarmAnimals",L"ShowHumanHostile",L"ShowHumanPeaceOrAngry",L"ShowOrcHostile",L"ShowOrcPeaceOrAngry",L"ShowUndeadHostile",L"ShowUndeadPeace",L"ShowMonsterHostile",L"ShowMonsterPeace",L"ShowQuestKillTargets"})hidden&=optional_mods::Get(game/locator::Ini,L"Filter",key)==L"false";
  check(hidden,"plants, living NPCs, animals, monsters and quest-kill override can all be hidden");
  bool kept=true;for(auto key:{L"ShowItemMeleeWeapon",L"ShowItemRangedWeapon",L"ShowItemMunition",L"ShowItemScroll",L"ShowItemMisc",L"ShowItemFood",L"ShowItemDoc",L"ShowItemPotion",L"ShowItemStoneTablet",L"ShowChest",L"ShowInteractBookstand",L"ShowInteractOre",L"ShowNpcDeadNotEmpty",L"ShowQuestItems"})kept&=optional_mods::Get(game/locator::Ini,L"Filter",key)==L"true";
  check(kept,"ordinary loot, chests, interactables, lootable corpses and quest items stay enabled");
  auto names=optional_mods::Collect(game);check(names.size()==61,"Union protection includes locator DLL, config and all icons");
  {optional_mods::LockSet protection;protection.Acquire(game,names);check(!DeleteFileW((game/locator::Dll).c_str()),"protected Union session cannot delete locator DLL");check(rejected([&]{s.distance=2500;locator::Apply(game,s);}),"locked DLL refuses config changes before mutation");}
  check(locator::Load(game).distance==2250,"failed apply preserves previous configuration");
  s.enabled=false;locator::Apply(game,s);check(!fs::exists(game/locator::Dll)&&fs::exists(game/locator::Ini),"disable unloads next launch and keeps calibrated options");s.enabled=true;locator::Apply(game,s);check(fs::exists(game/locator::Dll),"cached mod can be re-enabled without re-download");
  check(optional_mods::Hash(download/locator::Dll)==sourceHash,"downloaded original is untouched");
 }catch(std::exception const& error){printf("FAIL real import: %s\n",error.what());++failures;}}
 printf("Locator failures: %u\n",failures);return failures?1:0;
}
