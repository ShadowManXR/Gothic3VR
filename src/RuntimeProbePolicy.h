#pragma once
#include "FramePerformance.h"
#include <type_traits>

namespace diagnostic::probes {
#ifdef G3VR_CLEAN_RUNTIME
inline constexpr bool Enabled=false;
#else
// Offline tests and explicitly built development plugins retain the probes.
inline constexpr bool Enabled=true;
#endif
#ifdef G3VR_DX11_VALIDATION
inline constexpr bool Dx11ValidationOnly=true;
#else
inline constexpr bool Dx11ValidationOnly=false;
#endif
inline constexpr bool StaticTrial=Enabled||Dx11ValidationOnly;
inline uint64_t Now()noexcept {
    if constexpr(Enabled)return performance::Now();
    return 0;
}
struct SilentFrameCollector {
    void Begin()noexcept{}
    void Ready(uint64_t)noexcept{}
    void Add(performance::Stage,uint64_t)noexcept{}
    void Count(performance::Counter,uint64_t=1)noexcept{}
    void Cancel()noexcept{}
    void Pause()noexcept{}
    bool Finish(uint64_t,performance::Report&)noexcept{return false;}
};
using FrameCollector=std::conditional_t<Enabled,performance::Collector,SilentFrameCollector>;
}
