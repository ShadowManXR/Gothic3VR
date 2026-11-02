#include "NativeApplication.h"
#include "FocusHook.h"
#include "PupPrefetchBudget.h"
#include "PupLowPolyProfile.h"
#include "PupOcclusionProfile.h"
#include "NativeOcclusion.h"
#include <vector>
#include <cstdio>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
static unsigned fakeCalls=0;static uint32_t fakeStarted=0;static bool fakeForced=false;
void __cdecl FakeBody(bool forced,uint32_t started){++fakeCalls;fakeForced=forced;fakeStarted=started;}
__declspec(naked) void __fastcall FakeWork(uint32_t,bool){
    __asm {
        push ebp
        mov ebp,esp
        sub esp,20h
        push ecx
        movzx eax,dl
        push eax
        call FakeBody
        add esp,8
        mov esp,ebp
        pop ebp
        ret
    }
}
static FocusHook fakeHook;
void __fastcall ForwardWork(uint32_t started,bool forced){reinterpret_cast<void(__fastcall*)(uint32_t,bool)>(fakeHook.original)(started,forced);}
int __cdecl wmain(int argc,wchar_t** argv){
    CHECK(argc==2);unsigned char digest[32]{};CHECK(FileSha256(argv[1],digest)&&!memcmp(digest,pup::FileHash,32));
    HANDLE f=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);CHECK(f!=INVALID_HANDLE_VALUE);
    DWORD size=GetFileSize(f,nullptr),read=0;std::vector<unsigned char> bytes(size);bool ok=ReadFile(f,bytes.data(),size,&read,nullptr)&&read==size;CloseHandle(f);CHECK(ok);
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(bytes.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(bytes.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    auto at=[&](unsigned rva,size_t n)->unsigned char const*{for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+n<=s.SizeOfRawData){size_t off=s.PointerToRawData+rva-s.VirtualAddress;if(off+n<=bytes.size())return bytes.data()+off;}}return nullptr;};
    auto same=[&](unsigned rva,std::initializer_list<unsigned char> value){auto p=at(rva,value.size());return p&&std::equal(value.begin(),value.end(),p);};
    CHECK(nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386&&nt->OptionalHeader.SizeOfImage>pup::QueueLockRva+sizeof(CRITICAL_SECTION));
    CHECK(!memcmp(at(pup::WorkRva,sizeof(pup::WorkPrefix)),pup::WorkPrefix,sizeof(pup::WorkPrefix)));
    CHECK(same(0x111c6,{0x89,0x4d,0xe0})&&same(0x111ce,{0x88,0x55,0xff})); // ECX timestamp, DL force, no stack args
    CHECK(same(0x11578,{0x80,0x7d,0xff,0})&&same(0x11587,{0x83,0xf8,5})&&same(0x115e3,{0xc3}));
    DWORD lock=nt->OptionalHeader.ImageBase+pup::QueueLockRva;CHECK(at(0x111ca,4)&&!memcmp(at(0x111ca,4),&lock,4));
    CHECK(same(0x111d1,{0xff,0x15,0x98,0x79,0x06,0x10})&&same(0x115da,{0xff,0x15,0x94,0x79,0x06,0x10}));
    // PUP's own budget-exhaustion path compacts/preserves the remaining queue.
    CHECK(same(0x1158a,{0x73,0x7e})&&same(0x1160a,{0x8b,0x75,0xf0}));
    // Validate PUP's exact original-call wrapper, installer binding, and the
    // loader relocations for all three absolute operands. No PUP code runs.
    CHECK(at(pup::LowPolyWrapperRva,sizeof(pup::LowPolyBody))&&!memcmp(at(pup::LowPolyWrapperRva,sizeof(pup::LowPolyBody)),pup::LowPolyBody,sizeof(pup::LowPolyBody)));
    CHECK(same(0x11c65,{0x68,0x48,0x15,0x08,0x10})&&same(0x11c9a,{0x68,0x80,0x16,0x01,0x10}));
    constexpr char lowPolySymbol[]="?RenderLowPoly@eCCameraBase@@QAEXXZ";
    CHECK(at(0x81548,sizeof(lowPolySymbol))&&!memcmp(at(0x81548,sizeof(lowPolySymbol)),lowPolySymbol,sizeof(lowPolySymbol)));
    CHECK(nt->OptionalHeader.SizeOfImage>pup::LowPolyFlagRva);
    auto relocDir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];auto reloc=at(relocDir.VirtualAddress,relocDir.Size);CHECK(reloc);
    unsigned fixes=0;
    for(size_t cursor=0;cursor<relocDir.Size;){
        CHECK(relocDir.Size-cursor>=sizeof(IMAGE_BASE_RELOCATION));auto block=reinterpret_cast<IMAGE_BASE_RELOCATION const*>(reloc+cursor);
        CHECK(block->SizeOfBlock>=sizeof(*block)&&block->SizeOfBlock<=relocDir.Size-cursor&&!((block->SizeOfBlock-sizeof(*block))%2));
        auto rows=reinterpret_cast<WORD const*>(reloc+cursor+sizeof(*block));
        for(unsigned i=0;i<(block->SizeOfBlock-sizeof(*block))/2;++i){auto target=block->VirtualAddress+(rows[i]&0xfff);
            for(unsigned j=0;j<3;++j)if(target==pup::LowPolyWrapperRva+(j==0?2:j==1?9:15)){CHECK((rows[i]>>12)==IMAGE_REL_BASED_HIGHLOW);fixes|=1u<<j;}}
        cursor+=block->SizeOfBlock;
    }
    CHECK(fixes==7);
    unsigned char wrapper[sizeof(pup::LowPolyBody)]{};
    CHECK(pup::LowPolyBodyAtBase(nt->OptionalHeader.ImageBase,wrapper)&&!memcmp(wrapper,pup::LowPolyBody,sizeof(wrapper)));
    CHECK(pup::LowPolyBodyAtBase(0x028c0000,wrapper));uint32_t flag=0,slot=0,secondFlag=0;
    memcpy(&flag,wrapper+2,4);memcpy(&slot,wrapper+9,4);memcpy(&secondFlag,wrapper+15,4);
    CHECK(flag==0x029594ac&&secondFlag==flag&&slot==0x029573e8&&wrapper[6]==1&&wrapper[19]==0&&wrapper[20]==0xc3);
    CHECK(!pup::LowPolyBodyAtBase(0,wrapper)&&!pup::LowPolyBodyAtBase(0xffff0000,wrapper));
    CHECK(pup::LowPolyClass(0)==0&&pup::LowPolyClass(1)==1&&pup::LowPolyClass(2)==-1&&pup::LowPolyClass(255)==-1);
    // PUP's native-layout derived device keeps the Engine occlusion method
    // through an exact FF25 import thunk, rather than Engine's own vtable.
    uint32_t entry=0;CHECK(at(pup::OcclusionDeviceTableRva+diagnostic::occlusion::QueryInfoSlot*4,4));
    memcpy(&entry,at(pup::OcclusionDeviceTableRva+diagnostic::occlusion::QueryInfoSlot*4,4),4);
    CHECK(entry==nt->OptionalHeader.ImageBase+pup::OcclusionThunkRva);
    unsigned char occlusionThunk[6]{};CHECK(pup::OcclusionThunkAtBase(nt->OptionalHeader.ImageBase,occlusionThunk));
    CHECK(at(pup::OcclusionThunkRva,6)&&!memcmp(at(pup::OcclusionThunkRva,6),occlusionThunk,6));
    CHECK(pup::OcclusionThunkAtBase(0x028c0000,occlusionThunk));uint32_t occlusionCell=0;memcpy(&occlusionCell,occlusionThunk+2,4);CHECK(occlusionCell==0x029274d4);
    CHECK(!pup::OcclusionThunkAtBase(0,occlusionThunk)&&!pup::OcclusionThunkAtBase(0xffff0000,occlusionThunk));
    auto imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto imported=[&](uint32_t cell,char const* symbol){
        for(size_t off=0;off+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=imports.Size;off+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
            auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR const*>(at(imports.VirtualAddress+uint32_t(off),sizeof(IMAGE_IMPORT_DESCRIPTOR)));
            if(!d||!d->Name)return false;
            auto* name=reinterpret_cast<char const*>(at(d->Name,11));
            if(!name||strcmp(name,"Engine.dll"))continue;
            if(cell<d->FirstThunk||(cell-d->FirstThunk)%4||!d->OriginalFirstThunk)return false;
            auto* lookup=at(d->OriginalFirstThunk+cell-d->FirstThunk,4);if(!lookup)return false;
            uint32_t symbolRva=0;memcpy(&symbolRva,lookup,4);if(IMAGE_SNAP_BY_ORDINAL32(symbolRva))return false;
            auto* symbolName=at(symbolRva+2,strlen(symbol)+1);return symbolName&&!memcmp(symbolName,symbol,strlen(symbol)+1);
        }return false;
    };
    CHECK(imported(pup::OcclusionIatRva,diagnostic::occlusion::QueryInfoSymbol));
    CHECK(imported(0x67424,"??0eCDX9Device@@IAE@PAUIDirect3DDevice9@@@Z"));
    // Constructor calls the original Engine base ctor on unadjusted this,
    // then replaces only the vtable; RTTI PMD confirms base offset zero.
    CHECK(same(0x6697,{0x8b,0x75,8,0x8b,0xce,0xff,0x15,0x24,0x74,6,0x10,0xc7,6,0x70,0x0b,8,0x10}));
    CHECK(same(0x87140,{0,0,0,0,0xff,0xff,0xff,0xff,0,0,0,0}));
    bool thunkRelocation=false,tableRelocation=false;
    for(size_t cursor=0;cursor<relocDir.Size;){
        auto block=reinterpret_cast<IMAGE_BASE_RELOCATION const*>(reloc+cursor);auto rows=reinterpret_cast<WORD const*>(reloc+cursor+sizeof(*block));
        for(unsigned j=0;j<(block->SizeOfBlock-sizeof(*block))/2;++j){auto address=block->VirtualAddress+(rows[j]&0xfff);
            if(address==pup::OcclusionThunkRva+2){CHECK((rows[j]>>12)==IMAGE_REL_BASED_HIGHLOW);thunkRelocation=true;}
            if(address==pup::OcclusionDeviceTableRva+diagnostic::occlusion::QueryInfoSlot*4){CHECK((rows[j]>>12)==IMAGE_REL_BASED_HIGHLOW);tableRelocation=true;}}
        cursor+=block->SizeOfBlock;
    }
    CHECK(thunkRelocation&&tableRelocation);
    CHECK(fakeHook.Install(reinterpret_cast<void*>(&FakeWork),reinterpret_cast<void*>(&ForwardWork),pup::WorkPrefix,sizeof(pup::WorkPrefix)));
    FakeWork(0xfffffffdu,false);CHECK(fakeCalls==1&&fakeStarted==0xfffffffdu&&!fakeForced);
    FakeWork(17,true);CHECK(fakeCalls==2&&fakeStarted==17&&fakeForced);
    puts("PASS exact PUP1.1.1 work/lock/budget profile and original ECX/DL x86 trampoline; force/timestamp preserved; native PUP DLL not executed");return 0;
}
