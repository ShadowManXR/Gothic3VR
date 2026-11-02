"""Convert the existing approved RGBA logo to the native 24-bit color-key BMP.

No new artwork is generated. The original Engine splash loader uses RGB 128,
128,128 as its transparent color and reads splash.bmp from the game root.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--native-bmp', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original = args.native_bmp.read_bytes()
    if original[:2] != b'BM' or len(original) < 54:
        raise ValueError('Expected native splash bitmap')
    width, height = struct.unpack_from('<ii', original, 18)
    bits, compression = struct.unpack_from('<HI', original, 28)
    if (width, height, bits, compression) != (300, 300, 24, 0):
        raise ValueError('Native splash format is not the verified 300x300 RGB24 profile')
    if args.output.exists():
        raise FileExistsError('Refusing to overwrite an existing splash asset')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with Image.open(args.image) as source:
        if source.mode != 'RGBA' or source.getchannel('A').getextrema()[0] != 0:
            raise ValueError('Use the approved transparent logo')
        logo = source.copy()
        logo.thumbnail((width, height), Image.Resampling.LANCZOS)
        bitmap = Image.new('RGB', (width, height), (128, 128, 128))
        bitmap.paste(logo, ((width-logo.width)//2, (height-logo.height)//2), logo.getchannel('A'))
        bitmap.save(args.output, format='BMP')
    with Image.open(args.output) as decoded:
        decoded.load()
        if decoded.size != (width, height) or decoded.mode != 'RGB':
            raise ValueError('BMP format round-trip failed')
        for point in [(0, 0), (width-1, 0), (0, height-1), (width-1, height-1)]:
            if decoded.getpixel(point) != (128, 128, 128):
                raise ValueError('Native transparent color key must remain intact')
    digest = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
    receipt = dict(Resource='splash.bmp', Width=width, Height=height, BitsPerPixel=24,
                   Compression='BI_RGB', TransparentColor='808080', LogoSha=digest(args.image),
                   NativeSha=digest(args.native_bmp), Sha=digest(args.output), NativeCodeChanged=False)
    args.output.with_suffix('.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(receipt))


if __name__ == '__main__':
    main()
