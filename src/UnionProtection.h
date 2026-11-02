#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <cwctype>
#include <array>
namespace diagnostic::optional_mods {
namespace fs=std::filesystem;
inline void Require(bool ok,char const* message){if(!ok)throw std::runtime_error(message);}
inline std::wstring Lower(std::wstring text){for(auto& c:text)c=wchar_t(towlower(c));return text;}
inline std::wstring Full(fs::path const& p){return Lower(fs::absolute(p).lexically_normal().wstring());}
inline bool Below(fs::path const& p,fs::path const& root){auto r=Full(root);if(r.back()!=L'\\')r+=L'\\';return Full(p).rfind(r,0)==0;}
inline fs::path Relative(std::wstring const& name){fs::path p(name);p.make_preferred();Require(!p.empty()&&!p.is_absolute()&&!p.has_root_name(),"Invalid protected relative path");
 for(auto const& part:p){auto s=part.wstring();Require(!s.empty()&&s.back()!=L'.'&&s.back()!=L' '&&part!=L".."&&part!=L"."&&s.find(L':')==std::wstring::npos,"Invalid protected path component");}return p;}
inline void NoLinks(fs::path const& p){auto full=fs::absolute(p).lexically_normal();fs::path walk;for(auto const& part:full){walk/=part;auto a=GetFileAttributesW(walk.c_str());if(a!=INVALID_FILE_ATTRIBUTES)Require(!(a&FILE_ATTRIBUTE_REPARSE_POINT),"Linked game/backup paths are not supported by protection");}}
inline std::wstring Hash(fs::path const& path){HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);Require(f!=INVALID_HANDLE_VALUE,"Cannot read file for checksum");
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD n=0,size=0;std::vector<unsigned char> object;std::array<unsigned char,32> digest{};bool ok=false;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0&&BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),4,&n,0)>=0){object.resize(size);
  if(BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0){std::array<unsigned char,65536> buffer;ok=true;
   for(;;){DWORD read=0;if(!ReadFile(f,buffer.data(),DWORD(buffer.size()),&read,nullptr)){ok=false;break;}if(!read)break;if(BCryptHashData(hash,buffer.data(),read,0)<0){ok=false;break;}}
   if(ok)ok=BCryptFinishHash(hash,digest.data(),DWORD(digest.size()),0)>=0;}}
 if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(f);Require(ok,"Checksum failed");
 std::wstring out;for(auto b:digest){out+=L"0123456789ABCDEF"[b>>4];out+=L"0123456789ABCDEF"[b&15];}return out;}
inline std::vector<std::wstring> const& CoreNames(){static std::vector<std::wstring> names={
 L"Gothic3.exe",L"Engine.dll",L"Game.dll",L"SharedBase.dll",L"Script.dll",L"GFC.dll",L"GUI.dll",L"FileSystem.dll",L"fmod.dll",L"fmodex.dll",L"SCM.dll",
 L"paru_s1.dll",L"paru_s1_opt.dll",L"paru_s2.dll",L"d3d9.dll",L"dxvk.conf",L"G3_edd.ini",L"Data\\edd_mount.dat",
 L"Gothic3VRLauncher.exe",L"Gothic3VRLauncher.ini",L"Gothic3VRUnionGuard.exe",L"splash.bmp",L"Ini\\logo.ini",L"Ini\\ge3.ini",L"Ini\\mountlist.ini",
 L"scripts\\Script_Game.dll",L"scripts\\Script_paru.dll",L"scripts\\Script_NewGameConfiguration.dll",L"scripts\\Script_Gothic3VRDiagnostics.dll",L"scripts\\Script_Gothic3VRDiagnostics.ini",L"patches\\Patch_Gothic3VRResolution.dll",
 L"scripts\\Script_eCDELocator.dll",L"Ini\\eCDELocator.ini"};return names;}
inline bool Allowed(std::wstring const& name){auto key=Lower(Relative(name).wstring());for(auto const& n:CoreNames())if(Lower(n)==key)return true;
 auto asset=Relative(name);if(Lower(asset.parent_path().generic_wstring())==L"data/_compiledimage/ecdelocator"&&Lower(asset.filename().wstring()).rfind(L"icon_",0)==0&&Lower(asset.extension().wstring())==L".png")return true;
 auto prefix=std::wstring(L"scripts\\gothic3vrdiagnostics\\");return key.rfind(prefix,0)==0&&key.find(L"\\logs\\")==std::wstring::npos&&key.find(L"\\captures\\")==std::wstring::npos;}
