#include "NativeApplication.h"
#include "NativeRenderProfile.h"
#include "NativeSceneExecutionProfile.h"
#include "FocusHook.h"
#include "TemporalRenderPhase.h"
#include <vector>
#include <cstdio>
namespace {
diagnostic::FocusHook batchHook;
unsigned batchCalls=0;
using BatchCall=uint32_t(__stdcall*)();
uint32_t __stdcall SampleBatch(){++batchCalls;return reinterpret_cast<BatchCall>(batchHook.original)();}
bool VerifyRelocatedBatchTrampoline(){
    using namespace diagnostic::performance;
    auto* image=static_cast<unsigned char*>(VirtualAlloc(nullptr,InstanceBatchGlobalRva+4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!image)return false;
    auto const& entry=ExecutionEntries[1];unsigned char prefix[16]{};
    bool ok=ExecutionPrefixAtBase(entry,uint32_t(reinterpret_cast<uintptr_t>(image)),prefix);
    auto* code=image+entry.rva;uint32_t value=0x1234abcd;
    std::memcpy(image+InstanceBatchGlobalRva,&value,sizeof(value));
    std::memcpy(code,prefix,entry.bytes);
    // MOV EAX,EDI; POP EDI; POP ESI; POP ECX; RET after the original MOV.
    unsigned char tail[]={0x8b,0xc7,0x5f,0x5e,0x59,0xc3};std::memcpy(code+entry.bytes,tail,sizeof(tail));
    DWORD old=0;ok=ok&&VirtualProtect(code,entry.bytes+sizeof(tail),PAGE_EXECUTE_READ,&old);
    if(ok){FlushInstructionCache(GetCurrentProcess(),code,entry.bytes+sizeof(tail));
        auto call=reinterpret_cast<BatchCall>(code);ok=call()==value;
        auto bad=entry;bad.prefix[0]^=1;diagnostic::FocusHook refused;
        unsigned char invalid[16]{};ExecutionPrefixAtBase(bad,uint32_t(reinterpret_cast<uintptr_t>(image)),invalid);
        ok=ok&&!refused.Install(code,reinterpret_cast<void*>(&SampleBatch),invalid,entry.bytes)&&!std::memcmp(code,prefix,entry.bytes);
        ok=ok&&batchHook.Install(code,reinterpret_cast<void*>(&SampleBatch),prefix,entry.bytes);
        if(ok){ok=call()==value&&batchCalls==1;value=0x76543210;std::memcpy(image+InstanceBatchGlobalRva,&value,sizeof(value));ok=ok&&call()==value&&batchCalls==2;}
    }
    if(batchHook.original){VirtualFree(batchHook.original,0,MEM_RELEASE);batchHook.original=nullptr;}
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
}
int __cdecl wmain(int argc,wchar_t** argv){
    if(argc!=2)return 1;unsigned char actual[32]{},expected[32]{0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    if(!diagnostic::FileSha256(argv[1],actual)||memcmp(actual,expected,32))return 2;
    HANDLE f=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return 3;
    DWORD size=GetFileSize(f,nullptr),read=0;std::vector<unsigned char> data(size);bool ok=ReadFile(f,data.data(),size,&read,nullptr)&&read==size;CloseHandle(f);if(!ok)return 4;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(data.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(data.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    auto at=[&](unsigned rva,size_t n)->unsigned char const*{for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+n<=s.SizeOfRawData){size_t off=s.PointerToRawData+rva-s.VirtualAddress;if(off+n<=data.size())return data.data()+off;}}return nullptr;};
    auto directory=reinterpret_cast<IMAGE_EXPORT_DIRECTORY const*>(at(nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,sizeof(IMAGE_EXPORT_DIRECTORY)));if(!directory)return 5;
    auto names=reinterpret_cast<DWORD const*>(at(directory->AddressOfNames,directory->NumberOfNames*4));
    auto ordinals=reinterpret_cast<WORD const*>(at(directory->AddressOfNameOrdinals,directory->NumberOfNames*2));
    auto functions=reinterpret_cast<DWORD const*>(at(directory->AddressOfFunctions,directory->NumberOfFunctions*4));if(!names||!ordinals||!functions)return 6;
    for(auto const& entry:diagnostic::performance::RenderEntries){bool found=false;
        for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],strlen(entry.symbol)+1);if(name&&!memcmp(name,entry.symbol,strlen(entry.symbol)+1)&&ordinals[i]<directory->NumberOfFunctions&&functions[ordinals[i]]==entry.rva){found=true;break;}}
        auto prefix=at(entry.rva,entry.bytes);if(!found||!prefix||memcmp(prefix,entry.prefix,entry.bytes))return 7;
        // The four permitted sequences end on whole instructions before
        // any relative call/jump. No generic relocation is trusted.
        unsigned char normal[]{0x55,0x8b,0xec,0x83,0xe4,0xf8},update[]{0x83,0x41,0x04,0x01,0x51,0x83,0xc1,0x40},refresh[]{0x53,0x56,0x8b,0xf1,0x57},wait[]{0x56,0x8b,0xf1,0x8b,0x4e,0x0c};
        unsigned char cacheIn[]{0x51,0x56,0x8b,0xf1,0x80,0x7e,0x58,0x01},poolQueue[]{0x53,0x55,0x8b,0xe9,0x83,0x7d,0x04,0x00},poolIdle[]{0x56,0x8b,0xf1,0x83,0x7e,0x3c,0x00};
        if(!((entry.bytes==6&&(!memcmp(prefix,normal,6)||!memcmp(prefix,wait,6)))||(entry.bytes==8&&(!memcmp(prefix,update,8)||!memcmp(prefix,cacheIn,8)||!memcmp(prefix,poolQueue,8)))||(entry.bytes==7&&!memcmp(prefix,poolIdle,7))||(entry.bytes==5&&!memcmp(prefix,refresh,5))))return 8;
    }
    // Separate scene execution detours retain the complete verified prologue.
    // This includes the static __stdcall batch flush (not a thiscall method).
    for(auto const& entry:diagnostic::performance::ExecutionEntries){
        bool found=false;
        for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],strlen(entry.symbol)+1);
            if(name&&!memcmp(name,entry.symbol,strlen(entry.symbol)+1)&&ordinals[i]<directory->NumberOfFunctions&&functions[ordinals[i]]==entry.rva){found=true;break;}}
        auto prefix=at(entry.rva,entry.bytes);
        if(!found||!prefix||entry.bytes<5||entry.bytes>16||memcmp(prefix,entry.prefix,entry.bytes))return 15;
    }
    // Prove the exact operand is declared HIGHLOW by the native PE loader data.
    auto relocation=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    auto reloc=at(relocation.VirtualAddress,relocation.Size);bool batchRelocation=false;
    if(!reloc)return 16;
    for(size_t cursor=0;cursor<relocation.Size;){
        if(relocation.Size-cursor<sizeof(IMAGE_BASE_RELOCATION))return 16;
        auto block=reinterpret_cast<IMAGE_BASE_RELOCATION const*>(reloc+cursor);
        if(block->SizeOfBlock<sizeof(*block)||block->SizeOfBlock>relocation.Size-cursor||(block->SizeOfBlock-sizeof(*block))%2)return 16;
        auto fixups=reinterpret_cast<WORD const*>(reloc+cursor+sizeof(*block));
        for(unsigned i=0;i<(block->SizeOfBlock-sizeof(*block))/2;++i){
            if(block->VirtualAddress+(fixups[i]&0xfff)==0x1db2f5){if((fixups[i]>>12)!=IMAGE_REL_BASED_HIGHLOW)return 16;batchRelocation=true;}
        }
        cursor+=block->SizeOfBlock;
    }
    if(!batchRelocation||nt->OptionalHeader.ImageBase!=diagnostic::performance::ExecutionPreferredBase||!VerifyRelocatedBatchTrampoline())return 17;
    auto same=[&](unsigned rva,std::initializer_list<unsigned char> value){auto p=at(rva,value.size());return p&&std::equal(value.begin(),value.end(),p);};
    // These two native virtual Z-pass implementations are immediate no-op
    // returns (potentially shared stubs). Do not detour them as scene work.
    if(!same(0x24ff00,{0xb0,0x01,0xc2,0x04,0x00})||!same(0x1b68b0,{0x32,0xc0,0xc2,0x04,0x00}))return 18;
    // Prove the exact phase callback and relative call target against the real
    // engine file; its later dynamic-occludee pass must remain after EndSolid.
    using namespace diagnostic::temporal;
    for(auto const& entry:GeometryRenderEntries){
        bool exported=false;for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],strlen(entry.symbol)+1);if(name&&!strcmp(reinterpret_cast<char const*>(name),entry.symbol)&&functions[ordinals[i]]==entry.rva)exported=true;}
        auto code=at(entry.rva,entry.size);if(!exported||!code||memcmp(code,entry.prefix,entry.size))return 14;
    }
    auto phase=at(EndSolidRva,sizeof(EndSolidPrefix));auto call=at(EndSolidCallRva,sizeof(EndSolidCall));
    if(!phase||memcmp(phase,EndSolidPrefix,sizeof(EndSolidPrefix))||!call||memcmp(call,EndSolidCall,sizeof(EndSolidCall)))return 10;
    int32_t displacement=0;memcpy(&displacement,call+1,4);
    if(uint32_t(EndSolidReturnRva+displacement)!=EndSolidRva||
       !same(0x1a0ef3,{0xe8,0x38,0x39,0x00,0x00})||EndSolidReturnRva>=0x1a0ef3)return 11;
    bool phaseExport=false;
    for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],sizeof(EndSolidSymbol));if(name&&!memcmp(name,EndSolidSymbol,sizeof(EndSolidSymbol))&&functions[ordinals[i]]==EndSolidRva)phaseExport=true;}
    if(!phaseExport)return 12;
    auto animated=at(AnimatedRenderRva,sizeof(AnimatedRenderPrefix));bool animationExport=false;
    for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],sizeof(AnimatedRenderSymbol));if(name&&!memcmp(name,AnimatedRenderSymbol,sizeof(AnimatedRenderSymbol))&&functions[ordinals[i]]==AnimatedRenderRva)animationExport=true;}
    if(!animationExport||!animated||memcmp(animated,AnimatedRenderPrefix,sizeof(AnimatedRenderPrefix))||
       !same(AnimatedRenderRva+11,{0x83,0x7e,0x20,0x00})|| // SDK animation PS at context+20h
       !same(AnimatedRenderRva+0x6c,{0xe8,0x8f,0xcc,0xfd,0xff}))return 13; // AnimationAdmin::Render
    // After sub esp,0Ch and four register pushes, esp+20h is Render's
    // FIRST stack argument; esp+24h is the second (frustum rebuild).
    if(!same(0x5ee93,{0x80,0x7c,0x24,0x14,1})||
       !same(0x5ef01,{0x32,0xc0})|| // native camera-distance fallback -> false
       !same(0x5ef05,{0x8a,0x44,0x24,0x20})||
       !same(0x5ef1c,{0xa2,0x86,0x34,0x5e,0x10})|| // stores AL into the pool flag
       !same(0x1f4280,{0xa0,0x86,0x34,0x5e,0x10,0xc3})||
       !same(0x1f3c60,{0x8a,0x44,0x24,4,0xa2,0x86,0x34,0x5e,0x10,0xc2,4,0})||
       !same(0x1a6cae,{0x80,0x3d,0x86,0x34,0x5e,0x10,0}))return 9;
    printf("PASS native camera first-bool threading, synchronous distance fallback, pool getter/setter and PVS job dispatch; real Engine render/refresh/PVS exports/RVAs and %u whole relocatable prefixes; no native DLL execution\n",diagnostic::performance::RenderEntryCount);return 0;
}
