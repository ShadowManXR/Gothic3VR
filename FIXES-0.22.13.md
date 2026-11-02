# 0.22.13 — same-frame GPU transfers and stage timings

- Batch the two eye textures and UI texture within their existing XR frame.
- Finish shared D3D9 writes once; finish D3D11 reads once before XR image release.
- Include radial updates in the same final D3D11 fence.
- Preserve image ownership on partial frames and retry after fence/release errors.
- Avoid forcing D3D9/D3D11 command submission on every poll.
- Record fixed-storage frame-stage timings and frame interval statistics.
- Preserve both-eye frustum rebuilding, resolution, UI scale, controllers,
  camera behavior, hands/weapon calibration, progression and native locomotion.

See PERFORMANCE.md for measurement definitions, the comparison switch and tests.
In-game and headset performance acceptance remains pending.
