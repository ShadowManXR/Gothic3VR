#pragma once
// Launcher-only native INI editing. No game hooks or per-frame work.
#include <windows.h>
#include <shlobj.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace diagnostic::ini_tweaks {
namespace fs=std::filesystem;
enum class Kind { Number, Choice, Toggle };
inline constexpr unsigned ShaderCache=12, TestMode=13;
inline constexpr bool GameIniField(unsigned i){return i<3||i==TestMode;}
struct Field {wchar_t const* label;int minimum,maximum,step;Kind kind;wchar_t const* hint;};
inline constexpr Field Fields[]={
 {L"High-detail draw distance",1000,50000,500,Kind::Number,L"Detailed world and objects. Increasing this can add substantial CPU and GPU work. Edits the active native detail tier."},
 {L"Distant landscape distance",10000,300000,1000,Kind::Number,L"Low-poly distant scenery. Separate from nearby detail and NPC simulation. Disabled when distant scenery is Off in game settings."},
 {L"Vegetation draw distance",1000,30000,250,Kind::Number,L"Grass and vegetation visibility. More distant vegetation costs additional rendering work. Edits the active vegetation range tier."},
 {L"Detail streaming grid",1000,50000,500,Kind::Number,L"Render.PrefetchGridCellSize: native streaming cell size, not a RAM allowance. Larger is not automatically faster."},
 {L"Distant streaming grid",10000,300000,1000,Kind::Number,L"Render.PrefetchGridCellSizeLowPoly: distant-world streaming cell size. Large values can increase loading and memory pressure."},
 {L"NPC / creature ROI",500,20000,250,Kind::Number,L"Entity.ROI: dynamic entity processing radius, including NPCs and creatures. Higher values can be CPU-heavy. The engine caps it to the detailed-world far plane."},
 {L"Engine frame-rate limit",10,240,1,Kind::Number,L"FpS.Max: native frame-rate cap. Raising a cap does not guarantee more FPS. Headset and runtime limits still apply."},
 {L"Vegetation density",0,3,1,Kind::Choice,L"Native VegetationQuality. Lower levels reduce vegetation detail/density; the exact result depends on the installed assets."},
 {L"Shadow quality",0,4,1,Kind::Choice,L"Native ShadowQuality. Reducing it can lower rendering cost and changes shadow appearance."},
 {L"Texture filtering",0,4,1,Kind::Choice,L"Native TextureFilter: Linear or anisotropic 2x, 4x, 8x, 16x. Improves oblique surface clarity; does not change texture resolution."},
 {L"Depth of field",0,1,1,Kind::Toggle,L"Native DepthOfField post effect. Turn off to keep the view sharp instead of adding focus blur."},
 {L"Film grain / noise",0,1,1,Kind::Toggle,L"Native Noise post effect. Turn off for a cleaner headset image."},
 {L"Native shader cache",0,1,1,Kind::Toggle,L"Inverse of Render.DisableShaderCache. Keep enabled for native shader caching. Resource/RAM caches are managed automatically by CP 1.75."},
 {L"Test mode",0,1,1,Kind::Toggle,L"Sets [Game] TestMode. Applies the next time Gothic 3 starts."}
};
inline constexpr unsigned Count=sizeof(Fields)/sizeof(Fields[0]);
inline constexpr wchar_t const* NativeOptionKeys[]={L"DistanceHigh",L"DistanceLow",L"ObjectDetails",L"TextureQuality",L"TextureFilter",L"VegetationQuality",L"VegetationViewRange",L"ShadowQuality",L"LensFlare",L"Bloom",L"DepthOfField",L"Antialiasing",L"Noise",L"Feedback",L"ColorGrading"};
inline std::wstring Read(fs::path const& file,wchar_t const* section,wchar_t const* key,wchar_t const* fallback=L""){
 wchar_t text[2048]{};GetPrivateProfileStringW(section,key,fallback,text,2048,file.c_str());return text;
}
inline bool Integer(std::wstring const& raw,int& out){
 wchar_t* end=nullptr;double v=wcstod(raw.c_str(),&end);
 if(end==raw.c_str()||*end||!std::isfinite(v)||v<-100000000||v>100000000)return false;
 out=int(std::round(v));return true;
}
inline int Snap(unsigned i,int value){auto const& f=Fields[i];return std::clamp(f.minimum+int(std::round(double(value-f.minimum)/f.step))*f.step,f.minimum,f.maximum);}
struct Binding {bool user=false;std::wstring section,key,raw;};
struct Model {
 fs::path game,user;std::array<Binding,Count> bindings{};
 std::array<int,Count> values{},original{};std::array<bool,Count> available{};
 std::map<std::wstring,std::wstring> options;
 std::wstring performance;int highTier=3,lowTier=3,vegetationTier=4;bool ready=false,forceNativeDistances=false;
 bool Changed(unsigned i)const{return available[i]&&values[i]!=original[i];}
 bool Changed()const{for(unsigned i=0;i<Count;++i)if(Changed(i))return true;return forceNativeDistances;}
 bool DistancesChanged()const{for(unsigned i=0;i<5;++i)if(Changed(i))return true;return forceNativeDistances;}
};
inline Model Load(fs::path const& gameFile,fs::path const& userFile){
 Model m;m.game=gameFile;m.user=userFile;
 if(!fs::is_regular_file(gameFile)||!fs::is_regular_file(userFile))return m;
 WritePrivateProfileStringW(nullptr,nullptr,nullptr,gameFile.c_str());WritePrivateProfileStringW(nullptr,nullptr,nullptr,userFile.c_str());
 m.performance=Read(userFile,L"Options.Details",L"Performance",L"Custom");
 bool custom=m.performance==L"Custom";
 bool known=custom||m.performance==L"Low"||m.performance==L"Medium"||m.performance==L"High"||m.performance==L"VeryHigh";
 if(!known)return m;
 for(auto key:NativeOptionKeys)m.options[key]=Read(custom?userFile:gameFile,custom?L"Options.Custom":(L"Options."+m.performance).c_str(),key);
 auto option=[&](wchar_t const* key,int fallback){int n;if(!Integer(m.options[key],n))return fallback;return n;};
 m.highTier=std::clamp(option(L"DistanceHigh",3),1,3);m.lowTier=std::clamp(option(L"DistanceLow",3),0,3);m.vegetationTier=std::clamp(option(L"VegetationViewRange",4),1,4);
 wchar_t const* tiers[]={L"Low",L"Low",L"Medium",L"High",L"VeryHigh"};
 m.bindings[0]={false,L"Option.Sliders",L"DistanceHigh.fFarClippingPlane_"+std::wstring(tiers[m.highTier]),{}};
 m.bindings[1]={false,L"Option.Sliders",L"DistanceLow.fFarClippingPlaneLowPolyMesh_"+std::wstring(tiers[m.lowTier]),{}};
 m.bindings[2]={false,L"Option.Sliders",L"VegetationViewRange.fViewDistance_"+std::wstring(tiers[m.vegetationTier]),{}};
 m.bindings[3]={false,L"Engine.Setup",L"Render.PrefetchGridCellSize",{}};
 m.bindings[4]={false,L"Engine.Setup",L"Render.PrefetchGridCellSizeLowPoly",{}};
 m.bindings[5]={false,L"Engine.Setup",L"Entity.ROI",{}};
 m.bindings[6]={false,L"Engine.Setup",L"FpS.Max",{}};
 m.bindings[7]={true,L"Options.Custom",L"VegetationQuality",{}};
 m.bindings[8]={true,L"Options.Custom",L"ShadowQuality",{}};
 m.bindings[9]={true,L"Options.Custom",L"TextureFilter",{}};
 m.bindings[10]={true,L"Options.Custom",L"DepthOfField",{}};
 m.bindings[11]={true,L"Options.Custom",L"Noise",{}};
 m.bindings[12]={false,L"Engine.Setup",L"Render.DisableShaderCache",{}};
 m.bindings[TestMode]={false,L"Game",L"TestMode",{}};
 for(unsigned i=0;i<Count;++i){auto& b=m.bindings[i];b.raw=b.user?m.options[b.key]:Read(gameFile,b.section.c_str(),b.key.c_str(),i==TestMode?L"false":L"");
  int value=0;bool valid;
  if(i==ShaderCache||i==TestMode){
   const bool enabled=!_wcsicmp(b.raw.c_str(),L"true");
   const bool disabled=!_wcsicmp(b.raw.c_str(),L"false");
   valid=enabled||disabled;value=i==ShaderCache?disabled:enabled;
  }else valid=Integer(b.raw,value);
  m.values[i]=m.original[i]=value;m.available[i]=valid&&value>=Fields[i].minimum&&value<=Fields[i].maximum&&(i!=1||m.lowTier!=0);
 }
 m.ready=true;return m;
}
inline void Defaults(Model& m){
 std::array<int,Count> v={20000,200000,10000,10000,34000,10000,120,3,4,4,1,1,1,0};
 for(unsigned i=0;i<Count;++i)if(m.available[i])m.values[i]=v[i];
 m.forceNativeDistances=m.ready&&Read(m.game,L"Gothic3VR.IniTweaks",L"OwnDistances")!=L"1";
}
inline fs::path BackupRoot(fs::path const& gameFile){
 wchar_t local[MAX_PATH]{};if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,local)))return {};
 uint64_t hash=14695981039346656037ull;for(wchar_t c:fs::absolute(gameFile).wstring()){hash^=uint16_t(towlower(c));hash*=1099511628211ull;}
 wchar_t suffix[32];swprintf_s(suffix,L"%016llx",hash);return fs::path(local)/L"Gothic3VR"/L"IniTweaks"/suffix;
}
inline std::vector<char> Bytes(fs::path const& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read INI");return {std::istreambuf_iterator<char>(f),{}};}
inline bool Linked(fs::path p){for(;!p.empty()&&p!=p.parent_path();p=p.parent_path()){auto a=GetFileAttributesW(p.c_str());if(a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT))return true;}return false;}
inline fs::path NewBackupSession(fs::path const& base,std::wstring const& stamp){
 fs::create_directories(base);
 // Several saves can share the same Windows clock tick. Claim each folder
 // atomically so an existing session is never reused or overwritten.
 for(unsigned attempt=0;attempt<1024;++attempt){
  auto path=base/(stamp+L"-"+std::to_wstring(attempt));
  if(CreateDirectoryW(path.c_str(),nullptr))return path;
  if(GetLastError()!=ERROR_ALREADY_EXISTS)throw std::runtime_error("Could not create the external INI backup folder.");
 }
 throw std::runtime_error("Could not reserve a unique external INI backup folder.");
}
struct Result {bool ok=false,changed=false;std::wstring error;fs::path backup;};
// Optional commit observer exists for deterministic filesystem failure tests;
// the launcher always uses the default null observer.
using CommitObserver=void(*)(unsigned,fs::path const&,void*);
inline Result Save(Model const& m,fs::path const& backupBase={},CommitObserver observer=nullptr,void* context=nullptr){
 Result r;if(!m.ready){r.error=L"Run the flat game once to create valid native INI files, then reload.";return r;}
 if(!m.Changed()){r.ok=true;return r;}
 std::array<fs::path,2> files={m.game,m.user},staged{},saved{};
 std::array<bool,2> needed{};std::array<std::vector<char>,2> before;
 bool preset=false;
 for(unsigned i=0;i<Count;++i)if(m.Changed(i)){
  if(m.values[i]<Fields[i].minimum||m.values[i]>Fields[i].maximum){r.error=L"An INI value is outside its supported range.";return r;}
  needed[m.bindings[i].user?1:0]=true;
  if(i!=TestMode&&m.performance!=L"Custom")preset=true;
 }
 if(m.forceNativeDistances){needed[0]=true;preset=m.performance!=L"Custom";}
 if(preset)needed[1]=true;
 if(m.lowTier&&m.values[1]<=m.values[0]&&(m.Changed(0)||m.Changed(1))){r.error=L"Distant landscape distance must exceed high-detail distance.";return r;}
 unsigned committed=0;
 try{
  if(Read(m.user,L"Options.Details",L"Performance",L"Custom")!=m.performance)throw std::runtime_error("Native graphics preset changed. Reload INIs first.");
  auto current=Load(m.game,m.user);if(!current.ready||current.highTier!=m.highTier||current.lowTier!=m.lowTier||current.vegetationTier!=m.vegetationTier)throw std::runtime_error("Native detail tier changed. Reload INIs first.");
  for(unsigned i=0;i<Count;++i)if(m.Changed(i)&&current.bindings[i].raw!=m.bindings[i].raw)throw std::runtime_error("A value changed outside the launcher. Reload INIs first.");
  auto base=backupBase.empty()?BackupRoot(m.game):backupBase;
  if(base.empty()||Linked(base))throw std::runtime_error("Backup folder is unavailable or linked.");
  auto gameRoot=fs::weakly_canonical(m.game.parent_path().parent_path()),canonicalBase=fs::weakly_canonical(base);
  auto rel=canonicalBase.lexically_relative(gameRoot);if(!rel.empty()&&*rel.begin()!=L"..")throw std::runtime_error("Backups must stay outside the game directory.");
  SYSTEMTIME t;GetLocalTime(&t);wchar_t stamp[80];swprintf_s(stamp,L"%04u%02u%02u-%02u%02u%02u-%llu-%lu",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetTickCount64(),GetCurrentProcessId());
  r.backup=NewBackupSession(base,stamp);
  for(unsigned f=0;f<2;++f)if(needed[f]){
   auto attr=GetFileAttributesW(files[f].c_str());if(attr==INVALID_FILE_ATTRIBUTES||(attr&FILE_ATTRIBUTE_READONLY)||Linked(files[f]))throw std::runtime_error("INI is missing, read-only or linked.");
   HANDLE access=CreateFileW(files[f].c_str(),GENERIC_READ|GENERIC_WRITE|DELETE,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
   if(access==INVALID_HANDLE_VALUE)throw std::runtime_error("Close the game or editor that is locking the INI.");CloseHandle(access);
   before[f]=Bytes(files[f]);saved[f]=r.backup/(f?L"UserOptions.ini":L"ge3.ini");
   if(!CopyFileW(files[f].c_str(),saved[f].c_str(),TRUE)||Bytes(saved[f])!=before[f])throw std::runtime_error("Could not verify the external INI backup.");
   wchar_t temp[MAX_PATH]{};if(!GetTempFileNameW(files[f].parent_path().c_str(),L"gti",0,temp))throw std::runtime_error("Could not stage an INI file.");staged[f]=temp;
   if(!CopyFileW(files[f].c_str(),staged[f].c_str(),FALSE))throw std::runtime_error("Could not stage the original INI.");
  }
  auto write=[&](unsigned f,wchar_t const* section,wchar_t const* key,std::wstring const& value){if(!WritePrivateProfileStringW(section,key,value.c_str(),staged[f].c_str()))throw std::runtime_error("Could not write the staged INI.");};
  if(preset){for(auto const& p:current.options)if(!p.second.empty())write(1,L"Options.Custom",p.first.c_str(),p.second);write(1,L"Options.Details",L"Performance",L"Custom");}
  for(unsigned i=0;i<Count;++i)if(m.Changed(i)){
   auto const& b=m.bindings[i];auto value=std::to_wstring(m.values[i]);
   if(i==ShaderCache||i==TestMode)value=(m.values[i]!=0)==(i==TestMode)?L"true":L"false";
   write(b.user?1:0,b.section.c_str(),b.key.c_str(),value);
  }
  if(m.DistancesChanged())write(0,L"Gothic3VR.IniTweaks",L"OwnDistances",L"1");
  for(unsigned f=0;f<2;++f)if(needed[f])WritePrivateProfileStringW(nullptr,nullptr,nullptr,staged[f].c_str());
  for(unsigned f=0;f<2;++f)if(needed[f]&&Bytes(files[f])!=before[f])throw std::runtime_error("An INI changed during saving. Reload INIs first.");
  for(unsigned f=0;f<2;++f)if(needed[f]){
   if(observer)observer(f,files[f],context);
   if(!MoveFileExW(staged[f].c_str(),files[f].c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not replace a staged INI.");
   staged[f].clear();committed|=1u<<f;WritePrivateProfileStringW(nullptr,nullptr,nullptr,files[f].c_str());
  }
  r.ok=r.changed=true;
 }catch(std::exception const& e){
  bool restored=true;for(unsigned f=0;f<2;++f)if(committed&(1u<<f)){
   wchar_t temp[MAX_PATH]{};bool ok=GetTempFileNameW(files[f].parent_path().c_str(),L"gtr",0,temp)&&CopyFileW(saved[f].c_str(),temp,FALSE)&&MoveFileExW(temp,files[f].c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
   if(!ok)DeleteFileW(temp);restored&=ok;WritePrivateProfileStringW(nullptr,nullptr,nullptr,files[f].c_str());
  }
  int n=MultiByteToWideChar(CP_UTF8,0,e.what(),-1,nullptr,0);std::wstring message(n,L'\0');MultiByteToWideChar(CP_UTF8,0,e.what(),-1,message.data(),n);if(!message.empty())message.pop_back();r.error=message;
  if(!restored)r.error+=L" Restore the original INIs from the external backup: "+r.backup.wstring();
 }
 for(auto const& p:staged)if(!p.empty())DeleteFileW(p.c_str());return r;
}
}
