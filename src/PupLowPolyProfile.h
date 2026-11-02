#pragma once
#include <cstdint>
#include <cstring>
namespace diagnostic::pup {
// PUP1.1.1 paru_s2.dll, only after its existing complete FileHash check/PIN.
// Its verified wrapper sets this flag around its original RenderLowPoly call.
// We READ it from the existing renderer hook and never patch the PUP wrapper.
inline constexpr uint32_t LowPolyWrapperRva=0x11680,LowPolyFlagRva=0x994ac,LowPolyOriginalSlotRva=0x973e8;
inline constexpr unsigned char LowPolyBody[]={
    0xc6,0x05,0xac,0x94,0x09,0x10,0x01,
    0xff,0x15,0xe8,0x73,0x09,0x10,
    0xc6,0x05,0xac,0x94,0x09,0x10,0x00,0xc3
};
inline bool LowPolyBodyAtBase(uint32_t base,unsigned char (&out)[sizeof(LowPolyBody)])noexcept {
    if(!base||base>UINT32_MAX-LowPolyFlagRva)return false;
    std::memcpy(out,LowPolyBody,sizeof(out));
    uint32_t flag=base+LowPolyFlagRva,original=base+LowPolyOriginalSlotRva;
    std::memcpy(out+2,&flag,4);std::memcpy(out+9,&original,4);std::memcpy(out+15,&flag,4);
    return true;
}
inline int LowPolyClass(unsigned value)noexcept {return value<=1?int(value):-1;}
}
