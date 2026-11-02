"""Convert the supplied menu song to the native Gothic 3 resource format.

Creates one loose override; never modifies the game's original Music archive.
Requires a local ffmpeg executable with the libvorbis encoder.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inspect_ogg(path):
    data = path.read_bytes()
    identification = data.find(b"\x01vorbis", 0, 512)
    if identification < 0:
        raise ValueError("Missing Vorbis identification packet")
    version, channels, rate = struct.unpack_from("<IBI", data, identification + 7)
    if (version, channels, rate) != (0, 2, 48000):
        raise ValueError("Expected native stereo Vorbis at 48000 Hz")
    offset = 0
    final_granule = 0
    serial = None
    sequence = 0
    ended = False
    while offset < len(data):
        if ended or data[offset:offset + 5] != b"OggS\0" or offset + 27 > len(data):
            raise ValueError("Invalid or chained Ogg stream")
        flags = data[offset + 5]
        granule, stream, page = struct.unpack_from("<QII", data, offset + 6)
        if serial is None:
            serial = stream
            if not flags & 2:
                raise ValueError("Missing beginning of stream")
        if stream != serial or page != sequence:
            raise ValueError("Unexpected Ogg page sequence")
        sequence += 1
        segments = data[offset + 26]
        end = offset + 27 + segments
        if end > len(data):
            raise ValueError("Truncated Ogg lacing table")
        end += sum(data[offset + 27:end])
        if end > len(data):
            raise ValueError("Truncated Ogg page")
        if granule != 0xffffffffffffffff:
            final_granule = granule
        ended = bool(flags & 4)
        offset = end
    if not ended or final_granule <= 0:
        raise ValueError("Incomplete Ogg stream")
    return {"codec": "Vorbis", "sample_rate_hz": rate, "channels": channels,
            "duration_seconds": final_granule / rate, "pages": sequence}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--payload", required=True, type=Path)
    parser.add_argument("--ffmpeg", required=True, type=Path)
    args = parser.parse_args()
    relative = Path("Data/Music/fragments/myrtana_title_day_01.ogg")
    output = args.payload / relative
    if output.exists():
        raise FileExistsError("Refusing to overwrite an existing music payload")
    output.parent.mkdir(parents=True, exist_ok=True)
    source_hash = digest(args.input)
    command = [str(args.ffmpeg), "-hide_banner", "-nostdin", "-n", "-i", str(args.input),
               "-map", "0:a:0", "-vn", "-sn", "-dn", "-map_metadata", "-1",
               "-ac", "2", "-ar", "48000", "-c:a", "libvorbis", "-q:a", "7",
               "-metadata", "title=VRsta Point", str(output)]
    subprocess.run(command, check=True, capture_output=True)
    info = inspect_ogg(output)
    subprocess.run([str(args.ffmpeg), "-hide_banner", "-nostdin", "-v", "error", "-xerror",
                    "-i", str(output), "-map", "0:a:0", "-f", "null", "-"],
                   check=True, capture_output=True)
    if digest(args.input) != source_hash:
        raise RuntimeError("Source audio changed during conversion")
    manifest = {"source": str(args.input.resolve()), "source_sha256": source_hash,
                "resource": relative.as_posix(), "sha256": digest(output),
                "bytes": output.stat().st_size, "encoding": info,
                "vorbis_quality": 7, "full_decode_validation": "passed",
                "native_title_sample": "myrtana_title_day_01.wav",
                "original_archives_modified": False}
    (args.payload / "menu-music-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
