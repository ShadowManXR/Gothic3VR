#include "NativeApplication.h"
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
    unsigned char hold[]{0x32,0xc0,0x5b,0x59,0xc2,0x04,0x00};
    unsigned char cached[]{0x8b,0x4d,0x08,0x39,0x4b,0x2c,0x75,0x0a,0x83,0x7b,0x24,0x00,0x0f,0x85,0xa2,0x00,0x00,0x00};
    if(!Bytes(argv[1],"4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00",0xb84ad,hold,sizeof(hold))||
       !Bytes(argv[2],"991cf674a9cf1bc60cddc204be0993e8251336f4d3471543a37846cc6840bf5a",0x3abc4,cached,sizeof(cached)))return 2;
    // Verify every camera flag touched by animal transformation, and SetMode's
    // mode-11 redirection through the old FP flags before its normal branch.
    unsigned char active[]{0x38,0x9d,0xc1,0x03,0,0},temp[]{0x38,0x9d,0xd4,0x04,0,0},request[]{0x80,0xbf,0xc5,0x03,0,0,1},clear[]{0xc6,0x87,0xc1,0x03,0,0,0};
    auto camera=[&](unsigned rva,unsigned char const* bytes,size_t n){return Bytes(argv[1],"4728a47b244adcb0ea2ec3501db04994909dcfcff338c7b8075f2b68dd8d6e00",rva,bytes,n);};
    if(!camera(0x2cb6d,active,sizeof(active))||!camera(0x2cb75,temp,sizeof(temp))||!camera(0x2c564,request,sizeof(request))||!camera(0x2ca85,clear,sizeof(clear)))return 3;
    puts("PASS native bow carry/cache and animal camera persistent/request/temporary FP flag profile");return 0;
}
