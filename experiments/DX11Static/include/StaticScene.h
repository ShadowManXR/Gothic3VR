#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <array>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <cstdint>

namespace g3vr::dx11 {
struct Vertex {float position[3],normal[3],uv[2],color[4];};
static_assert(sizeof(Vertex)==48);
struct Section {std::string material;std::vector<Vertex> vertices;std::vector<uint32_t> indices;};
struct Mesh {std::string resource;uint64_t content=0;std::vector<Section> sections;};
Mesh ReadMesh(std::filesystem::path const& file);
struct Instance {uint64_t id=0;std::string resource;uint64_t content=0;DirectX::XMFLOAT4X4 world;};
struct Camera {DirectX::XMFLOAT4X4 viewProjection;};
struct EyeTarget {ID3D11RenderTargetView* color=nullptr;ID3D11DepthStencilView* depth=nullptr;unsigned width=0,height=0;};
struct Statistics {
    uint64_t geometryUploads=0,geometryBytes=0,instanceUploads=0,instanceBytes=0;
    uint64_t textureUploads=0,textureBytes=0;unsigned materials=0,texturedDraws[2]{};
    unsigned preparations=0,submitted=0,visible=0,culled=0,groups=0,draws[2]{};
};
// Owns pipeline state on a dedicated immediate context. Native D3D9 renderers,
// OpenXR session lifecycle, materials and dynamic entities remain separate.
// No hook is authorized to suppress native draws just because Upload succeeds.
class StaticScene {
    struct Impl;std::unique_ptr<Impl> impl;
public:
    StaticScene();~StaticScene();StaticScene(StaticScene const&)=delete;
    void Initialize(ID3D11Device*,ID3D11DeviceContext*);
    void Upload(Mesh const&); // Immutable content cache; replacement is transactional.
    void LoadMaterialPreview(std::filesystem::path const&); // Native diffuse/alpha, simplified lighting.
    void Evict(std::string const& resource);
    void Prepare(std::vector<Instance> const&,std::array<Camera,2> const&);
    void Render(unsigned eye,EyeTarget const&,bool instanced=true);
    Statistics Stats()const;
    void Reset()noexcept;
};
// Replacement is deliberately fail-open until materials and native depth are
// reproduced. Both eyes need current-frame ownership before a native draw can
// ever be omitted. Diffuse preview has simplified lighting, not exact parity.
struct ReplacementProof {
    uint64_t frame=0,epoch=0,geometryContent=0;
    uint64_t eyeFrame[2]{},eyeEpoch[2]{},eyeContent[2]{};
    bool materialsExact=false,depthCompatible=false,liveEntity=false;
    bool CanOmitNative()const noexcept;
};
}
