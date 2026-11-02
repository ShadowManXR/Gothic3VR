#pragma once
#include <windows.h>
#include <cwctype>
#include <string>
#include "ControllerMapping.h"
namespace diagnostic::mapping {
struct alignas(8) Shared {
 volatile LONG sequence=0;uint32_t magic=0,version=0,pid=0;
 uint64_t stamp=0;uint32_t buttons=0,available=0,focused=0,hands=0;
 volatile LONG capturedButtons=0;
 alignas(8) volatile LONG64 captureUntil=0;
};
struct Telemetry {uint64_t stamp=0;uint32_t pid=0,buttons=0,available=0,focused=0,hands=0,captured=0;};
class Link {
 HANDLE handle=nullptr;Shared* data=nullptr;std::wstring opened;
 static std::wstring Name(wchar_t const* ini){uint64_t hash=14695981039346656037ull;for(auto p=ini;*p;++p){hash^=uint16_t(*p==L'/'?L'\\':towlower(*p));hash*=1099511628211ull;}wchar_t name[96];swprintf_s(name,L"Local\\Gothic3VR.ControllerMapper.%016llx",hash);return name;}
public:
 ~Link(){Close();}void Close(){if(data)UnmapViewOfFile(data);if(handle)CloseHandle(handle);data=nullptr;handle=nullptr;opened.clear();}
 bool Open(wchar_t const* ini,bool server=false){auto name=Name(ini);if(data&&name==opened)return true;Close();
  handle=server?CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Shared),name.c_str()):OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name.c_str());
  if(!handle)return false;data=static_cast<Shared*>(MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared)));if(!data){Close();return false;}opened=name;
  if(server){InterlockedExchange(&data->sequence,0);data->stamp=0;data->buttons=data->available=data->focused=data->hands=0;InterlockedExchange(&data->capturedButtons,0);InterlockedExchange64(&data->captureUntil,0);data->magic=0x33434d50;data->version=2;data->pid=GetCurrentProcessId();}return true;
 }
 void Publish(ControllerFrame const& f,uint32_t buttons){if(!data)return;
  InterlockedIncrement(&data->sequence);data->stamp=f.timeMs;data->buttons=buttons;data->available=f.availableButtons;
  data->focused=f.focused;data->hands=(f.hands[0].active?1:0)|(f.hands[1].active?2:0);MemoryBarrier();InterlockedIncrement(&data->sequence);
  // Latch the largest simultaneous chord during listening. A 30 ms press
  // must survive the launcher's 100 ms timer; separate taps never become a
  // fabricated combination. No capture work when the lease is inactive.
  if(f.focused&&buttons&&Capturing(GetTickCount64())){
   auto old=InterlockedCompareExchange(&data->capturedButtons,0,0);
   auto count=[](uint32_t v){unsigned n=0;for(;v;v&=v-1)++n;return n;};
   if(count(buttons)>count(uint32_t(old)))InterlockedCompareExchange(&data->capturedButtons,LONG(buttons),old);
  }
 }
 bool Read(Telemetry& out)const {if(!data||data->magic!=0x33434d50||data->version!=2)return false;
  for(int i=0;i<3;++i){auto before=InterlockedCompareExchange(&data->sequence,0,0);if(before&1)continue;MemoryBarrier();
   Telemetry candidate{data->stamp,data->pid,data->buttons,data->available,data->focused,data->hands,uint32_t(InterlockedCompareExchange(&data->capturedButtons,0,0))};MemoryBarrier();
   if(before==InterlockedCompareExchange(&data->sequence,0,0)){out=candidate;return true;}}
  return false;
 }
 void ClearCaptured(){if(data)InterlockedExchange(&data->capturedButtons,0);}
 void Capture(uint64_t until){if(data)InterlockedExchange64(&data->captureUntil,LONG64(until));}
 bool Capturing(uint64_t now)const{if(!data)return false;auto until=uint64_t(InterlockedCompareExchange64(&data->captureUntil,0,0));return until>now&&until-now<=2000;}
};
}
