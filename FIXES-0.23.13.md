# 0.23.13 — one-shot real depth/color export for DLAA validation

The 19:51 live session, PID17892, completed all four stereo metadata frames in
0.23.12 (XR frames988–991). It identified a3840×2160 D24S8 depth surface with no
IDirect3DTexture9 container. Its sampled geometry VS uses matViewProjection in
c0–c3 and matWorld in c4–c6; the former matches the transposed SDK eye view×projection
in all eight samples (maximum normalized error about5.3e-7). This is a confirmed
geometry depth/occlusion shader, not proof of a complete skinned velocity pass.

This build extends the opt-in capture, not in-game DLAA:

- DXVK's ID3D9VkInteropTexture supports direct surface access. The diagnostic
  reader obtains the actual Vulkan backing format and copies only its depth
  aspect into a host-visible buffer. It materializes deferred clears, synchronizes
  the queue, restores the original depth/stencil layout, then decodes depth to
  float32 in[0,1]. It does not assume the D3D9 enum equals the Vulkan format.
- On the first successful captured stereo frame, save depth for each eye and
  a private matching color copy. Color is copied while D3D11 still owns the
  imported DXVK source, before releasing the keyed mutex. File writes/readback
  occur only for this opt-in diagnostic capture; this CPU path is not intended
  for per-frame DLAA transport.
- Log records vr_temporal_depth_snapshot identify the frame, eye, dimensions,
  formats, raw files, range, variation, file-success flags and errors. The .f32
  file is little-endian float32 depth; .bgr is top-left BGR8 color. This is raw
  camera depth, not linear meters. Projection data in the matching eye record
  must be used to reconstruct distance. Constant/empty depth is not a useful
  scene-depth validation even when file output succeeded.
- Shader discovery now collects up to eight distinct geometry shaders per eye.
  Its larger bytecode storage is allocated on the heap when arming, not on the
  game thread stack, and released when capture finishes. Raw depth vectors,
  private color textures and Vulkan transfer storage are also released promptly.
- Ordinary rendering with no request performs no depth capture or extra color
  copy. The existing request file still arms four metadata stereo frames, then
  is removed. Unsupported depth capture is reported; no synthetic depth/MVs are
  silently substituted.

GPU regression coverage extends the existing actual DXVK Runtime tests: full
and rectangular depth clears, repeated capture, discardable/non-discardable
D24S8 surfaces, exact per-pixel values and preserved bindings; actual armed
Runtime capture with a pending prior-eye color, UI transitions, matching saved
depth/color for each eye and released snapshot allocations. The rest of the
two-eye/UI/cinema/cancellation/lifetime regressions remain enabled. Vulkan GPU
timeouts still require the existing safe bridge failure/reconnect behavior.

The live game's depth pixels still need a next-run visual validation against
matching color. Moving-object/skinned motion vectors, per-frame GPU temporal
transport, jitter/history integration and in-game NGX evaluation are not yet
implemented. No claim of working in-game DLAA or improved Ardea FPS is made.

Implementation references: vendored upstream DXVK3.1.1 d3d9_interfaces.h;
[Vulkan copy rules](https://docs.vulkan.org/spec/latest/chapters/copies.html) and
[depth format definitions](https://docs.vulkan.org/spec/latest/chapters/formats.html).
