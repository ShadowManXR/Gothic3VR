# 0.23.68 — SMAA 1x High and Ultra presets

The existing Anti-aliasing selector adds SMAA 1x High and SMAA 1x Ultra.
The original Medium preset remains the default for existing SMAA settings.
All three are spatial SMAA 1x; these labels do not claim temporal or MSAA
supersampling. MSAA and the deferred DLSS5 investigation are untouched.

`SmaaQuality` persists Medium=0, High=1, Ultra=2. Unknown values safely select
Medium; switching to FXAA or Off preserves the stored quality preference.
Only the selected preset's shaders are created at startup. GPU scratch sizes
and the separate per-eye, gamma-correct pipeline are unchanged. A game restart
applies selection changes.

Each preset is compiled from the upstream definitions for both vertex and pixel
stages. Medium: threshold0.1/search8/no diagonal or corner detection. High:
threshold0.1/search16/diagonal8/corner25. Ultra: threshold0.05/search32/
diagonal16/corner25. Nothing is simulated by repeating the Medium filter.

Validation extends native/NIS/CAS hardware tests to all three presets and adds
a low-contrast edge fixture that Ultra must detect while Medium/High leave it
untouched. All presets preserve scene alpha, channel/gamma order and edge-to-flat
clearing across alternating input textures. D3D11 debug validation remains
clean. Settings tests cover each preset's roundtrip, safe resize propagation,
legacy Medium default and invalid-value fallback. Launcher interaction checks
exercise all five Anti-aliasing choices. Headset quality remains user validation.
