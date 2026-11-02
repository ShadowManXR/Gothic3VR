#pragma once
#include <d3d11.h>
#include <memory>
namespace diagnostic {
// SMAA 1x Medium/High/Ultra. Scratch images are shared sequentially between eyes, cleared
// every pass/frame; there is no temporal history, depth or CPU readback.
class GpuSmaa {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    GpuSmaa(ID3D11Device*,unsigned width,unsigned height,bool rgbaInput,int quality);
    ~GpuSmaa();
    void Process(ID3D11DeviceContext*,ID3D11RenderTargetView*,ID3D11Texture2D*);
};
}
