"""Exercise real release payloads in private game fixtures. Never run the game or Setup registration."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import uuid
import zipfile


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--reference-game', type=Path, required=True)
    parser.add_argument('--previous-package', type=Path, required=True)
    args = parser.parse_args()
    package, live, previous = args.package.resolve(), args.reference_game.resolve(), args.previous_package.resolve()
    manifest = json.loads((package / 'G3VR.Payload.json').read_text(encoding='utf-8'))
    old = json.loads((previous / 'G3VR.Payload.json').read_text(encoding='utf-8'))
    work = package.parents[1] / 'work' / ('release-' + manifest['Version']) / ('actual-payload-' + uuid.uuid4().hex[:8])
    state = work / 'external-state'
    work.mkdir(parents=True)
    checks = []

    def check(condition, label):
        assert condition, label
        checks.append(label)
        print('PASS', label, flush=True)

    with zipfile.ZipFile(package / 'G3VR.Payload.zip') as archive:
        check(sha(package / 'G3VR.Payload.zip') == manifest['PayloadSha'], 'Payload archive checksum')
        check(set(archive.namelist()) == {f['Path'].replace('\\', '/') for f in manifest['Files']}, 'Archive has only allowlisted release files')
        for entry in manifest['Files']:
            data = archive.read(entry['Path'].replace('\\', '/'))
            assert hashlib.sha256(data).hexdigest().upper() == entry['Sha'], entry['Path']
        check(not any(Path(n).suffix.lower() in ('.pdb', '.jsonl', '.log') or 'capture.request' in n for n in archive.namelist()), 'No development captures, logs or PDBs in game payload')
        check(archive.read('scripts/Script_Gothic3VRDiagnostics.ini') == (package / 'CreatorDefaults.ini').read_bytes(), 'Fresh-install creator preset is the frozen approved file')
    with zipfile.ZipFile(package / ('Gothic3VR-' + manifest['Version'] + '-Source.zip')) as archive:
        check('gothic3vr/release/Test-Package.py' in archive.namelist() and 'gothic3vr/docs/ITEM-LOCATOR.md' in archive.namelist(), 'Corresponding source includes packaging tests and current documentation')

    def fixture(name):
        game = work / name / 'Gothic 3 ș ț'
        game.mkdir(parents=True)
        (game / 'Gothic3.exe').write_text('Private non-executable test fixture')
        for entry in manifest['Required']:
            source, dest = live / entry['Path'], game / entry['Path']
            assert sha(source) == entry['Sha']
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, dest)
        (game / 'Ini').mkdir(exist_ok=True)
        (game / 'Ini/ge3.ini').write_text('[Game]\nQuickLoot=false\n[Engine.Setup]\nEntity.ROI=4000\n[Custom]\nKeep=unchanged\n')
        (game / 'Ini/logo.ini').write_bytes(b'Publisher.bik\r\nCPT.bik\r\n')
        (game / 'Data/Video').mkdir(parents=True)
        (game / 'Data/Video/G3_Intro.bik').write_bytes(b'UNCHANGED-STORY')
        (game / 'saves').mkdir()
        (game / 'saves/personal.sav').write_bytes(b'KEEP SAVE')
        return game

    def run(mode, game, source=package, components='core,graphics,branding,music'):
        command = [str(Path(os.environ['WINDIR']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'),
                   '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File', str(package / 'Deploy.ps1'),
                   '-Mode', mode, '-GameDirectory', str(game), '-StateDirectory', str(state),
                   '-ManifestFile', str(source / 'G3VR.Payload.json'), '-PayloadFile', str(source / 'G3VR.Payload.zip'),
                   '-Components', components]
        result = subprocess.run(command, capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW, timeout=90)
        assert result.returncode == 0, (mode, result.stdout.decode(errors='replace'), result.stderr.decode(errors='replace'))

    game = fixture('fresh')
    run('Check', game)
    check(not (game / 'Gothic3VRLauncher.exe').exists(), 'Real-payload preflight does not deploy files')
    run('Install', game, components='core')
    check(not (game / 'd3d9.dll').exists() and not (game / 'Data/Music').exists(), 'Core-only install omits optional graphics and music')
    check((game / 'scripts/Script_Gothic3VRDiagnostics.ini').read_bytes() == (package / 'CreatorDefaults.ini').read_bytes(), 'Real fresh install uses creator settings')
    run('Install', game)
    check(all(sha(game / f['Path']) == f['Sha'] for f in manifest['Files']), 'All selected real files match manifest after component upgrade')
    check((game / 'Ini/logo.ini').read_bytes() == b'', 'Startup advertisements are skipped')
    run('Uninstall', game)
    check(all(not (game / f['Path']).exists() for f in manifest['Files'] if f['Policy'] != 'if-missing' and f['Path'] != 'Ini\\logo.ini'), 'Real payload binaries and optional assets removed on uninstall')
    check((game / 'Ini/logo.ini').read_bytes() == b'Publisher.bik\r\nCPT.bik\r\n', 'Original startup playlist restored')
    check((game / 'saves/personal.sav').read_bytes() == b'KEEP SAVE' and (game / 'Data/Video/G3_Intro.bik').read_bytes() == b'UNCHANGED-STORY', 'Saves and story cinematics survive install/uninstall')

    game = fixture('upgrade-from-' + old['Version'])
    run('Install', game, source=previous)
    settings = game / 'scripts/Script_Gothic3VRDiagnostics.ini'
    settings.write_bytes(b'[Stereo]\r\nFirstPersonHeightOffsetCm=13\r\nFfrMode=0\r\nOpenXrRuntime=2\r\n[Controllers]\r\nInteractionAimUpPercent=-41\r\n')
    saved = settings.read_bytes()
    launcher = game / 'Gothic3VRLauncher.ini'
    launcher.write_text('[Launcher]\nUnionExecutable=E:\\Personal Union\\UnionStarter.exe\n', encoding='utf-16')
    run('Install', game)
    check(settings.read_bytes() == saved, 'Upgrade from previous released payload preserves user calibration byte-for-byte')
    c = configparser.ConfigParser()
    c.read(launcher, encoding='utf-16')
    check(c['Launcher']['UnionExecutable'] == r'E:\Personal Union\UnionStarter.exe', 'Upgrade preserves custom Union+ location')
    check(all(sha(game / f['Path']) == f['Sha'] for f in manifest['Files'] if f['Policy'] != 'if-missing'), 'Upgrade installs current binaries and assets')
    for f in manifest['Required']:
        assert sha(game / f['Path']) == f['Sha']
    run('Uninstall', game)
    check(settings.read_bytes() == saved and (game / 'saves/personal.sav').read_bytes() == b'KEEP SAVE', 'Uninstall after upgrade preserves user configuration and saves')
    check(all(sha(game / f['Path']) == f['Sha'] for f in manifest['Required']), 'Native patch modules remain byte-identical')
    check(not any(p.name == 'install.json' or p.name == 'rollback' for p in game.rglob('*')), 'Backups and journals stay outside game resources')

    result = dict(Version=manifest['Version'], PreviousVersion=old['Version'], Passed=len(checks), Checks=checks,
                  PayloadSha=manifest['PayloadSha'], GameLaunched=False, LiveGameChanged=False,
                  SetupRegistrationChanged=False, Work=str(work))
    (work / 'results.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print('RESULT', work / 'results.json')


if __name__ == '__main__':
    main()
