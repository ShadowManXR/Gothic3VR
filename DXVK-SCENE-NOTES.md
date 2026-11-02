# DXVK bridge implemented — 0.23.5, 2026-10-17

The earlier shared-handle failure below is now bypassed by a Windows DXVK
Vulkan/native-D3D11 import bridge; see DXVK-0.23.5.md. Real GPU tests of the
actual Runtime capture/drain methods pass at 4K/5K. Native D3D11 OpenXR and
VDXR remain in use. The plugin and supplied x86 d3d9.dll are installed with
backups. Live headset behavior and game performance gains remain unmeasured.
The PUP threading fix is preserved.

# PUP stereo threading correction — 2026-10-16

The0.22.24 probe exposed a concrete VR integration issue: the stereo camera call forced its native threading allowance false. All53856 PUP work calls used the force branch, and the largest gameplay PVS call still took277.45ms.0.22.25 forwards the native allowance with the verified PUP profile and restores the pre-stereo pool state. Native distance/loading fallbacks still apply. Shared maintenance budget alone was not effective in24; no live25 performance gain has been measured yet. This correction precedes any DXVK/backend work. See FIXES-0.22.25.md.

# Scene hitches and DXVK — 2026-10-16

The current task targets the large100–230ms gameplay renderer pauses. Small weapon-specific costs are out of scope. The scene/preload root cause is still unproven;0.22.23 subdivides the remaining native renderer work before changing its behavior.

DXVK translates the D3D graphics path, so it is relevant to driver work and graphics-resource handling. That alone does not establish that it will shorten this game's scene-data reset, PVS cell management, resource-file work or waits for engine jobs. The0.22.22 eye measurements exclude the four draw methods as the main time sink in the captured hitches, but include other driver/API calls in the remainder. Native loading versus other graphics work still needs separation.

Existing local evidence: DXVK3.1.1 ran the flat game, while VR initialization repeatedly failed at native D3D11 OpenSharedResource with0x80070057. Current SharedEye creates a legacy shared D3D9Ex BGRA8 texture and opens it through native D3D11. It also services the UI/cinema path. Replacing d3d9.dll alone does not replace that bridge. The upstream feature-support wiki explicitly excludes its shared-resource support on Windows. The current release notes' improvement for upstream Wine resource sharing does not document a Windows fix. [DXVK feature support](https://github.com/doitsujin/dxvk/wiki/Feature-support#shared-resources), [DXVK releases](https://github.com/doitsujin/dxvk/releases).

A possible alternative is to integrate at Vulkan instead of relying on those shared Direct3D handles. DXVK exposes ID3D9VkInteropDevice and ID3D9VkInteropTexture. They allow the application to obtain device/queue/image handles and layouts. Its contract requires flushing pending D3D work before Vulkan uses the image, preserving/restoring layouts and locking the submission queue around Vulkan submissions. Calling D3D methods while holding that queue lock can deadlock. Those ownership rules must be implemented for both eyes, UI, partial-frame cancellation and teardown. This is an engineering route, not a tested solution or an FPS claim. [DXVK interop definitions](https://raw.githubusercontent.com/doitsujin/dxvk/master/src/d3d9/d3d9_interfaces.h).

VDXR documents Vulkan support through XR_KHR_vulkan_enable and XR_KHR_vulkan_enable2. The supplied local runtime source also implements the Vulkan session path (session.cpp) and Vulkan interop (vulkan_interop.cpp). This makes a DXVK→Vulkan→OpenXR backend worth investigating if graphics/driver work proves material. Installed x86 runtime extensions, matching physical device, enabled device/instance extensions, image formats and synchronization still need a standalone compatibility probe. No live Vulkan/OpenXR submission has been tested here. [VDXR developer documentation](https://github.com/mbucchia/VirtualDesktop-OpenXR/wiki/Developers).

Current implementation0.22.23 keeps D3D9Ex/D3D11 and the known-good fences. No DXVK/graphics DLL, layer, registry or runtime is installed/changed. No scene job is disabled or moved off-thread. Next capture needs only world traversal: compare the new refresh/PVS phases against each large native-eye stall. A later Vulkan compatibility proof should be independent of accepted gameplay and should measure whole-frame behavior, not assume that lower driver CPU overhead fixes native resource stalls.
