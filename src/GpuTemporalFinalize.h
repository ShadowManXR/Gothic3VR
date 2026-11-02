#pragma once
#include <d3d11.h>
#include <memory>
namespace diagnostic::temporal {
// Merge the late sky only after the real eye render has finished. Unknown
// pixels stay unknown/reactive; a cleared depth buffer is not sky evidence.
class GpuTemporalFinalize {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    GpuTemporalFinalize();~GpuTemporalFinalize();
    void Initialize(ID3D11Device*,unsigned width,unsigned height);
    void Process(ID3D11DeviceContext*,ID3D11Texture2D* opaqueMotion,
        ID3D11Texture2D* skyMotion,bool skyCameraHistory);
    ID3D11Texture2D* Motion()const noexcept;
    ID3D11Texture2D* Reactive()const noexcept;
    void Reset()noexcept;
};
}
