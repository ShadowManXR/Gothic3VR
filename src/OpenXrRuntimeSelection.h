#pragma once
#include "OpenXrRuntimeChoice.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
namespace diagnostic::xr_runtime {
namespace fs=std::filesystem;
inline std::wstring Environment(wchar_t const* name){DWORD n=GetEnvironmentVariableW(name,nullptr,0);if(!n)return {};std::wstring value(n,L'\0');DWORD read=GetEnvironmentVariableW(name,value.data(),n);if(!read||read>=n)throw std::runtime_error("OpenXR environment changed during selection");value.resize(read);return value;}
inline std::wstring RegistryString(HKEY root,wchar_t const* key,wchar_t const* name,REGSAM view){
    HKEY h=nullptr;if(RegOpenKeyExW(root,key,0,KEY_QUERY_VALUE|view,&h)!=ERROR_SUCCESS)return {};
    DWORD type=0,bytes=0;auto result=RegQueryValueExW(h,name,nullptr,&type,nullptr,&bytes);
    if(result!=ERROR_SUCCESS||type!=REG_SZ||bytes>65536){RegCloseKey(h);return {};}
    std::wstring value(bytes/sizeof(wchar_t)+1,L'\0');result=RegQueryValueExW(h,name,nullptr,&type,reinterpret_cast<BYTE*>(value.data()),&bytes);RegCloseKey(h);
    if(result!=ERROR_SUCCESS)return {};value.resize(wcsnlen_s(value.c_str(),value.size()));return value;
}
inline fs::path Manifest32(fs::path path){
    auto file=path.filename().wstring();
    if(!_wcsicmp(file.c_str(),L"steamxr_win64.json"))return path.parent_path()/L"steamxr_win32.json";
    if(!_wcsicmp(file.c_str(),L"oculus_openxr_64.json"))return path.parent_path()/L"oculus_openxr_32.json";
    if(!_wcsicmp(file.c_str(),L"virtualdesktop-openxr.json"))return path.parent_path()/L"virtualdesktop-openxr-32.json";
    return path;
}
inline bool Matches(fs::path const& path,OpenXrRuntimeChoice choice){auto file=path.filename().wstring();return !_wcsicmp(file.c_str(),choice==OpenXrRuntimeChoice::Vdxr?L"virtualdesktop-openxr-32.json":choice==OpenXrRuntimeChoice::MetaLink?L"oculus_openxr_32.json":L"steamxr_win32.json");}
inline std::vector<fs::path> InstalledCandidates(OpenXrRuntimeChoice choice){
    std::vector<fs::path> paths;
    auto add=[&](fs::path path){path=Manifest32(path);if(Matches(path,choice)&&std::find(paths.begin(),paths.end(),path)==paths.end())paths.push_back(std::move(path));};
    for(REGSAM view:{KEY_WOW64_32KEY,KEY_WOW64_64KEY}){
        auto active=RegistryString(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Khronos\\OpenXR\\1",L"ActiveRuntime",view);if(!active.empty())add(active);
        HKEY h=nullptr;if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Khronos\\OpenXR\\1\\AvailableRuntimes",0,KEY_QUERY_VALUE|view,&h)==ERROR_SUCCESS){
            for(DWORD i=0;;++i){wchar_t name[32768];DWORD size=32768,type=0,enabled=1,bytes=sizeof(enabled);auto result=RegEnumValueW(h,i,name,&size,nullptr,&type,reinterpret_cast<BYTE*>(&enabled),&bytes);if(result==ERROR_NO_MORE_ITEMS)break;if(result==ERROR_SUCCESS&&type==REG_DWORD&&enabled==0)add(name);}
            RegCloseKey(h);
        }
    }
    auto pf=Environment(L"ProgramW6432");if(pf.empty())pf=Environment(L"ProgramFiles");
    if(choice==OpenXrRuntimeChoice::Vdxr&&!pf.empty())add(fs::path(pf)/L"Virtual Desktop Streamer/OpenXR/virtualdesktop-openxr-32.json");
    if(choice==OpenXrRuntimeChoice::MetaLink&&!pf.empty())add(fs::path(pf)/L"Oculus/Support/oculus-runtime/oculus_openxr_32.json");
    if(choice==OpenXrRuntimeChoice::SteamVR){
        auto steam=RegistryString(HKEY_CURRENT_USER,L"SOFTWARE\\Valve\\Steam",L"SteamPath",KEY_WOW64_32KEY);
        if(!steam.empty())add(fs::path(steam)/L"steamapps/common/SteamVR/steamxr_win32.json");
        auto x86=Environment(L"ProgramFiles(x86)");if(!x86.empty())add(fs::path(x86)/L"Steam/steamapps/common/SteamVR/steamxr_win32.json");
    }
    return paths;
}
// Read only the runtime library string. No manifest code or DLL is executed here.
inline std::wstring LibraryPath(std::string const& text){
    auto key=text.find("\"library_path\"");if(key==std::string::npos)throw std::runtime_error("OpenXR manifest has no runtime library_path");
    auto pos=text.find(':',key+14);if(pos==std::string::npos)throw std::runtime_error("Invalid OpenXR manifest");
    pos=text.find_first_not_of(" \t\r\n",pos+1);if(pos==std::string::npos||text[pos++]!='\"')throw std::runtime_error("Invalid OpenXR library_path");
    std::string value;bool ended=false;
    for(;pos<text.size();++pos){char c=text[pos];if(c=='\"'){ended=true;break;}if(c=='\\'){if(++pos>=text.size())break;c=text[pos];if(c!='\\'&&c!='/'&&c!='\"')throw std::runtime_error("Unsupported escape in OpenXR library_path");}if(static_cast<unsigned char>(c)<32)throw std::runtime_error("Invalid OpenXR library_path character");value+=c;}
    if(!ended||value.empty())throw std::runtime_error("Invalid OpenXR library_path");
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0);if(!count)throw std::runtime_error("Invalid UTF-8 OpenXR library_path");
    std::wstring wide(count,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),wide.data(),count);return wide;
}
inline void ValidateManifest(fs::path const& path,WORD machine=IMAGE_FILE_MACHINE_I386){
    if(!path.is_absolute())throw std::runtime_error("OpenXR runtime manifest must have an absolute path");
    std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("OpenXR 32-bit runtime manifest is missing");
    file.seekg(0,std::ios::end);auto size=file.tellg();if(size<=0||size>65536)throw std::runtime_error("Invalid OpenXR manifest size");file.seekg(0);
    std::string text(static_cast<size_t>(size),'\0');if(!file.read(text.data(),text.size()))throw std::runtime_error("Cannot read OpenXR manifest");
    fs::path library=LibraryPath(text);if(library.is_relative())library=path.parent_path()/library;
    std::ifstream dll(library,std::ios::binary);IMAGE_DOS_HEADER dos{};DWORD signature=0;IMAGE_FILE_HEADER header{};
    if(!dll.read(reinterpret_cast<char*>(&dos),sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<sizeof(dos)||dos.e_lfanew>1024*1024)throw std::runtime_error("OpenXR runtime DLL is missing or invalid");
    dll.seekg(dos.e_lfanew);dll.read(reinterpret_cast<char*>(&signature),sizeof(signature));dll.read(reinterpret_cast<char*>(&header),sizeof(header));
    if(!dll||signature!=IMAGE_NT_SIGNATURE||header.Machine!=machine||!(header.Characteristics&IMAGE_FILE_DLL))throw std::runtime_error(machine==IMAGE_FILE_MACHINE_I386?"Gothic 3 requires a 32-bit (x86) OpenXR runtime; a 64-bit runtime cannot be loaded":"The Meta host requires a 64-bit (x64) OpenXR runtime");
}
inline fs::path Select(OpenXrRuntimeChoice choice){
    if(choice==OpenXrRuntimeChoice::System){auto manifest=Environment(L"XR_RUNTIME_JSON");if(manifest.empty())manifest=RegistryString(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Khronos\\OpenXR\\1",L"ActiveRuntime",KEY_WOW64_32KEY);if(manifest.empty())throw std::runtime_error("No 32-bit system OpenXR runtime is registered");fs::path path(manifest);ValidateManifest(path);return path;}
    auto paths=InstalledCandidates(choice);for(auto const& path:paths)if(fs::is_regular_file(path)){ValidateManifest(path);return path;}
    throw std::runtime_error(choice==OpenXrRuntimeChoice::SteamVR?"SteamVR 32-bit OpenXR runtime is missing. Update SteamVR or select VDXR.":choice==OpenXrRuntimeChoice::MetaLink?"Meta Quest Link 32-bit OpenXR runtime is missing. Install Meta Quest Link or select VDXR.":"Virtual Desktop 32-bit OpenXR runtime is missing. Install Virtual Desktop Streamer or select another runtime.");
}
class ScopedManifest {
    std::wstring previous;bool existed=false;
public:
    explicit ScopedManifest(fs::path const& manifest){SetLastError(ERROR_SUCCESS);DWORD n=GetEnvironmentVariableW(L"XR_RUNTIME_JSON",nullptr,0);existed=n!=0||GetLastError()!=ERROR_ENVVAR_NOT_FOUND;previous=Environment(L"XR_RUNTIME_JSON");if(!SetEnvironmentVariableW(L"XR_RUNTIME_JSON",manifest.c_str()))throw std::runtime_error("Cannot select OpenXR runtime for this process");}
    ~ScopedManifest(){SetEnvironmentVariableW(L"XR_RUNTIME_JSON",existed?previous.c_str():nullptr);}
    ScopedManifest(ScopedManifest const&)=delete;ScopedManifest& operator=(ScopedManifest const&)=delete;
};
}
