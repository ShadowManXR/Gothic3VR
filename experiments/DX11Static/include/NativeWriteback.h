#pragma once
#include <d3d9.h>
#include <d3d11.h>
#include <memory>
#include <cstdint>

namespace g3vr::dx11 {
// Identifies the projection/pass, not merely a texture of the right size.
struct PassStamp {
    uint64_t frame=0,epoch=0,camera=0;unsigned eye=2;
    bool Valid()const noexcept{return frame&&epoch&&camera&&eye<2;}
    bool operator==(PassStamp const& b)const noexcept{return frame==b.frame&&epoch==b.epoch&&camera==b.camera&&eye==b.eye;}
};
// Experimental reverse bridge. No native hooks or game-draw suppression.
// A dedicated immediate DX11 context is required. Calls on the native render thread.
class NativeWriteback {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    NativeWriteback();~NativeWriteback();
    NativeWriteback(NativeWriteback const&)=delete;
    void Initialize(IDirect3DDevice9Ex*,ID3D11Device*,ID3D11DeviceContext*,unsigned width,unsigned height);
    // Producer color: BGRA8_UNORM[_SRGB] (sRGB-encoded bytes). Depth: R32_FLOAT SRV,
    // standard [0,1], same projection as the native pass, clear=1. No MSAA/reverse Z.
    void Transfer(ID3D11Texture2D* color,ID3D11ShaderResourceView* depth,PassStamp const&);
    // Inside caller's BeginScene. Preserves native render state and stencil.
    // Rejects stale/wrong-eye passes before submitting any native draw.
    void Composite(PassStamp const& expected,bool preserveNativeAlpha=false);
    bool UsesDxvk()const;
    void Reset()noexcept;
};
}
