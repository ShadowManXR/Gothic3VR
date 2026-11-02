# Launcher appearance

The native Windows launcher uses a dark sidebar, vertical navigation for all 18
existing pages, gold accents and separate settings cards. The supplied Gothic 3 VR
artwork is embedded in the executable beneath a dark overlay. No external asset
paths, web view, animation loop or additional game-side components are needed.

`tools/LauncherLayout.h` arranges the controls and switches to compact spacing
for shorter windows. `tools/LauncherTheme.h` paints the cached background and
native controls. Settings collection, atomic saves, launch behavior and protected
Union+ integration remain in `tools/Launcher.cpp`.

Page changes suspend redraw while controls are moved and the background is
prepared, then present the complete descendant tree with Windows compositing.
Static text updates bypass the native static control's immediate paint. Controls
and dropdown lists erase to the dark background rather than a white intermediate
surface. The UI check also exercises this erase stage on each visible control.

Sliders retain native input and value handling but paint their entire client area
into a small back buffer on every movement. This clears the full antialiased thumb
instead of relying on the smaller native thumb's dirty rectangle. The UI check
moves every visible slider in both directions and verifies full repaint coverage.
Full invalidation happens on input/value changes, never from inside `WM_PAINT`.
The native trackbar completes its own paint lifecycle before the next input.
An idle message-pump check detects recurring slider paints after updates settle.

The retired DLAA checkbox, messages and mutual-exclusion handlers have been removed.
Saving clears its old configuration flag without disabling FFR or the upscaler.
The UI check covers stale flags with an active FFR preset and an active NIS preset.

`tools/Launcher.rc` embeds the background, application metadata and multiresolution
G3 VR icon. The icon was edited with built-in image generation using the extracted
game icon as reference; its final prompt is `tools/assets/icon-prompt.txt`.
The PNG was packaged as an ICO with 16, 24, 32, 48, 64, 128 and 256 pixel entries.

Visual verification (no game launch and no settings writes):

```text
Gothic3VRLauncher.exe --ui-check <absolute-output.bmp> [width height dpi]
```

This checks all pages, visible control bounds, text clipping, checkbox toggles,
navigation, numeric value preservation and slider-to-edit updates. It saves a
snapshot and result for each page. The normal launcher starts without this mode.
