#pragma once
namespace diagnostic {
// Shared by the native engine hooks and runtime. A new runtime mode must never
// start without the same native integration. Truncated/malformed values stay off.
inline unsigned NativeTrialMode(wchar_t const* value,unsigned length,bool validationOnly=false)noexcept {
    unsigned mode=value&&length==1&&value[1]==0&&value[0]>=L'1'&&value[0]<=L'5'
        ?unsigned(value[0]-L'0'):0;
    return validationOnly&&mode!=5?0:mode;
}
}
