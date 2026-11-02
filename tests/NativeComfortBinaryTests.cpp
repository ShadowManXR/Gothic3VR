#include "NativeComfortProfile.h"
#include "NativeApplication.h"
#include "NativeHeroVoice.h"
#include "WorldBindingsLifecycle.h"
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
    if(argc!=3)return 1;
    auto game=[&](unsigned rva,unsigned char const* bytes,size_t n){return Bytes(argv[1],"4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00",rva,bytes,n);};
    unsigned char combatAction[]{0x89,0x96,0x54,0x01,0,0},combatStop[]{0x8b,0x81,0xd8,0,0,0,0x83,0xc1,0x64};
    unsigned char combatGate[]{0x80,0xbf,0x2e,0x02,0,0,0x01};
    unsigned char combatCleanup[]{0xc6,0x86,0x2c,0x02,0,0,0x01,0xc6,0x86,0x2d,0x02,0,0,0};
    if(!game(0x16abed,combatAction,sizeof(combatAction))||!game(0x1644d0,combatStop,sizeof(combatStop))||
       !game(0xe2af8,combatGate,sizeof(combatGate))||!game(0x1697fc,combatCleanup,sizeof(combatCleanup)))return 15;
    if(!game(diagnostic::VoiceStartRva,diagnostic::VoiceStartPrefix,sizeof(diagnostic::VoiceStartPrefix))||
       !game(diagnostic::VoiceUpdateRva,diagnostic::VoiceUpdatePrefix,sizeof(diagnostic::VoiceUpdatePrefix)))return 13;
    unsigned char voiceLocation[]{0x8b,0x4e,0x6c,0x83,0xc0,0x30,0x50};
    if(!game(0x16fdc1,voiceLocation,sizeof(voiceLocation)))return 14;
    unsigned char speedGetter[]{0xd9,0x81,0x58,0x01,0,0,0xc3};
    unsigned char mousePrefix[]{0x55,0x8b,0xec,0x83,0xe4,0xf8},store[]{0xd9,0x9e,0x18,0x02,0,0},call[]{0xe8,0x84,0x2a,0,0};
    if(!game(diagnostic::GroundTranslationRva,mousePrefix,6)||!game(0xeb7b0,speedGetter,7)||!game(diagnostic::CurrentVelocityRva,diagnostic::CurrentVelocityPrefix,10)||!game(diagnostic::MouseFilterXRva,mousePrefix,6)||!game(0x2673a,store,6)||!game(0x23c07,call,5))return 2;
    unsigned char stopPrefix[9]{0x51,0x53,0x55,0x8b,0x2d,0xc4,0xc8,0x24,0x10};
    unsigned char stopClearsPlayer[]{0x89,0x5e,0x28,0x89,0x5e,0x2c};
    if(!game(diagnostic::SessionStopRva,stopPrefix,9)||!game(0x1741d4,stopClearsPlayer,6))return 4;
    unsigned char groundCall[]{0xe8,0xd0,0x69,0,0},floorPrefix[]{0x55,0x8b,0xec,0x83,0xe4,0xc0},floorReturn[]{0xc2,0x08,0x00};
    if(!game(0xe2b1b,groundCall,5)||!game(0xe94f0,floorPrefix,6)||!game(0xe9e36,floorReturn,3))return 5;
    unsigned char resolvedModifier[]{0xd9,0x86,0x60,0x01,0,0},resolvedSlope[]{0xf3,0x0f,0x10,0x86,0x80,0x03,0,0},resolvedCaller[]{0xe8,0x8e,0x89,0,0};
    if(!game(0xebaf8,resolvedModifier,6)||!game(0xebc44,resolvedSlope,8)||!game(0xe2e6d,resolvedCaller,5))return 9;
    unsigned char keyRead[]{0x0f,0xb7,0x1f},flagsRead[]{0x8a,0x5f,0x0c},durationRead[]{0x8b,0x4f,0x08};
    if(!game(0xde48f,keyRead,3)||!game(0xde4ee,flagsRead,3)||!game(0xde550,durationRead,3))return 10;
    // Native movement consumes timers once and clears them to -1. Neutral is
    // absence of a direction event, not OnAction(false), which still adds a
    // vector. Also verify the OnPreProcess movement scope has a live property.
    unsigned char liveMovement[]{0x89,0x46,0x58},activeTick[]{0xc6,0x46,0x54,0x01};
    unsigned char timerClear[]{0x89,0x77,0x3c,0x89,0x77,0x40,0x89,0x77,0x44,0x89,0x77,0x48};
    unsigned char forwardTimer[]{0x8b,0x47,0x08,0x89,0x46,0x3c};
    unsigned char wishAdd[]{0x8d,0x4e,0x60,0xff,0x15,0x1c,0xde,0x24,0x10};
    if(!game(0xddcda,liveMovement,sizeof(liveMovement))||!game(0xddd0d,activeTick,sizeof(activeTick))||
       !game(0xde453,timerClear,sizeof(timerClear))||!game(0xde5e8,forwardTimer,sizeof(forwardTimer))||!game(0xde6fa,wishAdd,sizeof(wishAdd)))return 16;
    // Exact native horizontal camera sensitivity chain before UpdateDeltaRotX.
    unsigned char turnMode[]{0xd8,0x8f,0x58,0x03,0,0,0xd8,0x8f,0x48,0x05,0,0};
    unsigned char turnPreset[]{0x8b,0x87,0xe4,0x01,0,0},turnScale[]{0xd8,0x48,0x0c};
    float nativePixelScale=.05f; // the native constant is a double holding this float value
    double nativePixelDouble=double(nativePixelScale);
    if(!game(0x23bf0,turnMode,sizeof(turnMode))||!game(0x23bd8,turnPreset,sizeof(turnPreset))||!game(0x23bed,turnScale,sizeof(turnScale))||
       !game(0x3082b8,reinterpret_cast<unsigned char*>(&nativePixelDouble),sizeof(nativePixelDouble)))return 17;
    unsigned char wishedModeStore[]{0x89,0x81,0xac,0,0,0};
    if(!game(0xde9e4,wishedModeStore,sizeof(wishedModeStore)))return 12;
    unsigned char powerCost[]{0x6a,0xec},quickCost[]{0x8d,0x5c,0x1b,0x0a};
    auto script=[&](unsigned rva,unsigned char const* bytes,size_t n){return Bytes(argv[2],"4d29189281ec26eac7c704fa67ed323df5c090d50b068626cf600aecf4262a24",rva,bytes,n);};
    if(!script(0x47e4d,powerCost,2)||!script(0x48536,quickCost,4))return 6;
    unsigned char paradeState[]{0x68,0x60,0xf9,0x07,0x10},shieldType[]{0x83,0xfe,0x09},shieldSkill[]{0x68,0x30,0xf0,0x0f,0x10};
    if(!script(0xdd2f5,paradeState,5)||!script(0x2b4fd,shieldType,3)||!script(0x2b529,shieldSkill,5))return 11;
    unsigned char finisherState[]{0x68,0x60,0x05,0x08,0x10};
    if(!script(0xdd4d5,finisherState,5))return 7;
    unsigned char weaponSkills[]{0x83,0xf8,0x34,0x74,0x57,0x83,0xf8,0x03,0x74,0x52,0x83,0xf8,0x33,0x74,0x4d,0x83,0xf8,0x0c};
    if(!script(0x2e8cf,weaponSkills,sizeof(weaponSkills)))return 8;
    unsigned char end[]{0xff,0x15,0x0c,0x49,0x0e,0x10},species[]{0xc7,0x44,0x24,0x14,0x28,0,0,0},dead[]{0xc7,0x44,0x24,0x14,9,0,0,0};
    unsigned char move[]{0xff,0x15,0xc0,0x48,0x0e,0x10},decay[]{0xff,0x15,0x10,0x45,0x0e,0x10};
    float x=-220000.f,y=100000.f;
    if(!script(0x5852c,end,6)||!script(0x5853b,species,8)||!script(0x58552,dead,8)||!script(0x5856c,move,6)||!script(0x58576,decay,6)||!script(0x107404,reinterpret_cast<unsigned char*>(&x),4)||!script(0x107400,reinterpret_cast<unsigned char*>(&y),4))return 3;
    puts("PASS actual Game.dll velocity/filter instruction boundaries and Script_Game owned animal cleanup sequence/constants");return 0;
}
