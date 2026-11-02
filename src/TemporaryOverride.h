#pragma once

namespace diagnostic
{
using OverrideCallback = void(__cdecl*)(void*);
// The game's exception must propagate; cleanup also runs during SEH unwinding.
// POD context and explicit cdecl keep this independent of the SDK /Gz default.
inline void CallWithRestoration(OverrideCallback work, OverrideCallback cleanup, void* context)
{
    __try { work(context); }
    __finally { cleanup(context); }
}
}
