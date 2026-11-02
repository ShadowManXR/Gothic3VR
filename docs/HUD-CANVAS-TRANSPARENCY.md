# HUD and menu panel color boundary

Local plugin0.23.109 addresses the rectangular color boundary observed in both VDXR and Meta Link, absent on the desktop and moving with HUD vertical position. The world image and global gamma are unchanged.

The actual captured HUD had7348933pixels at alpha10/255 (88.6%of3840x2160). Empty sample pixels were RGBA(1,0,0,10), creating a faint dark red layer over the full floating panel. A bounded per-draw GPU atlas identified draw0 as the source in both gameplay HUD and the main menu. Native metadata: full viewport; one two-triangle quad; FVF0x142; source-alpha/inverse-source-alpha blending; alpha test enabled; native RGBA writes, no separate alpha blending;2048x2048atlas; no vertex/pixel shaders. Draw1 contains real menu artwork and later draws contain widgets.

The VR UI replay omits only the first backbuffer command matching that native base-canvas profile. Other texture sizes, custom shaders, partial viewports and non-quad draws do not match. Later draws using the same atlas are retained. This targets the observed native GUI; different UI replacements need a separate check. Native desktop commands still execute exactly once. There is no global alpha threshold, removal of dark widgets, filtering of all atlas uses, color compensation or extra fullscreen GPU pass.

Real D3D9/D3D11 tests reproduce the measured RGBA canvas, verify every empty XR pixel is zero, compare desktop pixels with the unmodified control, retain a later same-atlas translucent sprite, and retain normal opaque/translucent HUD elements. UI frame-submission and clean forwarding regressions pass. Headset confirmation of the installed fix is pending.

The bounded diagnostic build used only four explicitly armed UI snapshots and a nine-pixel128-draw atlas for one settled frame per UI state. General profiling remained disabled. The installed production build has G3VR_UI_COMPOSITION_CAPTURE=OFF along with all other development modes. Captures remain outside the game in the task workspace. No installer package was built.
