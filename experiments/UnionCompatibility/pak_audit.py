"""Read-only Gothic3 G3V0 archive index and bounded texture-header inspection.
Layout checked against the local UnionStarter archive format definitions.
Never extract or modify game/archive files.
"""
import struct,zlib,json,collections
from pathlib import Path
class Pak:
 def __init__(self,path):
  self.path=Path(path);self.size=self.path.stat().st_size;self.entries={}
  with self.path.open('rb') as f:
   header=f.read(48)
   if len(header)!=48:raise ValueError('Short archive header')
   version,magic,revision,encryption,compression,reserved,files,folders,volume=struct.unpack('<6I3Q',header)
   if magic!=0x30563347 or encryption or not(48<=folders<volume<=self.size-4):raise ValueError('Unsupported G3V0 archive')
   self.table_end=volume;f.seek(folders);self._entry(f,'',0)
 def _read(self,f,n):
  if n<0 or f.tell()+n>self.table_end:raise ValueError('Archive index out of bounds')
  data=f.read(n)
  if len(data)!=n:raise ValueError('Truncated archive')
  return data
 def _str(self,f):
  n=struct.unpack('<I',self._read(f,4))[0]
  if n>65535:raise ValueError('Overlong archive name')
  data=self._read(f,n)
  if n and self._read(f,1)!=b'\0':raise ValueError('Missing name terminator')
  try:return data.decode('utf-8')
  except UnicodeDecodeError:return data.decode('cp1251')
 def _entry(self,f,parent,depth):
  if depth>128 or len(self.entries)>500000:raise ValueError('Archive index limits')
  header=self._read(f,36);attrs=struct.unpack_from('<I',header,32)[0]
  if attrs&16:
   name=self._str(f);path=parent+'/'+name if parent else name
   for group in range(2):
    count=struct.unpack('<I',self._read(f,4))[0]
    if count>500000:raise ValueError('Too many children')
    for _ in range(count):self._entry(f,path,depth+1)
  else:
   offset,packed,size,encryption,compression=struct.unpack('<3Q2I',self._read(f,32));name=self._str(f);comment=self._str(f)
   if offset+packed>self.size or size>2**32:raise ValueError('Invalid resource extent')
   path=name.lstrip('/\\') if '/' in name or '\\' in name else (parent+'/'+name).lstrip('/')
   self.entries[path.casefold()]={'path':path,'offset':offset,'packed':packed,'size':size,'compression':compression,'encryption':encryption,'attributes':attrs}
 def prefix(self,e,limit=65536):
  if e['encryption']:raise ValueError('Encrypted resource')
  with self.path.open('rb') as f:
   f.seek(e['offset'])
   if e['compression']==0:return f.read(min(limit,e['packed']))
   if e['compression']!=2:raise ValueError('Unsupported compression')
   decoder=zlib.decompressobj();result=b'';remaining=e['packed']
   while remaining and len(result)<limit:
    raw=f.read(min(65536,remaining));remaining-=len(raw)
    if not raw:raise ValueError('Truncated compressed resource')
    result+=decoder.decompress(raw,limit-len(result))
   return result
def inspect(path):
 pak=Pak(path);formats=collections.Counter();dimensions=collections.Counter();mips=collections.Counter();examples=[];missing=[];estimated=0
 for e in pak.entries.values():
  if not e['path'].casefold().endswith('.ximg'):continue
  raw=pak.prefix(e);start=raw.find(b'DDS ');g3=raw.find(b'G3IMG',0,512)
  if start<0 and g3>=0 and g3+49<=len(raw) and struct.unpack_from('<H',raw,g3+5)[0]==2:
   w,h,depth=struct.unpack_from('<3I',raw,g3+9);levels=struct.unpack_from('<I',raw,g3+25)[0];fourcc=raw[g3+33:g3+37].decode('ascii',errors='replace');bits=0
   if not(1<=w<=32768 and 1<=h<=32768 and 1<=levels<=16):raise ValueError('Unsupported G3IMG dimensions/levels')
   numeric=struct.unpack_from('<I',raw,g3+33)[0]
   if numeric in (21,22):fourcc='RGBA8' if numeric==21 else 'RGBX8';bits=32
   elif numeric in (23,25,26):fourcc='RGB16';bits=16
   elif numeric==20:fourcc='RGB24';bits=24
   elif numeric in (28,50):fourcc='R8';bits=8
   if depth>1 or raw[g3+21]:raise ValueError('Volume/cube estimate not implemented')
  elif start>=0:
   if start+128>len(raw):raise ValueError('Truncated DDS header')
   h,w=struct.unpack_from('<2I',raw,start+12);levels=struct.unpack_from('<I',raw,start+28)[0];fourcc=raw[start+84:start+88].decode('ascii',errors='replace');bits=struct.unpack_from('<I',raw,start+88)[0]
  else:
   missing.append({'path':e['path'],'prefix':raw[:64].hex(),'bytes':e['size']});continue
  levels=max(1,levels);fmt=fourcc.strip('\0') or str(bits)+'bit';formats[fmt]+=1;dimensions[f'{w}x{h}']+=1;mips[levels]+=1
  block=8 if fmt=='DXT1' else 16 if fmt in ('DXT3','DXT5','ATI2','BC5U') else 0
  amount=sum(((max(1,w>>l)+3)//4)*((max(1,h>>l)+3)//4)*block if block else max(1,w>>l)*max(1,h>>l)*bits//8 for l in range(levels));estimated+=amount
  if len(examples)<6:examples.append({'path':e['path'],'width':w,'height':h,'mips':levels,'format':fmt})
 return {'archive':str(path),'archive_bytes':pak.size,'entries':len(pak.entries),'ximg':sum(e['path'].casefold().endswith('.ximg') for e in pak.entries.values()),'formats':dict(formats),'dimensions':dict(dimensions),'mip_levels':dict(mips),'estimated_texture_payload_bytes_if_all_resident':estimated,'header_examples':examples,'unsupported_ximg_headers':missing[:10],'unsupported_ximg_count':len(missing),'note':'G3IMG v2 description matches the SDK eSGfxLayersDesc and native Engine Write header. Archive size is not working-set VRAM. All-resident estimate excludes resource overhead, duplicate CPU buffers and runtime residency; unsupported XIMG layouts are not decoded.'}
if __name__=='__main__':
 import sys
 result=inspect(sys.argv[1]);Path(sys.argv[2]).write_text(json.dumps(result,indent=2),encoding='utf-8');print(json.dumps(result,indent=2))
