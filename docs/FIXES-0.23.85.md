# 0.23.85 — Medieval status icon artwork

Replaces the procedural eye, block shield and red finishing skull with a
cohesive set of aged brass, patinated steel and crimson metal relief artwork.
Original RGBA sources, final PNGs, generation prompts and conversion manifest
are retained in `assets/status-icons/`.

The embedded 128×128 premultiplied texels replace the previous 64×64 GDI
symbols. Physical dimensions, shared location above the HUD, one existing
compositor layer, state priority and state-change upload caching are preserved.
The missing-sneak-skill text notice is unchanged. No new runtime file loading,
decoder, render effect, state polling or diagnostic probes are introduced.

Validation: transparent padding, premultiplied alpha and 239 maximum opacity;
distinct icons and red skull; byte-exact embedded pixel copy; unchanged notice;
uniform placement across HUD scales, aspect ratios and offsets. The actual
D3D11/OpenXR submission fixture verifies texture upload, both-eye visibility,
unchanged state texture reuse and a single shared status layer.

Headset appearance remains to be checked by the player.