inline bool EditableIni(std::wstring const& name){auto key=Lower(Relative(name).wstring());return key==L"ini\\ge3.ini"||key==L"ini\\mountlist.ini";}
inline bool RecoverableStartupAsset(std::wstring const& name){auto key=Lower(Relative(name).wstring());return key==L"splash.bmp"||key==L"ini\\logo.ini";}
inline std::vector<std::wstring> Collect(fs::path const& game){NoLinks(game);std::set<std::wstring> names;
 for(auto const& name:CoreNames())if(Lower(name)!=L"scripts\\script_ecdelocator.dll"&&Lower(name)!=L"ini\\ecdelocator.ini"&&fs::is_regular_file(game/name))names.insert(name);

 auto runtime=game/L"scripts/Gothic3VRDiagnostics";if(fs::is_directory(runtime))for(fs::recursive_directory_iterator it(runtime),end;it!=end;++it){
  NoLinks(it->path());if(it->is_directory()&&(Lower(it->path().filename().wstring())==L"logs"||Lower(it->path().filename().wstring())==L"captures")){it.disable_recursion_pending();continue;}
  if(it->is_regular_file()){auto relative=it->path().lexically_relative(game).wstring();if(Allowed(relative))names.insert(relative);}}
 return {names.begin(),names.end()};}
inline std::wstring Get(fs::path const& ini,wchar_t const* section,wchar_t const* key){wchar_t text[32768]{};auto n=GetPrivateProfileStringW(section,key,L"",text,32768,ini.c_str());Require(n<32767,"Protection journal value too long");return text;}
inline void Put(fs::path const& ini,wchar_t const* section,wchar_t const* key,std::wstring const& value){
 if(GetFileAttributesW(ini.c_str())==INVALID_FILE_ATTRIBUTES){HANDLE file=CreateFileW(ini.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(file!=INVALID_HANDLE_VALUE){wchar_t bom=0xfeff;DWORD written=0;bool ok=WriteFile(file,&bom,2,&written,nullptr)&&written==2;CloseHandle(file);Require(ok,"Cannot initialize Unicode journal");}
  else Require(GetLastError()==ERROR_FILE_EXISTS,"Cannot create protection journal");}
 Require(WritePrivateProfileStringW(section,key,value.c_str(),ini.c_str())!=FALSE,"Cannot write protection journal");}
inline fs::path StateRoot(fs::path const& game){wchar_t local[MAX_PATH]{};Require(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,local)),"Cannot locate local application data");
 // A stable path key avoids sharing recovery state between game installations.
 uint64_t key=14695981039346656037ull;for(auto c:Full(game)){key^=uint64_t(c);key*=1099511628211ull;}wchar_t name[32];swprintf_s(name,L"%016llx",key);
 return fs::path(local)/L"Gothic3VR"/L"UnionProtection"/name;}
inline std::wstring MutexName(fs::path const& game){return L"Local\\Gothic3VR-Union-"+StateRoot(game).filename().wstring();}
inline bool Active(fs::path const& game){auto name=MutexName(game);HANDLE h=OpenMutexW(SYNCHRONIZE,FALSE,name.c_str());
 // Only observe existence: a read-only observer must not acquire ownership.
 // An elevated guard whose mutex cannot be opened also blocks game launch.
 if(h){CloseHandle(h);return true;}auto error=GetLastError();if(error==ERROR_ACCESS_DENIED)return true;Require(error==ERROR_FILE_NOT_FOUND,"Cannot inspect Union+ protection lock");return false;}
inline fs::path Pending(fs::path const& game){auto root=StateRoot(game),pointer=root/L"latest.ini";if(!fs::exists(pointer))return {};
 auto leaf=Get(pointer,L"Protection",L"Backup");if(leaf.empty())return {};auto relative=Relative(leaf);Require(relative.parent_path().empty(),"Invalid backup pointer");
 auto backup=root/relative;NoLinks(backup);auto journal=backup/L"protection.ini";auto state=Get(journal,L"Protection",L"State");return state==L"Verified"?fs::path{}:backup;}
