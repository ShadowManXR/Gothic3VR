"""Preserve original native streams/textures for the observed material locally.

Extra stream semantics and runtime-generated lightmaps are deliberately not
guessed. Does not change the preview importer format or installed game files.
"""
import argparse
import hashlib
import json
from pathlib import Path
from import_gothic_mesh import parse_mesh
from import_gothic_materials import Assets, parse_material, ximg_dds


def export(game, capture, output):
    records = [json.loads(line) for line in (capture / 'materials.jsonl').read_text().splitlines()]
    if len(records) != 2 or {row['eye'] for row in records} != {0,1}:
        raise ValueError('Expected verified stereo material records')
    mesh_names = {Path(r['mesh']).stem + '.xcmsh' for r in records}
    materials = {r['material'] for r in records}
    if len(mesh_names) != 1 or len(materials) != 1:
        raise ValueError('Only the observed single-object material is supported')
    output.mkdir(parents=True, exist_ok=True)
    mesh_assets, material_assets, images = [Assets(game, stem) for stem in ('_compiledMesh','_compiledMaterial','_compiledImage')]
    try:
        raw, source = mesh_assets.read(next(iter(mesh_names)))
        compact = parse_mesh(raw)
        sections = parse_mesh(raw, preserve_streams=True)
        streams = []
        for i, section in enumerate(sections):
            # Preserving data must never alter the previously validated preview.
            assert {k:section[k] for k in ('material','vertices','indices')} == compact[i]
            entries = {}
            for kind, (count, data) in section['source_streams'].items():
                name = f'section-{i}-stream-{kind}.bin'
                (output / name).write_bytes(data)
                entries[kind] = dict(count=count, bytes_per_element=len(data)//count if count else 0,
                                     file=name, sha256=hashlib.sha256(data).hexdigest())
            streams.append(dict(material=section['material'], streams=entries))
        material, material_source = material_assets.read(next(iter(materials)))
        graph = parse_material(material)
        (output / 'material-graph.json').write_text(json.dumps(graph, indent=2))
        textures = {}
        for node in graph['nodes']:
            for property_name, prop in node.get('properties',{}).items():
                if prop['type'] not in ('bCImageResourceString','bCImageOrMaterialResourceString') or not prop['value']:
                    continue
                name = Path(prop['value'].replace('\\','/')).stem + '._ximg'
                # Match the exact installed XIMG basename; never guess a sampler
                # slot from graph traversal order or native pointer addresses.
                if name.lower() not in images.names and name.lower() not in images.loose:
                    name = Path(prop['value'].replace('\\','/')).stem + '.ximg'
                if name in textures:continue
                image, provenance = images.read(name)
                dds, info = ximg_dds(image)
                filename = Path(name).stem + '.dds'
                (output / filename).write_bytes(dds)
                textures[name] = dict(**info, source=provenance, output=filename,
                                     sha256=hashlib.sha256(dds).hexdigest(), sampler_slot_unproven=True)
        report = dict(mesh_source=source, material_source=material_source, sections=streams,
                      textures=textures, observed_declaration=records[0]['declaration'],
                      native_scene_material_parity=False, runtime_lightmap_data_captured=False,
                      game_files_changed=False)
        (output / 'native-inputs.json').write_text(json.dumps(report, indent=2))
        print(json.dumps(dict(sections=len(streams), native_stream_kinds=sorted({k for s in streams for k in s['streams']}),
                             texture_count=len(textures), original_preview_unchanged=True), indent=2))
    finally:
        mesh_assets.close();material_assets.close();images.close()


if __name__ == '__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--game',type=Path,required=True)
    p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    export(args.game,args.capture,args.output)
