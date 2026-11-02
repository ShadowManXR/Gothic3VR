#pragma once
#include "DxvkTemporalBridge.h"
#include "GpuTemporalDepth.h"
#include "GpuTemporalMotion.h"
#include "TemporalInstanceIdentity.h"
#include "GpuTemporalFinalize.h"
#include <wrl/client.h>
#include <memory>

namespace g3vr {
// Live GPU input path. Capture each eye at EndSolid, before Gothic overwrites
// its shared native depth. Private decoded depth/motion survive the next eye.
// No NGX invocation, inferred static mask, CPU pixels or complete-coverage claim.
class GameTemporalInputs {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    struct Result {uint64_t frame=0;unsigned eye=2;bool cameraHistory=false;};
    GameTemporalInputs();~GameTemporalInputs();
    GameTemporalInputs(GameTemporalInputs const&)=delete;
    void Initialize(IDirect3DDevice9Ex* native,ID3D11Device* device,ID3D11DeviceContext* context,unsigned width,unsigned height);
    Result Capture(unsigned eye,diagnostic::temporal::GeometryFrameStamp const& stamp,
        float const* sdkView,float const* sdkProjection,IDirect3DSurface9* opaqueDepth,
        IDirect3DSurface9* skinMotion,IDirect3DSurface9* nativeMotion,std::array<float,2> rasterJitter={});
    // Late passes must finish before this call. A missing/unsupported sky
    // surface leaves unknown pixels reactive; it is never inferred from depth.
    void Finalize(unsigned eye,diagnostic::temporal::GeometryFrameStamp const& stamp,
        IDirect3DSurface9* skyMotion,bool skyCameraHistory);
    ID3D11Texture2D* Motion(unsigned eye)const noexcept;
    ID3D11Texture2D* Depth(unsigned eye)const noexcept;
    ID3D11Texture2D* Reactive(unsigned eye)const noexcept;
    bool Finalized(unsigned eye,uint64_t frame)const noexcept;
    bool Camera(unsigned eye,uint64_t frame,diagnostic::temporal::CameraMotionFrame& camera,
        diagnostic::temporal::GeometryFrameStamp& stamp)const noexcept;
    char const* Transport()const noexcept;
    void Reset()noexcept;
};
}
