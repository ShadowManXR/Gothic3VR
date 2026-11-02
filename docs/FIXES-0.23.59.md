# 0.23.59 — CPU overhead cleanup

The independent support libraries now explicitly use MSVC Release `/O2 /Ob2
/Gy`. The engine SDK clears CMake's normal Release flags, which previously left
these libraries without an explicit optimization level. Calling conventions,
exception handling, precise floating point behavior and GPU shaders are unchanged.

World-matrix draws now consult a thread-local held-particle scope. Ordinary world
geometry forwards immediately without querying the application thread or shared
particle state. The scope still changes only one identity transform for a held
effect, leaves native restoration intact, and resets independently for each eye.
Failed nesting cannot clear the outer scope; workers cannot inherit it.

The tracked-palette hook similarly uses a thread-local stereo-render marker.
Unrelated animation factories always run their native update. The marker resets
at the start of the outer finally block, including failed stereo frames.

This is a clean runtime build: scene/draw capture, renderer experiments and general
profilers are compiled out. No draw distance, vegetation, ROI, AI scheduling,
navigation behavior, camera pose, hand calibration or graphics quality is changed.
Native per-eye frusta and the accepted PUP threading behavior remain intact.

Validation: 134/134 CTests passed, including the actual DXVK 4K/5K bridge, FFR,
upscaler, native input, weapon/hand fixtures and the new particle thread-isolation
test. These are correctness checks. A headset FPS gain is not yet measured.
