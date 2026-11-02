#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
namespace diagnostic::navigation {
static_assert(sizeof(std::atomic<uint32_t>)==4,"x86 gate requires a single aligned 32-bit limit");
inline constexpr uint32_t ProcessRva=0x120720,WindowRva=0x120fd8,GateRva=0x120fe0,CursorRva=0x3fa4b8;
inline constexpr unsigned char ProcessPrefix[]={0x55,0x8b,0xec,0x83,0xe4,0xf8};
// Original timer call, elapsed subtraction, unsigned >15ms test, yield flag,
// and the native next-NPC cursor write. No calls or cursor writes are removed.
inline constexpr unsigned char Window[]={0xff,0x15,0x14,0xdd,0x24,0x10,0x2b,0xc7,0x83,0xf8,0x0f,0x76,0x05,0xc6,0x44,0x24,0x12,0x01,0x8d,0x4b,0x01,0x89,0x0d,0xb8,0xa4,0x3f,0x10};
inline bool RelocatedWindow(uint32_t base,unsigned char (&out)[sizeof(Window)])noexcept{
 if(!base||base>UINT32_MAX-CursorRva)return false;std::memcpy(out,Window,sizeof(out));
 uint32_t timer=base+0x24dd14,cursor=base+CursorRva;std::memcpy(out+2,&timer,4);std::memcpy(out+23,&cursor,4);return true;
}
class BudgetPolicy {
 uint64_t cycle=0,last=0;int previous=-1;bool active=false;
public:
 static constexpr uint32_t NativeLimit=15,VrLimit=3; // Yield after >=16ms / >=4ms, at unit boundaries only.
 static constexpr uint64_t CatchUpAfterMs=250;
 uint32_t Select(bool vr,uint64_t now,int cursor)noexcept{
  if(!vr||cursor<0||cursor>1000000){active=false;previous=-1;return NativeLimit;}
  if(!active||cursor==0||cursor<previous||now<last){cycle=now;active=true;}
  previous=cursor;last=now;
  // A slow backlog borrows the original native budget until the native cursor
  // wraps. Do not indefinitely postpone distant actors on a busy scene.
  return now-cycle>=CatchUpAfterMs?NativeLimit:VrLimit;
 }
};
class ConditionalGate {
 unsigned char* site=nullptr;unsigned char* code=nullptr;
 alignas(4) std::atomic<uint32_t> limit{BudgetPolicy::NativeLimit};
public:
 ConditionalGate()=default;ConditionalGate(ConditionalGate const&)=delete;
 bool Installed()const noexcept{return site!=nullptr;}
 void Limit(uint32_t value)noexcept{limit.store(value<=BudgetPolicy::NativeLimit?value:BudgetPolicy::NativeLimit,std::memory_order_relaxed);}
 bool Install(void* target)noexcept{
  if(site||!target||sizeof(void*)!=4)return false;auto* bytes=static_cast<unsigned char*>(target);
  if(std::memcmp(bytes,Window+8,10))return false;
  auto* next=static_cast<unsigned char*>(VirtualAlloc(nullptr,17,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!next)return false;
  // CMP EAX,[limit]; JBE native_after_yield_flag; JMP native_set_yield_flag.
  // No register/stack changes. EAX remains the original elapsed milliseconds.
  unsigned char gate[]={0x3b,0x05,0,0,0,0,0x0f,0x86,0,0,0,0,0xe9,0,0,0,0};
  uint32_t address=uint32_t(reinterpret_cast<uintptr_t>(&limit));std::memcpy(gate+2,&address,4);
  int32_t noYield=int32_t(reinterpret_cast<uintptr_t>(bytes+10)-reinterpret_cast<uintptr_t>(next+12));
  int32_t yield=int32_t(reinterpret_cast<uintptr_t>(bytes+5)-reinterpret_cast<uintptr_t>(next+17));
  std::memcpy(gate+8,&noYield,4);std::memcpy(gate+13,&yield,4);std::memcpy(next,gate,sizeof(gate));
  DWORD old=0;if(!VirtualProtect(next,17,PAGE_EXECUTE_READ,&old)){VirtualFree(next,0,MEM_RELEASE);return false;}
  FlushInstructionCache(GetCurrentProcess(),next,17);
  unsigned char jump[5]{0xe9};int32_t displacement=int32_t(reinterpret_cast<uintptr_t>(next)-reinterpret_cast<uintptr_t>(bytes+5));std::memcpy(jump+1,&displacement,4);
  if(!VirtualProtect(bytes,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(next,0,MEM_RELEASE);return false;}
  std::memcpy(bytes,jump,5);DWORD unused=0;VirtualProtect(bytes,5,old,&unused);FlushInstructionCache(GetCurrentProcess(),bytes,5);
  code=next;site=bytes;return true;
 }
 // Test/failed-setup cleanup only, while no caller can execute the fragment.
 // The installed runtime gate lives until process exit like other trampolines.
 bool RestoreQuiescent()noexcept{
  if(!site)return true;DWORD old=0;if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old))return false;
  std::memcpy(site,Window+8,5);DWORD unused=0;VirtualProtect(site,5,old,&unused);FlushInstructionCache(GetCurrentProcess(),site,5);
  site=nullptr;VirtualFree(code,0,MEM_RELEASE);code=nullptr;return true;
 }
};
class NativeBudget {
 ConditionalGate gate;BudgetPolicy policy;int const* cursor=nullptr;std::atomic<bool> reset{true};
public:
 bool Install(void* module)noexcept{
  auto base=static_cast<unsigned char*>(module);unsigned char expected[sizeof(Window)]{};
  if(!base||!RelocatedWindow(uint32_t(reinterpret_cast<uintptr_t>(base)),expected)||std::memcmp(base+ProcessRva,ProcessPrefix,sizeof(ProcessPrefix))||
     std::memcmp(base+WindowRva,expected,sizeof(expected))||!gate.Install(base+GateRva))return false;
  cursor=reinterpret_cast<int const*>(base+CursorRva);return true;
 }
 // Shutdown can arrive from another owner: publish only atomic state there.
 // Policy/cursor bookkeeping remains owned by the native render thread.
 void Native()noexcept{gate.Limit(BudgetPolicy::NativeLimit);reset.store(true,std::memory_order_release);}
 void Frame(bool vr,uint64_t now)noexcept{
  if(!gate.Installed())return;if(!vr){Native();return;}
  if(reset.exchange(false,std::memory_order_acq_rel))policy.Select(false,0,0);
  gate.Limit(policy.Select(true,now,*cursor));
 }
};
}
