#include "OwnedStonePass.h"
#include "NativeTrialBinding.h"
#include "../../../src/TemporalObjectHistory.h"
#include <wrl/client.h>
#include <vector>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <limits>

namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
namespace {
constexpr unsigned Vertices=2097,AtlasStride=24,MaximumDraws=128;
constexpr size_t MaximumCpuBytes=8*1024*1024;
void Need(bool valid,char const* message){if(!valid)throw std::runtime_error(message);}
void Check(HRESULT hr,char const* message){if(FAILED(hr))throw std::runtime_error(message);}
}

struct OwnedStoneImages::Impl {
    PassStamp stamp;
    ComPtr<ID3D11Device> device;
    std::array<ComPtr<ID3D11Texture2D>,4> images;
    std::array<ComPtr<ID3D11ShaderResourceView>,4> views;
};
OwnedStoneImages::OwnedStoneImages()=default;
OwnedStoneImages::~OwnedStoneImages()=default;

std::shared_ptr<OwnedStoneImages const> OwnedStoneImages::Capture(ID3D11Device* device,ID3D11DeviceContext* context,ResidentStoneInputs const& inputs,PassStamp const& stamp){
    uint64_t allocations=0;return CaptureMutable(device,context,inputs,stamp,{},allocations);
}
std::shared_ptr<OwnedStoneImages> OwnedStoneImages::CaptureMutable(ID3D11Device* device,ID3D11DeviceContext* context,ResidentStoneInputs const& inputs,PassStamp const& stamp,std::shared_ptr<OwnedStoneImages> reuse,uint64_t& allocations){
    Need(device&&context&&stamp.Valid(),"Invalid owned image pass");
    ComPtr<ID3D11Device> owner;context->GetDevice(&owner);
    Need(owner.Get()==device&&context->GetType()==D3D11_DEVICE_CONTEXT_IMMEDIATE,"Owned snapshots require matching immediate context");
    ID3D11ShaderResourceView* views[]{inputs.noise,inputs.shadows[0],inputs.shadows[1],inputs.lightmap};
    std::array<ComPtr<ID3D11Texture2D>,4> sources;
    std::array<D3D11_TEXTURE2D_DESC,4> descriptions{};
    std::array<D3D11_SHADER_RESOURCE_VIEW_DESC,4> viewDescriptions{};
    uint64_t pixels=0;
    for(unsigned i=0;i<4;++i){
        Need(views[i]!=nullptr,"Missing owned image input");views[i]->GetDevice(&owner);
        Need(owner.Get()==device,"Foreign owned image device");
        auto& view=viewDescriptions[i];views[i]->GetDesc(&view);
        Need(view.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&view.Texture2D.MostDetailedMip==0&&view.Texture2D.MipLevels==1,"Unsupported owned image view");
        Need(view.Format==((i==1||i==2)?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_B8G8R8A8_UNORM),"Unsupported owned image format");
        ComPtr<ID3D11Resource> resource;views[i]->GetResource(&resource);
        Check(resource.As(&sources[i]),"Owned image is not 2D");auto& d=descriptions[i];sources[i]->GetDesc(&d);
        Need(d.Width&&d.Height&&d.Width<=4096&&d.Height<=4096&&d.ArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1,"Owned image size or layout");
        if(i==0)Need(d.Width==128&&d.Height==128,"Owned noise dimensions");
        if(i==3)Need(d.Width<=2048&&d.Height<=2048,"Owned atlas dimensions");
        pixels+=uint64_t(d.Width)*d.Height;
    }
    Need(pixels<=8388608,"Owned image GPU budget exceeded");
    auto result=reuse?std::move(reuse):std::shared_ptr<OwnedStoneImages>(new OwnedStoneImages);
    bool compatible=result->impl&&result->impl->device.Get()==device;
    for(unsigned i=0;i<4&&compatible;++i){D3D11_TEXTURE2D_DESC old{};result->impl->images[i]->GetDesc(&old);auto const& current=descriptions[i];compatible=old.Width==current.Width&&old.Height==current.Height&&old.Format==current.Format;}
    std::unique_ptr<Impl> fresh;if(!compatible){
        fresh=std::make_unique<Impl>();fresh->device=device;
        // Allocate the entire new set before publishing it or issuing copies.
        for(unsigned i=0;i<4;++i){auto d=descriptions[i];d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.CPUAccessFlags=d.MiscFlags=0;
            Check(device->CreateTexture2D(&d,nullptr,&fresh->images[i]),"Owned image allocation");++allocations;
            Check(device->CreateShaderResourceView(fresh->images[i].Get(),&viewDescriptions[i],&fresh->views[i]),"Owned image view allocation");
        }
    }
    auto& target=fresh?*fresh:*result->impl;
    for(unsigned i=0;i<4;++i)if(target.images[i].Get()!=sources[i].Get())context->CopyResource(target.images[i].Get(),sources[i].Get());
    target.stamp=stamp;if(fresh)result->impl=std::move(fresh);return result;
}
struct OwnedStoneImagePool::Impl {
    unsigned maximum=0;uint64_t allocations=0;
    std::vector<std::shared_ptr<OwnedStoneImages>> slots;
};
OwnedStoneImagePool::OwnedStoneImagePool(unsigned slots){
    Need(slots&&slots<=8,"Owned image pool slot budget");impl=std::make_unique<Impl>();impl->maximum=slots;impl->slots.reserve(slots);
}
OwnedStoneImagePool::~OwnedStoneImagePool()=default;
std::shared_ptr<OwnedStoneImages const> OwnedStoneImagePool::Capture(ID3D11Device* device,ID3D11DeviceContext* context,ResidentStoneInputs const& inputs,PassStamp const& stamp){
    for(auto& slot:impl->slots)if(slot.use_count()==1){slot=OwnedStoneImages::CaptureMutable(device,context,inputs,stamp,slot,impl->allocations);return slot;}
    Need(impl->slots.size()<impl->maximum,"Owned image pool exhausted by live passes");
    auto slot=OwnedStoneImages::CaptureMutable(device,context,inputs,stamp,{},impl->allocations);impl->slots.push_back(slot);return slot;
}
uint64_t OwnedStoneImagePool::Allocations()const noexcept{return impl?impl->allocations:0;}
unsigned OwnedStoneImagePool::Slots()const noexcept{return impl?unsigned(impl->slots.size()):0;}
PassStamp OwnedStoneImages::Stamp()const noexcept{return impl?impl->stamp:PassStamp{};}
ResidentStoneInputs OwnedStoneImages::Inputs()const noexcept{
    if(!impl)return {};return {impl->views[0].Get(),{impl->views[1].Get(),impl->views[2].Get()},impl->views[3].Get()};
}

