from pathlib import Path
import argparse, copy, hashlib, json, struct
from import_gothic_materials import parse_material,preview_material,Assets,ximg_dds,fnv
def reject(fn):
    try:fn()
    except (ValueError,struct.error,UnicodeError):return
    raise AssertionError('Invalid material/texture accepted')
def main():
    p=argparse.ArgumentParser();p.add_argument('--assets',type=Path,required=True);p.add_argument('--game',type=Path,required=True);args=p.parse_args()
    root=args.assets/'materials';manifest=json.loads((root/'preview-manifest.json').read_text());graphs=json.loads((root/'graphs.json').read_text())
    assert len(graphs)==len(manifest['materials'])>=14
    rejections=0
    for graph in graphs:
        raw=(root/graph['name']).read_bytes();parsed=parse_material(raw);parsed['name']=graph['name'];preview=preview_material(parsed)
        assert preview['image'] and not preview['replacement_exact']
        for n in (0,13,len(raw)//2,len(raw)-1):reject(lambda:parse_material(raw[:n]));rejections+=1
        damaged=copy.deepcopy(parsed);damaged['roots'][0]['token']='f'*32;reject(lambda:preview_material(damaged));rejections+=1
        damaged=copy.deepcopy(parsed);damaged['properties']['BlendMode']['value']=2;reject(lambda:preview_material(damaged));rejections+=1
    images=Assets(args.game,'_compiledImage');checked=0;bc=set();total=0
    try:
        for name,info in manifest['textures'].items():
            raw,source=images.read(Path(name).with_suffix('.ximg').name);assert source==info['provenance']
            dds,metadata=ximg_dds(raw);stored=(root/'textures'/name).read_bytes()
            assert dds==stored and hashlib.sha256(stored).hexdigest()==info['dds_sha256'] and fnv(stored)==info['content_fnv']
            assert struct.unpack_from('<II',stored,12)==(info['height'],info['width'])
            block=8 if info['compression']=='DXT1' else 16
            largest=max(1,(info['width']+3)//4)*max(1,(info['height']+3)//4)*block
            # Independent largest-level check against the end of native pixels.
            native_end=struct.unpack_from('<I',raw,10)[0]
            assert stored[128:128+largest]==raw[native_end-largest:native_end]
            for truncated in (raw[:86],raw[:-1]):reject(lambda:ximg_dds(truncated));rejections+=1
            invalid=bytearray(raw);invalid[71:75]=b'DX10';reject(lambda:ximg_dds(invalid));rejections+=1
            invalid=bytearray(raw);struct.pack_into('<I',invalid,63,99);reject(lambda:ximg_dds(invalid));rejections+=1
            checked+=1;bc.add(info['compression']);total+=metadata['compressed_bytes']
    finally:images.close()
    assert checked>=14
    print(json.dumps(dict(result='PASS',materials=len(graphs),textures=checked,compressed_texture_bytes=total,
        bc_formats=sorted(bc),malformed_or_unsupported_rejections=rejections,all_preview_materials_marked_inexact=True)))
if __name__=='__main__':main()
