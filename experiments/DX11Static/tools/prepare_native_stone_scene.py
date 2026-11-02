"""Prepare local real scene inputs for an independent native/SM5 comparison."""
import argparse,json,struct,hashlib
from pathlib import Path
from import_gothic_materials import Assets
from import_gothic_mesh import parse_mesh
from import_gothic_lightmap import parse_lightmap,build_lightmap_inputs


def prepare(game,inputs,profile):
    lightmap=parse_lightmap((inputs/'original-entity-lightmap.xlmp').read_bytes())
    if lightmap['kind']!=1:raise ValueError('Observed profile requires bitmap lightmap type')
    meshes=Assets(game,'_compiledMesh')
    try:raw,source=meshes.read(lightmap['mesh'])
    finally:meshes.close()
    sections=parse_mesh(raw,preserve_streams=True)
    if len(sections)!=1 or len(lightmap['sections'])!=1:raise ValueError('Only observed single-section stone profile supported')
    section=sections[0];streams=section['source_streams'];n=streams[1][0]
    additional=build_lightmap_inputs(lightmap['sections'][0],section,lightmap['scale'])
    for kind in (1,3,4,12,64):
        if kind not in streams or streams[kind][0]!=n:raise ValueError('Required native stream count mismatch')
    main=bytearray();tangent=bytearray()
    for i in range(n):
        main+=streams[1][1][i*12:(i+1)*12]+streams[3][1][i*12:(i+1)*12]+streams[12][1][i*8:(i+1)*8]
        tangent+=streams[4][1][i*4:(i+1)*4]+streams[64][1][i*12:(i+1)*12]
    count,indices=streams[0]
    geometry=b'G3NATIV1'+struct.pack('<6I',1,n,count,32,16,24)+main+tangent+additional['additional_stream']+indices
    (profile/'native-scene.bin').write_bytes(geometry)
    size=additional['atlas_size'];(profile/'lightmap-atlas.bin').write_bytes(b'G3LMAP01'+struct.pack('<2I',size,size)+additional['atlas'])
    metadata=json.loads((inputs/'native-inputs.json').read_text())
    for name,item in metadata['textures'].items():
        # The observed default graph has exactly one direct normal and diffuse
        # sampler. Explicit filename/format correspondence is kept local here;
        # this is not a generic slot inference from arbitrary graph ordering.
        if name.lower()=='g3_nature_wall_stone_01_diffuse_01.ximg':target='diffuse.dds';compression='DXT1'
        elif name.lower()=='g3_nature_wall_stone_12_normal_01.ximg':target='normal.dds';compression='DXT5'
        else:raise ValueError('Unreviewed stone texture identity')
        data=(inputs/item['output']).read_bytes()
        if hashlib.sha256(data).hexdigest()!=item['sha256'] or item['compression']!=compression:raise ValueError('Original texture changed')
        (profile/target).write_bytes(data)
    report=dict(mesh_source=source,vertices=n,triangles=count//3,main_stride=32,tangent_stride=16,additional_stride=24,
                atlas_size=size,bitmaps_total=len(lightmap['sections'][0]['bitmaps']),bitmaps_packed=len(additional['placements']),
                mapped_vertices=additional['mapped_vertices'],placements=additional['placements'],uv_groups=len(section['source_uv_groups']),
                original_native_programs_used_as_reference=True,local_atlas_placement=True,native_live_atlas_parity=False,
                native_scene_material_parity=False,shadow_textures='controlled reference fixtures')
    (profile/'scene-inputs.json').write_text(json.dumps(report,indent=2))
    print(json.dumps({k:v for k,v in report.items() if k!='placements'},indent=2))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--game',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);p.add_argument('--profile',type=Path,required=True);a=p.parse_args();prepare(a.game,a.inputs,a.profile)
