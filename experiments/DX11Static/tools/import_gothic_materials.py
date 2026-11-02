"""Read-only material graph/texture import for the isolated DX11 experiment.

Format reference: Baltram/rmtools mi_genomematerial.cpp (public file-format
investigation; the reference implementation is not bundled/compiled).
No material is considered an exact replacement for the native shader yet.
"""
from pathlib import Path
import argparse, hashlib, json, struct, math
from import_gothic_mesh import Archive, Genome, Reader, LIMIT

class Assets:
    def __init__(self,game,stem):
        self.archives=[];self.names={};self.loose={}
        files=sorted((game/'Data').glob(stem+'.*'),key=lambda p:(p.suffix.lower()!='.pak',p.name.lower()))
        for path in files:
            if not path.is_file() or (path.suffix.lower()!='.pak' and not (len(path.suffix)==4 and path.suffix[1].lower()=='p' and path.suffix[2:].isdigit())):continue
            archive=Archive(path);self.archives.append(archive);seen={}
            for key in archive.entries:
                name=key.rsplit('/',1)[-1]
                if name in seen:raise ValueError('Ambiguous basename in archive: '+name)
                seen[name]=(archive,key)
            self.names.update(seen)
        loose=game/'Data'/stem
        if loose.is_dir():
            for path in loose.rglob('*'):
                if path.is_file():
                    name=path.name.lower()
                    if name in self.loose:raise ValueError('Ambiguous loose resource: '+name)
                    self.loose[name]=path
    def read(self,name):
        key=name.replace('\\','/').rsplit('/',1)[-1].lower()
        if key in self.loose:
            path=self.loose[key]
            if path.stat().st_size>LIMIT:raise ValueError('Excessive loose asset')
            raw=path.read_bytes();source=str(path)
        else:
            archive,path=self.names[key];raw=archive.read(path);source=str(archive.path)+'::'+path
        return raw,dict(source=source,sha256=hashlib.sha256(raw).hexdigest())
    def close(self):
        for archive in self.archives:archive.close()

def object_header(r):
    if r.unpack('HBHB')!=(1,1,1,1):raise ValueError('Material property-object signature')
    kind=r.string()
    if r.unpack('HB')!=(1,0):raise ValueError('Material property-object prefix')
    version,ps_version=r.unpack('HH');size=r.u32();end=r.pos+size
    if end>r.end:raise ValueError('Material object exceeds file')
    if ps_version<81:r.string()
    if ps_version<82:r.take(20)
    return dict(kind=kind,version=version,end=end,body=r.pos)

