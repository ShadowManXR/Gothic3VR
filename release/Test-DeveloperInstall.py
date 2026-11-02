"""Regression checks for developer deployment, using inert disposable packages.

Only process enumeration is replaced in a PRIVATE COPY of Deploy.ps1, so the
tests can model an open launcher without touching or closing real applications.
Production deployment code and the live game are never modified.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PS = Path(os.environ['WINDIR']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
CORE = [
    'scripts/Script_Gothic3VRDiagnostics.dll',
    'Gothic3VRLauncher.exe', 'Gothic3VRUnionGuard.exe',
    'patches/Patch_Gothic3VRResolution.dll',
    'scripts/Script_Gothic3VRDiagnostics.ini',
    'scripts/Gothic3VRDiagnostics/openxr_loader.dll',
    'scripts/Gothic3VRDiagnostics/openvr_api.dll',
    'scripts/Gothic3VRDiagnostics/meta64/Gothic3VRMetaHost64.exe',
    'scripts/Gothic3VRDiagnostics/meta64/openxr_loader64.dll',
]


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(value if isinstance(value, bytes) else value.encode('utf-8'))


def digest(value):
    return hashlib.sha256(value).hexdigest().upper()


def snapshot(folder):
    return {p.relative_to(folder).as_posix(): p.read_bytes() for p in folder.rglob('*') if p.is_file()}


with tempfile.TemporaryDirectory(prefix='g3vr-developer-install-') as temp:
    work = Path(temp)
    project = work / 'project'
    write(project / 'tools/Install-Diagnostics.ps1', (ROOT / 'tools/Install-Diagnostics.ps1').read_bytes())
    deploy = (ROOT / 'release/Deploy.ps1').read_text(encoding='utf-8-sig')
    mock = """function Get-Process {
 param([string]$Name)
 if ($env:G3VR_TEST_BUSY -eq $Name) { [pscustomobject]@{ProcessName=$Name} }
}
"""
    marker = "$ErrorActionPreference='Stop'"
    assert deploy.count(marker) == 1
    write(project / 'release/Deploy.ps1', deploy.replace(marker, marker + '\n' + mock, 1))
    game, build, package = work / 'game', work / 'build', work / 'package'
    write(game / 'Gothic3.exe', b'inert fixture, never executed')
    write(game / 'Native.dll', b'supported-fixture')
    write(game / 'Ini/ge3.ini', '[Game]\nQuickLoot=false\n[Engine.Setup]\nEntity.ROI=4000\n')
    write(game / 'Gothic3VRLauncher.ini', '[Launcher]\nUnionExecutable=C:\\custom\\UnionStarter.exe\n')
    write(game / 'scripts/Script_Gothic3VRDiagnostics.ini', '[Stereo]\nCalibration=47\n')
    payload = {name: ('new-fixture:' + name).encode() for name in CORE}
    package.mkdir()
    with zipfile.ZipFile(package / 'G3VR.Payload.zip', 'w') as archive:
        for name, data in payload.items():
            archive.writestr(name, data)
    manifest = dict(Format=1, Version='fixture', PayloadSha=digest((package / 'G3VR.Payload.zip').read_bytes()),
                    GraphicsSha='0' * 64, Required=[dict(Path='Native.dll', Sha=digest(b'supported-fixture'), Help='Test only')],
                    Files=[dict(Path=name.replace('/', '\\'), Sha=digest(data), Component='core',
                                Policy='if-missing' if name.endswith('Diagnostics.ini') else 'replace')
                           for name, data in payload.items()])
    manifest_path = package / 'G3VR.Payload.json'

    def save_manifest():
        write(manifest_path, json.dumps(manifest))

    save_manifest()
    for name in CORE[:4]:
        write(build / 'bin/Release' / Path(name).name, payload[name])
    baseline = snapshot(game)

    def run(expected, *, include_package=True, busy=''):
        args = [str(PS), '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File',
                str(project / 'tools/Install-Diagnostics.ps1'), '-GameDirectory', str(game),
                '-BuildDirectory', str(build), '-StateDirectory', str(work / 'external-state')]
        if include_package:
            args += ['-PackageDirectory', str(package)]
        env = dict(os.environ, G3VR_TEST_BUSY=busy)
        result = subprocess.run(args, env=env, capture_output=True, timeout=30,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        output = (result.stdout + result.stderr).decode('utf-8', errors='replace')
        assert (result.returncode == 0) == expected, output
        return output

    output = run(False, include_package=False)
    assert '-PackageDirectory' in output and snapshot(game) == baseline
    print('PASS loose-file installation is rejected without changing the game')

    removed = manifest['Files'].pop()
    save_manifest()
    output = run(False)
    assert 'Incomplete developer package' in output and snapshot(game) == baseline
    manifest['Files'].append(removed)
    save_manifest()
    print('PASS missing Meta runtime is rejected before deployment')

    plugin_build = build / 'bin/Release/Script_Gothic3VRDiagnostics.dll'
    write(plugin_build, b'unrelated-build')
    output = run(False)
    assert 'does not match' in output and snapshot(game) == baseline, output
    write(plugin_build, payload[CORE[0]])
    print('PASS stale package is rejected without changing the game')

    output = run(False, busy='Gothic3VRLauncher')
    assert 'Close Gothic3VRLauncher' in output and snapshot(game) == baseline
    print('PASS running launcher is rejected before any payload write')

    run(True)
    for name in CORE:
        if name.endswith('Diagnostics.ini'):
            assert (game / name).read_bytes() == baseline[name]
        else:
            assert (game / name).read_bytes() == payload[name]
    raw = (game / 'Gothic3VRLauncher.ini').read_bytes()
    text = raw.decode('utf-16') if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else raw.decode('utf-8-sig')
    assert 'C:\\custom\\UnionStarter.exe' in text
    assert 'Entity.ROI=4000' in (game / 'Ini/ge3.ini').read_text()
    assert not list(game.rglob('install.json'))
    print('PASS complete Meta/Union deployment preserves preferences, calibration and native ROI')

    print('RESULT: 5 developer installation regression checks passed')
