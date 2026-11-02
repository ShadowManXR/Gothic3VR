# Experimental game DLAA — 0.23.34

The optional Graphics checkbox enables the native-resolution game renderer
path. Default is Off. NVIDIA GPU required; Upscaler and FFR must be Off. Restart
after changing this option. The existing game renderer DLLs remain unchanged.

The x86 plugin sends GPU shared color, depth, motion and reactive textures to
its owned hidden x64 helper. The helper uses the preserved official NVIDIA NGX
DLAA implementation and independent histories for each eye. Install its worker
and nvngx_dlss.dll together in scripts/Gothic3VRDiagnostics/dlaa/.

Actual per-eye projection jitter is applied before native drawing. Native skin,
rigid, instanced and supported foliage companions remove raster jitter exactly
once. The exact supported late sky pass is replayed without changing native
color, depth or stencil. The finalizer keeps geometric validity distinct from
material reactivity.

This is explicitly a partial-motion experiment. Unsupported objects/materials
use depth-based camera reprojection with maximum BiasCurrentColorMask, rather
than claiming full scene motion. That mask biases current color; it does not
guarantee absence of ghosting or perfect reconstruction. Far-world depth ranges
and translucent effects still need visual assessment in game. No DLAA image
quality or performance result in Gothic 3 is claimed from synthetic tests.

Both eyes are copied and evaluated within the same XR frame before acquired
swapchain images are released. Color pixels stay on the GPU. Menus/HUD/radial
remain separate XR layers. Current and previous native aiming-reticle rectangles
bypass filtering; reticle projection remains unjittered. Missing inputs or helper
errors disable the experiment and restore both current raw eye images.

Normal AA play does not enable shader surveys, CPU pixel snapshots or per-draw
scene profiling. Those remain explicit capture-only diagnostics. No capture
request is needed for the visual DLAA trial. Look for vr_dlaa_ready followed by
vr_dlaa_frame with written_eyes=2; vr_dlaa_disabled/unavailable gives the fallback
reason. Initialization alone is not evidence of filtered game frames.

Validation includes real NVIDIA x86-to-x64 NGX transport, native D3D9 sharing,
analytical GPU fallback motion/reactive checks, original shader sky replay,
stereo isolation, invalid/stale input rejection and raw reticle preservation.
Headset quality, moving foliage/NPC trails and cost remain to be assessed live.
