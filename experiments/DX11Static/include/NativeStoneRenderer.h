#pragma once
#include "NativeMaterialCapture.h"
#include <d3d11.h>
#include <filesystem>
#include <memory>
namespace g3vr::dx11 {
struct ResidentStoneInputs {
    ID3D11ShaderResourceView* noise=nullptr;
    ID3D11ShaderResourceView* shadows[2]{};
    ID3D11ShaderResourceView* lightmap=nullptr;
};
// Single observed rigid stone profile. No generic material claim, no draw
// suppression. All live image inputs stay on the GPU; additional vertices are
// copied from the SDK after native atlas assignment at the original draw.
class NativeStoneRenderer {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    NativeStoneRenderer();~NativeStoneRenderer();
    // A null native device initializes only the resident DX11 path.
    void Initialize(IDirect3DDevice9Ex*,ID3D11Device*,ID3D11DeviceContext*,std::filesystem::path const& profile);
    bool Render(NativeMaterialDraw const&,ID3D11RenderTargetView*,ID3D11DepthStencilView*,unsigned width,unsigned height);
    // Offline pass prototype: bounded, same eye/frame and shared live image
    // inputs. All providers must remain valid during this call. Private targets
    // may be partially written on refusal; commit only after a true result.
    bool RenderBatch(NativeMaterialDraw const*,unsigned count,ID3D11RenderTargetView*,ID3D11DepthStencilView*,unsigned width,unsigned height);
    // Same-device opaque pass only: caller owns stable inputs/atlas providers.
    // Output is pre-postprocessing color/depth, never a final VR overlay.
    bool RenderResidentBatch(NativeMaterialDraw const*,unsigned count,ResidentStoneInputs const&,ID3D11RenderTargetView*,ID3D11DepthStencilView*,unsigned width,unsigned height,float const* clearColor=nullptr,bool allowInstancing=true);
    unsigned LastSubmittedDraws()const noexcept;
    uint64_t ResourceCaptures() const noexcept;
    char const* LastRefusal()const noexcept;
};
}
