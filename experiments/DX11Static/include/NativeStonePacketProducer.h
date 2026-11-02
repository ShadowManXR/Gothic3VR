#pragma once
#include "OwnedStonePass.h"

namespace g3vr::dx11 {
// Conservative live producer for the one known geometry/material profile.
// Re-reads actual atlas bytes INSIDE each native draw scope and compares exact
// contents before reusing a version. This is not a pointer-identity cache or a
// claim that native mutation hooks / generic geometry have been implemented.
class NativeStonePacketProducer {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    NativeStonePacketProducer();
    ~NativeStonePacketProducer();
    void Initialize(IDirect3DDevice9Ex*,ID3D11Device*,ID3D11DeviceContext*);
    std::unique_ptr<OwnedStonePass> Capture(NativeMaterialDraw const&,NativeStaticSnapshot const&,diagnostic::temporal::Geometry const&,PassStamp const&);
    uint64_t AtlasReads()const noexcept;
    uint64_t AtlasHits()const noexcept;
    uint64_t ImageCaptures()const noexcept;
};
}
