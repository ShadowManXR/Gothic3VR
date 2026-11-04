"""Build a closed, hash-pinned payload. Does not install or alter a live game."""
import argparse
import hashlib
import html
import json
from pathlib import Path
import shutil
import zipfile
import re

def markdown_html(text):
    """Small offline renderer for this guide's headings, lists, links and tables."""
    def inline(s):
        s=html.escape(s)
        s=re.sub(r'`([^`]+)`',r'<code>\1</code>',s)
        s=re.sub(r'\*\*([^*]+)\*\*',r'<strong>\1</strong>',s)
        s=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',r'<a href="\2">\1</a>',s)
        return s
    blocks=[]; lines=text.splitlines(); i=0
    while i<len(lines):
        line=lines[i];i+=1
        if not line.strip(): continue
        if line.startswith('#'):
            level=len(line)-len(line.lstrip('#'));blocks.append(f'<h{level}>'+inline(line[level:].strip())+f'</h{level}>')
        elif line.startswith('|'):
            rows=[line]
            while i<len(lines) and lines[i].startswith('|'): rows.append(lines[i]);i+=1
            body='<table>'
            for index,row in enumerate(rows):
                if index==1: continue
                tag='th' if index==0 else 'td';body+='<tr>'+''.join(f'<{tag}>'+inline(c.strip())+f'</{tag}>' for c in row.strip('|').split('|'))+'</tr>'
            blocks.append(body+'</table>')
        elif line.startswith('- ') or re.match(r'^\d+\. ',line):
            ordered=not line.startswith('- ');tag='ol' if ordered else 'ul';items=[line]
            while i<len(lines) and ((bool(re.match(r'^\d+\. ',lines[i])) if ordered else lines[i].startswith('- '))):items.append(lines[i]);i+=1
            blocks.append(f'<{tag}>'+''.join('<li>'+inline(re.sub(r'^(?:- |\d+\. )','',x))+'</li>' for x in items)+f'</{tag}>')
        else:
            paragraph=[line]
            while i<len(lines) and lines[i].strip() and not lines[i].startswith(('#','|','- ')) and not re.match(r'^\d+\. ',lines[i]):paragraph.append(lines[i]);i+=1
            blocks.append('<p>'+inline(' '.join(paragraph))+'</p>')
    return '\n'.join(blocks)

VERSION = '1.0.6'
ROOT = Path(__file__).resolve().parents[1]

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()

