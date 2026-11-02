#pragma once
#include "TemporalNativeGeometry.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <iterator>
namespace g3vr {
struct NativeMotionDraw {
    inline static constexpr D3DSAMPLERSTATETYPE AlphaSamplerStates[]={D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV,D3DSAMP_ADDRESSW,D3DSAMP_BORDERCOLOR,D3DSAMP_MAGFILTER,D3DSAMP_MINFILTER,D3DSAMP_MIPFILTER,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_MAXMIPLEVEL,D3DSAMP_MAXANISOTROPY,D3DSAMP_SRGBTEXTURE};
    struct Stream {Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> vertices;unsigned offset=0,stride=0,frequency=1;};
    Microsoft::WRL::ComPtr<IDirect3DVertexBuffer9> vertices;
    Microsoft::WRL::ComPtr<IDirect3DIndexBuffer9> indices;
    Microsoft::WRL::ComPtr<IDirect3DVertexDeclaration9> declaration;
    diagnostic::temporal::Geometry geometry;
    diagnostic::temporal::NativeGeometryKind kind=diagnostic::temporal::NativeGeometryKind::RigidWorld;
    diagnostic::temporal::NativeGeometryPose current,previous;
    unsigned offset=0,stride=0,frequency=1;DWORD cull=D3DCULL_NONE;
    std::array<Stream,15> auxiliaryStreams; // slots1..15; position remains slot0
    float depthBias=0,slopeBias=0;bool historyValid=false;
    std::array<uint8_t,17> instanceHistory{}; // explicit proof per current row
    // Only the exact validated wind/PS pair can populate this material.
    Microsoft::WRL::ComPtr<IDirect3DTexture9> alphaTexture;
    std::array<DWORD,std::size(AlphaSamplerStates)> alphaSampler{};
    DWORD alphaReference=0;bool masked=false;
};
// Companion for exact observed rigid/instanced/wind position programs. It
// takes verified same-geometry history, never assumes a rigid mesh is static.
// Used by the explicitly requested game temporal diagnostic collector.
class D3D9NativeMotion {
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<IDirect3DDevice9> device;std::array<Ptr<IDirect3DVertexShader9>,4> shaders;
    Ptr<IDirect3DVertexShader9> maskedWind;
    Ptr<IDirect3DPixelShader9> pixel,maskedPixel;Ptr<IDirect3DTexture9> output;Ptr<IDirect3DSurface9> surface;
    unsigned width=0,height=0,targets=0;inline static thread_local unsigned replayDepth=0;
public:
    static constexpr unsigned Limit=1024;
    struct Report {unsigned draws=0,historyDraws=0,constantBytes=0;};
    static bool Replaying()noexcept{return replayDepth!=0;}
    void Initialize(IDirect3DDevice9* native,unsigned width,unsigned height);
    void Reset()noexcept;
    Report Render(IDirect3DSurface9* opaqueDepth,NativeMotionDraw const* draws,unsigned count,
        std::array<float,2> currentRasterJitter={},std::array<float,2> previousRasterJitter={});
    IDirect3DTexture9* Texture()const noexcept{return output.Get();}
    IDirect3DSurface9* Surface()const noexcept{return surface.Get();}
};
}
