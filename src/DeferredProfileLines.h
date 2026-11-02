#pragma once
#include <array>
#include <cstring>
#include <cstdint>
namespace diagnostic::performance {
// Fixed storage: native eye callbacks may enqueue telemetry, but must not
// flush a file while that eye's CPU/GPU interval is still being measured.
class DeferredProfileLines {
public:
    static constexpr size_t LineCapacity=8192;
private:
    std::array<std::array<char,LineCapacity>,16> lines{};unsigned count=0;
    uint64_t dropped=0;
public:
    bool Push(char const* line){
        if(!line)return false;auto size=std::strlen(line);
        if(count==lines.size()||size>=lines[0].size()){++dropped;return false;}
        std::memcpy(lines[count++].data(),line,size+1);return true;
    }
    template<class Write> void Drain(Write write){
        // Clear the active count before calling out. If a logger throws, the
        // next eye must not replay stale events from an aborted interval.
        auto pending=count;count=0;
        for(unsigned i=0;i<pending;++i)write(lines[i].data());
    }
    uint64_t TakeDropped(){auto result=dropped;dropped=0;return result;}
};
}
