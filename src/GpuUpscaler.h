#pragma once
#include <d3d11.h>
#include <memory>
#include "UpscalePlan.h"
namespace diagnostic {
// Dedicated native D3D11 context. The caller owns the D3D9/shared-image and
// XR acquire/release fences; Process queues GPU work, never a CPU readback.
class GpuUpscaler {
public:
    GpuUpscaler();~GpuUpscaler();
    GpuUpscaler(GpuUpscaler const&)=delete;GpuUpscaler& operator=(GpuUpscaler const&)=delete;
    void Initialize(ID3D11Device* device,UpscalePlan const& plan);
    void Reset() noexcept;
    void Process(ID3D11DeviceContext* context,ID3D11Texture2D* output,ID3D11Texture2D* input);
private:struct Impl;std::unique_ptr<Impl> impl;
};
}
