#include "GameResolution.h"
#include "EarlyResolutionSettings.h"
#include <string>
#include <cstdio>
int wmain(int argc,wchar_t** argv) {
    if(argc==4){diagnostic::VrSettings s;s.renderWidth=_wtoi(argv[2]);s.renderHeight=_wtoi(argv[3]);DWORD code=0;
        bool ok=diagnostic::ApplyGameResolution(argv[1],s,&code);printf("Native resolution update: %s; Windows error %lu\n",ok?"PASS":"FAIL",code);return ok?0:1;}
    unsigned failures=0;auto check=[&](bool ok,char const* name){printf("%s %s\n",ok?"PASS":"FAIL",name);failures+=!ok;};
    wchar_t directory[MAX_PATH],ownedDirectory[MAX_PATH],path[MAX_PATH];GetTempPathW(MAX_PATH,directory);GetTempFileNameW(directory,L"grs",0,ownedDirectory);
    DeleteFileW(ownedDirectory);CreateDirectoryW(ownedDirectory,nullptr);swprintf_s(path,L"%s\\UserOptions.ini",ownedDirectory);
    constexpr char fixture[]="; preserve comments\r\n[Options.Video]\r\nWindowMode=2\r\nResolution.Width=2560\r\nResolution.Height=1440\r\nResolution.RefreshRate=75\r\nVSync=0\r\nGammaRed=0.25\r\n[Options.Audio]\r\nMusicVolume=54\r\n[SessionKey.Forward]\r\nKey1.Type=0\r\n";
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);DWORD bytes;WriteFile(file,fixture,sizeof(fixture)-1,&bytes,nullptr);CloseHandle(file);
    diagnostic::VrSettings settings;
    unsigned earlyWidth=0,earlyHeight=0;
    check(!diagnostic::EarlyResolutionTarget(settings,path,earlyWidth,earlyHeight),"from-game fullscreen does not enable custom windowed patch");
    check(diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"WindowMode",0,path)==2&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==2560,"no override leaves native resolution and fullscreen alone");
    settings.renderWidth=3840;settings.renderHeight=2160;
    check(diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==3840&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,path)==2160&&GetPrivateProfileIntW(L"Options.Video",L"WindowMode",-1,path)==0,"write native resolution with PUP windowed enum 0 rather than borderless enum 1");
    diagnostic::VrSettings fromGame;
    check(diagnostic::EarlyResolutionTarget(fromGame,path,earlyWidth,earlyHeight)&&earlyWidth==3840&&earlyHeight==2160,"from-game selection retains support for previously saved custom 4K mode");
    check(diagnostic::EarlyResolutionTarget(settings,nullptr,earlyWidth,earlyHeight)&&earlyWidth==3840,"explicit preset takes precedence without native configuration");
    wchar_t gamma[64];GetPrivateProfileStringW(L"Options.Video",L"GammaRed",L"",gamma,64,path);
    check(!wcscmp(gamma,L"0.25")&&GetPrivateProfileIntW(L"Options.Audio",L"MusicVolume",0,path)==54&&GetPrivateProfileIntW(L"SessionKey.Forward",L"Key1.Type",-1,path)==0,"preserve video calibration, audio and input mappings");
    std::wstring backup=std::wstring(path)+L".bak";
    check(GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,backup.c_str())==2560&&GetPrivateProfileIntW(L"Options.Video",L"WindowMode",0,backup.c_str())==2,"back up original video settings before replacement");
    settings.renderWidth=2880;settings.renderHeight=2880;
    check(diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,path)==2880,"square VR target reaches engine options");
    settings.renderWidth=99999;settings.renderHeight=99999;
    check(!diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==2880,"reject unknown/oversized resolution without altering video settings");
    settings.renderWidth=3200;settings.renderHeight=1800;HANDLE locked=CreateFileW(path,GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);
    check(!diagnostic::ApplyGameResolution(path,settings),"locked video options abort resolution update");CloseHandle(locked);
    check(GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==2880,"failed update preserves complete previous video settings");
    settings.renderWidth=3840;settings.renderHeight=2160;
    check(diagnostic::ApplyGameResolution(path,settings)&&GetFileAttributesW(path)!=INVALID_FILE_ATTRIBUTES&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Height",0,path)==2160,"repeated writes with real UserOptions.ini filename keep original path usable");
    // Preserve UTF-16 files as UTF-16, including unrelated Unicode text.
    constexpr wchar_t unicode[]=L"[Options.Video]\r\nWindowMode=2\r\nResolution.Width=2560\r\nResolution.Height=1440\r\n[Other]\r\nNote=Știință\r\n";
    file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);WORD bom=0xfeff;WriteFile(file,&bom,2,&bytes,nullptr);WriteFile(file,unicode,sizeof(unicode)-2,&bytes,nullptr);CloseHandle(file);
    check(diagnostic::ApplyGameResolution(path,settings),"UTF-16 native configuration accepts resolution update");
    file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);WORD savedBom=0;WritePrivateProfileStringW(nullptr,nullptr,nullptr,path);ReadFile(file,&savedBom,2,&bytes,nullptr);CloseHandle(file);
    wchar_t note[64];GetPrivateProfileStringW(L"Other",L"Note",L"",note,64,path);
    check(savedBom==0xfeff&&!wcscmp(note,L"Știință"),"preserve UTF-16 encoding and unrelated non-ASCII settings");
    std::wstring recovery=std::wstring(ownedDirectory)+L"\\__UserOptions.ini";
    MoveFileW(path,recovery.c_str());
    settings.renderWidth=2880;settings.renderHeight=2880;
    check(diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==2880,"recover configuration renamed by native game startup before applying resolution");
    DeleteFileW(path);DeleteFileW(recovery.c_str());settings=diagnostic::VrSettings{};
    check(diagnostic::ApplyGameResolution(path,settings)&&GetPrivateProfileIntW(L"Options.Video",L"Resolution.Width",0,path)==3840,"recover backup when game and native startup copy are both absent");
    DeleteFileW(path);DeleteFileW(backup.c_str());
    check(!diagnostic::ApplyGameResolution(path,settings),"missing original and both recovery files gives a clear failure");
    RemoveDirectoryW(ownedDirectory);return failures?1:0;
}
