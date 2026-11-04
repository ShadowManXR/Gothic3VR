"""Sign or verify a release checksum manifest using OpenSSH, without publishing it."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SIGNERS = ROOT / 'release/keys/allowed_signers'
IDENTITY = 'PeCOS2k5'
NAMESPACE = 'g3vr-release'


def check_manifest(manifest):
    """Authenticate bytes separately; reject missing, changed or out-of-folder assets."""
    manifest = manifest.resolve()
    seen = set()
    for line in manifest.read_text(encoding='utf-8-sig').splitlines():
        if not line.strip():
            continue
        match = re.fullmatch(r'([0-9a-fA-F]{64}) [ *](.+)', line)
        if not match:
            raise ValueError('Invalid checksum entry')
        digest, name = match.groups()
        if (name in ('.', '..') or '/' in name or '\\' in name or ':' in name
                or name.casefold() in seen):
            raise ValueError('Unsafe or duplicate checksum filename')
        seen.add(name.casefold())
        asset = manifest.parent / name
        if asset.resolve().parent != manifest.parent or not asset.is_file():
            raise ValueError('Missing or external release asset: ' + name)
        hasher = hashlib.sha256()
        with asset.open('rb') as source:
            for chunk in iter(lambda: source.read(1024 * 1024), b''):
                hasher.update(chunk)
        actual = hasher.hexdigest()
        if actual.lower() != digest.lower():
            raise ValueError('Release checksum mismatch: ' + name)
    if not seen:
        raise ValueError('Empty checksum manifest')
    return manifest


def verify(manifest, signers=SIGNERS):
    manifest = Path(manifest).resolve()
    subprocess.run(['ssh-keygen', '-Y', 'verify', '-f', str(signers), '-I', IDENTITY,
                    '-n', NAMESPACE, '-s', str(manifest) + '.sig'],
                   input=manifest.read_bytes(), check=True)
    check_manifest(manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['sign', 'verify'])
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--key', type=Path, help='Private signing key, stored outside the repository')
    parser.add_argument('--allowed-signers', type=Path, default=SIGNERS)
    args = parser.parse_args()
    if args.action == 'sign':
        manifest = check_manifest(args.manifest)
        if not args.key or not args.key.is_file():
            parser.error('Signing requires --key pointing to the private release key')
        if Path(str(manifest) + '.sig').exists():
            parser.error('A signature already exists; verify it instead of overwriting it')
        subprocess.run(['ssh-keygen', '-Y', 'sign', '-f', str(args.key), '-n', NAMESPACE,
                        str(manifest)], check=True)
    verify(args.manifest, args.allowed_signers)
    print('PASS: PeCOS2k5 release signature and all listed asset checksums verified.')


if __name__ == '__main__':
    main()
