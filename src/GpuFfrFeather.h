#pragma once
#include <d3d11.h>
#include <memory>
#include "FfrFeather.h"
namespace diagnostic {
// One reusable full-size scene target, shared sequentially by both eyes. The
// existing scene AA/upscaler writes here; Present filters directly into XR.
class GpuFfrFeather {
public:
    GpuFfrFeather();~GpuFfrFeather();
    GpuFfrFeather(GpuFfrFeather const&)=delete;GpuFfrFeather& operator=(GpuFfrFeather const&)=delete;
    ID3D11Texture2D* Target(ID3D11Device*,unsigned width,unsigned height);
    void Present(ID3D11DeviceContext*,ID3D11Texture2D* output,FfrFeatherParameters const&);
    void Reset()noexcept;
private:struct Impl;std::unique_ptr<Impl> impl;
};
}
