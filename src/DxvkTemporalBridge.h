#pragma once
#include <d3d9.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>
namespace g3vr {
// Describes the *actual Vulkan depth backing*, which may differ from the
// requested D3D9 format. Color16F is used for native motion/coverage images.
enum class TemporalEncoding:uint32_t {Color16F=0,Depth16Unorm=1,Depth24Uint=2,Depth32Float=3};
// Temporal-input transport used by the opt-in game diagnostic provider.
// DXVK image -> device-local Vulkan buffer -> imported D3D11 texture for
// depth; direct image copy for RGBA16F. No host-visible pixels/map/readback.
class DxvkTemporalBridge {
    void* state=nullptr;
public:
    DxvkTemporalBridge()=default;~DxvkTemporalBridge();
    DxvkTemporalBridge(DxvkTemporalBridge const&)=delete;
    DxvkTemporalBridge& operator=(DxvkTemporalBridge const&)=delete;
    bool Initialize(IDirect3DDevice9Ex* native);
    // The same native surface may be exported twice, because Gothic reuses
    // one world-depth surface for both eyes. Complete each eye BEFORE the
    // source is overwritten. Aliased sources cannot share a submission.
    TemporalEncoding Register(IDirect3DSurface9* source,ID3D11Device* device,
        Microsoft::WRL::ComPtr<ID3D11Texture2D>& output);
    // Up to six distinct images per submission (two eyes, depth/motion/mask).
    // Each texture must have been registered on this bridge, and reads from
    // its previous batch released after the D3D11 completion fence.
    void Queue(ID3D11Texture2D* output);
    void Complete();
    void ReleaseReads();
    void Reset()noexcept;
};
}
