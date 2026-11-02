#pragma once
#include <d3d11.h>
#include <array>
#include <memory>

namespace diagnostic::temporal {
using Matrix = std::array<double,16>;
// SDK row vectors: clip = world * view * projection. Invert in double and
// combine before converting to GPU floats; large Gothic world origins must
// not be reconstructed in single precision for every pixel.
std::array<float,16> CameraReprojection(Matrix const& currentVP,Matrix const& previousVP);
struct CameraMotionFrame {
    std::array<float,16> currentClipToPreviousClip{};
    // Actual image displacement in pixels, +X right / +Y down.
    std::array<float,2> currentRasterJitter{},previousRasterJitter{};
    bool historyValid=false;
    // Existing D3D9 companions can remove jitter themselves. Preserve that
    // contract by default; raw future layers must explicitly opt in so their
    // previous-current raster delta is removed here exactly once.
    bool geometryIncludesRasterJitter=false;
};
// Independently tested GPU building block, NOT a complete game temporal
// provider. A dedicated immediate context must be used. Output must be unbound
// on entry. No readback/wait, scene classification, or NGX invocation here.
// Known-static mask is mandatory: zero means UNKNOWN, never assumed static.
class GpuTemporalMotion {
public:
    GpuTemporalMotion();~GpuTemporalMotion();
    GpuTemporalMotion(GpuTemporalMotion const&)=delete;
    GpuTemporalMotion& operator=(GpuTemporalMotion const&)=delete;
    void Initialize(ID3D11Device* device,unsigned width,unsigned height);
    void Reset()noexcept;
    // R32_FLOAT D3D depth [0,1], RGBA16F native skin (RG pixels,B occupied,A
    // history valid), R8_UNORM exact boolean known-static mask. No aliasing,
    // MSAA, subviews or different dimensions/devices. Unknown and sky invalid.
    void Process(ID3D11DeviceContext* context,ID3D11ShaderResourceView* depth,
        ID3D11ShaderResourceView* skin,ID3D11ShaderResourceView* knownStatic,
        CameraMotionFrame const& frame);
    // Add the rigid/instanced/wind motion layer. Conflicting coplanar vectors
    // invalidate history instead of selecting an arbitrary surface's motion.
    void ProcessLayers(ID3D11DeviceContext* context,ID3D11ShaderResourceView* depth,
        ID3D11ShaderResourceView* skin,ID3D11ShaderResourceView* nativeGeometry,
        ID3D11ShaderResourceView* knownStatic,CameraMotionFrame const& frame);
    ID3D11Texture2D* Texture()const noexcept;
private:struct Impl;std::unique_ptr<Impl> impl;
};
}
