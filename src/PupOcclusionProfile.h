#pragma once
#include <cstdint>
#include <cstring>
namespace diagnostic::pup {
// PUP1.1.1 eCDX9DeviceEx derives at offset zero from the original Engine
// eCDX9Device and inherits its occlusion implementation through this IAT thunk.
// Use ONLY after the complete PUP FileHash check and module PIN.
inline constexpr uint32_t OcclusionDeviceTableRva=0x80b70,OcclusionThunkRva=0x3bd34,OcclusionIatRva=0x674d4;
inline bool OcclusionThunkAtBase(uint32_t base,unsigned char (&out)[6])noexcept {
    if(!base||base>UINT32_MAX-OcclusionDeviceTableRva)return false;
    out[0]=0xff;out[1]=0x25;uint32_t cell=base+OcclusionIatRva;std::memcpy(out+2,&cell,4);return true;
}
}
