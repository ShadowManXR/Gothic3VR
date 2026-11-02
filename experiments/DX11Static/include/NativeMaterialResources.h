#pragma once
#include <d3d9.h>
#include <d3d11.h>
#include <cstdint>

namespace g3vr::dx11 {
// Experimental bridge for the observed material's native lightmap and two
// shadow maps. The immediate D3D11 context must belong exclusively to the trial.
// All pixel transfers and depth normalization stay on the GPU.
class NativeMaterialResources {
public:
    NativeMaterialResources() = default;
    ~NativeMaterialResources();
    NativeMaterialResources(NativeMaterialResources const&) = delete;
    NativeMaterialResources& operator=(NativeMaterialResources const&) = delete;
    bool Initialize(IDirect3DDevice9Ex*, ID3D11Device*, ID3D11DeviceContext*);
    void Capture(IDirect3DTexture9* lightmap, IDirect3DTexture9* shadow,
        IDirect3DTexture9* detailShadow, IDirect3DTexture9* noise=nullptr);
    // Views remain valid until the next Capture or Reset. FinishReads fences
    // the owned D3D11 context before returning shared ownership to DXVK.
    ID3D11ShaderResourceView* Lightmap() const;
    ID3D11ShaderResourceView* Noise() const;
    ID3D11ShaderResourceView* Shadow(unsigned index) const;
    void FinishReads();
    void Reset() noexcept;
    uint64_t Captures() const;
    uint64_t Allocations() const;
    unsigned ShadowBackingFormat(unsigned index) const;
private:
    void* state_ = nullptr;
};
}
