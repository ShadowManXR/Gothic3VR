#include "UnionProtection.h"
#include "UnionStarterAudit.h"
#include "LauncherCore.h"
#include <shellapi.h>
#include <fstream>
using namespace diagnostic::optional_mods;
namespace {
struct Handle{HANDLE value=nullptr;~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);}};
bool Elevated(){Handle token;Require(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token.value)!=FALSE,"Cannot inspect protection permissions");TOKEN_ELEVATION elevation{};DWORD size=0;
 Require(GetTokenInformation(token.value,TokenElevation,&elevation,sizeof(elevation),&size)!=FALSE,"Cannot inspect protection elevation");return elevation.TokenIsElevated!=0;}
int RelaunchElevated(wchar_t const* arguments){wchar_t self[32768]{};Require(GetModuleFileNameW(nullptr,self,32768)>0,"Cannot locate VR protection executable");
 SHELLEXECUTEINFOW launch{};launch.cbSize=sizeof(launch);launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC|SEE_MASK_FLAG_NO_UI;launch.lpVerb=L"runas";launch.lpFile=self;launch.lpParameters=arguments;launch.nShow=SW_SHOWNORMAL;
 if(!ShellExecuteExW(&launch)){auto code=GetLastError();if(code==ERROR_CANCELLED)throw std::runtime_error("Union+ requires Windows administrator permission. Launch was cancelled; no installer ran.");throw std::runtime_error("Cannot elevate VR protection (Windows error "+std::to_string(code)+")");}
 Handle child;child.value=launch.hProcess;Require(child.value!=nullptr,"Cannot monitor elevated VR protection");Require(WaitForSingleObject(child.value,INFINITE)==WAIT_OBJECT_0,"Elevated protection wait failed");DWORD code=1;Require(GetExitCodeProcess(child.value,&code)!=FALSE,"Cannot read protection result");return int(code);}
bool Running(wchar_t const* name){HANDLE h=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);Require(h!=INVALID_HANDLE_VALUE,"Cannot inspect running applications");PROCESSENTRY32W e{};e.dwSize=sizeof(e);bool found=false;
 if(Process32FirstW(h,&e))do{if(!_wcsicmp(e.szExeFile,name)){found=true;break;}}while(Process32NextW(h,&e));CloseHandle(h);return found;}
