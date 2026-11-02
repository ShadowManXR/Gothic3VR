#include "NativeApplication.h"
#include "NativeRenderProfile.h"
#include <vector>
#include <cstdio>
int __cdecl wmain(int argc,wchar_t** argv){
    if(argc!=2)return 1;unsigned char hash[32]{};
    if(!diagnostic::FileSha256(argv[1],hash)||memcmp(hash,diagnostic::performance::GameSceneHash,32))return 2;
    HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(file==INVALID_HANDLE_VALUE)return 3;
    DWORD size=GetFileSize(file,nullptr),read=0;std::vector<unsigned char> data(size);bool ok=ReadFile(file,data.data(),size,&read,nullptr)&&read==size;CloseHandle(file);if(!ok)return 4;
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER const*>(data.data());auto nt=reinterpret_cast<IMAGE_NT_HEADERS32 const*>(data.data()+dos->e_lfanew);auto sections=IMAGE_FIRST_SECTION(nt);
    auto at=[&](unsigned rva,size_t count)->unsigned char const*{for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){auto const& s=sections[i];if(rva>=s.VirtualAddress&&rva-s.VirtualAddress+count<=s.SizeOfRawData){size_t offset=s.PointerToRawData+rva-s.VirtualAddress;if(offset+count<=data.size())return data.data()+offset;}}return nullptr;};
    auto directory=reinterpret_cast<IMAGE_EXPORT_DIRECTORY const*>(at(nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,sizeof(IMAGE_EXPORT_DIRECTORY)));if(!directory)return 5;
    auto names=reinterpret_cast<DWORD const*>(at(directory->AddressOfNames,directory->NumberOfNames*4));auto ordinals=reinterpret_cast<WORD const*>(at(directory->AddressOfNameOrdinals,directory->NumberOfNames*2));auto functions=reinterpret_cast<DWORD const*>(at(directory->AddressOfFunctions,directory->NumberOfFunctions*4));if(!names||!ordinals||!functions)return 6;
    for(auto const& entry:diagnostic::performance::GameSceneEntries){bool found=false;
        for(unsigned i=0;i<directory->NumberOfNames;++i){auto name=at(names[i],strlen(entry.symbol)+1);if(name&&!memcmp(name,entry.symbol,strlen(entry.symbol)+1)&&ordinals[i]<directory->NumberOfFunctions&&functions[ordinals[i]]==entry.rva){found=true;break;}}
        auto prefix=at(entry.rva,entry.bytes);if(!found||!prefix||memcmp(prefix,entry.prefix,entry.bytes))return 7;
        unsigned char routine[]{0x53,0x55,0x56,0x57,0x8b,0xf1},normal[]{0x55,0x8b,0xec,0x83,0xe4,0xf8};
        if(entry.bytes!=6||(memcmp(prefix,routine,6)&&memcmp(prefix,normal,6)))return 8;
    }
    puts("PASS exact Game hash, AI-routine/navigation exports and whole relocatable x86 entry instructions; native Game DLL not executed");return 0;
}