struct OwnedStonePass::Impl {
    struct Packet {
        NativeMaterialDraw draw;
        NativeStaticSnapshot entity;
        std::shared_ptr<OwnedStoneAtlas const> atlas;
        NativeMaterialDraw View()noexcept{auto result=draw;result.lightmap=atlas->Source();return result;}
    };
    State state=State::Empty;
    PassStamp stamp;
    std::shared_ptr<OwnedStoneImages const> images;
    std::vector<std::unique_ptr<Packet>> packets;
    std::vector<NativeMaterialDraw> views;
    size_t bytes=0;
    bool Reject()noexcept{state=State::Rejected;views.clear();packets.clear();images.reset();bytes=0;return false;}
};
OwnedStonePass::OwnedStonePass()=default;
OwnedStonePass::~OwnedStonePass()=default;
OwnedStonePass::OwnedStonePass(OwnedStonePass&&)noexcept=default;
OwnedStonePass& OwnedStonePass::operator=(OwnedStonePass&&)noexcept=default;

bool OwnedStonePass::Begin(PassStamp const& stamp,std::shared_ptr<OwnedStoneImages const> images){
    // A Begin can never silently discard an unconsumed pass.
    if(impl&&(impl->state==State::Collecting||impl->state==State::Sealed))return impl->Reject();
    impl=std::make_unique<Impl>();
    if(!stamp.Valid()||!images||!(images->Stamp()==stamp))return impl->Reject();
    impl->stamp=stamp;impl->images=std::move(images);impl->packets.reserve(MaximumDraws);impl->views.reserve(MaximumDraws);impl->state=State::Collecting;return true;
}
bool OwnedStonePass::Append(NativeMaterialDraw const& draw,NativeStaticSnapshot const& entity,diagnostic::temporal::Geometry const& geometry){
    return AppendImpl(draw,entity,geometry,{});
}
bool OwnedStonePass::AppendCached(NativeMaterialDraw const& draw,NativeStaticSnapshot const& entity,diagnostic::temporal::Geometry const& geometry,std::shared_ptr<OwnedStoneAtlas const> atlas,StoneAtlasVersion const& expected){
    if(!impl)return false;
    if(!expected.Valid()||expected.epoch!=impl->stamp.epoch||!atlas||!(atlas->Version()==expected))return impl->Reject();
    return AppendImpl(draw,entity,geometry,std::move(atlas));
}
bool OwnedStonePass::AppendImpl(NativeMaterialDraw const& draw,NativeStaticSnapshot const& entity,diagnostic::temporal::Geometry const& geometry,std::shared_ptr<OwnedStoneAtlas const> atlas){
    if(!impl)return false;auto& s=*impl;
    try{
    if(s.state!=State::Collecting)return s.Reject();
    constexpr size_t PacketBytes=sizeof(Impl::Packet)+size_t(Vertices)*AtlasStride+sizeof(NativeMaterialDraw);
    static_assert(PacketBytes<=MaximumCpuBytes);
    if(s.packets.size()>=MaximumDraws||s.bytes>MaximumCpuBytes-PacketBytes)return s.Reject();
    if(draw.frame!=s.stamp.frame||draw.eye!=s.stamp.eye||draw.vertexHash!=0x7a13cd383a26eb4bull||draw.pixelHash!=0x26cb84265f99bd7dull||draw.matchedRow!=0||draw.drawKind!=1)return s.Reject();
    if(!ValidNativeIdentity(entity)||NativeResourceKey(entity.resource)!="g3_myrtana_ardea_stairs_02.g3mesh")return s.Reject();
    for(float value:entity.worldMeters)if(!std::isfinite(value))return s.Reject();
    if(entity.worldMeters[3]||entity.worldMeters[7]||entity.worldMeters[11]||entity.worldMeters[15]!=1)return s.Reject();
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float expected=draw.vertex[(12+r)*4+c]*(c==3?.01f:1.f);if(expected!=entity.worldMeters[c*4+r])return s.Reject();}
    if(geometry.primitive!=D3DPT_TRIANGLELIST||geometry.base||geometry.minimum||geometry.first||geometry.vertices!=Vertices||geometry.count!=1130)return s.Reject();
    for(auto const& old:s.packets)if(old->entity.identity==entity.identity)return s.Reject();
    if(!atlas)atlas=OwnedStoneAtlas::Capture(draw.lightmap);
    if(!atlas)return s.Reject();
    auto packet=std::make_unique<Impl::Packet>();packet->draw=draw;packet->draw.lightmap={};packet->entity=entity;packet->atlas=std::move(atlas);
    s.packets.push_back(std::move(packet));s.bytes+=PacketBytes;return true;
    }catch(...){s.Reject();throw;}
}
bool OwnedStonePass::Seal(){
    if(!impl)return false;auto& s=*impl;if(s.state!=State::Collecting||s.packets.empty())return s.Reject();
    try{for(auto& packet:s.packets)s.views.push_back(packet->View());}catch(...){s.Reject();throw;}
    s.state=State::Sealed;return true;
}
bool OwnedStonePass::Render(NativeStoneRenderer& renderer,PassStamp const& expected,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,unsigned width,unsigned height,float const* clearColor){
    if(!impl)return false;auto& s=*impl;if(s.state!=State::Sealed||!(s.stamp==expected))return s.Reject();
    try{if(!renderer.RenderResidentBatch(s.views.data(),unsigned(s.views.size()),s.images->Inputs(),target,depth,width,height,clearColor))return s.Reject();
        s.state=State::Consumed;return true;
    }catch(...){s.Reject();throw;}
}
bool OwnedStonePass::RenderSamplerControl(NativeStoneRenderer& renderer,PassStamp const& expected,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth,unsigned width,unsigned height){
    if(!impl)return false;auto& s=*impl;
    if(s.state!=State::Sealed||!(s.stamp==expected))return s.Reject();
    try{auto controls=s.views;
        for(auto& draw:controls)for(unsigned slot:{4u,5u})draw.samplers[slot].state[3]=draw.samplers[slot].state[4]=D3DTEXF_LINEAR;
        if(!renderer.RenderResidentBatch(controls.data(),unsigned(controls.size()),s.images->Inputs(),target,depth,width,height))return s.Reject();
        return true;
    }catch(...){s.Reject();throw;}
}
OwnedStonePass::State OwnedStonePass::Status()const noexcept{return impl?impl->state:State::Empty;}
unsigned OwnedStonePass::Count()const noexcept{return impl?unsigned(impl->packets.size()):0;}
size_t OwnedStonePass::CpuBytes()const noexcept{return impl?impl->bytes:0;}
}
