# Gothic 3 VR status artwork

Created with the built-in `image_gen` tool (transparent background enabled),
2026-10-27. The [prompt set](prompts.md) documents the three individual requests.

- `sneak.png`: watchful eye engraved in aged brass.
- `parry.png`: dark steel shield with worn brass rim.
- `kill.png`: crimson skull relief, preserving the red finishing cue.
- `source/`: original high-resolution generated RGBA artwork.
- `preview.png`: exact final-size icons over dark and light HUD backgrounds.
- `manifest.json`: source and packaged PNG hashes.

Run `python tools/PrepareStatusArtwork.py` from any directory to reproduce the
128×128 PNGs and `src/WorldStatusArtwork.h`. This is mechanical aspect-preserving
resampling, padding and pixel-format conversion; the generated art and alpha
are retained. Maximum runtime alpha remains 239/255, as before.

The runtime uses embedded, premultiplied BGRA texels. It performs no PNG decoding
or external asset lookup. Existing state-change texture caching remains active;
no per-frame image processing, new compositor layer or larger swapchain is added.
All three icons keep their previous common centered 7 cm square directly above
the HUD, with unchanged native sneak/block/finisher state logic and priority.
Only their raster resolution rises from 64×64 to 128×128 for cleaner contours.
The English missing-sneak-skill notice keeps its existing 256×64 layout.
