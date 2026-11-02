#include "NativeApplication.h"
#include "ParticlePerformance.h"
#include <vector>
#include <cstdio>
bool Bytes(wchar_t const* path,char const* digest,unsigned rva,unsigned char const* bytes,size_t count){
    unsigned char expected[32]{},actual[32]{};for(unsigned i=0;i<32;++i){unsigned value=0;if(sscanf_s(digest+i*2,"%2x",&value)!=1)return false;expected[i]=static_cast<unsigned char>(value);}
    if(!diagnostic::FileSha256(path,actual)||memcmp(actual,expected,32))return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
    DWORD size=GetFileSize(file,nullptr),read=0;if(size==INVALID_FILE_SIZE||size<1024||size>64*1024*1024){CloseHandle(file);return false;}
    std::vector<unsigned char> raw(size);bool readOK=ReadFile(file,raw.data(),size,&read,nullptr)&&read==size;CloseHandle(file);if(!readOK)return false;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(raw.data());if(dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>raw.size())return false;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(raw.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||reinterpret_cast<unsigned char const*>(sections+nt->FileHeader.NumberOfSections)>raw.data()+size)return false;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva<s.VirtualAddress||rva-s.VirtualAddress+count>s.SizeOfRawData)continue;
        size_t offset=s.PointerToRawData+rva-s.VirtualAddress;return offset+count<=raw.size()&&!memcmp(raw.data()+offset,bytes,count);}
    return false;
}
int __cdecl wmain(int argc,wchar_t** argv){
    if(argc!=4)return 1;
    char const* engine="991cf674a9cf1bc60cddc204be0993e8251336f4d3471543a37846cc6840bf5a";
    char const* game="4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00";
    unsigned char nativeParticleFrame[]{0x8b,0x80,0xe8,0x05,0,0};
    unsigned char nativeParticleCache[]{0x39,0x85,0x48,0x03,0,0};
    if(!Bytes(argv[1],engine,diagnostic::particles::VerticesRva,diagnostic::particles::VerticesPrefix,sizeof(diagnostic::particles::VerticesPrefix))||
       !Bytes(argv[1],engine,diagnostic::particles::RefractionRva,diagnostic::particles::RefractionPrefix,sizeof(diagnostic::particles::RefractionPrefix))||
       !Bytes(argv[1],engine,0x153647,nativeParticleFrame,sizeof(nativeParticleFrame))||!Bytes(argv[1],engine,0x15364d,nativeParticleCache,sizeof(nativeParticleCache)))return 6;
    unsigned char process[]{0x53,0x55,0x56,0x57,0x8b,0xf1};
    unsigned char alpha[]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
    unsigned char opaqueNoop[]{0xc2,0x04,0x00};
    unsigned char childLink[]{0x8b,0x47,0x28,0x85,0xc0,0x74,0x17,0x83,0xc0,0xff,0x83,0xf8,0x01,0x77,0x0f,0x56,0x51,0x8b,0xcb,0xe8,0xe6,0x51,0x14,0x00};
    if(!Bytes(argv[1],engine,0x14ed10,process,sizeof(process))||!Bytes(argv[1],engine,0x14ee30,alpha,sizeof(alpha))||
       !Bytes(argv[1],engine,0x276700,opaqueNoop,sizeof(opaqueNoop))||!Bytes(argv[2],game,0x54fc2,childLink,sizeof(childLink)))return 2;
    // Native target rebinding rebuilds the camera from the target's world
    // matrix, then invokes the native mode setter. Required for activity return.
    unsigned char targetPrefix[]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
    unsigned char targetWorld[]{0x8d,0x8e,0xec,0x03,0x00,0x00,0xff,0x15,0x54,0xcd,0x24,0x10,0x8b,0xc8,0xff,0x15,0x44,0xcd,0x24,0x10};
    if(!Bytes(argv[2],game,0x2c040,targetPrefix,sizeof(targetPrefix))||!Bytes(argv[2],game,0x2c3e0,targetWorld,sizeof(targetWorld)))return 4;
    // Exact native attachment list and hand-bone metadata layout, CP1.75.14.
    unsigned char attachmentCount[]{0x8b,0x81,0xc8,0,0,0,0xc3},attachmentArray[]{0x8d,0xbe,0xc4,0,0,0,0x8b,0x77,4};
    unsigned char attachmentFields[]{0x8b,0x7c,0x24,0x10,0x8d,0x4e,0x68,0x89,0x7e,0x60,0x89,0x5e,0x64};
    unsigned char attachmentEntity[]{0x8b,0x41,8,0x85,0xc0,0x56,0x8d,0x71,4};
    if(!Bytes(argv[1],engine,0x31160,attachmentCount,sizeof(attachmentCount))||!Bytes(argv[1],engine,0x30dea,attachmentArray,sizeof(attachmentArray))||
       !Bytes(argv[1],engine,0x2a344,attachmentFields,sizeof(attachmentFields))||!Bytes(argv[1],engine,0xa900,attachmentEntity,sizeof(attachmentEntity)))return 5;
    unsigned char vertices[]{0x55,0x8b,0xec,0x83,0xe4,0xf8},world[]{0x83,0x79,0x0c,0,0x53,0x55},sign[]{0x56,0x8b,0x74,0x24,8};
    unsigned char potionHealth[]{0x49,0x74,0x5f,0x50,0x6f,0x74,0x69,0x6f,0x6e,0x5f,0x48,0x65,0x61,0x6c,0x74,0x68,0};
    unsigned char pierce[]{0x83,0xff,0x0b},nativeSkill[]{0xff,0x15,0x80,0x42,0x0e,0x10};
    // The native Pierce branch calls IsHumanoid, whose accepted species are
    // exactly Human=0 and Orc=5. Match our target routing to that game rule.
    unsigned char humanoidHuman[]{0x85,0xc0,0x75,0x12};
    unsigned char humanoidOrc[]{0x83,0xf8,0x05,0x74,0xe9};
    unsigned char pierceHumanoid[]{0xe8,0x2c,0x55,0xfe,0xff};
    char const* script="4d29189281ec26eac7c704fa67ed323df5c090d50b068626cf600aecf4262a24";
    if(!Bytes(argv[1],engine,0x154c80,vertices,sizeof(vertices))||!Bytes(argv[1],engine,0x88c20,world,sizeof(world))||
       !Bytes(argv[2],game,0x7a040,sign,sizeof(sign))||!Bytes(argv[3],script,0xfdd50,potionHealth,sizeof(potionHealth))||
       !Bytes(argv[3],script,0x2b947,pierce,sizeof(pierce))||!Bytes(argv[3],script,0x2b971,nativeSkill,sizeof(nativeSkill))||
       !Bytes(argv[3],script,0x10fba,humanoidHuman,sizeof(humanoidHuman))||!Bytes(argv[3],script,0x10fd0,humanoidOrc,sizeof(humanoidOrc))||
       !Bytes(argv[3],script,0x2b9ef,pierceHumanoid,sizeof(pierceHumanoid)))return 3;
    puts("PASS native potion inventory template, Pierce/Assassin skill branch, sign label predicate, particle vertices/world matrix entries; exact Engine particle Process/OnRenderAlpha hook prologues, opaque render no-op and Game TargetEntity/TargetBone->CreateChildEntityFromTemplate ownership branch");return 0;
}
