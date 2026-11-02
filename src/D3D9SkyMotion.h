#pragma once
#include "TemporalInstanceIdentity.h"
#include <d3d9.h>
#include <memory>
#include <filesystem>
#include <ostream>
namespace g3vr {
// Opt-in late sky raster. Replays the exact successful indexed sky draw
// immediately, before depth/stencil or source buffers can be overwritten.
class D3D9SkyMotion {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    struct Report {unsigned candidates=0,draws=0,rejected=0;bool history=false,materialReactive=true;char error[160]{};};
    D3D9SkyMotion();~D3D9SkyMotion();
    static bool Replaying()noexcept;
    void Begin(unsigned eye,diagnostic::temporal::GeometryFrameStamp const&,unsigned width,unsigned height,std::array<float,2> rasterJitter={});
    void OpaqueBoundary(IDirect3DDevice9*,IDirect3DSurface9*);
    void Replay(IDirect3DDevice9*,diagnostic::temporal::Geometry const&);
    void End()noexcept;
    void ResetHistory()noexcept;
    void Reset()noexcept;
    // RG=previous-current pixels; B=actual contributed draw coverage;
    // A=reactivity (NOT the geometry layer's history-valid channel).
    IDirect3DSurface9* Surface(unsigned eye)const noexcept;
    Report const& Status(unsigned eye)const noexcept;
    void Write(std::ostream&,unsigned eye,std::filesystem::path const& directory,std::string const& prefix,bool pixels)const;
};
}
