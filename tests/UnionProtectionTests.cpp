#include "UnionProtection.h"
#include "UnionStarterAudit.h"
#include <fstream>
using namespace diagnostic::optional_mods;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
void Write(fs::path const& p,char const* text){fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f<<text;}
int wmain(int argc,wchar_t** argv){try{
 if(argc==3&&!wcscmp(argv[1],L"--audit-union")){auto selected=ResolveAuditedUnion(argv[2]);CHECK(AuditedUnionHash(Hash(selected)));std::wprintf(L"PASS audited executable selected: %ls\n",selected.c_str());return 0;}
 if(argc==4&&!wcscmp(argv[1],L"--cache-upgrade")){
  auto fixture=fs::absolute(argv[3])/(L"union-cache-"+std::to_wstring(GetCurrentProcessId()));CHECK(!fs::exists(fixture));fs::create_directories(fixture/L"Union/Mods");
  auto configured=fixture/L"Union/UnionStarter.exe";CHECK(CopyFileW(argv[2],configured.c_str(),TRUE));auto cached=PrepareAuditedUnion(configured);CHECK(cached!=configured&&AuditedUnionHash(Hash(cached)));
  Write(configured,"future unknown updated Union+");CHECK(ResolveAuditedUnion(configured)==cached);CHECK(!fs::exists(fixture/L"Union/UnionStarter_old.exe"));CHECK(PrepareAuditedUnion(configured)==cached);
  CHECK(DeleteFileW(configured.c_str()));CHECK(ResolveAuditedUnion(configured)==cached);
  Write(cached,"damaged persistent copy");bool refused=false;try{ResolveAuditedUnion(configured);}catch(...){refused=true;}CHECK(refused);
  std::puts("PASS persistent verified copy survives unknown updates, deleted original and absent _old; corrupt copy refused");return 0;}
 if(argc==3&&!wcscmp(argv[1],L"--job-child")){CHECK(!DeleteFileW(argv[2])&&GetLastError()==ERROR_SHARING_VIOLATION);
  auto game=fs::path(argv[2]).parent_path().parent_path();auto ini=game/L"Ini/ge3.ini";
  HANDLE edit=CreateFileW(ini.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);CHECK(edit!=INVALID_HANDLE_VALUE);CloseHandle(edit);
  Put(ini,L"Engine.Setup",L"Entity.ROI",L"9000");Put(ini,L"Engine.Setup",L"NewModKey",L"enabled");Put(ini,L"Game",L"QuickLoot",L"true");
  CHECK(WritePrivateProfileStringW(L"Camera",L"Empty",nullptr,ini.c_str())!=FALSE);
  auto mounts=game/L"Ini/mountlist.ini";Put(mounts,L"List",L"Scripts",L"WrongScripts");Put(mounts,L"List",L"Data/NewMod",L"Data/NewMod");
  wchar_t self[MAX_PATH];GetModuleFileNameW(nullptr,self,MAX_PATH);std::wstring command=L"\""+std::wstring(self)+L"\" --leaf";STARTUPINFOW start{};start.cb=sizeof(start);PROCESS_INFORMATION child{};
  bool spawned=CreateProcessW(self,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&start,&child)!=FALSE;if(spawned){TerminateProcess(child.hProcess,1);CloseHandle(child.hThread);CloseHandle(child.hProcess);}CHECK(!spawned);return 42;}
 if(argc==2&&!wcscmp(argv[1],L"--leaf"))return 9;
 CHECK(argc==2);auto root=fs::absolute(argv[1]);fs::create_directories(root);auto fixture=root/(L"fixture-"+std::to_wstring(GetCurrentProcessId())+L"-ă");CHECK(!fs::exists(fixture));auto game=fixture/L"game",backup=fixture/L"backup";
 fs::create_directories(fixture);bool missingReported=false;try{RunInstaller(fixture/L"missing.exe");}catch(std::exception const& error){missingReported=std::string(error.what()).find("Windows error 2)")!=std::string::npos;}CHECK(missingReported);
 CHECK(AuditedUnionHash(AuditedUnionSha)&&AuditedUnionHash(CurrentUnionSha)&&!AuditedUnionHash(L"unknown"));
 Write(fixture/L"union/UnionStarter.exe","unknown updated starter");Write(fixture/L"union/UnionStarter_old.exe","unknown older starter");bool unverifiedRejected=false;
 try{ResolveAuditedUnion(fixture/L"union/UnionStarter.exe");}catch(...){unverifiedRejected=true;}CHECK(unverifiedRejected);
 // Update preference is a transaction, retaining all newly selected mods.
 {auto config=fixture/L"pack/US.ini",exe=fixture/L"pack/Union/UnionStarter.exe",receipt=fixture/L"config-session";
  fs::create_directories(receipt);Write(config,"[Main]\nLanguage=EN\n[InstalledModsNames]\nOriginal=1\n");Put(receipt/L"protection.ini",L"Protection",L"ExecutedUnion",exe.wstring());
  SuspendUnionUpdates(exe,receipt);CHECK(Get(config,L"Main",L"AutomaticUpdateDisabled")==L"1");Put(config,L"InstalledModsNames",L"Added",L"1");RestoreUnionUpdates(receipt);RestoreUnionUpdates(receipt);
  CHECK(Get(config,L"Main",L"AutomaticUpdateDisabled").empty()&&Get(config,L"InstalledModsNames",L"Added")==L"1"&&Get(config,L"Main",L"Language")==L"EN");
  for(auto value:{L"0",L"1"}){auto saved=fixture/(std::wstring(L"config-")+value);fs::create_directories(saved);Put(config,L"Main",L"AutomaticUpdateDisabled",value);Put(saved/L"protection.ini",L"Protection",L"ExecutedUnion",exe.wstring());SuspendUnionUpdates(exe,saved);RestoreUnionUpdates(saved);CHECK(Get(config,L"Main",L"AutomaticUpdateDisabled")==value);}
  auto damaged=fixture/L"config-corrupt";fs::create_directories(damaged);Put(damaged/L"protection.ini",L"Protection",L"ExecutedUnion",exe.wstring());SuspendUnionUpdates(exe,damaged);
  Write(damaged/L"Union-US.ini.before","damaged configuration");bool refused=false;try{RestoreUnionUpdates(damaged);}catch(...){refused=true;}CHECK(refused&&Get(config,L"Main",L"AutomaticUpdateDisabled")==L"1");
  auto missingExe=fixture/L"fresh/Union/UnionStarter.exe",missingReceipt=fixture/L"config-missing";fs::create_directories(missingExe.parent_path());fs::create_directories(missingReceipt);Put(missingReceipt/L"protection.ini",L"Protection",L"ExecutedUnion",missingExe.wstring());SuspendUnionUpdates(missingExe,missingReceipt);RestoreUnionUpdates(missingReceipt);CHECK(Get(fixture/L"fresh/US.ini",L"Main",L"AutomaticUpdateDisabled").empty());

 }
 Write(game/L"splash.bmp","G3 VR native splash");Write(game/L"Ini/logo.ini","");
 CHECK(Allowed(L"splash.bmp")&&Allowed(L"Ini/logo.ini")&&RecoverableStartupAsset(L"splash.bmp")&&RecoverableStartupAsset(L"Ini/logo.ini"));
 Write(game/L"scripts/Script_eCDELocator.dll","Union-owned locator");Write(game/L"Ini/eCDELocator.ini","Union-owned settings");Write(game/L"Data/_compiledImage/eCDELocator/icon_test.png","Union-owned icon");
 {LockSet locks;auto names=Collect(game);for(auto const& name:names)CHECK(Lower(name).find(L"ecdelocator")==std::wstring::npos);locks.Acquire(game,names);
 CHECK(DeleteFileW((game/L"scripts/Script_eCDELocator.dll").c_str()));Write(game/L"scripts/Script_eCDELocator.dll","updated Union locator");}
 CHECK(Allowed(L"scripts/Script_eCDELocator.dll")&&Allowed(L"Data/_compiledImage/eCDELocator/icon_test.png")); // Legacy receipts can still recover.
 Write(game/L"scripts/Script_Gothic3VRDiagnostics.dll","fake VR for kernel file sharing test");Write(game/L"scripts/Script_Gothic3VRDiagnostics.ini","keep calibration");Write(game/L"Data/example.m00","unmanaged mod");auto file=game/L"scripts/Script_Gothic3VRDiagnostics.dll";auto original=Hash(file);
 Write(game/L"Ini/ge3.ini","[Engine.Setup]\nEntity.ROI=4000\n[Game]\nQuickLoot=false\n[Camera]\nDistance=25\nEmpty=\n");Write(game/L"Ini/mountlist.ini","[List]\nScripts=Scripts\n");
 {LockSet startup;startup.Acquire(game,Collect(game));auto startupBackup=fixture/L"startup-backup";startup.Backup(game,startupBackup);
  // Union+ can replace its default artwork without a sharing error. Restore
  // just these non-code assets at completion, including a deleted playlist.
  Write(game/L"splash.bmp","native installer splash");CHECK(DeleteFileW((game/L"Ini/logo.ini").c_str()));CHECK(startup.Verify(game));
  Recover(game,startupBackup,false);CHECK(Hash(game/L"splash.bmp")==Hash(startupBackup/L"files/splash.bmp"));CHECK(Hash(game/L"Ini/logo.ini")==Hash(startupBackup/L"files/Ini/logo.ini"));
 }
 CHECK(!Active(game));{auto mutex=CreateMutexW(nullptr,FALSE,MutexName(game).c_str());CHECK(mutex&&Active(game));CloseHandle(mutex);}CHECK(!Active(game));
 CHECK(Allowed(L"scripts\\Script_Gothic3VRDiagnostics.dll"));CHECK(!Allowed(L"Data\\example.m00"));bool rejected=false;try{Relative(L"..\\outside");}catch(...){rejected=true;}CHECK(rejected);
 {LockSet protection;protection.Acquire(game,{L"scripts\\Script_Gothic3VRDiagnostics.dll",L"scripts\\Script_Gothic3VRDiagnostics.ini",L"Ini\\ge3.ini",L"Ini\\mountlist.ini"});protection.Backup(game,backup);CHECK(protection.Size()==4);
  // Match Union+'s actual cleanup/replacement operations, using owned files.
  CHECK(!DeleteFileW(file.c_str())&&GetLastError()==ERROR_SHARING_VIOLATION);
  CHECK(!MoveFileExW(file.c_str(),(game/L"renamed.dll").c_str(),MOVEFILE_REPLACE_EXISTING));
  HANDLE writer=CreateFileW(file.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_ALWAYS,0,nullptr);CHECK(writer==INVALID_HANDLE_VALUE);
  CHECK(!CopyFileW((game/L"Data/example.m00").c_str(),file.c_str(),FALSE));
  CHECK(DeleteFileW((game/L"Data/example.m00").c_str())); // Union+ data cleanup remains possible.
  wchar_t self[MAX_PATH];GetModuleFileNameW(nullptr,self,MAX_PATH);CHECK(RunInstaller(self,L"--job-child \""+file.wstring()+L"\"")==42);
  CHECK(protection.Verify(game)&&Hash(file)==original);
  CHECK(Get(game/L"Ini/ge3.ini",L"Engine.Setup",L"Entity.ROI")==L"9000");Recover(game,backup,false);
  CHECK(Get(game/L"Ini/ge3.ini",L"Engine.Setup",L"Entity.ROI")==L"4000");CHECK(Get(game/L"Ini/ge3.ini",L"Engine.Setup",L"NewModKey")==L"enabled");CHECK(Get(game/L"Ini/ge3.ini",L"Game",L"QuickLoot")==L"true");
  wchar_t probe[2]{};CHECK(GetPrivateProfileStringW(L"Camera",L"Empty",L"x",probe,2,(game/L"Ini/ge3.ini").c_str())==0);
  CHECK(Get(game/L"Ini/mountlist.ini",L"List",L"Scripts")==L"Scripts");CHECK(Get(game/L"Ini/mountlist.ini",L"List",L"Data/NewMod")==L"Data/NewMod");}
 // Union resets the native option to false. Keep native quick loot enabled
 // when no replacement is installed, but never enable both providers.
 Put(game/L"Ini/ge3.ini",L"Game",L"QuickLoot",L"false");Recover(game,backup,false);CHECK(Get(game/L"Ini/ge3.ini",L"Game",L"QuickLoot")==L"true");CHECK(Get(backup/L"protection.ini",L"Protection",L"QuickLootProvider")==L"Native Update Pack");
 auto replacement=game/L"scripts/Script_QuickLoot.dll";Write(replacement,"fixture replacement");Recover(game,backup,false);CHECK(Get(game/L"Ini/ge3.ini",L"Game",L"QuickLoot")==L"false");CHECK(Get(backup/L"protection.ini",L"Protection",L"QuickLootProvider")==L"Script_QuickLoot.dll");
 CHECK(DeleteFileW(replacement.c_str()));Recover(game,backup,false);CHECK(Get(game/L"Ini/ge3.ini",L"Game",L"QuickLoot")==L"true");
 CHECK(DeleteFileW(file.c_str()));Recover(game,backup);CHECK(Hash(file)==original);CHECK(Get(backup/L"protection.ini",L"Protection",L"State")==L"Verified");
 CHECK(DeleteFileW((game/L"Ini/ge3.ini").c_str()));Recover(game,backup);CHECK(Get(game/L"Ini/ge3.ini",L"Engine.Setup",L"Entity.ROI")==L"4000");
 // Older interrupted journals must retain their original exact-file recovery.
 Put(backup/L"protection.ini",L"Protection",L"IniMergePolicy",L"");Put(game/L"Ini/ge3.ini",L"Game",L"QuickLoot",L"true");Recover(game,backup);CHECK(Hash(game/L"Ini/ge3.ini")==Hash(backup/L"files/Ini/ge3.ini"));
 // Validate every backup before the first restore. A corrupt backup must not
 // partly overwrite other protected files.
 Write(backup/L"files/scripts/Script_Gothic3VRDiagnostics.ini","corrupt");Write(file,"changed");auto changed=Hash(file);rejected=false;try{Recover(game,backup);}catch(...){rejected=true;}CHECK(rejected&&Hash(file)==changed);
 std::puts("PASS core file locks, exclusive installer INI writes, preserved engine/mount values, retained mod additions, missing INI and legacy recovery, corrupt-backup refusal");return 0;
 }catch(std::exception const& error){std::fprintf(stderr,"FAIL %s\n",error.what());return 1;}}
