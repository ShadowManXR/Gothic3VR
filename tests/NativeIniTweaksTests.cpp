#include "NativeIniTweaks.h"
#include "EddDrawDistance.h"
#include <cstdio>
using namespace diagnostic::ini_tweaks;
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
void Text(fs::path const& p,std::string const& s){std::ofstream f(p,std::ios::binary);f<<s;}
void Utf16(fs::path const& p,std::wstring const& s){std::ofstream f(p,std::ios::binary);unsigned short bom=0xfeff;f.write((char*)&bom,2);f.write((char*)s.data(),s.size()*sizeof(wchar_t));}
int main(){
 wchar_t temp[MAX_PATH],unique[MAX_PATH];GetTempPathW(MAX_PATH,temp);GetTempFileNameW(temp,L"twi",0,unique);DeleteFileW(unique);
 fs::path root=unique,game=root/L"game",ini=game/L"Ini"/L"ge3.ini",user=root/L"UserOptions.ini",backups=root/L"backups";
 fs::create_directories(ini.parent_path());
 std::string engine="; CP notes and a mod must survive\r\n[Engine.Setup]\r\nEntity.ROI=4000.0\r\nFpS.Max=120\r\nRender.PrefetchGridCellSize=10000\r\nRender.PrefetchGridCellSizeLowPoly=34000\r\nRender.DisableShaderCache=false\r\n[Option.Sliders]\r\nDistanceHigh.fFarClippingPlane_Low=6000.0\r\nDistanceHigh.fFarClippingPlane_Medium=8000.0\r\nDistanceHigh.fFarClippingPlane_High=10000.0\r\nDistanceLow.fFarClippingPlaneLowPolyMesh_Low=25000.0\r\nDistanceLow.fFarClippingPlaneLowPolyMesh_Medium=50000.0\r\nDistanceLow.fFarClippingPlaneLowPolyMesh_High=100000.0\r\nVegetationViewRange.fViewDistance_Low=4000.0\r\nVegetationViewRange.fViewDistance_Medium=5000.0\r\nVegetationViewRange.fViewDistance_High=6500.0\r\nVegetationViewRange.fViewDistance_VeryHigh=7500.0\r\n[Options.High]\r\nDistanceHigh=3\r\nDistanceLow=3\r\nVegetationViewRange=3\r\nVegetationQuality=3\r\nShadowQuality=3\r\nTextureFilter=2\r\nTextureQuality=2\r\nBloom=1\r\nDepthOfField=1\r\nNoise=0\r\n[Mod]\r\nUntouched=keep\r\n";
 std::wstring options=L"; User settings\r\n[Options.Details]\r\nPerformance=Custom\r\n[Options.Custom]\r\nDistanceHigh=2\r\nDistanceLow=1\r\nVegetationViewRange=3\r\nVegetationQuality=1\r\nShadowQuality=2\r\nTextureFilter=4\r\nDepthOfField=0\r\nNoise=0\r\nBloom=0\r\n[Options.Controls]\r\nMouseSensitivityX=0.405000\r\n[Mod]\r\nUnicode=\x0219\x021b\r\n";
 auto reset=[&]{Text(ini,engine);Utf16(user,options);WritePrivateProfileStringW(nullptr,nullptr,nullptr,ini.c_str());WritePrivateProfileStringW(nullptr,nullptr,nullptr,user.c_str());};reset();
 auto initialGame=Bytes(ini),initialUser=Bytes(user);auto m=Load(ini,user);
 CHECK(m.ready&&m.values[0]==8000&&m.values[1]==25000&&m.values[2]==6500&&m.values[12]==1);
 CHECK(Save(m,backups).ok&&Bytes(ini)==initialGame&&Bytes(user)==initialUser&&!fs::exists(backups));
 puts("PASS read-only load and unchanged save preserve both exact files; active detail tiers are used");
 auto firstSession=NewBackupSession(backups,L"fixed-timestamp");Text(firstSession/L"original.ini","keep");
 auto secondSession=NewBackupSession(backups,L"fixed-timestamp"),thirdSession=NewBackupSession(backups,L"fixed-timestamp");
 CHECK(firstSession!=secondSession&&firstSession!=thirdSession&&secondSession!=thirdSession);
 CHECK(Bytes(firstSession/L"original.ini")==std::vector<char>({'k','e','e','p'}));
 puts("PASS repeated identical timestamps create distinct backup sessions without overwriting originals");

 m.values[0]=12000;m.values[3]=14000;m.values[5]=5000;m.values[8]=1;
 auto saved=Save(m,backups);CHECK(saved.ok&&saved.changed&&Bytes(saved.backup/L"ge3.ini")==initialGame&&Bytes(saved.backup/L"UserOptions.ini")==initialUser);
 CHECK(Read(ini,L"Option.Sliders",L"DistanceHigh.fFarClippingPlane_Medium")==L"12000"&&Read(ini,L"Option.Sliders",L"DistanceHigh.fFarClippingPlane_High")==L"10000.0");
 CHECK(Read(user,L"Options.Custom",L"ShadowQuality")==L"1"&&Read(user,L"Options.Controls",L"MouseSensitivityX")==L"0.405000"&&Read(user,L"Mod",L"Unicode")==L"\x0219\x021b");
 CHECK(Read(ini,L"Mod",L"Untouched")==L"keep"&&Read(ini,L"Gothic3VR.IniTweaks",L"OwnDistances")==L"1");
 CHECK(Read(ini,L"Engine.Setup",L"ResourceCache.u32ImageMaxMemUsage_High").empty());
 puts("PASS targeted native edits, UTF-16, unknown keys, distinct backups, and inactive tiers are preserved");
 Text(game/L"G3_edd.ini","[ULTRA DISTANCE = TRUE]\n[SH = 40000]\n[SL = 280000]\n[VR = 20000]");DWORD error=0;auto owned=Bytes(ini);
 CHECK(diagnostic::edd::Apply(game.wstring(),error)&&Bytes(ini)==owned);
 puts("PASS explicit native distances are not overwritten by EDD on next launch");
 reset();m=Load(ini,user);m.values[5]=4500;CHECK(Save(m,backups).ok&&Read(ini,L"Gothic3VR.IniTweaks",L"OwnDistances").empty());
 reset();m=Load(ini,user);m.values[5]=4500;WritePrivateProfileStringW(L"Engine.Setup",L"Entity.ROI",L"6000",ini.c_str());auto changed=Bytes(ini);
 CHECK(!Save(m,backups).ok&&Bytes(ini)==changed);
 reset();m=Load(ini,user);m.values[0]=60000;CHECK(!Save(m,backups).ok&&Bytes(ini)==initialGame);
 m=Load(ini,user);m.values[0]=30000;CHECK(!Save(m,backups).ok&&Bytes(ini)==initialGame);
 m=Load(ini,user);m.values[5]=4500;CHECK(!Save(m,game/L"bad-backups").ok);
 puts("PASS external edits, native ranges, distance ordering and in-game backup destinations are rejected");
 reset();WritePrivateProfileStringW(L"Options.Details",L"Performance",L"High",user.c_str());m=Load(ini,user);CHECK(m.values[0]==10000&&m.values[2]==6500&&m.values[8]==3);m.values[8]=1;
 CHECK(Save(m,backups).ok&&Read(user,L"Options.Details",L"Performance")==L"Custom"&&Read(user,L"Options.Custom",L"ShadowQuality")==L"1"&&Read(user,L"Options.Custom",L"TextureQuality")==L"2"&&Read(user,L"Options.Custom",L"Bloom")==L"1");
 puts("PASS a preset becomes Custom without silently resetting its other effective options");
 reset();m=Load(ini,user);m.values[5]=4500;m.values[8]=1;HANDLE lock=CreateFileW(user.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
 CHECK(lock!=INVALID_HANDLE_VALUE);CHECK(!Save(m,backups).ok&&Bytes(ini)==initialGame);CloseHandle(lock);
 reset();m=Load(ini,user);m.values[5]=4500;m.values[8]=1;HANDLE late=INVALID_HANDLE_VALUE;
 auto observer=[](unsigned f,fs::path const& p,void* data){if(f==1)*static_cast<HANDLE*>(data)=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);};
 auto failed=Save(m,backups,observer,&late);if(late!=INVALID_HANDLE_VALUE)CloseHandle(late);
 CHECK(!failed.ok&&Bytes(ini)==initialGame&&Bytes(user)==initialUser);
 puts("PASS locked-file preflight and second-file commit failure preserve or roll back original settings");
 reset();WritePrivateProfileStringW(L"Options.Custom",L"DistanceLow",L"0",user.c_str());m=Load(ini,user);CHECK(!m.available[1]);m.values[5]=4500;CHECK(Save(m,backups).ok&&Read(user,L"Options.Custom",L"DistanceLow")==L"0");
 puts("PASS distant scenery Off stays Off when adjusting an unrelated INI setting");
 reset();WritePrivateProfileStringW(L"Engine.Setup",L"FpS.Max",L"-1",ini.c_str());m=Load(ini,user);CHECK(!m.available[6]);m.values[5]=4500;CHECK(Save(m,backups).ok&&Read(ini,L"Engine.Setup",L"FpS.Max")==L"-1");
 reset();m=Load(ini,user);Defaults(m);CHECK(m.forceNativeDistances&&Save(m,backups).ok&&Read(ini,L"Gothic3VR.IniTweaks",L"OwnDistances")==L"1");
 puts("PASS unsupported existing values are retained; explicit CP defaults also take precedence over EDD");
 // The directory is created by this test only; canonical containment is checked
 // before recursive cleanup.
 CHECK(fs::weakly_canonical(root).parent_path()==fs::weakly_canonical(fs::path(temp)));fs::remove_all(root);
 puts("Native INI tweaks PASS");return 0;
}
