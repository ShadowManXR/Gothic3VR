"""Preserve matching native pass cameras for local integration regressions."""
import argparse,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--capture',type=Path,required=True);p.add_argument('--profile',type=Path,required=True);a=p.parse_args()
rows=[json.loads(line) for line in (a.capture/'native-static-trial.jsonl').read_text().splitlines()]
materials=[json.loads(line) for line in (a.capture/'native-materials/materials.jsonl').read_text().splitlines() if 'native_material_draw' in line]
values=[]
for eye in range(2):
    material=[m for m in materials if m['eye']==eye and m['vertex_shader']=='vs-7a13cd383a26eb4b.cso' and m['pixel_shader']=='ps-26cb84265f99bd7d.cso']
    if len(material)!=1:raise ValueError('Ambiguous material eye')
    matches=[r for r in rows if r.get('type')=='dx11_static_trial_eye' and r['eye']==eye and r['frame']==material[0]['frame'] and r['resource']=='g3_myrtana_ardea_stairs_02.g3mesh']
    if len(matches)!=1:raise ValueError('Missing matching native camera')
    values+=matches[0]['native_view_cm']+matches[0]['native_projection_cm']
(a.profile/'native-cameras.bin').write_bytes(b'G3CAM001'+struct.pack('<I64f',2,*values))
print('Preserved both native cameras matched to the actual material draws; local fixture only.')
