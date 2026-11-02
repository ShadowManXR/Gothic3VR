#pragma once
#include "NativeMaterialCapture.h"
#include <memory>

namespace g3vr::dx11 {
// Assigned by a producer which observes ALL writes and allocation reuse.
// A native pointer is never a substitute for generation/revision tracking.
struct StoneAtlasVersion {
    uint64_t epoch=0,source=0,generation=0,revision=0;
    bool Valid()const noexcept{return epoch&&source&&generation&&revision;}
    bool operator==(StoneAtlasVersion const& b)const noexcept{
        return epoch==b.epoch&&source==b.source&&generation==b.generation&&revision==b.revision;
    }
};

class OwnedStoneAtlasCache;
class OwnedStoneAtlas {
    struct Impl;
    std::unique_ptr<Impl> impl;
    OwnedStoneAtlas();
    static bool __cdecl Copy(void*,void*,unsigned,unsigned)noexcept;
    friend class OwnedStoneAtlasCache;
public:
    ~OwnedStoneAtlas();
    OwnedStoneAtlas(OwnedStoneAtlas const&)=delete;
    // Captures and validates the known 2097-vertex, 24-byte atlas stream.
    static std::shared_ptr<OwnedStoneAtlas const> Capture(NativeLightmapSource const&);
    StoneAtlasVersion Version()const noexcept;
    unsigned char const* Data()const noexcept;
    size_t Bytes()const noexcept;
    NativeLightmapSource Source()const noexcept;
    // Only accepts this class's private immutable-source callback. As for the
    // existing borrowed provider, the owner must survive the immediate call.
    static OwnedStoneAtlas const* Resolve(NativeLightmapSource const&)noexcept;
};

// Single render-thread producer/consumer cache. Publish always copies and
// validates a NEW version. Acquire never calls a borrowed provider. Old pinned
// versions survive replacement/reset unchanged and count against the budget.
// No native mutation hook is implemented by this offline cache.
class OwnedStoneAtlasCache {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    explicit OwnedStoneAtlasCache(uint64_t epoch,unsigned sources=128,size_t byteBudget=8*1024*1024);
    ~OwnedStoneAtlasCache();
    std::shared_ptr<OwnedStoneAtlas const> Publish(StoneAtlasVersion const&,NativeLightmapSource const&);
    std::shared_ptr<OwnedStoneAtlas const> Acquire(StoneAtlasVersion const&)const noexcept;
    bool Invalidate(uint64_t source)noexcept;
    bool Reset(uint64_t newEpoch)noexcept;
    size_t LiveBytes()const noexcept;
    uint64_t Captures()const noexcept;
};
}
