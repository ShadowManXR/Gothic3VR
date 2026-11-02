import argparse, importlib.util, json, struct
from pathlib import Path
spec=importlib.util.spec_from_file_location('mesh_import',Path(__file__).with_name('import_gothic_mesh.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
def reject(fn):
    try:fn()
    except (ValueError,struct.error,UnicodeError):return
    raise AssertionError('Malformed input accepted')
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=Path,required=True);args=ap.parse_args()
    count=0;vertices=triangles=0
    for path in (args.game/'Data').glob('_compiledMesh.*'):
        archive=module.Archive(path)
        for key in archive.entries:
            if 'ardea' not in key or not key.endswith('.xcmsh') or 'lod' in key:continue
            raw=archive.read(key);mesh=module.parse_mesh(raw)
            for cut in (0,1,13,len(raw)//2,len(raw)-1):reject(lambda:module.parse_mesh(raw[:cut]))
            broken=bytearray(raw);broken[0]^=255;reject(lambda:module.parse_mesh(broken))
            for s in mesh:
                assert len(s['indices'])%3==0 and max(s['indices'])<len(s['vertices'])
                assert s['material'].lower().endswith('.xshmat')
                vertices+=len(s['vertices']);triangles+=len(s['indices'])//3
            count+=1
        archive.close()
    assert count>=12
    print(json.dumps(dict(result='PASS',real_meshes=count,vertices=vertices,triangles=triangles,truncated_and_bad_signature_rejections=count*6)))
if __name__=='__main__':main()
