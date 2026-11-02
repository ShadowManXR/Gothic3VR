#include "NativeApplication.h"
#include "NativeOcclusion.h"
#include <vector>
#include <cstdio>
#include <intrin.h>
using namespace diagnostic::occlusion;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
struct Query : IDirect3DQuery9 {
    unsigned calls=0,pending=0,refs=0;DWORD flags=99,pixels=13;
    HRESULT status=S_OK;D3DQUERYTYPE type=D3DQUERYTYPE_OCCLUSION;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override{return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{++refs;return 1;}
    ULONG STDMETHODCALLTYPE Release()override{++refs;return 1;}
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**)override{return E_NOTIMPL;}
    D3DQUERYTYPE STDMETHODCALLTYPE GetType()override{return type;}
    DWORD STDMETHODCALLTYPE GetDataSize()override{return 4;}
    HRESULT STDMETHODCALLTYPE Issue(DWORD)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE GetData(void* data,DWORD size,DWORD value)override{
        ++calls;flags=value;if(size!=4||!data)return E_INVALIDARG;
        if(pending){--pending;return S_FALSE;}if(FAILED(status))return status;
        std::memcpy(data,&pixels,4);return S_OK;
    }
    void Reset(unsigned n=0,DWORD p=13,HRESULT hr=S_OK){calls=refs=0;pending=n;flags=99;pixels=p;status=hr;type=D3DQUERYTYPE_OCCLUSION;}
} query;
using Info=uint32_t(__thiscall*)(void*,void*,DWORD&,bool);
using Resolve=DWORD(__thiscall*)(void*,void*,bool);
Info nativeInfo=nullptr;bool world=false;uintptr_t expected=0;
unsigned char admin[0x5a0]{};
void* __cdecl GetAdmin(){return admin;}
__declspec(naked) void ReserveExisting(){__asm ret}
uint32_t __fastcall Hook(void* self,void*,void* handle,DWORD& samples,bool wait){
    if(!Eligible(world,wait,reinterpret_cast<uintptr_t>(_ReturnAddress()),expected))return nativeInfo(self,handle,samples,wait);
    auto* borrowed=BorrowQuery(self,reinterpret_cast<uintptr_t>(handle));
    if(!borrowed)return nativeInfo(self,handle,samples,wait);
    auto result=Poll(borrowed,samples);
    if(result==Result::Native)return nativeInfo(self,handle,samples,wait);
    return result==Result::Error?0:1;
}
void RelocateCall(unsigned char* code,unsigned offset,void* target){
    code[offset]=0xe8;int32_t delta=int32_t(reinterpret_cast<uintptr_t>(target)-reinterpret_cast<uintptr_t>(code+offset+5));std::memcpy(code+offset+1,&delta,4);
}
}
int __cdecl wmain(int argc,wchar_t** argv){
    CHECK(argc==2);unsigned char actual[32]{},hash[]={0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    CHECK(diagnostic::FileSha256(argv[1],actual)&&!memcmp(actual,hash,32));
    HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);CHECK(file!=INVALID_HANDLE_VALUE);
    DWORD size=GetFileSize(file,nullptr),read=0;std::vector<unsigned char> bytes(size);bool readOk=ReadFile(file,bytes.data(),size,&read,nullptr)&&read==size;CloseHandle(file);CHECK(readOk);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(bytes.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(bytes.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    auto at=[&](uint32_t rva,size_t n)->unsigned char const*{for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+n<=s.SizeOfRawData){size_t off=s.PointerToRawData+rva-s.VirtualAddress;if(off+n<=bytes.size())return bytes.data()+off;}}return nullptr;};
    auto same=[&](uint32_t rva,std::initializer_list<unsigned char> value){auto* p=at(rva,value.size());return p&&std::equal(value.begin(),value.end(),p);};
    CHECK(nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386);
    CHECK(at(QueryInfoRva,sizeof(QueryInfoPrefix))&&!memcmp(at(QueryInfoRva,sizeof(QueryInfoPrefix)),QueryInfoPrefix,sizeof(QueryInfoPrefix)));
    uint32_t entry=0;CHECK(at(DeviceTableRva+4*QueryInfoSlot,4));std::memcpy(&entry,at(DeviceTableRva+4*QueryInfoSlot,4),4);CHECK(entry==nt->OptionalHeader.ImageBase+QueryInfoRva);
    // The native blocking loop uses GetData(...,4,FLUSH) until not S_FALSE.
    CHECK(same(0xaa6db,{0x6a,1,0x6a,4,0x57,0x56,0xff,0xd2,0x83,0xf8,1,0x74,0xee}));
    CHECK(same(0xaa6f4,{0x6a,0,0x6a,4,0x51,0x56,0xff,0xd2}));
    // Native Resolve calls the same slot then returns the handle to its pool
    // ONLY on success. UINT_MAX remains the unresolved/error sentinel.
    CHECK(same(ResolveReturnRva-2,{0xff,0xd2,0x83,0xf8,1,0x74,9}));
    CHECK(same(0x1a4419,{0x0f,0xb7,0x46,2,0x8b,0x4e,4,0x89,0x3c,0x81,0x66,0x83,0x46,2,1}));
    // The final pending-node branch compares pixels UNSIGNED with threshold.
    CHECK(same(0x19eb63,{0x8b,0x46,4,0x8b,0x4c,0x24,0x18,0x3b,0x81,0xb4,1,0,0,0x76,0x34}));
    CHECK(ConservativeVisible!=UINT32_MAX&&ConservativeVisible>0&&ConservativeVisible>1024&&int32_t(ConservativeVisible)>0);
    // Execute the actual native bodies in an isolated fixture. Normal branches
    // contain only local jumps/virtual calls. Resolve's TWO external direct
    // calls are relocated to a fixture admin and a pre-reserved pool helper.
    // No game DLL is loaded/initialized, and no native global/import is used.
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(code);
    constexpr unsigned InfoSize=0x99,ResolveSize=0xa2;
    CHECK(at(QueryInfoRva,InfoSize)&&at(ResolveRva,ResolveSize));
    std::memcpy(code,at(QueryInfoRva,InfoSize),InfoSize);auto* resolveCode=code+256;std::memcpy(resolveCode,at(ResolveRva,ResolveSize),ResolveSize);
    CHECK(resolveCode[0x15]==0xe8&&resolveCode[0x84]==0xe8);
    RelocateCall(resolveCode,0x15,reinterpret_cast<void*>(&GetAdmin));RelocateCall(resolveCode,0x84,reinterpret_cast<void*>(&ReserveExisting));
    auto* thunk=code+512;thunk[0]=0xff;thunk[1]=0x25;uint32_t cell=uint32_t(reinterpret_cast<uintptr_t>(code+768));std::memcpy(thunk+2,&cell,4);
    void* nativeEntry=code;std::memcpy(code+768,&nativeEntry,4);
    DWORD old=0;CHECK(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);
    nativeInfo=reinterpret_cast<Info>(thunk);auto resolve=reinterpret_cast<Resolve>(resolveCode);expected=reinterpret_cast<uintptr_t>(resolveCode)+0x73;
    alignas(4) unsigned char device[0x68]{},processor[0x68]{};void* table[200]{};void** tablePointer=table;std::memcpy(device,&tablePointer,4);
    void* dummy=reinterpret_cast<void*>(1);std::memcpy(device+8,&dummy,4);uint16_t count=1;std::memcpy(device+0x46,&count,2);
    IDirect3DQuery9* queries[]={&query};auto** queryTable=queries;std::memcpy(device+0x48,&queryTable,4);
    void* wrapper=device;std::memcpy(admin+0x59c,&wrapper,4);table[QueryInfoSlot]=thunk;diagnostic::VirtualHook virtualHook;
    CHECK(virtualHook.Prepare(table,QueryInfoSlot,reinterpret_cast<void*>(&Hook))&&virtualHook.Install());
    void* pool[1024]{};void** poolPointer=pool;std::memcpy(processor+0x10,&poolPointer,4);uint16_t capacity=1024;std::memcpy(processor+0xc,&capacity,2);
    auto poolCount=[&](){uint16_t n=0;std::memcpy(&n,processor+0xe,2);return n;};
    CHECK(BorrowQuery(device,1)==&query&&!BorrowQuery(device,0)&&!BorrowQuery(device,2)&&!BorrowQuery(nullptr,1));
    CHECK(!Eligible(false,true,expected,expected)&&!Eligible(true,false,expected,expected)&&!Eligible(true,true,expected+1,expected)&&!Eligible(true,true,0,0));
    query.Reset(3);world=false;
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==13&&query.calls==4&&query.flags==1&&poolCount()==1&&pool[0]==reinterpret_cast<void*>(1));
    query.Reset(1000);world=true;
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==ConservativeVisible&&query.calls==1&&query.flags==0&&poolCount()==2&&query.refs==0);
    query.Reset(0,0);
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==0&&query.calls==1&&query.flags==0&&poolCount()==3);
    query.Reset(0,413);
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==413&&poolCount()==4);
    query.Reset(3);
    CHECK(resolve(processor,reinterpret_cast<void*>(1),false)==UINT32_MAX&&query.calls==1&&query.flags==0&&poolCount()==4);
    query.Reset(0,0,E_FAIL);
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==UINT32_MAX&&query.calls==1&&query.flags==0&&poolCount()==4&&query.refs==0);
    query.Reset(3);++expected;
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==13&&query.calls==4&&query.flags==1&&poolCount()==5);--expected;
    query.Reset(3);query.type=D3DQUERYTYPE_EVENT;
    CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==13&&query.calls==4&&query.flags==1&&poolCount()==6);
    CHECK(resolve(processor,nullptr,true)==0&&poolCount()==6);
    // Production policy must keep delayed *occluded* results, rather than
    // declaring every pending object visible and recycling its query early.
    // Execute the actual native Resolve and GetData loop through the hook's
    // bypass, including the zero pixels that must reject hidden geometry.
    for(auto* policy:{static_cast<wchar_t const*>(nullptr),L"",L"0",L"false",L"true",L"10",L"1 ",L"-1"}){
        CHECK(!PendingVisibleOptIn(policy));world=PendingVisibleOptIn(policy);
        auto before=poolCount();query.Reset(7,0);
        CHECK(resolve(processor,reinterpret_cast<void*>(1),true)==0&&query.calls==8&&query.flags==1&&poolCount()==before+1);
    }
    CHECK(PendingVisibleOptIn(L"1"));
    CHECK(virtualHook.Restore()&&table[QueryInfoSlot]==thunk);VirtualFree(code,0,MEM_RELEASE);
    puts("PASS real native occlusion loop, x86 ABI, conservative pending result, ready culling, native pool ownership, errors and bypass");
}
