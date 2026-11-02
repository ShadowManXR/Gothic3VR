"""Exercise the actual Windows deployment helper against disposable installations."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import uuid
import zipfile
import configparser
import base64
import time

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[0]
WORK=ROOT.parent / 'release-0.23.119' / ('deployment-tests-'+uuid.uuid4().hex[:8])
WORK.mkdir(parents=True)
PS=Path(os.environ['WINDIR'])/'System32/WindowsPowerShell/v1.0/powershell.exe'
STATE=WORK/'external-state'
results=[]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()
def write(p,data):
    p.parent.mkdir(parents=True,exist_ok=True)
    p.write_bytes(data if isinstance(data,bytes) else data.encode('utf-8'))
def case(name):
    folder=WORK/name
    game=folder/'Joc Gothic ș ț'
    write(game/'Gothic3.exe',b'fixture, never executable')
    write(game/'Native.dll',b'expected-native-profile')
    write(game/'Ini/ge3.ini','[Game]\r\nQuickLoot=false\r\n[Engine.Setup]\r\nEntity.ROI=4000\r\n[Custom]\r\nKeep=yes\r\n')
    write(game/'Ini/logo.ini','G3_Logo_01.bik\r\nPublisher.bik\r\nCPT.bik\r\n')
    write(game/'splash.bmp',b'original-native-splash')
    write(game/'Data/Video/G3_Intro.bik',b'story-intro-unchanged')
    write(game/'Data/Video/G3_Credits.bik',b'credits-unchanged')
    write(game/'saves/personal.sav',b'KEEP MY SAVE')
    payload={'scripts/Script_Gothic3VRDiagnostics.dll':b'new-plugin','scripts/Script_Gothic3VRDiagnostics.ini':b'[Stereo]\r\nCreator=1\r\n','Gothic3VRLauncher.exe':b'new-launcher','Ini/logo.ini':b'','splash.bmp':b'G3 VR splash','d3d9.dll':b'known-graphics','Data/art.dds':b'new-art','Data/music.ogg':b'new-music'}
    archive=folder/'payload.zip'
    with zipfile.ZipFile(archive,'w') as z:
        for k,v in payload.items():z.writestr(k,v)
    files=[]
    for name,data in payload.items():
        comp='graphics' if name=='d3d9.dll' else 'branding' if name.endswith('.dds') else 'music' if name.endswith('.ogg') else 'core'
        files.append(dict(Path=name.replace('/','\\'),Sha=hashlib.sha256(data).hexdigest().upper(),Component=comp,Policy='if-missing' if name.endswith('Script_Gothic3VRDiagnostics.ini') else 'replace'))
    manifest=dict(Format=1,Version='test-1',PayloadSha=sha(archive),GraphicsSha=hashlib.sha256(payload['d3d9.dll']).hexdigest().upper(),Required=[dict(Path='Native.dll',Sha=sha(game/'Native.dll'),Help='Fixture prerequisite')],Files=files)
    m=folder/'manifest.json';write(m,json.dumps(manifest));return game,m,archive,manifest
def run(mode,game,m,archive,ok=True,components='core,graphics,branding,music'):
    p=subprocess.run([str(PS),'-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',str(HERE/'Deploy.ps1'),'-Mode',mode,'-GameDirectory',str(game),'-ManifestFile',str(m),'-PayloadFile',str(archive),'-StateDirectory',str(STATE),'-Components',components],capture_output=True,timeout=90,creationflags=subprocess.CREATE_NO_WINDOW)
    output=(p.stdout+p.stderr).decode('utf-8',errors='replace')
    if (p.returncode==0)!=ok:raise AssertionError(f'{mode}: exit {p.returncode}\n{output}')
    return output
def check(ok,label):
    assert ok,label
    results.append(label);print('PASS',label,flush=True)
def ini(p):
    raw=p.read_bytes();text=raw.decode('utf-16') if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else raw.decode('utf-8-sig')
    c=configparser.ConfigParser();c.read_string(text);return c
def unchanged(game):return not (game/'Gothic3VRLauncher.exe').exists()

g,m,z,d=case('rejected-native');write(g/'Native.dll',b'wrong');out=run('Check',g,m,z,False);check('Unsupported prerequisite' in out and unchanged(g),'Wrong native build is rejected before copying')
g,m,z,d=case('duplicate');write(g/'old-backup/Script_Gothic3VRDiagnostics.dll',b'old');out=run('Install',g,m,z,False);check('Duplicate resource' in out and unchanged(g),'Duplicate resource backups are rejected')
g,m,z,d=case('unknown-proxy');write(g/'d3d9.dll',b'unrecognized');out=run('Install',g,m,z,False);check('Unrecognized d3d9.dll' in out and unchanged(g),'Unknown graphics proxy is preserved and rejected')
g,m,z,d=case('unsafe-payload');d['Files'][0]['Path']='..\\escaped.dll';write(m,json.dumps(d));out=run('Install',g,m,z,False);check('Unsafe payload path' in out and not (g.parent/'escaped.dll').exists(),'Traversal cannot escape the selected game')
g,m,z,d=case('bad-archive');write(z,b'corrupt');out=run('Install',g,m,z,False);check('checksum mismatch' in out and unchanged(g),'Corrupt archive is rejected before copying')
g,m,z,d=case('fresh');run('Check',g,m,z);check(unchanged(g),'Preflight is read-only for the game')
run('Install',g,m,z,components='core')
check((g/'scripts/Script_Gothic3VRDiagnostics.ini').read_bytes().startswith(b'[Stereo]'),'Fresh install gets the frozen preset')
check(not (g/'d3d9.dll').exists() and not (g/'Data/art.dds').exists(),'Unselected optional components are not installed')
check((g/'splash.bmp').read_bytes()==b'G3 VR splash','Core-only install includes the G3 VR startup splash')
check((g/'Ini/logo.ini').read_bytes()==b'' and (g/'Data/Video/G3_Intro.bik').read_bytes()==b'story-intro-unchanged' and (g/'Data/Video/G3_Credits.bik').read_bytes()==b'credits-unchanged','Empty startup playlist skips advertising without changing story or credits movies')
check(ini(g/'Gothic3VRLauncher.ini')['Launcher']['GameExecutable']==str(g/'Gothic3.exe'),'Unicode and spaces in the selected game path survive')
check(ini(g/'Ini/ge3.ini')['Game']['QuickLoot']=='true' and ini(g/'Ini/ge3.ini')['Engine.Setup']['Entity.ROI']=='4000' and ini(g/'Ini/ge3.ini')['Custom']['Keep']=='yes','Only the native Quick Loot key changes; distances and unrelated keys survive')
write(g/'scripts/Script_Gothic3VRDiagnostics.ini','[Stereo]\r\nCreator=0\r\nMyCalibration=47\r\n')
write(g/'Gothic3VRLauncher.ini','[Launcher]\r\nGameExecutable=old\r\nUnionExecutable=C:\\my union\\UnionStarter.exe\r\n')
run('Install',g,m,z)
check(ini(g/'scripts/Script_Gothic3VRDiagnostics.ini')['Stereo']['MyCalibration']=='47' and ini(g/'Gothic3VRLauncher.ini')['Launcher']['UnionExecutable']==r'C:\my union\UnionStarter.exe','Repair/upgrade preserves calibration and custom launcher preferences')
write(g/'scripts/Script_QuickLoot.dll',b'provider');run('Install',g,m,z)
check(ini(g/'Ini/ge3.ini')['Game']['QuickLoot']=='false','Separate Quick Loot provider prevents duplicate native Quick Loot')
run('Uninstall',g,m,z)
check(not (g/'scripts/Script_Gothic3VRDiagnostics.dll').exists() and (g/'scripts/Script_Gothic3VRDiagnostics.ini').exists() and (g/'saves/personal.sav').read_bytes()==b'KEEP MY SAVE','Uninstall removes new binaries and keeps configuration, saves and unrelated mods')
check(ini(g/'Ini/ge3.ini')['Game']['QuickLoot']=='false','Uninstall restores the pre-install native Quick Loot value')
check((g/'Ini/logo.ini').read_bytes()==b'G3_Logo_01.bik\r\nPublisher.bik\r\nCPT.bik\r\n','Uninstall restores the startup playlist from before the first install, including after repair')
check((g/'splash.bmp').read_bytes()==b'original-native-splash','Uninstall restores the original startup splash')

g,m,z,d=case('restore-and-modified');write(g/'scripts/Script_Gothic3VRDiagnostics.dll',b'previous-plugin');write(g/'Data/art.dds',b'previous-art');write(g/'scripts/Script_Gothic3VRDiagnostics.ini','[Stereo]\r\nExisting=1\r\n')
before=(g/'scripts/Script_Gothic3VRDiagnostics.ini').read_bytes();run('Install',g,m,z)
check((g/'scripts/Script_Gothic3VRDiagnostics.ini').read_bytes()==before,'Existing settings remain byte-identical on first managed installation')
write(g/'Data/music.ogg',b'user-edited-music');write(g/'Ini/logo.ini','custom-story-startup.bik\r\n');run('Uninstall',g,m,z)
check((g/'scripts/Script_Gothic3VRDiagnostics.dll').read_bytes()==b'previous-plugin' and (g/'Data/art.dds').read_bytes()==b'previous-art' and (g/'Data/music.ogg').read_bytes()==b'user-edited-music','Uninstall restores originals and retains files edited after installation')
check((g/'Ini/logo.ini').read_bytes()==b'custom-story-startup.bik\r\n','Uninstall retains a startup playlist edited by the user after installation')

# Force an error late in the transaction: a read-shared native INI can be backed
# up but not written. A real Windows file lock is released before repair.
g,m,z,d=case('rollback');write(g/'scripts/Script_Gothic3VRDiagnostics.dll',b'before-failure')
signal=g.parent/'hold.lock';ready=g.parent/'ready';write(signal,b'hold')
command="$f=[IO.File]::Open('%s',[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::Read);[IO.File]::WriteAllText('%s','ready');try{while([IO.File]::Exists('%s')){Start-Sleep -Milliseconds 100}}finally{$f.Dispose()}" % (str(g/'Ini/ge3.ini').replace("'","''"),str(ready).replace("'","''"),str(signal).replace("'","''"))
holder=subprocess.Popen([str(PS),'-NoProfile','-NonInteractive','-EncodedCommand',base64.b64encode(command.encode('utf-16-le')).decode()],creationflags=subprocess.CREATE_NO_WINDOW)
try:
    for i in range(100):
        if ready.exists():break
        time.sleep(.05)
    assert ready.exists(),'File-lock fixture did not start'
    out=run('Install',g,m,z,False)
finally:
    signal.unlink(missing_ok=True);holder.wait(timeout=20)
check((g/'scripts/Script_Gothic3VRDiagnostics.dll').read_bytes()==b'before-failure' and not (g/'Gothic3VRLauncher.exe').exists(),'A late write failure rolls back payload binaries instead of leaving a partial install')
run('Install',g,m,z);run('Uninstall',g,m,z)
check((g/'scripts/Script_Gothic3VRDiagnostics.dll').read_bytes()==b'before-failure','Repair recovers a pending transaction and preserves the true pre-install file')

g,m,z,d=case('missing-native-key');write(g/'Ini/ge3.ini','[Game]\r\nOther=yes\r\n');run('Install',g,m,z);run('Uninstall',g,m,z)
check('QuickLoot' not in ini(g/'Ini/ge3.ini')['Game'] and ini(g/'Ini/ge3.ini')['Game']['Other']=='yes','Uninstall deletes a previously absent INI key without leaving an empty override')

g,m,z,d=case('damaged-original');write(g/'Data/art.dds',b'original-art');run('Install',g,m,z)
key=hashlib.sha256(str(g).lower().encode()).hexdigest().upper()[:24]
journal=json.loads((STATE/key/'install.json').read_text())
art=next(x for x in journal['Files'] if x['Path']=='Data\\art.dds');write(Path(art['Original']['Saved']),b'damaged')
out=run('CheckUninstall',g,m,z,False)
check('backup is damaged' in out and (g/'Gothic3VRLauncher.exe').exists(),'Uninstall validates original backups before deleting any binaries')
check(not any('rollback' in p.parts or p.name=='install.json' for p in g.rglob('*')),'Rollback snapshots and journals remain outside the engine resource tree')
(WORK/'results.json').write_text(json.dumps(dict(passed=len(results),checks=results),indent=2))
print('RESULT',len(results),'checks passed;',WORK)
