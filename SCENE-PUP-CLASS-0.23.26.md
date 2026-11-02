# Read-only PUP low-poly classification

Actual0.23.25 PID20268 started with execution mask130047: the native LowPoly
entry was already modified. Offline inspection of hash-verified paru_s2.dll
proves PUP installs its wrapper at RVA11680 on RenderLowPoly. That wrapper sets
byte994AC, calls its original-method slot973E8, clears the byte, and returns.

0.23.26 validates this entire21byte wrapper with its three loader-relocated
absolute addresses after the existing PUP file hash/PIN check. The existing
native renderer hook reads the byte on selected main-thread world frames and
records two appended metrics: sampled_pup_low_poly_renderer_inclusive and
sampled_pup_non_low_poly_renderer_inclusive (IDs49/50, old IDs unchanged).

No new detour is installed. PUP wrapper, flag and native entry are never written.
The earlier native LowPoly hook may remain refused; the new classification is
reported separately by pup_low_poly_flag_telemetry_ready. Invalid/missing PUP
state is unclassified, never treated as a measured zero. This measures renderer
work within PUP's low-poly state, not the entire wrapper's camera setup.

The non-low-poly renderer may CONTAIN the low-poly renderer. Metrics are sparse,
inclusive and overlap; never add them. Existing sampling/capture exclusion and
original forwarding are retained. This release is diagnostic, not an FPS fix.

Tests verify the actual PUP wrapper, installer binding to the native method,
all three HIGHLOW relocations, alternate image bases and invalid flags. PUP code
is not executed by the test. User settings, launcher, DXVK/PUP/native files and
gameplay are preserved; DLAA remains disabled.
