#pragma once
#include <array>
#include <cstdint>
class eCRenderContext;
class eCRenderOp2;
class eCVertexbufferPoolItemMain;
namespace g3vr::dx11 {
struct NativeLightmapSource;
// Copied on the native render/application thread. No retained SDK pointers,
// no resource load, no GPU locks and no file I/O inside the future hook.
struct NativeStaticSnapshot {
    std::array<uint32_t,5> identity{};
    std::array<float,16> worldMeters{};
    char resource[1024]{};
    int materialSwitch=0;
};
// Must be called only for a live context by a separately version-validated
// integration. This helper does not install a hook or omit any native work.
bool __cdecl CopyNativeStatic(eCRenderContext*,NativeStaticSnapshot&)noexcept;
bool __cdecl CopyNativeMaterialName(eCRenderOp2 const&,char (&name)[1024])noexcept;
bool __cdecl CopyNativePoolMaterialName(eCVertexbufferPoolItemMain*,char (&name)[1024])noexcept;
NativeLightmapSource __cdecl NativePoolLightmapSource(eCVertexbufferPoolItemMain*,std::array<uint32_t,5> const&)noexcept;
}
