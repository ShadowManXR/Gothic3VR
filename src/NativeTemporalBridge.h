#pragma once
#include "DxvkTemporalBridge.h"
namespace g3vr {
// NVIDIA native D3D9 depth conversion / shared D3D11 textures. Pixels stay on
// the GPU. Uses the installed driver, never a replacement renderer DLL.
// Caller fences D3D11 reads before ReleaseReads. Reset quarantines outstanding
// ownership on failure rather than destroying resources still in GPU use.
class NativeTemporalBridge {
    void* state=nullptr;
public:
    NativeTemporalBridge()=default;
    ~NativeTemporalBridge();
    NativeTemporalBridge(NativeTemporalBridge const&)=delete;
    NativeTemporalBridge& operator=(NativeTemporalBridge const&)=delete;
    bool Initialize(IDirect3DDevice9Ex* native);
    TemporalEncoding Register(IDirect3DSurface9* source,ID3D11Device* device,
        Microsoft::WRL::ComPtr<ID3D11Texture2D>& output);
    void Queue(ID3D11Texture2D* output);
    void Complete();
    void ReleaseReads();
    void Reset()noexcept;
};
}