class LockSet {
 struct Held{HANDLE handle;std::wstring relative,sha;};std::vector<Held> files;
public:
 LockSet()=default;LockSet(LockSet const&)=delete;
 ~LockSet(){for(auto const& f:files)if(f.handle)CloseHandle(f.handle);}
 void Acquire(fs::path const& game,std::vector<std::wstring> const& names){Require(files.empty(),"Protection already acquired");NoLinks(game);
  for(auto const& name:names){Require(Allowed(name),"Unrecognized protected file");auto p=game/Relative(name);NoLinks(p);
   // Union+ legitimately merges these INIs, sometimes with exclusive access.
   // Snapshot them without retaining a handle; preserve original engine/VR
   // settings after the installer exits, before allowing the game to launch.
   if(EditableIni(name)||RecoverableStartupAsset(name)){files.push_back({nullptr,name,Hash(p)});continue;}
   HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);Require(h!=INVALID_HANDLE_VALUE,"A VR file is busy; close the game and save settings before opening Union+");
   files.push_back({h,name,L""});files.back().sha=Hash(p);}}
 void Backup(fs::path const& game,fs::path const& backup){Require(!Below(backup,game)&&Full(backup)!=Full(game),"Backup must be outside the game");NoLinks(backup);Require(!fs::exists(backup),"Backup already exists");fs::create_directories(backup);
  auto journal=backup/L"protection.ini";Put(journal,L"Protection",L"Game",fs::absolute(game).wstring());Put(journal,L"Protection",L"State",L"Preparing");Put(journal,L"Protection",L"IniMergePolicy",L"1");Put(journal,L"Protection",L"Count",std::to_wstring(files.size()));
  for(size_t i=0;i<files.size();++i){auto const& f=files[i];auto destination=backup/L"files"/Relative(f.relative);fs::create_directories(destination.parent_path());Require(CopyFileW((game/Relative(f.relative)).c_str(),destination.c_str(),TRUE)!=FALSE,"Backup copy failed");Require(Hash(destination)==f.sha,"Backup checksum mismatch");auto section=L"File"+std::to_wstring(i);Put(journal,section.c_str(),L"Path",f.relative);Put(journal,section.c_str(),L"SHA256",f.sha);}
  Put(journal,L"Protection",L"State",L"Protected");}
 bool Verify(fs::path const& game)const{for(auto const& f:files)if(!EditableIni(f.relative)&&!RecoverableStartupAsset(f.relative)&&Hash(game/Relative(f.relative))!=f.sha)return false;return true;}
 size_t Size()const{return files.size();}
};
inline unsigned PreserveIni(fs::path const& saved,fs::path const& original,bool ge3){
 if(!fs::exists(original)){fs::create_directories(original.parent_path());Require(CopyFileW(saved.c_str(),original.c_str(),TRUE)!=FALSE,"Cannot restore missing game INI");return 1;}
 Require(fs::is_regular_file(original),"Invalid game INI target");
 std::vector<wchar_t> sections(262144),keys(262144);auto n=GetPrivateProfileSectionNamesW(sections.data(),DWORD(sections.size()),saved.c_str());Require(n<sections.size()-2,"Too many saved INI sections");unsigned changed=0;
 for(auto section=sections.data();*section;section+=wcslen(section)+1){
  // Keep Union+'s explicitly selected gameplay options (QuickLoot, XP etc.).
  // Graphics, distances, ROI, camera, controls, language and existing mounts
  // retain their original values; new mod keys/mounts are left in place.
  if(ge3&&Lower(section)==L"game")continue;
  n=GetPrivateProfileStringW(section,nullptr,L"",keys.data(),DWORD(keys.size()),saved.c_str());Require(n<keys.size()-2,"Too many saved INI keys");
  for(auto key=keys.data();*key;key+=wcslen(key)+1){auto value=Get(saved,section,key);bool different=Get(original,section,key)!=value;
   // An existing empty value and a deleted key must not be conflated.
   if(value.empty()){wchar_t probe[2]{};different=GetPrivateProfileStringW(section,key,L"x",probe,2,original.c_str())!=0;}
   if(different){Put(original,section,key,value);Require(Get(original,section,key)==value,"Cannot preserve game INI setting");++changed;}}
 }
 return changed;
}
inline bool ConfigureVrQuickLoot(fs::path const& game){
 // Union+'s default ge3.ini disables the Update Pack quick loot. Its optional
 // Script_QuickLoot replacement explicitly requires that native option off;
 // select one provider, based on the installed DLL rather than saved checkboxes.
 auto replacement=game/L"scripts/Script_QuickLoot.dll";NoLinks(replacement);bool native=!fs::is_regular_file(replacement);
 auto ini=game/L"Ini/ge3.ini";NoLinks(ini);Require(fs::is_regular_file(ini),"Game INI is missing; cannot configure VR quick loot");auto value=native?L"true":L"false";
 if(Lower(Get(ini,L"Game",L"QuickLoot"))!=value)Put(ini,L"Game",L"QuickLoot",value);
 Require(Lower(Get(ini,L"Game",L"QuickLoot"))==value,"Cannot configure VR quick loot");return native;
}
inline void Recover(fs::path const& game,fs::path const& backup,bool repairCore=true){Require(!Below(backup,game)&&Full(backup)!=Full(game),"Unsafe backup directory");NoLinks(backup);auto journal=backup/L"protection.ini";
 Require(Full(Get(journal,L"Protection",L"Game"))==Full(game),"Backup belongs to another game");unsigned count=GetPrivateProfileIntW(L"Protection",L"Count",0,journal.c_str());Require(count>0&&count<512,"Invalid backup inventory");
 auto policy=Get(journal,L"Protection",L"IniMergePolicy");Require(policy.empty()||policy==L"1","Unsupported INI preservation policy");
 struct Entry{fs::path original,saved;std::wstring sha;bool editable,ge3;};std::vector<Entry> entries;
 // Validate the complete inventory before restoring anything.
 for(unsigned i=0;i<count;++i){auto section=L"File"+std::to_wstring(i);auto name=Get(journal,section.c_str(),L"Path"),sha=Get(journal,section.c_str(),L"SHA256");Require(Allowed(name)&&sha.size()==64,"Invalid protected backup entry");
  auto relative=Relative(name),saved=backup/L"files"/relative,original=game/relative;NoLinks(saved);NoLinks(original);Require(Hash(saved)==sha,"Damaged backup; nothing restored");bool editable=policy==L"1"&&EditableIni(name);
  if(!repairCore&&!editable&&!RecoverableStartupAsset(name))Require(fs::is_regular_file(original)&&Hash(original)==sha,"Protected VR files changed unexpectedly; use Verify / recover VR");
  entries.push_back({original,saved,sha,editable,Lower(relative.wstring())==L"ini\\ge3.ini"});}
 unsigned preserved=0;
 for(auto const& e:entries){if(e.editable){preserved+=PreserveIni(e.saved,e.original,e.ge3);continue;}
  if(!fs::exists(e.original)||Hash(e.original)!=e.sha){fs::create_directories(e.original.parent_path());Require(CopyFileW(e.saved.c_str(),e.original.c_str(),FALSE)!=FALSE,"Recovery failed; keep the game closed");Require(Hash(e.original)==e.sha,"Recovery checksum mismatch");}}
 Put(journal,L"Protection",L"PreservedIniValues",std::to_wstring(preserved));
 if(policy==L"1")Put(journal,L"Protection",L"QuickLootProvider",ConfigureVrQuickLoot(game)?L"Native Update Pack":L"Script_QuickLoot.dll");
 Put(journal,L"Protection",L"State",L"Verified");}
