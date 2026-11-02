#include "OwnedStoneAtlas.h"
#include <vector>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace g3vr::dx11 {
namespace {
constexpr unsigned Vertices=2097,Stride=24;
constexpr size_t AtlasBytes=size_t(Vertices)*Stride;
struct Budget {size_t live=0,maximum=0;};
}
struct OwnedStoneAtlas::Impl {
    StoneAtlasVersion version;
    std::vector<unsigned char> bytes;
    std::shared_ptr<Budget> budget;
    ~Impl(){if(budget)budget->live-=bytes.size();}
};
OwnedStoneAtlas::OwnedStoneAtlas()=default;
OwnedStoneAtlas::~OwnedStoneAtlas()=default;
std::shared_ptr<OwnedStoneAtlas const> OwnedStoneAtlas::Capture(NativeLightmapSource const& source){
    if(!source.context||!source.copy)return {};
    auto result=std::shared_ptr<OwnedStoneAtlas>(new OwnedStoneAtlas);
    result->impl=std::make_unique<Impl>();auto& bytes=result->impl->bytes;bytes.resize(AtlasBytes);
    if(!source.copy(source.context,bytes.data(),Vertices,unsigned(bytes.size())))return {};
    static_assert(sizeof(float)==sizeof(uint32_t)&&std::numeric_limits<float>::is_iec559);
    // Packed vertex color is not a float and may legitimately have NaN bits.
    for(unsigned vertex=0;vertex<Vertices;++vertex)for(unsigned component=1;component<6;++component){
        uint32_t bits;std::memcpy(&bits,bytes.data()+vertex*Stride+component*4,4);
        if((bits&0x7f800000u)==0x7f800000u)return {};
    }
    return result;
}
StoneAtlasVersion OwnedStoneAtlas::Version()const noexcept{return impl->version;}
unsigned char const* OwnedStoneAtlas::Data()const noexcept{return impl->bytes.data();}
size_t OwnedStoneAtlas::Bytes()const noexcept{return impl->bytes.size();}
bool __cdecl OwnedStoneAtlas::Copy(void* context,void* target,unsigned vertices,unsigned bytes)noexcept{
    auto const* source=static_cast<OwnedStoneAtlas const*>(context);
    if(!source||!target||vertices!=Vertices||bytes!=source->Bytes())return false;
    std::memcpy(target,source->Data(),bytes);return true;
}
NativeLightmapSource OwnedStoneAtlas::Source()const noexcept{return {const_cast<OwnedStoneAtlas*>(this),&Copy};}
OwnedStoneAtlas const* OwnedStoneAtlas::Resolve(NativeLightmapSource const& source)noexcept{
    return source.copy==&Copy?static_cast<OwnedStoneAtlas const*>(source.context):nullptr;
}

struct OwnedStoneAtlasCache::Impl {
    struct Entry {StoneAtlasVersion version;std::shared_ptr<OwnedStoneAtlas const> atlas;};
    uint64_t epoch=0,captures=0;unsigned maximum=0;
    std::shared_ptr<Budget> budget;
    std::vector<Entry> entries;
};
OwnedStoneAtlasCache::OwnedStoneAtlasCache(uint64_t epoch,unsigned sources,size_t byteBudget){
    if(!epoch||!sources||sources>128||byteBudget<AtlasBytes||byteBudget>8*1024*1024)throw std::runtime_error("Invalid stone atlas cache budget");
    impl=std::make_unique<Impl>();impl->epoch=epoch;impl->maximum=sources;
    impl->budget=std::make_shared<Budget>();impl->budget->maximum=byteBudget;impl->entries.reserve(sources);
}
OwnedStoneAtlasCache::~OwnedStoneAtlasCache()=default;
std::shared_ptr<OwnedStoneAtlas const> OwnedStoneAtlasCache::Publish(StoneAtlasVersion const& version,NativeLightmapSource const& source){
    auto& s=*impl;if(!version.Valid()||version.epoch!=s.epoch)return {};
    Impl::Entry* existing=nullptr;
    for(auto& entry:s.entries)if(entry.version.source==version.source){existing=&entry;break;}
    if(existing){auto const& old=existing->version;
        if(version.generation<old.generation||(version.generation==old.generation&&version.revision<=old.revision))return {};
    }else if(s.entries.size()>=s.maximum)return {};
    // Reserve enough room for both versions until the old one can be released.
    if(s.budget->live>s.budget->maximum-AtlasBytes)return {};
    auto atlas=OwnedStoneAtlas::Capture(source);if(!atlas)return {};
    // Capture returns an unpublished const owner. Only the cache may stamp it.
    auto& data=*atlas->impl;data.version=version;data.budget=s.budget;s.budget->live+=data.bytes.size();
    if(existing){existing->version=version;existing->atlas=atlas;}
    else s.entries.push_back({version,atlas});
    ++s.captures;return atlas;
}
std::shared_ptr<OwnedStoneAtlas const> OwnedStoneAtlasCache::Acquire(StoneAtlasVersion const& version)const noexcept{
    if(!version.Valid()||version.epoch!=impl->epoch)return {};
    for(auto const& entry:impl->entries)if(entry.version==version)return entry.atlas;
    return {};
}
bool OwnedStoneAtlasCache::Invalidate(uint64_t source)noexcept{
    for(auto& entry:impl->entries)if(entry.version.source==source){entry.atlas.reset();return true;}
    return false;
}
bool OwnedStoneAtlasCache::Reset(uint64_t newEpoch)noexcept{
    if(newEpoch<=impl->epoch)return false;impl->entries.clear();impl->epoch=newEpoch;return true;
}
size_t OwnedStoneAtlasCache::LiveBytes()const noexcept{return impl->budget->live;}
uint64_t OwnedStoneAtlasCache::Captures()const noexcept{return impl->captures;}
}
