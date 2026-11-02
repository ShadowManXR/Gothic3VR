#pragma once
#include "NativeSceneAdapter.h"
#include "NativeMaterialCapture.h"
#include <d3d9.h>
#include <dxgi.h>
#include <filesystem>
#include <memory>

namespace diagnostic::temporal {struct Geometry;}
namespace g3vr::dx11 {
// Modes1..3 preserve original draws. Mode4 replaces only the known stone DIP
// after live isolated color/depth validation, at its original submission point.
// Mode5 freezes real native inputs and validates a later owned pass off-screen;
// it never composites to the game or omits a native draw.
// Mode1 renders off
// screen, mode 2 composites a simplified-material comparison over the native
// object at the matching solid-pass boundary. Normal launches never enable it.
class NativeStaticTrial {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    NativeStaticTrial();~NativeStaticTrial();
    NativeStaticTrial(NativeStaticTrial const&)=delete;
    void Initialize(IDirect3DDevice9Ex*,IDXGIAdapter*,std::filesystem::path const& assets,std::filesystem::path const& output,unsigned width,unsigned height,unsigned mode);
    void BeginEye(unsigned eye,uint64_t frame,int context)noexcept;
    bool WantsContext(uintptr_t liveContextCookie)const noexcept;
    void Observe(uintptr_t liveContextCookie,NativeStaticSnapshot const&,float const* nativeViewCm,float const* nativeProjectionCm)noexcept;
    bool WantsMaterial(uintptr_t liveContextCookie)const noexcept;
    bool MaterialObservationEnabled()const noexcept;
    bool NeedsMaterialWork()const noexcept;
    bool WantsMaterialEntity(std::array<uint32_t,5> const&)const noexcept;
    unsigned BeginMaterial(uintptr_t liveContextCookie,char const* materialName)noexcept;
    unsigned BeginDeferredMaterial(std::array<uint32_t,5> const&,float const* worldCm,char const* materialName,NativeLightmapSource lightmap={})noexcept;
    void EndMaterial(unsigned token)noexcept;
    bool ObserveMaterialDraw(IDirect3DDevice9*,unsigned drawKind,diagnostic::temporal::Geometry const* geometry=nullptr)noexcept;
    bool PendingPass(float const* nativeViewCm,float const* nativeProjectionCm)const noexcept;
    void SolidBoundary(float const* nativeViewCm,float const* nativeProjectionCm)noexcept;
    void EndEye()noexcept;
    void Reset()noexcept;
};
}