void Execute(fs::path const& game,fs::path const& configuredUnion){
 Require(!Running(L"UnionStarter.exe")&&!Running(L"UnionStarter_old.exe"),"Close the existing Union+ window, then open it through Protected Union+");VerifyCore(game);
 for(auto sha:{CurrentUnionSha,AuditedUnionSha})Require(!Running(AuditedUnionCache(configuredUnion,sha).filename().c_str()),"Close the existing protected Union+ window first");
 auto unionExe=PrepareAuditedUnion(configuredUnion);
 Handle verified;verified.value=CreateFileW(unionExe.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 Require(verified.value!=INVALID_HANDLE_VALUE&&AuditedUnionHash(Hash(unionExe)),"Cannot lock the verified Union+ executable");
 Require(Pending(game).empty(),"A previous protected session needs Verify / recover VR first");
 auto root=StateRoot(game);NoLinks(root);fs::create_directories(root);SYSTEMTIME now{};GetLocalTime(&now);wchar_t leaf[80];swprintf_s(leaf,L"%04u%02u%02u-%02u%02u%02u-%03u-%lu",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds,GetCurrentProcessId());auto backup=root/leaf;
 LockSet protection;protection.Acquire(game,Collect(game));protection.Backup(game,backup);
 Put(root/L"latest.ini",L"Protection",L"Backup",leaf);
 Put(backup/L"protection.ini",L"Protection",L"ConfiguredUnion",fs::absolute(configuredUnion).wstring());
 Put(backup/L"protection.ini",L"Protection",L"ExecutedUnion",fs::absolute(unionExe).wstring());
 Put(backup/L"protection.ini",L"Protection",L"UnionSHA256",Hash(unionExe));
 Put(backup/L"protection.ini",L"Protection",L"AuditedBackupUsed",Full(unionExe)!=Full(configuredUnion)?L"1":L"0");
 auto config=unionExe.parent_path().parent_path()/L"US.ini";
 DWORD exitCode=1;try{
 SuspendUnionUpdates(unionExe,backup);
 Put(config,L"Main",L"GothicExePath",fs::absolute(game/L"Gothic3.exe").wstring());
 if(Get(config,L"InstalledModsNames",L"ICS Classic Camera")==L"1")MessageBoxW(nullptr,L"We do not recommend installing ICS Classic Camera with Gothic 3 VR, as it may interfere with VR camera controls.",L"Gothic 3 VR — mod compatibility",MB_OK|MB_ICONINFORMATION);
 // The guard outlives the VR launcher. Closing/crashing the guard kills the
 // installer job; core files deny write/delete, editable INIs are reconciled
 // from the checked backup before releasing the session.
 exitCode=RunInstaller(unionExe);RestoreUnionUpdates(backup);
 }catch(...){
  RestoreUnionUpdates(backup);
  // A refused process start must not strand an unchanged installation in the
  // pending state. A failed verification still requires explicit recovery.
  if(protection.Verify(game))Recover(game,backup,false);throw;
 }
 Put(backup/L"protection.ini",L"Protection",L"UnionExitCode",std::to_wstring(exitCode));
 Require(protection.Verify(game),"Protected VR files changed unexpectedly; use Verify / recover VR");
 Recover(game,backup,false);
 auto message=L"Union+ is closed. VR core files were verified; original engine, graphics and camera settings were preserved.\nNew mod settings and mount entries were retained.\nQuick Loot provider: "+Get(backup/L"protection.ini",L"Protection",L"QuickLootProvider")+L".\nStart Gothic 3 from the VR launcher.";
 MessageBoxW(nullptr,message.c_str(),L"Gothic 3 VR — Union+ protection",MB_OK|MB_ICONINFORMATION);
}
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,LPWSTR commandLine,int){try{
 int count=0;auto raw=CommandLineToArgvW(GetCommandLineW(),&count);std::vector<std::wstring> args;for(int i=1;i<count;++i)args.emplace_back(raw[i]);LocalFree(raw);
 fs::path game,unionExe;bool recover=false;for(size_t i=0;i<args.size();++i){if(args[i]==L"--game"&&i+1<args.size())game=fs::path(args[++i]).parent_path();else if(args[i]==L"--union"&&i+1<args.size())unionExe=args[++i];else if(args[i]==L"--recover")recover=true;else throw std::runtime_error("Invalid protection arguments");}
 Require(!game.empty(),"Select Gothic3.exe first");Require(!diagnostic::GothicRunning(),"Close Gothic 3 before changing mods or recovering VR files");
 // UnionStarter's manifest requires administrator permission. Elevate the guard
 // before acquiring locks; then CreateProcess can keep suspended launch + job
 // protection intact. Shell-launching only Union+ would bypass that guarantee.
 if(!recover&&!Elevated())return RelaunchElevated(commandLine);
 Handle mutex;auto name=MutexName(game);mutex.value=CreateMutexW(nullptr,FALSE,name.c_str());Require(mutex.value!=nullptr,"Cannot create protection lock");auto wait=WaitForSingleObject(mutex.value,0);Require(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED,"Protected Union+ is already running for this game");
 if(recover){auto pending=Pending(game);if(!pending.empty()){RestoreUnionUpdates(pending);Recover(game,pending);}VerifyCore(game);MessageBoxW(nullptr,L"VR core verified. Any interrupted protected session has been recovered.\nThis verifies VR files; it does not certify every gameplay mod.",L"Gothic 3 VR — compatibility",MB_OK|MB_ICONINFORMATION);}
 else{Require(!unionExe.empty(),"Select UnionStarter.exe first");Execute(game,unionExe);}
 ReleaseMutex(mutex.value);return 0;
 }catch(std::exception const& error){std::string text=error.what();auto n=MultiByteToWideChar(CP_UTF8,0,text.c_str(),-1,nullptr,0);std::wstring wide(size_t(n),L'\0');MultiByteToWideChar(CP_UTF8,0,text.c_str(),-1,wide.data(),n);
 MessageBoxW(nullptr,wide.c_str(),L"Gothic 3 VR — Union+ protection",MB_OK|MB_ICONERROR);return 1;}}
