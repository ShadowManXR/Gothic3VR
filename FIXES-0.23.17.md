# 0.23.17 — native lighting stream in animated geometry capture

The live0.23.16 capture (PID13712, frames596..599) identified the exact rejection:
354 supported-shader draws all hit multiple_vertex_streams. Native lighting
adds declaration element7: stream1, offset0, D3DCOLOR(type4), COLOR(usage10),
index0, after the seven native68-byte skin elements in stream0.

The observer now accepts auxiliary default-method D3DCOLOR/COLOR streams.
The three exact whitelisted shaders compute position from stream0 POSITION,
BLENDWEIGHT and BLENDINDICES only. Color buffers do not enter geometry identity;
rotating a lighting buffer must not discard animation history. The complete
declaration remains part of the key. Secondary position/skin data and unknown
shader layouts remain unsupported and explicitly reported.

The real DXVK test reproduces the actual captured stream1 declaration and
changes its lighting buffer between frames. Same-eye skin histories must
survive, while secondary POSITION data must still be rejected. Native states
and device lifetimes remain unchanged. No game shaders or lighting are edited.

This is diagnostic input capture. DLAA and rasterized motion vectors are still
not enabled. Actual per-entity animation correspondence needs live validation.
