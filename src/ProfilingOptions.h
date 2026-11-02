#pragma once
#include <windows.h>

namespace diagnostic::performance {
// Detailed profiling is an explicit process-local diagnostic mode. Reading an
// absent, malformed or truncated value must never enable hot-path detours.
inline bool ProfilingOptIn(wchar_t const* name) noexcept {
    wchar_t value[2]{};
    return GetEnvironmentVariableW(name,value,2)==1&&value[0]==L'1';
}
}
