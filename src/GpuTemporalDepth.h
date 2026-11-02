#pragma once
#include "DxvkTemporalBridge.h"
#include <memory>
namespace diagnostic::temporal {
// Decode transported D16/D24/D32 into the R32_FLOAT contract of the camera
// compositor. Dedicated immediate D3D11 context; caller owns keyed mutex and
// frame synchronization. No readback or wait. Output unbound on entry.
class GpuTemporalDepth {
public:
    GpuTemporalDepth();~GpuTemporalDepth();
    GpuTemporalDepth(GpuTemporalDepth const&)=delete;
    GpuTemporalDepth& operator=(GpuTemporalDepth const&)=delete;
    void Initialize(ID3D11Device* device,unsigned width,unsigned height,g3vr::TemporalEncoding encoding);
    void Process(ID3D11DeviceContext* context,ID3D11Texture2D* raw);
    void Reset()noexcept;
    ID3D11Texture2D* Texture()const noexcept;
private:struct Impl;std::unique_ptr<Impl> impl;
};
}