inline void VerifyCore(fs::path const& game){Require(Hash(game/L"Engine.dll")==L"991CF674A9CF1BC60CDDC204BE0993E8251336F4D3471543A37846CC6840BF5A","Unsupported Engine.dll; VR compatibility must be checked first");
 Require(Hash(game/L"Game.dll")==L"4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00","Unsupported Game.dll; VR compatibility must be checked first");
 for(auto name:{L"scripts/Script_Gothic3VRDiagnostics.dll",L"scripts/Script_Gothic3VRDiagnostics.ini",L"patches/Patch_Gothic3VRResolution.dll"})Require(fs::is_regular_file(game/name),"VR installation is incomplete");}
inline DWORD RunInstaller(fs::path const& executable,std::wstring arguments=L""){
 struct Owned{HANDLE value=nullptr;~Owned(){if(value)CloseHandle(value);}};
 Owned job;job.value=CreateJobObjectW(nullptr,nullptr);Require(job.value!=nullptr,"Cannot create protected installer job");
 JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_ACTIVE_PROCESS;limits.BasicLimitInformation.ActiveProcessLimit=1;
 Require(SetInformationJobObject(job.value,JobObjectExtendedLimitInformation,&limits,sizeof(limits))!=FALSE,"Cannot constrain installer lifetime");
 Require(executable.wstring().find(L'"')==std::wstring::npos,"Invalid executable path");auto command=L"\""+executable.wstring()+L"\""+(arguments.empty()?L"":L" "+arguments);
 STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
 if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED,nullptr,executable.parent_path().c_str(),&startup,&child)){
  auto error=GetLastError();throw std::runtime_error("Cannot start protected installer (Windows error "+std::to_string(error)+")");}
 Owned process,thread;process.value=child.hProcess;thread.value=child.hThread;
 if(!AssignProcessToJobObject(job.value,process.value)){TerminateProcess(process.value,1);throw std::runtime_error("Cannot attach installer protection; installer was not allowed to run");}
 if(ResumeThread(thread.value)==DWORD(-1)){TerminateJobObject(job.value,1);throw std::runtime_error("Cannot resume protected installer");}
 Require(WaitForSingleObject(process.value,INFINITE)==WAIT_OBJECT_0,"Installer wait failed; recovery remains required");DWORD code=1;Require(GetExitCodeProcess(process.value,&code)!=FALSE,"Cannot read installer result");
 TerminateJobObject(job.value,0);return code;
}
}
