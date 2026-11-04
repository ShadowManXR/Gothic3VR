# Verify an official release

Gothic 3 VR was created and is maintained by **PeCOS2k5**. Official downloads:

- https://github.com/PeCOS2k5/Gothic3VR/releases
- https://www.nexusmods.com/gothic3/mods/115

## Checksums and release signatures

For a release that provides `SHA256SUMS.txt.sig`, download the matching
`SHA256SUMS.txt`, its signature and the files listed in that manifest into one folder.
Keep the original filenames. Do not edit the checksum manifest or convert its line endings.

Obtain `release/keys/PeCOS2k5-release.pub`, `release/keys/allowed_signers` and
`release/Sign-Release.py` from the official repository. The public Ed25519 release
key fingerprint is:

```text
SHA256:wNrD0/sbgby3LJt8/5cvUYXQk3zSxoXnVZJGNaWioac
```

With Windows OpenSSH and Python 3.10 or newer available, run:

```powershell
ssh-keygen -lf .\PeCOS2k5-release.pub
python .\Sign-Release.py verify .\SHA256SUMS.txt --allowed-signers .\allowed_signers
```

The helper verifies the manifest signature with the `PeCOS2k5` identity and
`g3vr-release` namespace, then verifies every listed file's SHA-256 checksum. It
rejects changed, missing or duplicate files and paths outside the download folder.
A matching file hash alone does not authenticate its publisher: use the signed
manifest and compare the public key fingerprint through the official channel.

A signature authenticates the signed file manifest, not a promise of bug-free or
malware-free software. It is separate from Windows Authenticode signing. Previously
published unsigned tags and installers do not become signed executables retroactively.
An independently rebuilt fork can have different valid checksums and still comply
with the GPL; these instructions identify builds signed by this project's release key.

## Maintainer workflow

The private release key must stay outside Git and release archives. Keep a secure
backup. Publish only its `.pub` file and the allowed-signers record. These instructions
do not require adding an SSH authentication key or changing repository visibility.

After finalizing and checking the release assets and checksum manifest:

```powershell
python release/Sign-Release.py sign "<release folder>/SHA256SUMS.txt" --key "<private release signing key>"
```

Upload the signature alongside that exact manifest and its unchanged assets.
For a future release tag, configure this repository's signing key (not the global
Git configuration), then create and verify a signed tag:

```powershell
git config --local gpg.format ssh
git config --local user.signingkey "<private release signing key>"
git config --local gpg.ssh.allowedSignersFile "<absolute repository path>/release/keys/allowed_signers"
git tag -s vX.Y.Z -m "Gothic 3 VR X.Y.Z by PeCOS2k5"
git verify-tag vX.Y.Z
```

GitHub's Verified badge additionally requires the matching public SSH signing key
to be registered on the account. Local signature verification remains available
with the published key. Do not replace earlier release tags to add signatures.
