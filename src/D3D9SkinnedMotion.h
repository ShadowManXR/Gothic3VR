#pragma once
#include "TemporalObjectHistory.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <array>
#include <cstddef>

namespace g3vr {
struct NativeSkinVertex {
    float position[3]{},weights[4]{1,0,0,0};unsigned char indices[4]{};
    float normal[3]{0,0,1},tangent[3]{1,0,0};uint32_t color=0xffffffff;float uv[2]{};
};
static_assert(sizeof(NativeSkinVertex)==68&&offsetof(NativeSkinVertex,weights)==12&&offsetof(NativeSkinVertex,indices)==28);
struct SkinnedMotionDraw {
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> vertices;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> indices;
    diagnostic::temporal::Geometry geometry;
    unsigned offset=0,stride=68;DWORD cull=D3DCULL_NONE;
    float depthBias=0,slopeBias=0;
    diagnostic::temporal::SkinPose current,previous;
    bool historyValid=false;
};
// GPU-only rasterizer used by the bounded one-shot game diagnostic. Not yet a
// complete scene motion provider or connected to NGX. Call per independently
// owned eye target, while its original opaque depth is still available.
class D3D9SkinnedMotion {
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<IDirect3DDevice9> device;
    Ptr<IDirect3DVertexShader9> vertexShader;
    Ptr<IDirect3DPixelShader9> pixelShader;
    Ptr<IDirect3DVertexDeclaration9> declaration;
    Ptr<IDirect3DTexture9> palette,output;
    Ptr<IDirect3DSurface9> surface;
    unsigned width=0,height=0,targets=0;
    inline static thread_local unsigned replayDepth=0;
public:
    static bool Replaying()noexcept {return replayDepth!=0;}
    static constexpr unsigned Limit=diagnostic::temporal::ObjectHistory::Limit;
    struct Report {unsigned draws=0,historyDraws=0,paletteBytes=0;};
    void Initialize(IDirect3DDevice9* native,unsigned width,unsigned height);
    Report Render(IDirect3DSurface9* nativeOpaqueDepth,SkinnedMotionDraw const* draws,unsigned count,
        std::array<float,2> currentRasterJitter={},std::array<float,2> previousRasterJitter={});
    void Reset()noexcept;
    IDirect3DTexture9* Texture()const noexcept {return output.Get();}
    IDirect3DSurface9* Surface()const noexcept {return surface.Get();}
};
}
