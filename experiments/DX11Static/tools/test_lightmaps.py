import argparse,copy,math,struct
from pathlib import Path
from import_gothic_lightmap import parse_lightmap,build_lightmap_inputs
from import_gothic_mesh import parse_mesh
from import_gothic_materials import Assets

def reject(call):
    try:call()
    except (ValueError,struct.error,UnicodeError,IndexError):return
    raise AssertionError('Malformed lightmap input accepted')

def main():
    p=argparse.ArgumentParser();p.add_argument('--game',type=Path,required=True);p.add_argument('--inputs',type=Path,required=True);a=p.parse_args()
    raw=(a.inputs/'original-entity-lightmap.xlmp').read_bytes();lm=parse_lightmap(raw)
    assert lm['kind']==1 and len(lm['sections'])==1
    section=lm['sections'][0];assert len(section['colors'])==2097*4 and len(section['incident'])==2097*12 and len(section['bitmaps'])==106
    assert sum(bool(b['pixels']) for b in section['bitmaps'])==6
    for cut in (0,1,13,len(raw)//2,len(raw)-1):reject(lambda:parse_lightmap(raw[:cut]))
    broken=bytearray(raw);broken[0]^=255;reject(lambda:parse_lightmap(broken))
    at=raw.index(b'\x44\x33\x22\x11');broken=bytearray(raw);struct.pack_into('<H',broken,at+4,2);reject(lambda:parse_lightmap(broken))
    broken=bytearray(raw);struct.pack_into('<I',broken,at+26,1);reject(lambda:parse_lightmap(broken))
    meshes=Assets(a.game,'_compiledMesh')
    try:data,_=meshes.read(lm['mesh'])
    finally:meshes.close()
    default=parse_mesh(data);preserved=parse_mesh(data,preserve_streams=True)
    assert [{key:s[key] for key in ('material','vertices','indices')} for s in preserved]==default
    mesh=preserved[0];out=build_lightmap_inputs(section,mesh,lm['scale'])
    assert len(out['additional_stream'])==2097*24 and len(out['atlas'])==512*512*4 and out['mapped_vertices']==50 and len(out['placements'])==6
    assert out['atlas'][-4:]==bytes(4)
    for color,x,y,z,u,v in struct.iter_unpack('<I5f',out['additional_stream']):assert all(math.isfinite(t) for t in (x,y,z,u,v)) and 0<=u<=1 and 0<=v<=1
    reject(lambda:build_lightmap_inputs(section,mesh,float('nan')))
    reject(lambda:build_lightmap_inputs(section,mesh,lm['scale'],17))
    bad=copy.deepcopy(section);next(b for b in bad['bitmaps'] if b['pixels'])['group']=0xffffffff;reject(lambda:build_lightmap_inputs(bad,mesh,lm['scale']))
    bad=copy.deepcopy(section);populated=next(b for b in bad['bitmaps'] if b['pixels']);bad['bitmaps'].append(populated);reject(lambda:build_lightmap_inputs(bad,mesh,lm['scale']))
    print('PASS actual XLMP,106 groups,6 bitmaps,original vertices preserved,12 malformed-input/identity/atlas refusals')

if __name__=='__main__':main()