def properties(r):
    r.u16();count=r.u32()
    if count>4096:raise ValueError('Excessive material properties')
    result={}
    for _ in range(count):
        name,kind=r.string(),r.string();version=r.u16();size=r.u32();end=r.pos+size
        if name in result or size>1048576 or end>r.end:raise ValueError('Invalid material property')
        if kind=='float' and size==4:value=r.unpack('f')[0]
        elif kind in ('bool','char') and size==1:value=r.u8()
        elif kind=='int' and size==4:value=r.unpack('i')[0]
        elif kind.startswith('bTPropertyContainer<enum ') and size==6:r.u16();value=r.u32()
        elif kind in ('bCImageResourceString','bCImageOrMaterialResourceString','bCString'):value=r.string()
        elif kind in ('bCVector2','bCEulerAngles') and size in (8,12):value=list(r.unpack('f'*(size//4)))
        elif kind=='bCFloatColor' and size==16:r.u32();value=list(r.unpack('3f'))
        else:value=dict(raw_hex=r.take(size).hex())
        if r.pos!=end:raise ValueError('Material property length mismatch')
        result[name]=dict(type=kind,value=value,version=version)
    return result

def token(raw):
    if len(raw)!=20 or raw[16] not in (0,1):raise ValueError('Material GUID validity')
    return raw[:16].hex() if raw[16] else None

def shader_refs(r):
    version=r.u16()
    if version not in (1,2,3):raise ValueError('Unsupported default shader data version')
    values=[]
    for _ in range(6+int(version>=2)+int(version>=3)):
        component=r.u32()
        if component>=666:r.take(8)
        values.append(dict(component=component,token=token(r.take(20))))
    return values

def base_element(r):
    r.u16();version=r.u16();guid=token(r.take(20));r.take(16)
    if version>=2:r.u8()
    return guid

def parse_material(raw):
    r=Genome(raw);resource=object_header(r)
    if resource['kind']!='eCResourceShaderMaterial_PS':raise ValueError('Not a Gothic 3 material')
    root_properties=properties(r)
    start=r.pos;r.u16();v=r.u16();r.pos=start;r.take(8 if v>=30 else 11 if v>=23 else 7 if v>=2 else 6)
    shader=object_header(r);shader_properties=properties(r)
    if shader['kind']!='eCShaderDefault':raise ValueError('Unsupported shader class: '+shader['kind'])
    refs=shader_refs(r);base_element(r);count=r.u32()
    if count>4096:raise ValueError('Excessive material nodes')
    nodes=[]
    for _ in range(count):
        node=object_header(r);props=properties(r);data_start=r.pos
        if node['end']<data_start:raise ValueError('Material node data length')
        payload=r.data[data_start:node['end']]
        # Shared element base is version1 (38 bytes) or version2 (39 bytes).
        # A class-data GUID is a graph identity, never a guess based on image name.
        tail=None
        for length,version in ((39,2),(38,1)):
            if len(payload)>=length and struct.unpack_from('<H',payload,len(payload)-length)[0]==version:
                tail=len(payload)-length;break
        if tail is None:raise ValueError('Unknown shader element base layout')
        identity=token(payload[tail+2:tail+22])
        node_refs=[];extra={};prefix=Reader(payload,0,tail)
        kind=node['kind']
        if kind=='eCColorSrcSampler':
            extra['data_version']=prefix.u16();extra['uv_component']=prefix.u32();node_refs.append(token(prefix.take(20)));extra['sampler_type']=prefix.u32()
        elif kind in ('eCColorSrcCombiner','eCColorSrcBlend','eCColorSrcMask'):
            extra['data_version']=prefix.u16()
            for _ in range({'eCColorSrcCombiner':2,'eCColorSrcBlend':3,'eCColorSrcMask':1}[kind]):
                component=prefix.u32()
                if component>=666:prefix.take(8)
                node_refs.append(token(prefix.take(20)))
        # Other node payloads stay explicit/unsupported until interpreted.
        nodes.append(dict(kind=kind,identity=identity,properties=props,references=node_refs,
            extra=extra,class_data_hex=payload.hex(),prefix_hex=payload[:tail].hex()))
        r.pos=node['end']
    if r.pos!=shader['end'] or r.pos!=resource['end']:raise ValueError('Material object boundary mismatch')
    return dict(shader=shader['kind'],resource_properties=root_properties,properties=shader_properties,roots=refs,nodes=nodes)

def fnv(data):
    h=14695981039346656037
    for b in data:h=((h^b)*1099511628211)&0xffffffffffffffff
    return h

def ximg_dds(raw):
    # Gothic stores BC mip levels from smallest to largest, unlike DDS/D3D11.
    # Validate the Genome boundary; never scan for an accidental 'DDS ' token.
    wrapper=Genome(raw)
    if raw[:8]!=b'GENOMFLE' or len(raw)<87 or raw[38:43]!=b'G3IMG':raise ValueError('Unsupported XIMG layout')
    width,height=struct.unpack_from('<II',raw,47);mips=struct.unpack_from('<I',raw,63)[0];fourcc=raw[71:75]
    if fourcc not in (b'DXT1',b'DXT3',b'DXT5') or not (4<=width<=16384 and 4<=height<=16384 and width%4==height%4==0):raise ValueError('Unsupported XIMG format/dimensions')
    if not 1<=mips<=1+max(width,height).bit_length()-1:raise ValueError('Invalid XIMG mip count')
    block=8 if fourcc==b'DXT1' else 16
    sizes=[max(1,(max(1,width>>i)+3)//4)*max(1,(max(1,height>>i)+3)//4)*block for i in range(mips)]
    if 87+sum(sizes)!=wrapper.end:raise ValueError('XIMG payload/mip boundary mismatch')
    position=87;levels=[]
    for size in reversed(sizes):levels.append(raw[position:position+size]);position+=size
    flags=0x81007|(0x20000 if mips>1 else 0);caps=0x1000|(0x400008 if mips>1 else 0)
    header=[124,flags,height,width,sizes[0],0,mips]+[0]*11+[32,4,struct.unpack('<I',fourcc)[0],0,0,0,0,0,caps,0,0,0,0]
    if len(header)!=31:raise AssertionError('DDS header size')
    dds=b'DDS '+struct.pack('<31I',*header)+b''.join(reversed(levels))
    return dds,dict(width=width,height=height,mips=mips,compression=fourcc.decode(),compressed_bytes=sum(sizes))

def preview_material(graph):
    nodes={n['identity']:n for n in graph['nodes'] if n['identity']}
    if len(nodes)!=len(graph['nodes']):raise ValueError('Duplicate/invalid shader node identity')
    root=graph['roots'][0]
    if root['component']!=0 or root['token'] not in nodes:raise ValueError('Unsupported diffuse root')
    sampler=nodes[root['token']]
    if sampler['kind']!='eCColorSrcSampler':raise ValueError('Diffuse requires a graph operation not supported in preview')
    value=lambda obj,key,default=None:obj.get('properties',{}).get(key,{}).get('value',default)
    if value(sampler,'AnimationSpeed',0)!=0:raise ValueError('Animated diffuse sampler requires native path')
    address=[value(sampler,'TexRepeatU'),value(sampler,'TexRepeatV')]
    if any(v not in (0,1,2) for v in address):raise ValueError('Unknown sampler address mode')
    uv_scale=[1.,1.];unsupported=[]
    uv=sampler['references'][0]
    if uv:
        if uv not in nodes:raise ValueError('Missing UV graph node')
        node=nodes[uv]
        if node['kind']=='eCTexCoordSrcScale':uv_scale=value(node,'Scale');unsupported.append('UV scale chain not fully interpreted')
        elif node['kind']=='eCTexCoordSrcBumpOffset':unsupported.append('parallax offset not reproduced')
        else:raise ValueError('Unsupported diffuse UV graph')
    blend=value(graph,'BlendMode');mask=value(graph,'MaskReference')
    if blend not in (0,1) or not isinstance(mask,int) or not 0<=mask<=255:raise ValueError('Unsupported material blend mode')
    if graph['roots'][1]['token']:raise ValueError('Separate opacity graph requires native path')
    image=value(sampler,'ImageFilePath')
    if not image or not all(math.isfinite(x) and abs(x)<10000 for x in uv_scale):raise ValueError('Invalid diffuse image/UV scale')
    return dict(name=graph['name'],image=image,address=address,uv_scale=uv_scale,blend=blend,mask=mask,
        unsupported=unsupported+['native lighting, normals/specular and shadows not reproduced'],replacement_exact=False)

def main():
    p=argparse.ArgumentParser();p.add_argument('--game',type=Path,required=True);p.add_argument('--assets',type=Path,required=True);args=p.parse_args()
    manifest=json.loads((args.assets/'manifest.json').read_text());names=sorted({name for a in manifest['assets'] for name in a['materials']})
    assets=Assets(args.game,'_compiledMaterial');out=args.assets/'materials';out.mkdir(exist_ok=True);reports=[]
    try:
        for name in names:
            if name!=Path(name).name or any(c in name for c in ':\\/') or not name.lower().endswith('.xshmat'):raise ValueError('Material output must be a basename')
            raw,source=assets.read(name);(out/name).write_bytes(raw);graph=parse_material(raw);graph.update(name=name,provenance=source)
            reports.append(graph);print(name,[(n['kind'],n['properties'].get('ImageFilePath',{}).get('value')) for n in graph['nodes']])
    finally:assets.close()
    (out/'graphs.json').write_text(json.dumps(reports,indent=2))
    images=Assets(args.game,'_compiledImage');texture_dir=out/'textures';texture_dir.mkdir(exist_ok=True);previews=[];texture_reports={}
    try:
        for graph in reports:
            material=preview_material(graph);name=Path(material['image'].replace('\\','/')).with_suffix('.ximg').name
            texture_name=Path(name).with_suffix('.dds').name.lower()
            if texture_name not in texture_reports:
                raw,source=images.read(name);dds,info=ximg_dds(raw);(texture_dir/texture_name).write_bytes(dds)
                texture_reports[texture_name]=dict(**info,provenance=source,dds_sha256=hashlib.sha256(dds).hexdigest(),content_fnv=fnv(dds))
            material['texture']=texture_name;material['texture_content']=texture_reports[texture_name]['content_fnv'];previews.append(material)
    finally:images.close()
    with (out/'preview.g3mat').open('wb') as f:
        f.write(struct.pack('<8sII',b'G3DX11A1',1,len(previews)))
        for m in previews:
            for value in (m['name'],m['texture']):
                data=value.encode('utf8');f.write(struct.pack('<I',len(data)));f.write(data)
            f.write(struct.pack('<Q4I2f',m['texture_content'],m['blend'],m['mask'],*m['address'],*m['uv_scale']))
    (out/'preview-manifest.json').write_text(json.dumps(dict(materials=previews,textures=texture_reports,replacement_exact=False),indent=2))
    print('Imported native diffuse textures:',len(texture_reports),'; material previews:',len(previews))
if __name__=='__main__':main()
