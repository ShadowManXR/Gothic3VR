#pragma once
#include <d3d9.h>
#include <filesystem>
#include <memory>
#include <cstdint>
#include <iosfwd>
#include <array>
namespace g3vr::dx11 {
// Borrowed only while the matching original render call is on the stack.
// The provider copies the additional stream after native atlas setup, never
// before ExecuteInstance has assigned the live atlas coordinates.
struct NativeLightmapSource {
    void* context=nullptr;
    bool (__cdecl* copy)(void*,void*,unsigned vertices,unsigned bytes)noexcept=nullptr;
};
struct NativeSamplerCapture {
    uintptr_t identity=0;std::array<DWORD,10> state{};
    D3DSURFACE_DESC image{};unsigned levels=0,type=0;bool valid=false;
};
struct NativeMaterialDraw {
    uint64_t frame=0,vertexHash=0,pixelHash=0;unsigned eye=2,drawKind=0;
    int matchedRow=-1;
    std::array<float,1024> vertex{};std::array<float,896> pixel{};
    std::array<DWORD,11> states{};std::array<NativeSamplerCapture,16> samplers;
    std::array<D3DVERTEXELEMENT9,MAXD3DDECLLENGTH+1> declaration{};UINT elements=0;
    NativeLightmapSource lightmap;
};
// Read-only evidence from a live original draw, not from a later solid boundary.
// No shader/constant/sampler mutation, GPU readback or disk I/O in Observe.
class NativeMaterialCapture {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    NativeMaterialCapture();~NativeMaterialCapture();
    void Initialize(std::filesystem::path const&,unsigned width,unsigned height,bool continuous=false);
    unsigned Begin(unsigned eye,uint64_t frame,char const* mesh,char const* material,float const* deferredWorldCm=nullptr,NativeLightmapSource lightmap={})noexcept;
    void End(unsigned token)noexcept;
    void Observe(IDirect3DDevice9*,unsigned drawKind)noexcept;
    void Flush()noexcept;
    unsigned Count()const noexcept;
    bool InScope()const noexcept;
    // Invalidated at every Observe, including rejected draws. Never reuses the
    // last successful material for a later unrelated draw in the same frame.
    NativeMaterialDraw const* Latest(unsigned eye,uint64_t frame)const noexcept;
    void WriteDiagnostics(std::ostream&)const;
};
}
