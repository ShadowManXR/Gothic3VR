#pragma once
#include "VirtualHook.h"
#include <d3d9.h>
#include <cstdint>

namespace diagnostic::occlusion {
// The pending-visible shortcut discards native visibility results. In the
// Ardea capture 67032/67213 queries stayed pending, effectively disabling
// occlusion during gameplay while menu views retained native culling.
// Keep it available only for explicit offline comparisons, never by default.
inline bool PendingVisibleOptIn(wchar_t const* value)noexcept {
    return value&&value[0]==L'1'&&value[1]==L'\0';
}
inline constexpr uint32_t QueryInfoRva=0xaa670,ResolveRva=0x1a4390,ResolveReturnRva=0x1a4403,DeviceTableRva=0x42aafc;
inline constexpr unsigned QueryInfoSlot=159;
inline constexpr char QueryInfoSymbol[]="?GetOcclusionTestInfo@eCDX9Device@@UAE?AW4bEResult@@QAXAAK_N@Z";
inline constexpr unsigned char QueryInfoPrefix[]={0x83,0x79,0x08,0x00,0x75,0x05,0x33,0xc0,0xc2,0x0c,0x00};
// UINT_MAX is the engine's unresolved sentinel. Use a distinct, conservative
// positive visible sample count; ready zero results still cull as before.
inline constexpr DWORD ConservativeVisible=INT32_MAX;
enum class Result {Native,Ready,Conservative,Error};
inline bool Eligible(bool worldEye,bool wait,uintptr_t caller,uintptr_t expected)noexcept {
    return worldEye&&wait&&expected&&caller==expected;
}
// Borrow the exact native table entry. No AddRef/Release, native pool mutation
// or retaining a query across frames. The caller verifies the native profile.
inline IDirect3DQuery9* BorrowQuery(void const* device,uintptr_t handle)noexcept {
    static_assert(sizeof(void*)==4,"Native x86 query table");
    if(!handle||handle>UINT16_MAX||!Readable(device,0x50))return nullptr;
    auto* bytes=static_cast<unsigned char const*>(device);
    void* nativeDevice=nullptr;uint16_t count=0;IDirect3DQuery9** queries=nullptr;
    std::memcpy(&nativeDevice,bytes+8,4);std::memcpy(&count,bytes+0x46,2);std::memcpy(&queries,bytes+0x48,4);
    if(!nativeDevice||handle-1>=count||!queries||!Readable(queries+(handle-1),4))return nullptr;
    IDirect3DQuery9* query=nullptr;std::memcpy(&query,queries+(handle-1),4);
    if(!query||reinterpret_cast<uintptr_t>(query)==UINT32_MAX||!Readable(query,4))return nullptr;
    return query;
}
inline Result Poll(IDirect3DQuery9* query,DWORD& samples) {
    if(!query||query->GetType()!=D3DQUERYTYPE_OCCLUSION)return Result::Native;
    DWORD measured=0;
    HRESULT hr=query->GetData(&measured,sizeof(measured),0);
    if(hr==S_OK){samples=measured;return Result::Ready;}
    if(hr==S_FALSE){samples=ConservativeVisible;return Result::Conservative;}
    // Preserve failure as failure: never turn a lost/invalid device into a
    // successful visibility result. Native Resolve retains ownership on error.
    return Result::Error;
}
}
