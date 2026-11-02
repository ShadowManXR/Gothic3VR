# 0.23.56 — clean normal VR runtime

Normal plugin builds now use G3VR_RUNTIME_PROBES=OFF (default), defining
G3VR_CLEAN_RUNTIME in the game plugin and stereo runtime. The separate DX11
development binary and captured evidence remain outside the game directory.

Excluded from normal execution, even with stale environment variables/requests:

- Native temporal solid/animated/mesh/instancing interception.
- DX11 static trials, material observation and native draw suppression.
- Temporal shader/depth/motion/color readback captures and request arming.
- CPU/update/frame/per-draw profiling and GPU timestamp query creation/polling.
- Automatic camera/controller/arm-pose sampling, traces and UI diagnostic dumps.
- Experimental pending-visible occlusion substitution; native results retained.

UI draw hooks still perform the required VR menu replay. Their world path now
forwards the original draw directly, with no temporal observer or timer. Copy
completion fences still synchronize required eye/UI transfers. Gameplay clocks
and the PUP optional queue's shared stereo budget remain functional.

The SDK clears CMake Release flags. Explicit /O2 /Ob2 /Gy are applied to the
independent stereo runtime so it is optimized and unreachable probe paths are
removed. Gameplay features and all saved settings/distances remain unchanged.

Validation includes a real GPU clean UI test: a deliberately installed stale
observer cannot suppress world draws, execute callbacks or collect shader
metadata; native pixels, UI alpha, target/depth state and movie copy remain
correct. Actual compositor submission is also tested in clean mode. Binary
audit verifies experimental activation strings and profiler/capture markers
are absent. Headset FPS improvement still requires live comparison.

DX11 development stays in experiments/DX11Static/bin at 0.23.55. Its launcher
validates both clean and development hashes; experimental modes refuse the
clean plugin rather than silently doing nothing. Prepare/deploy the separate
development binary explicitly when resuming DX11 tests. Default mode 0 keeps
using ordinary native VR. No development binary is loaded by normal launches.
