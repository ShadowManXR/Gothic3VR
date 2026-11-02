import json
from pathlib import Path
from pak_audit import Pak
import struct
def desc(pak,e):
 raw=pak.prefix(e);g=raw.find(b'G3IMG',0,512)
 if g<0 or len(raw)<g+49 or struct.unpack_from('<H',raw,g+5)[0]!=2:return None
 w,h=struct.unpack_from('<2I',raw,g+9);levels=struct.unpack_from('<I',raw,g+25)[0];fmt=raw[g+33:g+37];numeric=struct.unpack_from('<I',raw,g+33)[0]
 block=8 if fmt==b'DXT1' else 16 if fmt in (b'DXT3',b'DXT5') else 0;bits=32 if numeric in (21,22) else 0
 if not block and not bits:return None
 size=sum(((max(1,w>>l)+3)//4)*((max(1,h>>l)+3)//4)*block if block else max(1,w>>l)*max(1,h>>l)*bits//8 for l in range(levels))
 return {'width':w,'height':h,'mips':levels,'format':fmt.decode('ascii') if block else 'RGBA8','payload':size}
game=Path('D:/SteamLibrary/steamapps/common/Gothic 3/Data');baseline={}
for name in ('_compiledImage.pak','_compiledImage.p00','_compiledImage.p01','_compiledImage.p02'):
 pak=Pak(game/name)
 for key,e in pak.entries.items():baseline[key]=(pak,e)
mod=Pak(Path('E:/Downloads/Gothic 3 Union Plus v1.7.3-46-1-1778015782/Union+ v1.7.3/Mods/J_USMTexturesPack/ModFiles/_compiledImage.m0x'));rows=[]
for key,e in mod.entries.items():
 if not key.endswith('.ximg'):continue
 old=baseline.get(key);before=desc(*old) if old else None;after=desc(mod,e)
 rows.append({'path':e['path'],'before_archive':old[0].path.name if old else None,'before':before,'usm':after})
result={'incoming_ximg':len(rows),'matched_current_baseline':sum(bool(r['before']) for r in rows),'unknown_or_new':sum(not r['before'] for r in rows),
 'matched_current_payload_bytes':sum(r['before']['payload'] for r in rows if r['before']),
 'matched_usm_payload_bytes':sum(r['usm']['payload'] for r in rows if r['before'] and r['usm']),
 'usm_rgba8_candidates':[r for r in rows if r['usm'] and r['usm']['format']=='RGBA8'],
 'note':'Hypothetical package overlay, not a running-process residency measurement. Existing pak/p00/p01/p02 order; loose files and future Union archives require a new inventory.', 'textures':rows}
Path('work/mod-manager/usm-baseline-comparison.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k!='textures'},indent=2))
