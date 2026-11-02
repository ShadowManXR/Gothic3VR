"""Convert bounded live matrices to an offline replay; no game process needed."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct


def prepare(log, assets, output):
    if log.stat().st_size > 2 * 1024 * 1024:
        raise ValueError('Excessive trial log')
    rows = [json.loads(line) for line in log.read_text(encoding='utf-8-sig').splitlines()]
    ready = [r for r in rows if r.get('type') == 'dx11_static_trial_ready']
    if len(ready) != 1:
        raise ValueError('Missing/ambiguous trial dimensions')
    width, height = ready[0]['width'], ready[0]['height']
    if not (64 <= width <= 8192 and 64 <= height <= 8192):
        raise ValueError('Invalid dimensions')
    records = [r for r in rows if r.get('type') == 'dx11_static_trial_eye']
    if not 2 <= len(records) <= 64:
        raise ValueError('Invalid record count')
    manifest = json.loads((assets / 'manifest.json').read_text())
    known = {a['output']: a for a in manifest['assets']}
    payload = bytearray(struct.pack('<8s4I', b'G3DXRP1\0', 1, width, height, len(records)))
    seen, sources = set(), {}
    previews = json.loads((assets / 'materials/preview-manifest.json').read_text())
    materials = {m['name']: m for m in previews['materials']}
    material_audit = []
    for r in records:
        key = (r['frame'], r['eye'])
        resource = r['resource']
        if key in seen or r['eye'] not in (0, 1) or r['frame'] <= 0 or r['epoch'] <= 0:
            raise ValueError('Duplicate/invalid eye ownership')
        seen.add(key)
        if Path(resource).name != resource or '/' in resource or '\\' in resource or ':' in resource or resource not in known:
            raise ValueError('Unknown/nonlocal replay resource')
        mesh = assets / resource
        digest = hashlib.sha256(mesh.read_bytes()).hexdigest()
        if digest != known[resource]['output_sha256']:
            raise ValueError('Mesh content changed since import')
        # Reference raster is opaque; masked materials need their native texture
        # alpha in the D3D9 reference before they can be admitted to this probe.
        for name in known[resource]['materials']:
            if name not in materials or materials[name]['blend'] != 0:
                raise ValueError('Reference requires resolved opaque materials')
        if resource not in sources:
            sources[resource] = digest
            material_audit.extend(dict(name=name, **{k: materials[name][k] for k in ('image', 'blend', 'unsupported', 'replacement_exact')}) for name in known[resource]['materials'])
        matrices = []
        for name in ('world_m', 'native_view_cm', 'native_projection_cm'):
            matrix = r[name]
            if len(matrix) != 16 or not all(isinstance(v, (int, float)) and math.isfinite(v) for v in matrix):
                raise ValueError('Invalid recorded transform')
            matrices.extend(matrix)
        encoded = resource.encode('ascii')
        if len(encoded) > 1023:
            raise ValueError('Excessive resource name')
        payload.extend(struct.pack('<QQII', r['frame'], r['epoch'], r['eye'], len(encoded)))
        payload.extend(encoded)
        payload.extend(struct.pack('<48f', *matrices))
    if {r['eye'] for r in records} != {0, 1}:
        raise ValueError('Both eyes required')
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    report = dict(source=str(log), source_sha256=hashlib.sha256(log.read_bytes()).hexdigest(), width=width, height=height,
                  records=len(records), meshes=sources, materials=material_audit, native_material_parity=False,
                  game_loaded=False)
    output.with_suffix('.json').write_text(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--log', type=Path, required=True)
    p.add_argument('--assets', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    report = prepare(args.log, args.assets, args.output)
    print(f"Prepared {report['records']} recorded eye passes at {report['width']}x{report['height']}; no game process.")
