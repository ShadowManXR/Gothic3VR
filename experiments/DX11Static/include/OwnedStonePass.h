#pragma once
#include "NativeStoneRenderer.h"
#include "NativeSceneAdapter.h"
#include "NativeWriteback.h"
#include "OwnedStoneAtlas.h"
#include <memory>

namespace diagnostic::temporal {struct Geometry;}
namespace g3vr::dx11 {

// Same-device immutable snapshots. Copy commands are ordered on the owned
// immediate context; no CPU readback, flush or wait is added by Capture.
class OwnedStoneImages {
    struct Impl;
    std::unique_ptr<Impl> impl;
    OwnedStoneImages();
    static std::shared_ptr<OwnedStoneImages> CaptureMutable(ID3D11Device*,ID3D11DeviceContext*,ResidentStoneInputs const&,PassStamp const&,std::shared_ptr<OwnedStoneImages>,uint64_t& allocations);
    friend class OwnedStoneImagePool;
public:
    ~OwnedStoneImages();
    OwnedStoneImages(OwnedStoneImages const&)=delete;
    static std::shared_ptr<OwnedStoneImages const> Capture(ID3D11Device*,ID3D11DeviceContext*,ResidentStoneInputs const&,PassStamp const&);
    PassStamp Stamp()const noexcept;
    ResidentStoneInputs Inputs()const noexcept;
};

// A slot is reusable only after all packet owners release it. Reuse is ordered
// on the same immediate context after prior reads; exhaustion refuses instead
// of waiting or overwriting a live snapshot.
class OwnedStoneImagePool {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    explicit OwnedStoneImagePool(unsigned slots=4);
    ~OwnedStoneImagePool();
    std::shared_ptr<OwnedStoneImages const> Capture(ID3D11Device*,ID3D11DeviceContext*,ResidentStoneInputs const&,PassStamp const&);
    uint64_t Allocations()const noexcept;
    unsigned Slots()const noexcept;
};

// Only the known stairs/stone profile is accepted. The renderer already owns
// that profile's geometry; packets copy entity metadata, constants and atlas
// vertices while the original SDK provider is alive. No SDK pointer is retained.
class OwnedStonePass {
    struct Impl;
    std::unique_ptr<Impl> impl;
    bool AppendImpl(NativeMaterialDraw const&,NativeStaticSnapshot const&,diagnostic::temporal::Geometry const&,std::shared_ptr<OwnedStoneAtlas const>);
public:
    enum class State { Empty,Collecting,Sealed,Consumed,Rejected };
    OwnedStonePass();
    ~OwnedStonePass();
    OwnedStonePass(OwnedStonePass&&)noexcept;
    OwnedStonePass& operator=(OwnedStonePass&&)noexcept;
    OwnedStonePass(OwnedStonePass const&)=delete;
    bool Begin(PassStamp const&,std::shared_ptr<OwnedStoneImages const>);
    bool Append(NativeMaterialDraw const&,NativeStaticSnapshot const&,diagnostic::temporal::Geometry const&);
    // The caller must name the exact content version for this entity's draw.
    // A missing/stale/foreign-epoch version rejects the entire pass.
    bool AppendCached(NativeMaterialDraw const&,NativeStaticSnapshot const&,diagnostic::temporal::Geometry const&,std::shared_ptr<OwnedStoneAtlas const>,StoneAtlasVersion const& expected);
    bool Seal();
    // Diagnostic only: render a trilinear material-sampling control without
    // changing the sealed packet or consuming its subsequent actual render.
    bool RenderSamplerControl(NativeStoneRenderer&,PassStamp const& expected,ID3D11RenderTargetView*,ID3D11DepthStencilView*,unsigned width,unsigned height);
    bool Render(NativeStoneRenderer&,PassStamp const& expected,ID3D11RenderTargetView*,ID3D11DepthStencilView*,unsigned width,unsigned height,float const* clearColor=nullptr);
    State Status()const noexcept;
    unsigned Count()const noexcept;
    // Charged packet payload, not a process working-set measurement.
    size_t CpuBytes()const noexcept;
};
}
