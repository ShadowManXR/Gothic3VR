#include "NativeTurnInterpolation.h"
#include "NativeApplication.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>
#include <stdexcept>
using namespace diagnostic;
#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
// Execute only copied, verified arithmetic/getter fragments, with fixture
// application/time imports. No LoadLibrary/Game initialization or game process.
struct Pe {
    std::vector<unsigned char> data;
    explicit Pe(wchar_t const* path){std::ifstream in(std::filesystem::path(path),std::ios::binary);data.assign(std::istreambuf_iterator<char>(in),{});}
    unsigned char const* At(unsigned rva,size_t count)const{
        if(data.size()<1024)throw std::runtime_error("short PE");
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(data.data());
        if(dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>data.size())throw std::runtime_error("bad DOS header");
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(data.data()+dos->e_lfanew);auto* sections=IMAGE_FIRST_SECTION(nt);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||
           reinterpret_cast<unsigned char const*>(sections+nt->FileHeader.NumberOfSections)>data.data()+data.size())throw std::runtime_error("bad PE");
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva<s.VirtualAddress||rva-s.VirtualAddress+count>s.SizeOfRawData)continue;
            size_t at=s.PointerToRawData+rva-s.VirtualAddress;if(at+count<=data.size())return data.data()+at;}
        throw std::runtime_error("unmapped RVA");
    }
};
float gDt=0,gZero=0,gOne=1;
void* gAlpha=nullptr;
void* __cdecl App(){return &gDt;}
float __fastcall FrameTime(void* self,void*){return *static_cast<float*>(self);}
float __stdcall UnexpectedRamp(float){RaiseException(0xe0470001,0,0,nullptr);return 0;}
void* gAppEntry=reinterpret_cast<void*>(&App),*gFrameEntry=reinterpret_cast<void*>(&FrameTime);
__declspec(naked) float __fastcall NativeAlpha(void*){
    __asm {push ebx
        push edi
        sub esp,32
        mov edi,ecx
        call dword ptr [gAlpha]
        add esp,32
        pop edi
        pop ebx
        ret}
}
float WithLease(void* camera,float dt,bool fault=false){
    NativeTurnInterpolation lease;
    if(!lease.Begin(camera,0x558,dt))return -1;
    __try {if(fault)RaiseException(0xe0470002,0,0,nullptr);return NativeAlpha(camera);}
    __finally {lease.End();}
}
bool CatchLeaseFault(void* camera){
    __try {WithLease(camera,gDt,true);return false;}
    __except(GetExceptionCode()==0xe0470002?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
}
void Put(void* camera,size_t offset,float value){std::memcpy(static_cast<unsigned char*>(camera)+offset,&value,4);}
int __cdecl wmain(int argc,wchar_t** argv){
    CHECK(argc==2);unsigned char hash[32]{};
    unsigned char expected[]{0x47,0x28,0xa4,0x7b,0x24,0x4a,0xdc,0xb0,0xea,0x2e,0xc3,0x50,0x1d,0xb0,0x49,0x94,0x90,0x9d,0xcf,0xcf,0xf3,0x38,0xc7,0xb8,0x07,0x5f,0x2b,0x68,0xdd,0x8d,0x6e,0x00};
    CHECK(FileSha256(argv[1],hash)&&!memcmp(hash,expected,32));Pe pe(argv[1]);
    CHECK(!memcmp(pe.At(TurnRampLoadRva,sizeof(TurnRampLoad)),TurnRampLoad,sizeof(TurnRampLoad)));
    CHECK(!memcmp(pe.At(TurnRateMultiplyRva,sizeof(TurnRateMultiply)),TurnRateMultiply,sizeof(TurnRateMultiply)));
    constexpr unsigned begin=0x24cdf,end=0x24d98;auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));CHECK(code);
    std::memcpy(code,pe.At(begin,end-begin),end-begin);
    auto operand=[&](unsigned va,void const* value){auto pointer=reinterpret_cast<uint32_t>(value);std::memcpy(code+va-begin,&pointer,4);};
    // Relocate only this fragment's real absolute references. Branch targets
    // inside it remain native; the unused positive-ramp helper fails explicitly.
    operand(0x24ce1,&gAppEntry);operand(0x24ceb,&gFrameEntry);operand(0x24d61,&gFrameEntry);
    operand(0x24d28,&gZero);operand(0x24d4f,&gOne);operand(0x24d8e,&gOne);
    CHECK(code[0x24d40-begin]==0xe8);int32_t relative=int32_t(reinterpret_cast<uintptr_t>(&UnexpectedRamp)-reinterpret_cast<uintptr_t>(code+0x24d45-begin));std::memcpy(code+0x24d41-begin,&relative,4);
    unsigned char tail[]{0xd9,0x44,0x24,0x14,0xc3};std::memcpy(code+end-begin,tail,sizeof(tail));
    unsigned char const getter[]{0x8b,0x44,0x24,0x04,0xd9,0x81,0xc0,0x02,0,0,0xd9,0x18,0xd9,0x81,0xc4,0x02,0,0,0xd9,0x58,0x04,0xd9,0x81,0xc8,0x02,0,0,0xd9,0x58,0x08,0xc2,0x04,0};
    CHECK(!memcmp(pe.At(0xe6fd0,sizeof(getter)),getter,sizeof(getter)));std::memcpy(code+1024,getter,sizeof(getter));
    DWORD old=0;CHECK(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),code,4096);gAlpha=code;
    unsigned char camera[0x558]{},saved[sizeof(camera)]{};camera[10]=0x71;camera[0x558-1]=0x29;
    for(unsigned hz:{30u,60u,72u,90u,120u,144u,240u}){
        gDt=1.f/hz;Put(camera,TurnElapsedOffset,.15f);Put(camera,TurnRampOffset,0);Put(camera,TurnRateOffset,8);
        CHECK(std::fabs(NativeAlpha(camera)-8.f*gDt)<.00001f); // Actual native interpolation is partial.
        Put(camera,TurnElapsedOffset,.15f);Put(camera,TurnRampOffset,3);std::memcpy(saved,camera,sizeof(camera));
        for(unsigned repeat=0;repeat<20;++repeat){CHECK(WithLease(camera,gDt)==1);CHECK(!memcmp(saved,camera,sizeof(camera)));}
        CHECK(CatchLeaseFault(camera)&&!memcmp(saved,camera,sizeof(camera)));
    }
    for(float dt:{0.f,-.1f,NAN,INFINITY,2.f}){CHECK(WithLease(camera,dt)==-1);CHECK(!memcmp(saved,camera,sizeof(camera)));}
    NativeTurnInterpolation rejected;CHECK(!rejected.Begin(camera,TurnRateOffset+3,.01f));rejected.End();CHECK(!memcmp(saved,camera,sizeof(camera)));
    // The *actual getter* reads the consumed result even when a fresh request
    // and sensor result have different directions. It cannot justify changing
    // this update's request. Does not simulate collision/physics or SetGoalPose.
    unsigned char movement[0x400]{};float oldGoal[3]{100,200,-4},newGoal[3]{100,200,8},out[3]{};
    std::memcpy(movement+0x2c0,oldGoal,sizeof(oldGoal));std::memcpy(movement+0x2dc,newGoal,sizeof(newGoal));std::memcpy(movement+0x360,newGoal,sizeof(newGoal));
    auto readGoal=reinterpret_cast<float*(__thiscall*)(void*,float*)>(code+1024);CHECK(readGoal(movement,out)==out&&!memcmp(out,oldGoal,sizeof(out)));CHECK(!memcmp(movement+0x360,newGoal,sizeof(newGoal)));
    VirtualFree(code,0,MEM_RELEASE);gAlpha=nullptr;
    puts("PASS actual native alpha fragment at30..240Hz: partial baseline, alpha1 with scoped lease, exact restoration/repeated calls/SEH; actual old-goal getter; no game DLL loaded");return 0;
}
