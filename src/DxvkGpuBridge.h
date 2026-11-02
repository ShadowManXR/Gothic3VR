#pragma once
#include "StereoRenderer.h"
#include <d3d9.h>
#include <cstdint>
namespace g3vr {
struct DepthSnapshot {
    UINT width=0,height=0;uint32_t sourceFormat=0,vulkanFormat=0;
    std::vector<float> depth;
};
// Optional Windows DXVK backend. Public declarations keep Vulkan headers and
// imports out of the ordinary D3D9Ex/D3D11 build path. No CPU pixel transfers.
class DxvkGpuBridge {
    void* state=nullptr;
public:
    DxvkGpuBridge()=default;
    DxvkGpuBridge(DxvkGpuBridge const&)=delete;
    DxvkGpuBridge& operator=(DxvkGpuBridge const&)=delete;
    ~DxvkGpuBridge();
    bool Initialize(IDirect3DDevice9Ex* native);
    bool Active() const noexcept {return state!=nullptr;}
    void Create(IDirect3DDevice9Ex* native,ID3D11Device* device,UINT width,UINT height,
        stereo::ComPtr<IDirect3DTexture9>& texture9,stereo::ComPtr<IDirect3DSurface9>& surface9,
        stereo::ComPtr<ID3D11Texture2D>& texture11);
    void Queue(ID3D11Texture2D* source);
    void Complete();
    void ReleaseReads();
    // Diagnostic readback only. Not the per-frame DLAA GPU transport.
    bool CaptureDepthSnapshot(IDirect3DSurface9* surface,DepthSnapshot& result);
    void ReleaseDepthSnapshotStorage() noexcept;
    void Reset() noexcept;
};
}
