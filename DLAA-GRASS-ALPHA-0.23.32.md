# Exact grass motion and alpha coverage — 0.23.32

This extends the opt-in temporal input collector. It does not enable game DLAA
or claim complete scene coverage. Normal play does not run these replay passes.

Four wind vertex programs now normalize their actual projection/player/wave
uniforms to the previously verified wind companion. Their complete native wind
branch is compared after register renaming, then the remaining POSITION output
is proved symbolically from the original bytecode. Original shaders are also
tested directly on the GPU with wind phase, weights and player proximity.

Alpha coverage is deliberately restricted to the exact native pair:

- VS `15e5561db503fe47`, PS `0cef3519ea7bd097`.
- Original alpha: saturate(diffuse texture slot 3 alpha * interpolated COLOR0
  alpha). Original UV comes from TEXCOORD0.
- Native COLOR alpha uses camera distance to the wind-displaced world position,
  dissolve end and reciprocal dissolve range. The companion preserves the
  native rsq/rcp arithmetic order and COLOR interpolation semantic.
- Only native GREATER alpha comparison, 8-bit reference, zero UV wrap flags and
  a static 2D source texture (usage zero) are admitted. Other shaders, compare
  modes, missing UV, render target/dynamic textures remain rejected.

Each accepted draw retains the actual source texture, all eleven applicable
sampler states, threshold and minimal position/wind/UV declaration. Texture,
filter or threshold changes invalidate matching object history. Geometry and
buffer-version checks remain required. GPU replay preserves alpha holes even
when background geometry shares exactly the same depth; Z/EQUAL alone is not
used as a substitute for material coverage.

A separate masked wind vertex/pixel companion writes RGBA16F motion and validity
flags. Its shader discard implements the original material test; output alpha
continues to mean history validity, not material opacity. State blocks restore
the caller's shaders, constants, texture bindings, sampler and render states.
No CPU texture pixel copies or runtime shader compilation are introduced.

Offline evidence:

- Five wind position proofs (canonical plus four variants), three negative
  symbolic mutations. Twenty-four actual wind GPU cases / 26,992 checked pixels,
  maximum motion error 0.000606 pixel in the reference test.
- Exact native grass alpha dependency proof from fresh original VS/PS bytecode,
  with four negative shader mutations.
- 64 original-VS/PS coverage comparisons on native NVIDIA D3D9 and 64 on DXVK,
  491,520 pixels per backend. A8 and DXT5, thresholds 0/119/120/121/255,
  point/linear/anisotropic filters, mip levels, bias, wrap/clamp/border, native
  fade, both eyes, consecutive-frame history and coplanar holes are exercised.
- Five rejected material cases per backend: different PS, comparison mode,
  coordinate wrap, render target texture and missing UV. Original color and
  overridden sampler bindings/states must be restored. Device references must
  return to their baseline after teardown.

The material proof concerns this exact pair, not all foliage. Coverage for other
materials, sky, reactivity, native jitter/color semantics and the game NGX/output
connection remains unfinished. Live alpha coverage has not been measured yet;
no new user capture is requested by this build.

Supporting original API semantics: [D3D9 shader inputs and outputs](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-writing-shaders-9).
