#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iomanip>

namespace {
void Need(bool value,char const* why){if(!value)throw std::runtime_error(std::string(why)+" error="+std::to_string(GetLastError()));}
struct Handle {HANDLE value=nullptr;~Handle(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);} };
std::wstring Quote(std::wstring const& value){
    std::wstring out=L"\"";unsigned slashes=0;
    for(wchar_t c:value){if(c==L'\\'){++slashes;continue;}out.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;out+=c;}
    out.append(slashes*2,L'\\');out+=L'"';return out;
}
void Stack(HANDLE process,HANDLE thread,DWORD id,std::ofstream& log){
    CONTEXT context{};context.ContextFlags=CONTEXT_FULL;
    if(!GetThreadContext(thread,&context)){log<<"THREAD "<<id<<" context error "<<GetLastError()<<'\n';return;}
    log<<"THREAD "<<id<<" EIP=0x"<<std::hex<<context.Eip<<" ESP=0x"<<context.Esp<<" EBP=0x"<<context.Ebp<<std::dec<<'\n';
    STACKFRAME64 frame{};frame.AddrPC.Offset=context.Eip;frame.AddrFrame.Offset=context.Ebp;frame.AddrStack.Offset=context.Esp;
    frame.AddrPC.Mode=frame.AddrFrame.Mode=frame.AddrStack.Mode=AddrModeFlat;
    DWORD64 previous=0;
    for(unsigned i=0;i<40&&frame.AddrPC.Offset&&frame.AddrPC.Offset!=previous;++i){
        previous=frame.AddrPC.Offset;IMAGEHLP_MODULE64 module{};module.SizeOfStruct=sizeof(module);
        SymGetModuleInfo64(process,frame.AddrPC.Offset,&module);
        alignas(SYMBOL_INFO) char data[sizeof(SYMBOL_INFO)+512]{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(data);symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;DWORD64 offset=0;
        bool named=SymFromAddr(process,frame.AddrPC.Offset,&offset,symbol)!=FALSE;
        log<<"  #"<<i<<" 0x"<<std::hex<<frame.AddrPC.Offset<<' '<<module.ModuleName<<"+0x"<<frame.AddrPC.Offset-module.BaseOfImage;
        if(named)log<<' '<<symbol->Name<<"+0x"<<offset;log<<std::dec<<'\n';
        if(!StackWalk64(IMAGE_FILE_MACHINE_I386,process,thread,&frame,&context,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr))break;
    }
    log.flush();
}
void Dump(HANDLE process,DWORD pid,DWORD tid,EXCEPTION_DEBUG_INFO const* exception,std::filesystem::path const& path,std::ofstream& log){
    Handle file;file.value=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file.value==INVALID_HANDLE_VALUE){log<<"DUMP create error "<<GetLastError()<<'\n';return;}
    Handle thread;thread.value=OpenThread(THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,tid);
    CONTEXT context{};context.ContextFlags=CONTEXT_FULL;EXCEPTION_RECORD record{};EXCEPTION_POINTERS pointers{};MINIDUMP_EXCEPTION_INFORMATION info{};bool withException=false;
    if(exception&&thread.value&&GetThreadContext(thread.value,&context)){
        record=exception->ExceptionRecord;pointers.ExceptionRecord=&record;pointers.ContextRecord=&context;
        info.ThreadId=tid;info.ExceptionPointers=&pointers;info.ClientPointers=FALSE;withException=true;
    }
    auto flags=MINIDUMP_TYPE(MiniDumpWithThreadInfo|MiniDumpWithIndirectlyReferencedMemory|MiniDumpWithUnloadedModules);
    BOOL ok=MiniDumpWriteDump(process,pid,file.value,flags,withException?&info:nullptr,nullptr,nullptr);
    log<<"DUMP "<<(ok?"written":"failed")<<" error="<<(ok?0:GetLastError())<<'\n';log.flush();
}
void Hung(HANDLE process,DWORD pid,std::filesystem::path const& folder,std::ofstream& log){
    Handle snapshot;snapshot.value=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);THREADENTRY32 entry{};entry.dwSize=sizeof(entry);
    if(Thread32First(snapshot.value,&entry))do{if(entry.th32OwnerProcessID!=pid)continue;
        Handle thread;thread.value=OpenThread(THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME|THREAD_QUERY_INFORMATION,FALSE,entry.th32ThreadID);
        if(thread.value&&SuspendThread(thread.value)!=DWORD(-1)){Stack(process,thread.value,entry.th32ThreadID,log);ResumeThread(thread.value);}
    }while(Thread32Next(snapshot.value,&entry));
    Dump(process,pid,0,nullptr,folder/L"hang.dmp",log);
}
}
int wmain(int argc,wchar_t** argv){try{
    Need(argc>=3,"Usage: G3DX11DebugProbe <output> <isolated probe exe> [args]");std::filesystem::path folder=argv[1],executable=std::filesystem::absolute(argv[2]);
    Need(executable.filename()==L"G3DX11NativeStoneParityProbe.exe"||executable.filename()==L"G3DX11MaterialResourcesProbe.exe","Only isolated experiment probes can be debugged");
    std::filesystem::create_directories(folder);std::ofstream log(folder/L"debug.txt");Need(bool(log),"Open debug log");
    std::wstring command=Quote(executable.wstring());for(int i=3;i<argc;++i){command+=L' ';command+=Quote(argv[i]);}
    STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
    Need(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,DEBUG_ONLY_THIS_PROCESS|CREATE_NO_WINDOW,nullptr,executable.parent_path().c_str(),&startup,&child)!=FALSE,"Create isolated debugger child");
    Handle process,initialThread;process.value=child.hProcess;initialThread.value=child.hThread;
    SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_UNDNAME|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS|SYMOPT_IGNORE_NT_SYMPATH);
    Need(SymInitialize(process.value,"",FALSE)!=FALSE,"Initialize local symbols");
    auto start=GetTickCount64();unsigned exceptions=0;bool ended=false;DWORD exitCode=1;
    while(!ended){DEBUG_EVENT event{};
        if(!WaitForDebugEvent(&event,500)){
            if(GetLastError()!=ERROR_SEM_TIMEOUT)Need(false,"Wait debug event");
            if(GetTickCount64()-start<20000)continue;
            log<<"TIMEOUT after20seconds\n";Hung(process.value,child.dwProcessId,folder,log);TerminateProcess(process.value,1460);exitCode=1460;break;
        }
        DWORD continuation=DBG_CONTINUE;
        switch(event.dwDebugEventCode){
        case CREATE_PROCESS_DEBUG_EVENT:
            SymLoadModuleEx(process.value,event.u.CreateProcessInfo.hFile,nullptr,nullptr,reinterpret_cast<DWORD64>(event.u.CreateProcessInfo.lpBaseOfImage),0,nullptr,0);
            if(event.u.CreateProcessInfo.hFile)CloseHandle(event.u.CreateProcessInfo.hFile);break;
        case LOAD_DLL_DEBUG_EVENT:{
            auto const& dll=event.u.LoadDll;wchar_t path[32768]{};GetFinalPathNameByHandleW(dll.hFile,path,32768,FILE_NAME_NORMALIZED);
            SymLoadModuleEx(process.value,dll.hFile,nullptr,nullptr,reinterpret_cast<DWORD64>(dll.lpBaseOfDll),0,nullptr,0);
            log<<"MODULE 0x"<<std::hex<<reinterpret_cast<uintptr_t>(dll.lpBaseOfDll)<<std::dec<<' '<<std::filesystem::path(path).string()<<'\n';
            if(dll.hFile)CloseHandle(dll.hFile);break;}
        case EXCEPTION_DEBUG_EVENT:{
            auto const& e=event.u.Exception;auto code=e.ExceptionRecord.ExceptionCode;
            if(code==EXCEPTION_BREAKPOINT&&exceptions==0){++exceptions;break;}
            continuation=DBG_EXCEPTION_NOT_HANDLED;
            if(code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_ILLEGAL_INSTRUCTION||!e.dwFirstChance){
                log<<"EXCEPTION 0x"<<std::hex<<code<<" address=0x"<<reinterpret_cast<uintptr_t>(e.ExceptionRecord.ExceptionAddress)<<std::dec<<" firstChance="<<e.dwFirstChance<<" thread="<<event.dwThreadId;
                if(e.ExceptionRecord.NumberParameters>=2)log<<" operation="<<e.ExceptionRecord.ExceptionInformation[0]<<" target=0x"<<std::hex<<e.ExceptionRecord.ExceptionInformation[1]<<std::dec;log<<'\n';
                Handle thread;thread.value=OpenThread(THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,event.dwThreadId);if(thread.value)Stack(process.value,thread.value,event.dwThreadId,log);
                if(!e.dwFirstChance)Dump(process.value,child.dwProcessId,event.dwThreadId,&e,folder/L"crash.dmp",log);
            }
            ++exceptions;break;}
        case EXIT_PROCESS_DEBUG_EVENT:exitCode=event.u.ExitProcess.dwExitCode;log<<"EXIT "<<exitCode<<'\n';ended=true;break;
        default:break;
        }
        log.flush();ContinueDebugEvent(event.dwProcessId,event.dwThreadId,continuation);
    }
    SymCleanup(process.value);std::cout<<"Isolated probe exit "<<exitCode<<"; debug log saved\n";return exitCode==0?0:1;
}catch(std::exception const& e){std::cerr<<e.what()<<'\n';return 1;}}
