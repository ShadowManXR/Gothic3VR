# Valve OpenVR SDK subset

- Repository: https://github.com/ValveSoftware/openvr
- Tag: v2.5.1
- Commit: ae46a8dd0172580648c8922658a100439115d3eb
- Files: headers/openvr.h, bin/win32/openvr_api.dll, LICENSE.
- Interface used: IVRSystem_022, supported through OpenVR's versioned interface mechanism.
- DLL SHA-256: 6FED734924B4B38FAA6417A6C711E3A78D1B162B1FD3042B7D9908EC8D41D2B5.

The DLL is loaded by absolute path at runtime. The plugin does not import it at process startup. Original Valve license is preserved in LICENSE.
