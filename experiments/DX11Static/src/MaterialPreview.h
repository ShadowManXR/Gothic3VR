#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <map>
#include <string>
#include <cstdint>
namespace g3vr::dx11 {
// Native diffuse and alpha only. Lighting/normal/parallax parity is not implied.
class MaterialPreview {
    struct Texture {Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;uint64_t bytes=0;};
public:
    struct Entry {Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuse;Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;float scale[2]{1,1};bool masked=false;float alphaReference=0;};
private:
    std::map<std::string,Entry> entries;std::map<uint64_t,Texture> textures;
    uint64_t content=0,bytes=0,uploads=0;
public:
    bool Load(ID3D11Device*,std::filesystem::path const&);
    Entry const* Find(std::string const&)const noexcept;
    uint64_t Bytes()const noexcept{return bytes;}
    uint64_t Uploads()const noexcept{return uploads;}
    unsigned Count()const noexcept{return unsigned(entries.size());}
};
}
