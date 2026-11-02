#include "NativeApplication.h"
#include "VrSettings.h"
#include "ResolutionContext.h"
#include "EarlyResolutionSettings.h"
#include <shlobj.h>
#include <cstdio>
#include <string>

// CP 1.75's graphics startup rejects dimensions absent from the monitor's
// mode list even for windowed D3D9. PUP loads this DLL before engine creation.
// Adapt one native context for this game session. The native UI must resolve
// the same custom dimensions via ModeIndex after creation. Restore on failure.
// No Windows monitor mode is added or changed.
namespace {
unsigned char* engine;
void* graphicsOriginal;
using GetGfx=unsigned char* (__cdecl*)();
GetGfx getGfx;
DWORD creationThread;
unsigned targetWidth,targetHeight;
unsigned targetAdapter;
diagnostic::ScopedResolutionContext borrowed;
std::wstring logPath;
void Log(char const* event,int code=0) {
    FILE* f=nullptr;_wfopen_s(&f,logPath.c_str(),L"ab");if(!f)return;
    fprintf(f,"{\"event\":\"%s\",\"pid\":%lu,\"width\":%u,\"height\":%u,\"code\":%d}\n",event,GetCurrentProcessId(),targetWidth,targetHeight,code);fclose(f);
}
unsigned Read(unsigned char const* p,size_t offset){unsigned n;memcpy(&n,p+offset,4);return n;}
void Write(unsigned char* p,size_t offset,unsigned n){memcpy(p+offset,&n,4);}
unsigned char* __cdecl ContextForVr() {
    auto gfx=getGfx();
    if(GetCurrentThreadId()==creationThread&&borrowed.Select(gfx,targetWidth,targetHeight,targetAdapter))Log("custom_windowed_context_selected");
    return gfx;
}
int __fastcall CreateGraphics(void* app,void*,unsigned char* setup,bool mode) {
    using Create=int(__thiscall*)(void*,unsigned char*,bool);
    if(!diagnostic::Readable(setup,0x210))return reinterpret_cast<Create>(graphicsOriginal)(app,setup,mode);
    creationThread=GetCurrentThreadId();targetAdapter=Read(setup,0x30);
    // PUP's WindowMode=0 is windowed; SDK's eEWindowMode is a different enum.
    // Refresh 0 accepts any enumerated native refresh rate during the lookup.
    setup[0x34]=0;setup[0x35]=0;Write(setup,0x24,targetWidth);Write(setup,0x28,targetHeight);Write(setup,0x2c,0);
    int result=0;
    __try {result=reinterpret_cast<Create>(graphicsOriginal)(app,setup,mode);}
    __finally {
        if(result==1)borrowed.Commit();else borrowed.Restore();
        creationThread=0;
    }
    Log("graphics_creation_finished",result);return result;
}
bool Install() {
    auto entry=engine+0x12e30;auto call=engine+0x12e76;
    constexpr unsigned char prefix[]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
    if(!diagnostic::Readable(entry,6)||memcmp(entry,prefix,6)||!diagnostic::Readable(call,5)||call[0]!=0xe8)return false;
    int displacement;memcpy(&displacement,call+1,4);
    if(call+5+displacement!=engine+0x17b80)return false;
    getGfx=reinterpret_cast<GetGfx>(call+5+displacement);
    auto trampoline=static_cast<unsigned char*>(VirtualAlloc(nullptr,11,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!trampoline)return false;
    memcpy(trampoline,entry,6);trampoline[6]=0xe9;displacement=int(entry+6-(trampoline+11));memcpy(trampoline+7,&displacement,4);
    DWORD old;if(!VirtualProtect(trampoline,11,PAGE_EXECUTE_READ,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    // Both sites lie on the same page, before any graphics creation is running.
    if(!VirtualProtect(entry,0x100,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(trampoline,0,MEM_RELEASE);return false;}
    graphicsOriginal=trampoline;
    unsigned char replacement[6]={0xe9,0,0,0,0,0x90};displacement=int(reinterpret_cast<unsigned char*>(&CreateGraphics)-(entry+5));memcpy(replacement+1,&displacement,4);
    memcpy(entry,replacement,6);displacement=int(reinterpret_cast<unsigned char*>(&ContextForVr)-(call+5));memcpy(call+1,&displacement,4);
    DWORD unused;VirtualProtect(entry,0x100,old,&unused);FlushInstructionCache(GetCurrentProcess(),trampoline,11);FlushInstructionCache(GetCurrentProcess(),entry,0x100);return true;
}
}
extern "C" __declspec(dllexport) void __stdcall PatchInit() {
    wchar_t executable[MAX_PATH];if(!GetModuleFileNameW(nullptr,executable,MAX_PATH))return;
    std::wstring root=executable;root.resize(root.find_last_of(L"\\/"));logPath=root+L"\\scripts\\Gothic3VRDiagnostics\\resolution-startup.jsonl";
    auto settings=diagnostic::LoadVrSettings((root+L"\\scripts\\Script_Gothic3VRDiagnostics.ini").c_str());
    wchar_t documents[MAX_PATH]{};std::wstring nativeOptions;
    if(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,documents))){
        nativeOptions=std::wstring(documents)+L"\\Gothic3\\UserOptions.ini";
        if(GetFileAttributesW(nativeOptions.c_str())==INVALID_FILE_ATTRIBUTES)nativeOptions=std::wstring(documents)+L"\\Gothic3\\__UserOptions.ini";
    }
    if(!diagnostic::EarlyResolutionTarget(settings,nativeOptions.empty()?nullptr:nativeOptions.c_str(),targetWidth,targetHeight))return;
    engine=reinterpret_cast<unsigned char*>(GetModuleHandleW(L"Engine.dll"));wchar_t path[MAX_PATH];unsigned char digest[32];
    constexpr unsigned char expected[]={0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    if(!engine||!GetModuleFileNameW(reinterpret_cast<HMODULE>(engine),path,MAX_PATH)||!diagnostic::FileSha256(path,digest)||memcmp(digest,expected,32)){Log("engine_profile_refused");return;}
    Log(Install()?"resolution_patch_installed":"resolution_patch_refused");
}
