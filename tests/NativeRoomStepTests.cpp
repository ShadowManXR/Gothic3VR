#include "NativeApplication.h"
#include "RoomScale.h"
#include "GameStereoMath.h"
#include <cstdio>
#include <vector>

int __cdecl wmain(int argc,wchar_t** argv){
    if(argc!=2)return 1;
    unsigned char hash[32]{},expectedHash[32]{
        0x99,0x1c,0xf6,0x74,0xa9,0xcf,0x1b,0xc6,0x0c,0xdd,0xc2,0x04,0xbe,0x09,0x93,0xe8,
        0x25,0x13,0x36,0xf4,0xd3,0x47,0x15,0x43,0xa3,0x78,0x46,0xcc,0x68,0x40,0xbf,0x5a};
    if(!diagnostic::FileSha256(argv[1],hash)||memcmp(hash,expectedHash,32))return 2;
    HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
    if(file==INVALID_HANDLE_VALUE)return 3;
    DWORD size=GetFileSize(file,nullptr),read=0;
    if(size==INVALID_FILE_SIZE||size<1024||size>64*1024*1024){CloseHandle(file);return 4;}
    std::vector<unsigned char> raw(size);
    bool ok=ReadFile(file,raw.data(),size,&read,nullptr)&&read==size;CloseHandle(file);if(!ok)return 5;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(raw.data());
    if(dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>raw.size())return 6;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(raw.data()+dos->e_lfanew);
    auto sections=IMAGE_FIRST_SECTION(nt);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||
       reinterpret_cast<unsigned char const*>(sections+nt->FileHeader.NumberOfSections)>raw.data()+size)return 7;
    // This exact CP/PUP setter has no imports, calls, globals or relative
    // branches. Execute only its verified isolated bytes on a fixture; never
    // load Engine.dll, initialize a game actor or run a DLL entry point.
    constexpr unsigned char setter[]{
        0x8b,0x44,0x24,0x04,0xd9,0x00,0xd9,0x99,0xb8,0x00,0x00,0x00,0xd9,0x40,0x04,0xd9,0x99,0xbc,0x00,0x00,0x00,
        0xd9,0x40,0x08,0xd9,0x99,0xc0,0x00,0x00,0x00,0xd9,0x00,0xd9,0x99,0x9c,0x00,0x00,0x00,0xd9,0x40,0x04,0xd9,
        0x99,0xa0,0x00,0x00,0x00,0xd9,0x40,0x08,0xd9,0x99,0xa4,0x00,0x00,0x00,0xc2,0x04,0x00};
    unsigned char const* native=nullptr;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];
        if(0x179460>=s.VirtualAddress&&0x179460-s.VirtualAddress+sizeof(setter)<=s.SizeOfRawData){
            size_t offset=s.PointerToRawData+0x179460-s.VirtualAddress;
            if(offset<=raw.size()&&sizeof(setter)<=raw.size()-offset)native=raw.data()+offset;
        }
    }
    if(!native||memcmp(native,setter,sizeof(setter)))return 8;
    auto code=VirtualAlloc(nullptr,sizeof(setter),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!code)return 9;
    memcpy(code,native,sizeof(setter));DWORD previous=0;
    if(!VirtualProtect(code,sizeof(setter),PAGE_EXECUTE_READ,&previous)){VirtualFree(code,0,MEM_RELEASE);return 10;}
    FlushInstructionCache(GetCurrentProcess(),code,sizeof(setter));
    using Callback=void(__thiscall*)(void*,float const*);
    alignas(16) unsigned char body[0xf8],expected[0xf8];memset(body,0x5a,sizeof(body));memcpy(expected,body,sizeof(body));
    float finalPosition[3]{87604,5302,-9497},accepted[3]{4,2,3},oldPosition[3]{87600,5300,-9500};
    memcpy(body+0x9c,oldPosition,sizeof(oldPosition));memcpy(body+0xb8,oldPosition,sizeof(oldPosition));
    float origin[3]{0,1.6f,0},head[3]{.3f,1.6f,.2f},oldEye[3]{},newEye[3]{},euler[3]{};
    diagnostic::Orientation center;diagnostic::Rotation3 rotation;
    diagnostic::StereoEyePose eye{{head[0],head[1],head[2]},{0,0,0,1},{}};
    diagnostic::GameEyeTransform(rotation,oldPosition,center,origin,eye,100,oldEye,euler);
    diagnostic::ConsumeRoomStep(center,rotation,accepted,100,origin);
    // Replay interpolation after an origin rebase: stale endpoints visibly
    // pull the headset backwards; synchronizing both removes that pull at
    // every physics blend fraction, including terrain Y and forward motion.
    bool stalePull=false;
    for(float fraction:{0.f,.25f,.5f,.75f,1.f}){
        float interpolated[3]{};for(unsigned j=0;j<3;++j){float a,b;memcpy(&a,body+0x9c+4*j,4);memcpy(&b,body+0xb8+4*j,4);interpolated[j]=a+(b-a)*fraction;}
        diagnostic::GameEyeTransform(rotation,interpolated,center,origin,eye,100,newEye,euler);
        stalePull=stalePull||fabs(newEye[0]-oldEye[0])>3.f;
    }
    reinterpret_cast<Callback>(code)(body,finalPosition);
    memcpy(expected+0x9c,finalPosition,sizeof(finalPosition));memcpy(expected+0xb8,finalPosition,sizeof(finalPosition));
    ok=stalePull&&!memcmp(body,expected,sizeof(body));
    for(float fraction:{0.f,.25f,.5f,.75f,1.f}){
        float interpolated[3]{};for(unsigned j=0;j<3;++j){float a,b;memcpy(&a,body+0x9c+4*j,4);memcpy(&b,body+0xb8+4*j,4);interpolated[j]=a+(b-a)*fraction;}
        diagnostic::GameEyeTransform(rotation,interpolated,center,origin,eye,100,newEye,euler);
        ok=ok&&fabs(newEye[0]-oldEye[0])<.02f&&fabs(newEye[2]-oldEye[2])<.02f&&fabs(newEye[1]-oldEye[1]-2.f)<.02f;
    }
    VirtualFree(code,0,MEM_RELEASE);
    printf("%s native Engine SetFixedStepTranslation x86 ABI, both absolute endpoints, unrelated bytes preserved, headset continuity across physics blends\n",ok?"PASS":"FAIL");
    return ok?0:11;
}
