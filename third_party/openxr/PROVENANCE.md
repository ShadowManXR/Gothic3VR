# Khronos OpenXR loader subset

- Official release: https://github.com/KhronosGroup/OpenXR-SDK/releases/tag/release-1.1.63
- Package: OpenXR.Loader.1.1.63.nupkg from the official GitHub release.
- Files: native/Win32/release/bin/openxr_loader.dll and include/openxr/{openxr.h,openxr_platform.h,openxr_platform_defines.h}.
- Package license: Apache-2.0 OR MIT. Repository LICENSE is preserved.
- Loader SHA-256: D4DD992E067EAABBDFA651C90EB4578E44DB6651FB4539AEC0964DBC22BCFDBA.
- Requested API: OpenXR 1.0; required extensions XR_MND_headless and XR_KHR_win32_convert_performance_counter_time.

The loader is loaded by absolute path. Graphical VR selects the requested installed runtime through a temporary process-local XR_RUNTIME_JSON override, then restores the previous value. Runtime implementations belong to the user's installations and are not redistributed.

## Meta x64 host loader

- Same official OpenXR.Loader 1.1.63 release package; entry native/x64/release/bin/openxr_loader.dll.
- Package SHA-256: 4E5A50A8807EF66F25180FF224E7D8150B594AA8EE4B07590F9ADE55A8E98703.
- x64 loader SHA-256: 66946EA6E9CC2649E299EC412521C5EAFE56C635D05055097FE4DB81E11932DF.
- Source: x64/openxr_loader.dll. Installed: meta64/openxr_loader64.dll.
- Renamed to avoid duplicate resource basenames in Gothic 3; PE binary unchanged.
- Repository LICENSE is installed alongside as OPENXR-META64-LICENSE.txt to avoid a duplicate resource basename with the x86 license.
- The graphical host requests OpenXR 1.0 and XR_KHR_D3D11_enable and uses Meta's installed x64 runtime. No Meta DLL is redistributed.
