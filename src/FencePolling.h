#pragma once
#include <cstdint>
namespace g3vr {
struct FencePollStats {uint64_t polls=0,flushes=0;};
struct FencePollSchedule {
    uint64_t lastFlush=0;
    bool submitted=false;
    bool Flush(uint64_t now) noexcept {
        if(!submitted||now<lastFlush||now-lastFlush>=2){submitted=true;lastFlush=now;return true;}
        return false;
    }
};
}
