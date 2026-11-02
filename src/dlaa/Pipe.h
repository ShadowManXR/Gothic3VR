#pragma once
#include "Gpu.h"
#include "Protocol.h"
#include <objbase.h>
namespace dlaa {
struct Pipe {
    Handle handle;bool asynchronous=false;
    void Transfer(void* data,uint32_t size,bool write,unsigned timeout=10000){
        auto* bytes=static_cast<unsigned char*>(data);uint32_t done=0;
        while(done<size){DWORD count=0;BOOL ok=FALSE;
            if(asynchronous){Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));Require(event.value!=nullptr,"Pipe event");OVERLAPPED ov{};ov.hEvent=event.value;
                ok=write?WriteFile(handle.value,bytes+done,size-done,&count,&ov):ReadFile(handle.value,bytes+done,size-done,&count,&ov);
                if(!ok&&GetLastError()==ERROR_IO_PENDING){if(WaitForSingleObject(event.value,timeout)!=WAIT_OBJECT_0){CancelIoEx(handle.value,&ov);GetOverlappedResult(handle.value,&ov,&count,TRUE);throw std::runtime_error("DLAA helper pipe timeout");}
                    ok=GetOverlappedResult(handle.value,&ov,&count,FALSE);}}
            else ok=write?WriteFile(handle.value,bytes+done,size-done,&count,nullptr):ReadFile(handle.value,bytes+done,size-done,&count,nullptr);
            Require(ok&&count,"DLAA helper pipe closed / I/O failed");done+=count;
        }
    }
    Header ReadHeader(){Header h;Transfer(&h,sizeof(h),false);Require(h.magic==Magic&&h.version==ProtocolVersion,"Unknown DLAA protocol/version");return h;}
    template<class T>void Send(Message type,T const& body){Header h;h.type=type;h.bytes=sizeof(body);Transfer(&h,sizeof(h),true);Transfer(const_cast<T*>(&body),sizeof(body),true);}
    void Stop(){Header h;h.type=Message::Stop;Transfer(&h,sizeof(h),true);}
    template<class T>T Receive(Message type){auto h=ReadHeader();Require(Valid(h,type,sizeof(T)),"Unexpected DLAA message/size");T body{};Transfer(&body,sizeof(body),false);return body;}
    void OpenWorker(wchar_t const* name){handle.value=CreateFileW(name,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);Require(handle.value!=INVALID_HANDLE_VALUE,"Open local prototype pipe");}
};
struct WorkerProcess {
    Pipe pipe;Handle process;uint32_t pid=0;bool stopped=false;
    explicit WorkerProcess(std::filesystem::path const& executable){
        Require(std::filesystem::is_regular_file(executable),"x64 DLAA worker executable missing");
        GUID guid{};Hr(CoCreateGuid(&guid),"Unique prototype channel");wchar_t token[64]{};StringFromGUID2(guid,token,64);
        std::wstring name=L"\\\\.\\pipe\\G3VR_DLAA_"+std::to_wstring(GetCurrentProcessId())+token;
        pipe.handle.value=CreateNamedPipeW(name.c_str(),PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,4096,4096,0,nullptr);
        Require(pipe.handle.value!=INVALID_HANDLE_VALUE,"Create local prototype pipe");pipe.asynchronous=true;
        std::wstring command=L"\""+executable.wstring()+L"\" --pipe \""+name+L"\"";
        STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;PROCESS_INFORMATION child{};
        Require(CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,executable.parent_path().c_str(),&startup,&child)!=FALSE,"Start owned x64 DLAA worker");
        process.value=child.hProcess;pid=child.dwProcessId;CloseHandle(child.hThread);
        try{Handle event(CreateEventW(nullptr,TRUE,FALSE,nullptr));Require(event.value!=nullptr,"Connect pipe event");OVERLAPPED ov{};ov.hEvent=event.value;
            if(!ConnectNamedPipe(pipe.handle.value,&ov)){auto error=GetLastError();if(error==ERROR_IO_PENDING){
                if(WaitForSingleObject(event.value,10000)!=WAIT_OBJECT_0){CancelIoEx(pipe.handle.value,&ov);DWORD ignored=0;GetOverlappedResult(pipe.handle.value,&ov,&ignored,TRUE);throw std::runtime_error("Connect DLAA worker timeout");}
                DWORD ignored=0;Require(GetOverlappedResult(pipe.handle.value,&ov,&ignored,FALSE)!=FALSE,"Connect worker failed");}
                else Require(error==ERROR_PIPE_CONNECTED,"Connect DLAA worker error");}
        }catch(...){TerminateProcess(process.value,1);WaitForSingleObject(process.value,5000);throw;}
    }
    uint64_t Duplicate(HANDLE shared){HANDLE remote=nullptr;Require(DuplicateHandle(GetCurrentProcess(),shared,process.value,&remote,0,FALSE,DUPLICATE_SAME_ACCESS)!=FALSE,"Duplicate GPU NT handle to x64 worker");return uint64_t(reinterpret_cast<uintptr_t>(remote));}
    void Finish(){if(stopped)return;pipe.Stop();auto r=pipe.Receive<Reply>(Message::Reply);Require(r.success!=0,"Worker shutdown failed");Require(WaitForSingleObject(process.value,10000)==WAIT_OBJECT_0,"Worker shutdown timeout");DWORD code=1;GetExitCodeProcess(process.value,&code);Require(code==0,"Worker failed during shutdown");stopped=true;}
    ~WorkerProcess(){pipe.handle.Close();if(process.value&&WaitForSingleObject(process.value,100)!=WAIT_OBJECT_0){TerminateProcess(process.value,1);WaitForSingleObject(process.value,5000);}}
};
}
