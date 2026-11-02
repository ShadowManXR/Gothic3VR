#include "NativeStonePacketProducer.h"
#include "NativeMaterialResources.h"
#include "NativeTrialBinding.h"
#include "../../../src/TemporalObjectHistory.h"
#include <wrl/client.h>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace g3vr::dx11 {
using Microsoft::WRL::ComPtr;
struct NativeStonePacketProducer::Impl {
    struct Entry {std::array<uint32_t,5> identity;StoneAtlasVersion version;};
    ComPtr<IDirect3DDevice9Ex> native;ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    NativeMaterialResources resources;OwnedStoneImagePool images{4};
    std::unique_ptr<OwnedStoneAtlasCache> atlas;
    std::vector<Entry> entries;
    uint64_t epoch=0,reads=0,hits=0,nextSource=1;
};
NativeStonePacketProducer::NativeStonePacketProducer()=default;
NativeStonePacketProducer::~NativeStonePacketProducer()=default;
void NativeStonePacketProducer::Initialize(IDirect3DDevice9Ex* native,ID3D11Device* device,ID3D11DeviceContext* context){
    if(!native||!device||!context)throw std::runtime_error("Missing live packet devices");
    auto s=std::make_unique<Impl>();s->native=native;s->device=device;s->context=context;s->entries.reserve(128);
    if(!s->resources.Initialize(native,device,context))throw std::runtime_error("Live packets require verified DXVK interop");impl=std::move(s);
}
std::unique_ptr<OwnedStonePass> NativeStonePacketProducer::Capture(NativeMaterialDraw const& draw,NativeStaticSnapshot const& entity,diagnostic::temporal::Geometry const& geometry,PassStamp const& stamp){
    if(!impl||!stamp.Valid()||draw.frame!=stamp.frame||draw.eye!=stamp.eye||!ValidNativeIdentity(entity)||
        NativeResourceKey(entity.resource)!="g3_myrtana_ardea_stairs_02.g3mesh"||
        draw.vertexHash!=0x7a13cd383a26eb4bull||draw.pixelHash!=0x26cb84265f99bd7dull||draw.matchedRow!=0||draw.drawKind!=1||
        geometry.primitive!=D3DPT_TRIANGLELIST||geometry.vertices!=2097||geometry.count!=1130)return {};
    auto& s=*impl;if(stamp.epoch<s.epoch)return {};
    if(stamp.epoch!=s.epoch){s.atlas=std::make_unique<OwnedStoneAtlasCache>(stamp.epoch);s.entries.clear();s.epoch=stamp.epoch;s.nextSource=1;}
    ++s.reads;auto observed=OwnedStoneAtlas::Capture(draw.lightmap);if(!observed)return {};
    Impl::Entry* entry=nullptr;for(auto& candidate:s.entries)if(candidate.identity==entity.identity){entry=&candidate;break;}
    if(!entry){if(s.entries.size()==128)return {};s.entries.push_back({entity.identity,{s.epoch,s.nextSource++,1,0}});entry=&s.entries.back();}
    auto owned=s.atlas->Acquire(entry->version);
    if(owned&&owned->Bytes()==observed->Bytes()&&!std::memcmp(owned->Data(),observed->Data(),owned->Bytes()))++s.hits;
    else {
        if(entry->version.revision==UINT64_MAX)return {};
        // A failed/new publication must never leave a stale lookup usable.
        s.atlas->Invalidate(entry->version.source);++entry->version.revision;
        owned=s.atlas->Publish(entry->version,observed->Source());if(!owned)return {};
    }
    std::array<ComPtr<IDirect3DTexture9>,4> inputs;
    unsigned slots[]{0,1,2,3};
    for(unsigned i=0;i<4;++i){ComPtr<IDirect3DBaseTexture9> texture;
        if(FAILED(s.native->GetTexture(slots[i],&texture))||!texture||reinterpret_cast<uintptr_t>(texture.Get())!=draw.samplers[slots[i]].identity||FAILED(texture.As(&inputs[i])))return {};
    }
    // Snapshot while interop still owns the shared images. FinishReads fences
    // these GPU copies before returning their original ownership to DXVK.
    s.resources.Capture(inputs[3].Get(),inputs[1].Get(),inputs[2].Get(),inputs[0].Get());
    std::shared_ptr<OwnedStoneImages const> images;
    try{images=s.images.Capture(s.device.Get(),s.context.Get(),{s.resources.Noise(),{s.resources.Shadow(0),s.resources.Shadow(1)},s.resources.Lightmap()},stamp);s.resources.FinishReads();}
    catch(...){try{s.resources.FinishReads();}catch(...){}throw;}
    auto pass=std::make_unique<OwnedStonePass>();
    // The imported, private GPU mesh has local indices. Native pool buffer
    // offsets belong only to the original DIP/reference, not this owned mesh.
    // Actual native buffer-content equivalence is NOT inferred here; mode5's
    // original-DIP image/depth comparison is still required and never suppresses.
    diagnostic::temporal::Geometry ownedGeometry{D3DPT_TRIANGLELIST,0,0,2097,0,1130};
    if(!pass->Begin(stamp,images)||!pass->AppendCached(draw,entity,ownedGeometry,owned,entry->version)||!pass->Seal())return {};
    return pass;
}
uint64_t NativeStonePacketProducer::AtlasReads()const noexcept{return impl?impl->reads:0;}
uint64_t NativeStonePacketProducer::AtlasHits()const noexcept{return impl?impl->hits:0;}
uint64_t NativeStonePacketProducer::ImageCaptures()const noexcept{return impl?impl->resources.Captures():0;}
}
