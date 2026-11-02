#pragma once
#include <d3d9.h>
namespace diagnostic {
// The native engine reuses its current camera for small auxiliary passes.
// Camera identity alone does not establish that this is the headset world.
// Observe actual GPU state; do not restore/expand the auxiliary pass viewport.
struct StereoWorldPass {
    enum class Kind {Unavailable,World,AuxiliaryViewport,AuxiliaryTarget};
    Kind kind=Kind::Unavailable;
    D3DVIEWPORT9 viewport{};unsigned backWidth=0,backHeight=0,targetWidth=0,targetHeight=0;
    bool IsWorld()const noexcept{return kind==Kind::World;}
};
inline StereoWorldPass InspectStereoWorldPass(IDirect3DDevice9* device)noexcept {
    StereoWorldPass out;if(!device||FAILED(device->GetViewport(&out.viewport)))return out;
    IDirect3DSurface9* surface=nullptr;D3DSURFACE_DESC back{},target{};
    if(FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&surface))||!surface)return out;
    auto result=surface->GetDesc(&back);surface->Release();if(FAILED(result)||!back.Width||!back.Height)return out;
    out.backWidth=back.Width;out.backHeight=back.Height;
    // Test the viewport first. In the captured failure it was still 512x512
    // on a 3840x2160 backbuffer when the first eye began and cleared depth.
    if(out.viewport.X||out.viewport.Y||out.viewport.Width!=back.Width||out.viewport.Height!=back.Height){out.kind=StereoWorldPass::Kind::AuxiliaryViewport;return out;}
    surface=nullptr;if(FAILED(device->GetRenderTarget(0,&surface))||!surface)return out;
    result=surface->GetDesc(&target);surface->Release();if(FAILED(result))return out;
    out.targetWidth=target.Width;out.targetHeight=target.Height;
    // Full-size native HDR intermediates are valid, regardless of format or
    // surface identity. Small preview/shadow/reflection targets stay native.
    out.kind=target.Width==back.Width&&target.Height==back.Height?StereoWorldPass::Kind::World:StereoWorldPass::Kind::AuxiliaryTarget;
    return out;
}
}
