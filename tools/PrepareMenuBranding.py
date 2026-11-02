"""Encode approved menu artwork into the existing native DXT3 texture slot.

This is format conversion for an imagegen-edited image, not an image editor.
No native archives, icon mappings or runtime code are changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--native-dds", required=True, type=Path)
    parser.add_argument("--payload", required=True, type=Path)
    args = parser.parse_args()
    original = args.native_dds.read_bytes()
    if original[:4] != b"DDS " or original[84:88] != b"DXT3":
        raise ValueError("Expected the verified native DXT3 menu atlas")
    height, width = struct.unpack_from("<II", original, 12)
    if (width, height) != (1024, 1024) or len(original) != 128 + width * height:
        raise ValueError("Unexpected native menu atlas dimensions or mip layout")
    relative = Path("Data/gui/icons/compiled/g3_hud_backgrounds_02.dds")
    output = args.payload / relative
    if output.exists():
        raise FileExistsError("Refusing to overwrite an existing branding payload")
    output.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(args.image) as source:
        if source.mode != "RGBA":
            raise ValueError("Approved image must preserve its alpha channel")
        image = source.resize((width, height), Image.Resampling.LANCZOS)
        image.save(output, pixel_format="DXT3")
    encoded = bytearray(output.read_bytes())
    if len(encoded) != len(original) or encoded[84:88] != b"DXT3":
        raise ValueError("Texture no longer matches the native format and allocation")
    # Keep the atlas's exact native header and BC2 coverage bits, including
    # reserved/unused texels and the thin ornamental border. Only compressed
    # colour blocks come from the approved artwork; this is native-format
    # packing, so the image generator cannot accidentally make padding opaque.
    encoded[:128] = original[:128]
    for block in range(128, len(encoded), 16):
        encoded[block:block + 8] = original[block:block + 8]
    output.write_bytes(encoded)
    with Image.open(output) as decoded:
        decoded.load()
        if decoded.size != (width, height):
            raise ValueError("DDS round-trip dimensions differ")
        if decoded.getchannel("A").crop((0, 768, width, height)).getextrema() != (0, 0):
            raise ValueError("Unused atlas area must remain transparent")
        decoded.save(args.payload / "main-menu-vr-preview.png")
    manifest = {"resource": relative.as_posix(), "format": "DXT3 / BC2",
                "width": width, "height": height, "mip_levels": 1,
                "bytes": len(encoded), "same_bytes_as_native": True,
                "sha256": hashlib.sha256(encoded).hexdigest(),
                "native_atlas_sha256": hashlib.sha256(original).hexdigest(),
                "artwork_sha256": hashlib.sha256(args.image.read_bytes()).hexdigest(),
                "native_alpha_blocks_preserved": True,
                "dds_round_trip": "passed", "original_archives_modified": False}
    (args.payload / "menu-branding-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
