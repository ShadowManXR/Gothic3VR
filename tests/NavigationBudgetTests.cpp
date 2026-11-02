#include "NativeNavigationBudget.h"
#include "NativeApplication.h"
#include "NativeRenderProfile.h"
#include <vector>
#include <cstdio>
#include <cmath>
using namespace diagnostic::navigation;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
void* fragment=nullptr;uint32_t resumeCursor=0;
struct State{uint32_t now,start,index,flag,elapsed;};
__declspec(naked) void __fastcall Run(State*,void*){
 __asm {
  push ebx
  push edi
  sub esp,20h
  mov [esp],ecx
  xor eax,eax
  mov [esp+12h],eax
  mov eax,[ecx]
  mov edi,[ecx+4]
  mov ebx,[ecx+8]
  sub eax,edi
  jmp dword ptr [fragment]
 }
}
__declspec(naked) void Finish(){
 __asm {
  mov edx,[esp]
  mov [edx+10h],eax
  mov eax,[esp+12h]
  mov [edx+0ch],eax
  add esp,20h
  pop edi
  pop ebx
  ret
 }
}
}
int __cdecl wmain(int argc,wchar_t** argv){
 CHECK(argc==2);unsigned char hash[32]{};
 CHECK(diagnostic::FileSha256(argv[1],hash)&&!memcmp(hash,diagnostic::performance::GameSceneHash,32));
 HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);CHECK(file!=INVALID_HANDLE_VALUE);
 DWORD size=GetFileSize(file,nullptr),read=0;std::vector<unsigned char> data(size);bool ok=ReadFile(file,data.data(),size,&read,nullptr)&&size==read;CloseHandle(file);CHECK(ok);
 auto* dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(data.data());auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(data.data()+dos->e_lfanew);auto* sections=IMAGE_FIRST_SECTION(nt);
 auto at=[&](uint32_t rva,size_t n)->unsigned char const*{for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+n<=s.SizeOfRawData){auto off=s.PointerToRawData+rva-s.VirtualAddress;if(off+n<=data.size())return data.data()+off;}}return nullptr;};
 auto* native=at(WindowRva,sizeof(Window));CHECK(native&&!memcmp(native,Window,sizeof(Window)));
 CHECK(!memcmp(at(ProcessRva,sizeof(ProcessPrefix)),ProcessPrefix,sizeof(ProcessPrefix)));
 unsigned char relocated[sizeof(Window)]{};CHECK(RelocatedWindow(0x20000000,relocated));uint32_t operand=0;
 memcpy(&operand,relocated+2,4);CHECK(operand==0x2024dd14);memcpy(&operand,relocated+23,4);CHECK(operand==0x203fa4b8);
 CHECK(!RelocatedWindow(UINT32_MAX,relocated));
 // Execute the verified native branch fragment in owned fixture memory,
 // redirecting ONLY its cursor store. No game DLL is loaded or executed.
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(code);
 memcpy(code,native+8,sizeof(Window)-8);uint32_t cursorAddress=uint32_t(reinterpret_cast<uintptr_t>(&resumeCursor));memcpy(code+15,&cursorAddress,4);
 code[19]=0xe9;int32_t finish=int32_t(reinterpret_cast<uintptr_t>(&Finish)-reinterpret_cast<uintptr_t>(code+24));memcpy(code+20,&finish,4);
 DWORD old=0;CHECK(VirtualProtect(code,64,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,64);fragment=code;
 auto verify=[&](unsigned limit){for(uint32_t elapsed:{0u,1u,3u,4u,15u,16u,17u,0xffffffffu}){
  State state{elapsed+123u,123,42,999,999};resumeCursor=0;Run(&state,nullptr);
  if(state.flag!=uint32_t(elapsed>limit)||state.elapsed!=elapsed||resumeCursor!=43){printf("limit=%u elapsed=%u flag=%u observed=%u cursor=%u\n",limit,elapsed,state.flag,state.elapsed,resumeCursor);return false;}
 }return true;};
 CHECK(verify(15));ConditionalGate gate;CHECK(gate.Install(code));CHECK(verify(15));gate.Limit(3);CHECK(verify(3));gate.Limit(15);CHECK(verify(15));
 ConditionalGate duplicate;CHECK(!duplicate.Install(code));CHECK(gate.RestoreQuiescent()&&verify(15));VirtualFree(code,0,MEM_RELEASE);
 BudgetPolicy policy;CHECK(policy.Select(false,0,0)==15);CHECK(policy.Select(true,10,0)==3);CHECK(policy.Select(true,200,10)==3);
 CHECK(policy.Select(true,260,15)==15&&policy.Select(true,300,16)==15);CHECK(policy.Select(true,310,0)==3);
 CHECK(policy.Select(true,100,1)==3);CHECK(policy.Select(true,100,-1)==15);CHECK(policy.Select(false,100,5)==15);
 // Native cursor continuation visits every deferred actor. Native simple
 // movement consumes its accumulated time when visited, preserving travel.
 unsigned cursor=0;std::vector<unsigned> visits(100);std::vector<float> accumulated(100),travel(100);
 for(unsigned frame=0;frame<100;++frame){for(auto& dt:accumulated)dt+=.02f;unsigned elapsed=0;
  while(cursor<100){unsigned actor=cursor++;++visits[actor];travel[actor]+=accumulated[actor]*100.f;accumulated[actor]=0;elapsed+=2;if(elapsed>3)break;}
  if(cursor==100)cursor=0;
 }
 for(unsigned i=0;i<100;++i){CHECK(visits[i]==2);CHECK(std::fabs(travel[i]+accumulated[i]*100.f-200.f)<.001f);}
 puts("PASS exact native unsigned timer gate, preserved registers/stack/cursor, wraparound, native restore, bounded catch-up and deferred progress");
}