def build(args):
    game, build_dir, out = args.game.resolve(), args.build.resolve(), args.output.resolve()
    assert not out.is_relative_to(game), 'Keep the distribution outside the game resource tree'
    cache = (build_dir / 'CMakeCache.txt').read_text()
    for flag in ('RUNTIME_PROBES', 'DX11_VALIDATION', 'RENDERER_FEASIBILITY', 'SCENE_COST_TRIAL', 'DRAW_COST_TRIAL', 'ENTITY_COST_TRIAL', 'STEREO_FRAME_CAPTURE', 'UI_COMPOSITION_CAPTURE', 'TEST_NATIVE_NVIDIA_TEMPORAL'):
        assert f'G3VR_{flag}:BOOL=OFF' in cache, f'Development mode enabled: {flag}'
    project = (ROOT / 'CMakeLists.txt').read_text(encoding='utf-8')
    assert f'project(Gothic3VRDiagnostics VERSION {VERSION} ' in project, 'Package/source version mismatch'
    assert not any(out.iterdir()) if out.exists() else True, 'Use a new empty output directory'
    out.mkdir(parents=True, exist_ok=True)
    files = []
    def add(source, dest, component='core', policy='replace', pin=None):
        source = Path(source)
        digest = sha(source)
        assert pin is None or digest == pin.upper(), f'Unvalidated source: {source}'
        files.append(dict(Path=dest.replace('/', '\\'), Sha=digest, Component=component, Policy=policy, source=source))
    binaries = build_dir / 'bin/Release'
    add(binaries / 'Gothic3VRLauncher.exe', 'Gothic3VRLauncher.exe')
    add(binaries / 'Script_Gothic3VRDiagnostics.dll', 'scripts/Script_Gothic3VRDiagnostics.dll')
    add(ROOT / 'release/CreatorDefaults.ini', 'scripts/Script_Gothic3VRDiagnostics.ini', policy='if-missing')
    # The native startup playlist contains advertising logos only. An empty
    # playlist skips startup branding without altering the actual Bink movies.
    # Deployment journals the original file for rollback and uninstall.
    add(ROOT / 'assets/startup/Ini/logo.ini', 'Ini/logo.ini')
    # All locally built components must match the selected build, as enforced
    # again by Install-Diagnostics. Accepted external runtimes stay pinned below.
    add(binaries / 'Gothic3VRUnionGuard.exe', 'Gothic3VRUnionGuard.exe')
    add(binaries / 'Patch_Gothic3VRResolution.dll', 'patches/Patch_Gothic3VRResolution.dll')
    runtime = 'scripts/Gothic3VRDiagnostics/'
    for name in ('openxr_loader.dll', 'OPENXR-LICENSE.txt', 'openvr_api.dll', 'OPENVR-LICENSE.txt', 'UPSCALER-LICENSES.txt', 'FXAA-LICENSE.txt'):
        add(game / runtime / name, runtime + name)
    add(ROOT / 'third_party/upscalers/smaa/LICENSE.txt', runtime + 'SMAA-LICENSE.txt')
    host = runtime + 'meta64/'
    add(game / host / 'Gothic3VRMetaHost64.exe', host + 'Gothic3VRMetaHost64.exe', pin='16B6988E81760944A57BE8C9F7092456CF183887AD5B0004A1B912A8A83F11B9')
    add(game / host / 'openxr_loader64.dll', host + 'openxr_loader64.dll')
    add(game / host / 'OPENXR-META64-LICENSE.txt', host + 'OPENXR-META64-LICENSE.txt')
    graphics_sha = '5545BDCEC6923418E5280E7D7DE9541D04D9BAEB1CFA23238F6F0277802E18D5'
    add(ROOT / 'third_party/dxvk-ffr/x32/d3d9.dll', 'd3d9.dll', 'graphics', pin=graphics_sha)
    add(ROOT / 'third_party/dxvk-ffr/LICENSE', runtime + 'DXVK-FFR-LICENSE.txt', 'graphics')
    add(ROOT / 'third_party/dxvk-ffr/UPSTREAM.txt', runtime + 'DXVK-FFR-SOURCE.txt', 'graphics')
    for patch in ('g3vr-ffr-v1.patch', 'g3vr-d3d9-stable-interop.patch'):
        add(ROOT / 'third_party/dxvk-ffr' / patch, runtime + patch, 'graphics')
    add(ROOT / 'assets/menu-branding/Data/gui/icons/compiled/g3_hud_backgrounds_02.dds', 'Data/gui/icons/compiled/g3_hud_backgrounds_02.dds', 'branding')
    add(ROOT / 'assets/startup-splash/splash.bmp', 'splash.bmp')
    add(ROOT / 'assets/menu-music/Data/Music/fragments/myrtana_title_day_01.ogg', 'Data/Music/fragments/myrtana_title_day_01.ogg', 'music')
    assert len({Path(f['Path']).name.lower() for f in files}) == len(files), 'Duplicate resource basename'
    archive = out / 'G3VR.Payload.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for f in files:
            z.write(f['source'], f['Path'].replace('\\', '/'))
    required = {
        'Engine.dll': '991CF674A9CF1BC60CDDC204BE0993E8251336F4D3471543A37846CC6840BF5A',
        'Game.dll': '4728A47B244ADCB0EA2EC3501DB04994909DCFCFF338C7B8075F2B68DD8D6E00',
        'scripts/Script_Game.dll': '4D29189281EC26EAC7C704FA67ED323DF5C090D50B068626CF600AECF4262A24',
        'SharedBase.dll': '3087BB4E6C8C7F3BFCF87F79BA889A8C9DF223CB71D977963E97DFB51206D4A4',
        'Script.dll': '03C483148D0EA88E007D2F18EAB30400A6012439EBBE4FEA6EBF837AA2ABD981',
        'GUI.dll': 'BFAD654C722F34D5E2BD5A7D0705AC5E3B67BD5EAE2F7E3470F2A8A8F28D221E',
        'GFC.dll': '831E08580F16F7A94AA76EAD834DBD2505C81B226323929FB91806F30D376B30',
        'scripts/Script_paru.dll': '26E0A499B7E2A3D9A7F53D72D2EE1E3C0253A7A4D71ADF648525C4B3CDA2CE9C',
        'paru_s1.dll': 'D8A245FFE9DB1E639DC038842B1835B060AF366AB4BA543132D00E5550DC2F9A',
        'paru_s1_opt.dll': '9334D3E331DF563EED0D3B51148A1BE1B9F20EFA9159FE543857F3051FB5682F',
        'paru_s2.dll': '97760FFCD3EEF09B7DBCD8EF925B9995A198CB8BBA96951298969C48A84CD8F0',
    }
    prereqs = []
    for name, digest in required.items():
        assert sha(game / name) == digest, 'Reference profile changed: ' + name
        help_text = ('Install Parallel Universe Patch 1.1.1 after the native updates.' if 'paru' in name else
                     'Install Community Patch 1.75.14, Update Pack 1.04.11, then the complete 1.05.10 Public Beta archive. See START-HERE.html.')
        prereqs.append(dict(Path=name.replace('/', '\\'), Sha=digest, Help=help_text))
    packages = json.loads((ROOT / 'release/Prerequisites.json').read_text(encoding='utf-8'))['Packages']
    assert len(packages) == 4 and [p['Order'] for p in packages] == [1, 2, 3, 4]
    manifest = dict(Format=1, Version=VERSION, PayloadSha=sha(archive), GraphicsSha=graphics_sha, PrerequisitePackages=packages,
                    Required=prereqs, Files=[{k:v for k,v in f.items() if k!='source'} for f in files])
    (out / 'G3VR.Payload.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    for name in ('Deploy.ps1', 'START-HERE.md', 'RELEASE-NOTES.md', 'CreatorDefaults.ini', 'CreatorDefaults.json', 'Prerequisites.json', 'CONTROLLER-MAPPING.md'):
        shutil.copy2(ROOT / 'release' / name, out / name)
    for name in ('OPTIONAL-MODS-UNION.md', 'VR-MOD-COMPATIBILITY.md', 'CONTROLLER-COMPATIBILITY.md', 'ITEM-LOCATOR.md', 'INI-TWEAKS.md'):
        shutil.copy2(ROOT / 'docs' / name, out / name)
    # Local, self-contained HTML; no remote scripts, fonts, tracking or assets.
    md = (out / 'START-HERE.md').read_text(encoding='utf-8')
    body = markdown_html(md)
    page = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Gothic 3 VR — Start here</title><style>
    body{background:#11171c;color:#ddd7cc;font:17px/1.65 system-ui,sans-serif;margin:0}main{max-width:940px;margin:0 auto;padding:48px 28px 90px}h1,h2{color:#e4ba6d;line-height:1.2}h1{font-size:38px}h2{margin-top:48px;border-top:1px solid #394047;padding-top:28px}a{color:#e5bc79}code{background:#242c33;padding:2px 5px;overflow-wrap:anywhere}table{border-collapse:collapse;width:100%;font-size:15px}td,th{border-bottom:1px solid #3d4449;padding:10px;text-align:left}li{margin:9px 0}@media print{body{background:white;color:black}h1,h2,a{color:#523b17}main{padding:0}h2{break-after:avoid}}</style><main>''' + body + '</main></html>'
    (out / 'START-HERE.html').write_text(page, encoding='utf-8')
    (out / 'BEFORE-INSTALL.txt').write_text('''GOTHIC 3 VR — BEFORE YOU INSTALL

Required: Gothic 3 (not Forsaken Gods), Windows 10/11 x64, SSD, a PC VR headset and two tracked controllers.

Install all FOUR native patch packages first, in this order:
1. Community Patch 1.75.14
   Gothic_3_EE_Patch_v1.75.14_Int_Full.exe
2. Update Pack 1.04.11 (EXE installer)
   Gothic_3_EE_v1.75_Int_Update_Pack_v1.04.11.exe
3. Update Pack 1.05.10 Public Beta (extract the entire ZIP over 1.04.11)
   3. Update Pack v1.05.10 Public Beta.zip
4. Parallel Universe Patch 1.1.1
   Gothic_3_Parallel_Universe_Patch_v1.1.1.exe

The two Update Packs are separate required downloads. Installing only the Public Beta does not replace the initial 1.04.11 step. Reference download hashes are in Prerequisites.json.

Start and exit the flat game before installing VR. Read START-HERE.html for download links and setup instructions. Native patches, Microsoft runtimes and VR providers are separate downloads.

Close Gothic 3, the VR launcher and Union+ before continuing. Select the folder containing Gothic3.exe. Setup verifies the exact supported native DLLs and refuses unknown graphics proxies.

New installs and Defaults use the creator preset. Existing settings survive updates. Choose your VR provider in Camera & controls before first launch; the initial preset uses Virtual Desktop / VDXR. The optional FFR backend is intended for the tested NVIDIA path.

Startup advertising logos are skipped automatically using the native logo.ini playlist. Story cinematics and video files are retained. The original playlist is backed up outside the game folder and restored on uninstall if unchanged.

Backups stay outside the game folder. Uninstall restores or removes unchanged owned files; user settings, saves and changed files are retained.

Gothic 3 VR 1.0 — public release package.
''', encoding='utf-8-sig')
    licenses = out / 'licenses'; licenses.mkdir(exist_ok=True)
    for src, dst in ((args.sdk / 'LICENSE','GOTHIC3SDK-LICENSE.txt'), (args.sdk / 'COPYING','GPL-3.0.txt'),
                     (ROOT / 'third_party/vulkan/LICENSE.md','VULKAN-LICENSE.md')):
        shutil.copy2(src, licenses / dst)
    for f in files:
        if 'LICENSE' in f['Path'].upper(): shutil.copy2(f['source'], licenses / Path(f['Path']).name)
    (out / 'THIRD-PARTY-NOTICES.txt').write_text(f'''Gothic 3 VR {VERSION} — third-party notices

Gothic 3 and its artwork remain the property of their respective owners. This is an unofficial VR modification. A separately purchased game is required.

Gothic 3 SDK: Copyright (C) 2013-2025 Georgeto, GNU GPL version 3 or later. The VR plugin uses these bindings. The accompanying source archive contains the project, bindings, build scripts and applicable notices. See licenses/GPL-3.0.txt and licenses/GOTHIC3SDK-LICENSE.txt.

OpenXR loader/headers: Khronos, Apache-2.0; OpenVR: Valve, BSD license; Vulkan headers: respective Khronos contributors. Their notices accompany the runtime/source.

NVIDIA NIS / FXAA, AMD FidelityFX CAS, SMAA (iryoku contributors): original notices in licenses and source/third_party/upscalers. These are spatial filters; no NVIDIA DLSS/NGX runtime is bundled.

Optional DXVK backend: Philip Rebohle and contributors, zlib license. This binary is modified for Gothic 3 VR FFR and stable interop. Source tag v3.1.1, commit b1a1c99ab52b687cf950d62c88bc2fa316b41663; https://github.com/doitsujin/dxvk. The two local patches and exact upstream/submodule references accompany the source and optional payload.

Installer built with Inno Setup (https://jrsoftware.org/). The Inno Setup compiler is not included in this distribution.

Main-menu music and custom visual assets are creator-supplied. Third-party content packs, Community Patch, Update Pack, Parallel Universe Patch, Union+, EDD and commercial VR providers are not bundled. Their authors retain their rights. Free download availability alone does not grant redistribution permission.
''', encoding='utf-8')
    # Corresponding source is a separate deliverable, never a folder under the game.
    source_zip = out / f'Gothic3VR-{VERSION}-Source.zip'
    with zipfile.ZipFile(source_zip, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        roots = ['src','tools','tests','release','assets','third_party','docs',
                 # These sources are referenced unconditionally by the root
                 # CMake project, despite their historical directory name.
                 'experiments/DX11Static/src', 'experiments/DX11Static/include',
                 'experiments/DX11Static/shaders']
        for folder in roots:
            for p in (ROOT / folder).rglob('*'):
                if p.is_file() and not any(x in p.parts for x in ('.git','__pycache__','node_modules')):
                    z.write(p, 'gothic3vr/' + p.relative_to(ROOT).as_posix())
        z.write(ROOT / 'CMakeLists.txt', 'gothic3vr/CMakeLists.txt')
        for name in ('README.md', 'LICENSE', 'THIRD-PARTY-NOTICES.md', '.gitignore'):
            z.write(ROOT / name, 'gothic3vr/' + name)
        z.write(ROOT / 'Script_Gothic3VRDiagnostics.ini', 'gothic3vr/Script_Gothic3VRDiagnostics.ini')
        for p in args.sdk.rglob('*'):
            if p.is_file() and not any(x in p.parts for x in ('.git','build','out')):
                z.write(p, 'gothic3sdk/' + p.relative_to(args.sdk).as_posix())
        z.writestr('BUILD.txt', f'''Gothic 3 VR {VERSION} source
Windows x64 build machine, Visual Studio 2022 Build Tools (C++ x86/x64, Windows SDK with fxc), CMake >=3.24.
cmake -S gothic3vr -B build -A Win32 -DGOTHIC3SDK_DIR=../gothic3sdk -DG3VR_RUNTIME_PROBES=OFF -DG3VR_DX11_VALIDATION=OFF -DG3VR_RENDERER_FEASIBILITY=OFF -DG3VR_SCENE_COST_TRIAL=OFF -DG3VR_DRAW_COST_TRIAL=OFF -DG3VR_ENTITY_COST_TRIAL=OFF -DG3VR_STEREO_FRAME_CAPTURE=OFF -DG3VR_UI_COMPOSITION_CAPTURE=OFF -DG3VR_TEST_NATIVE_NVIDIA_TEMPORAL=OFF
cmake --build build --config Release --target Script_Gothic3VRDiagnostics Gothic3VRLauncher Gothic3VRUnionGuard -- /m:1 /nr:false
The separate Meta host uses: cmake -S gothic3vr/tools/MetaBridgeHost -B build-meta -A x64; cmake --build build-meta --config Release --target Gothic3VRMetaHost64. OpenXR runtime loaders retain their upstream licenses. The optional DXVK binary is built from the upstream revision and patches recorded in third_party/dxvk-ffr/UPSTREAM.txt.
The release packaging tool deliberately requires a supported local native installation to validate prerequisite hashes and copy the accepted runtime/startup components. It never includes the game's native patch binaries.
''')
    (out / 'BUILD-RECEIPT.json').write_text(json.dumps(dict(Version=VERSION, PresetSha=sha(ROOT / 'release/CreatorDefaults.ini'),
        RuntimeProbes=False, Files=manifest['Files'], SourceSha=sha(source_zip)), indent=2), encoding='utf-8')
    print(json.dumps(dict(output=str(out), payload_files=len(files), payload_sha=sha(archive), source_bytes=source_zip.stat().st_size)))

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('--game',type=Path,required=True);p.add_argument('--build',type=Path,required=True);p.add_argument('--sdk',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    build(p.parse_args())
