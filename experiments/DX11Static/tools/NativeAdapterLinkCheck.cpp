#include "NativeSceneAdapter.h"
// Build/link verification only. No DllMain, hooks or automatic initialization.
extern "C" __declspec(dllexport) bool G3DX11CopyNativeStatic(
    eCRenderContext* context,g3vr::dx11::NativeStaticSnapshot* output)noexcept {
    return output&&g3vr::dx11::CopyNativeStatic(context,*output);
}
