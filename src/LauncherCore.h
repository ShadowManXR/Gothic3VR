#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>
namespace diagnostic {
inline bool GothicRunning() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return true;
    PROCESSENTRY32W entry{};entry.dwSize=sizeof(entry);bool found=false;
    if(Process32FirstW(snapshot,&entry))do{if(!_wcsicmp(entry.szExeFile,L"Gothic3.exe")){found=true;break;}}while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);return found;
}
inline std::wstring ParentDirectory(std::wstring const& path) {auto i=path.find_last_of(L"\\/");return i==std::wstring::npos?L"":path.substr(0,i);}
inline bool StartExecutable(std::wstring const& executable,std::wstring const& arguments,PROCESS_INFORMATION& process) {
    // Explicit application + quoted argv[0], and the game's directory as cwd.
    std::wstring command=L"\""+executable+L"\"";if(!arguments.empty())command+=L" "+arguments;
    auto directory=ParentDirectory(executable);STARTUPINFOW startup{};startup.cb=sizeof(startup);
    return CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&startup,&process)!=FALSE;
}
}
