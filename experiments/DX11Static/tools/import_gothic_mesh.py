"""Read-only Gothic 3 PAK/xcmsh importer for the isolated DX11 prototype.

Format references (not linked into the renderer):
https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_xcmshreader.cpp
https://github.com/Baltram/rmtools/blob/master/mimicry/source/Mimicry/mi_genomevolume.cpp
Native coordinates are Y-up, centimeters. No artist-space coordinate conversion.
Only locally owned game assets are extracted; outputs are not redistributable.
"""
from pathlib import Path
import argparse, hashlib, json, math, mmap, struct, zlib

LIMIT = 128 * 1024 * 1024

class Reader:
    def __init__(self, data, position=0, end=None):
        self.data, self.pos, self.end = data, position, len(data) if end is None else end
    def take(self, size):
        if size < 0 or self.pos + size > self.end: raise ValueError('Truncated or excessive input')
        value = self.data[self.pos:self.pos+size]; self.pos += size; return value
    def unpack(self, fmt): return struct.unpack('<'+fmt, self.take(struct.calcsize('<'+fmt)))
    def u8(self): return self.unpack('B')[0]
    def u16(self): return self.unpack('H')[0]
    def u32(self): return self.unpack('I')[0]
    def u64(self): return self.unpack('Q')[0]
    def string32(self):
        size=self.u32()
        if size>32768: raise ValueError('Excessive archive path')
        if not size: return ''
        value=self.take(size)
        if self.u8()!=0: raise ValueError('Archive path terminator')
        return value.decode('cp1252')

class Archive:
    def __init__(self, path):
        self.path=Path(path);self.file=self.path.open('rb')
        self.data=mmap.mmap(self.file.fileno(),0,access=mmap.ACCESS_READ)
        h=Reader(self.data);version=h.u32()
        if h.take(4)!=b'G3V0' or version not in (0,1): raise ValueError('Unsupported PAK header')
        h.u32()
        if h.u32()!=0 or h.u32()>2: raise ValueError('Encrypted/unsupported PAK')
        h.take(12);root=h.u64()
        if not 48<=root<len(self.data): raise ValueError('Invalid PAK directory')
        self.entries={};self.root=root;r=Reader(self.data,root);self.nodes=0
        def add(name,offset,packed,size,compressed,attributes):
            if attributes&0xc000 or not (packed or size):return
            if not 0<=packed<=LIMIT or not 0<=size<=LIMIT or offset<48 or offset+packed>root:
                raise ValueError(f'PAK payload outside data region: {name} {offset}/{packed}/{size} root={root}')
            if not attributes&0xc000:
                key=name.replace('\\','/').lstrip('/').lower()
                if key in self.entries:raise ValueError('Duplicate PAK path')
                self.entries[key]=(name,offset,packed,size,compressed)
        def visit(depth=0,prefix='',is_dir=True):
            self.nodes+=1
            if depth>32 or self.nodes>200000:raise ValueError('Excessive PAK directory')
            if version==0:
                r.take(32);attributes=r.u32()
                if attributes&0x10:
                    folder=r.string32()
                    for _ in range(r.u32()):visit(depth+1,folder)
                    for _ in range(r.u32()):visit(depth+1,folder)
                else:
                    offset,packed,size=r.unpack('QQQ');r.u32();compressed=bool(r.u32()&2)
                    name=r.string32();r.string32();add(name,offset,packed,size,compressed,attributes)
            elif is_dir:
                folder=prefix+r.string32()+'/';r.take(28)
                for _ in range(r.u32()):visit(depth+1,folder,bool(r.u32()&0x10))
            else:
                name=prefix+r.string32();offset=r.u64();r.take(24);attributes=r.u32();r.u32()
                compressed=bool(r.u32());packed,size=r.unpack('II');add(name,offset,packed,size,compressed,attributes)
        visit()
    def read(self, key):
        name,offset,packed,size,compressed=self.entries[key]
        raw=self.data[offset:offset+packed]
        if compressed:
            decoder=zlib.decompressobj();raw=decoder.decompress(raw,size+1)
            if not decoder.eof or decoder.unconsumed_tail or decoder.unused_data:raise ValueError('Invalid bounded PAK compression')
        if len(raw)!=size:raise ValueError('PAK payload size mismatch')
        return raw
    def close(self): self.data.close();self.file.close()

class Genome(Reader):
    def __init__(self,data):
        super().__init__(data);self.strings=None
        if data[:8]==b'GENOMFLE':
            if len(data)<23:raise ValueError('Truncated Genome header')
            end=struct.unpack_from('<I',data,10)[0]
            table=Reader(data,end)
            if table.u32()!=0xdeadbeef or table.u8()!=1:raise ValueError('Genome string table marker')
            count=table.u32()
            if count>65536:raise ValueError('Excessive Genome strings')
            self.strings=[table.take(table.u16()).decode('cp1252') for _ in range(count)]
            self.pos=14;self.end=end
    def string(self):
        n=self.u16()
        if self.strings is None:return self.take(n).decode('cp1252')
        if n>=len(self.strings):raise ValueError('Genome string index')
        return self.strings[n]
    def array_skip(self, size, genome=True):
        if genome:self.u8()
        count=self.u32()
        if count>4000000:raise ValueError('Excessive mesh array')
        self.take(count*size)

def parse_mesh(data, *, preserve_streams=False):
    r=Genome(data)
    if r.unpack('HBHB')!=(1,1,1,1) or r.string()!='eCResourceMeshComplex_PS' or r.unpack('BH')!=(1,0):
        raise ValueError('Not a static complex mesh')
    version=r.u16();r.take(6)
    if version<81:r.string()
    if version<82:r.take(20)
    r.u16();properties=r.u32()
    if properties>4096:raise ValueError('Excessive properties')
    for _ in range(properties):r.string();r.string();r.u16();r.take(r.u32())
    mesh_version,base_version=r.unpack('HH')
    if base_version>22:r.u32()
    if base_version<30 and r.u16()>1:r.u8()
    r.take(4);count=r.u32()
    if mesh_version<34 or count>4096:raise ValueError('Unsupported mesh references/version')
    sections=[]
    widths={0:4,1:12,2:16,3:12,4:4,5:4,6:4,12:8,15:8,18:8,21:8,64:12,72:12,73:8}
    for _ in range(count):
        element_version=r.u16();r.u32();r.take(28);material=r.string();streams={};n=r.u32()
        if n>32:raise ValueError('Excessive mesh streams')
        for _ in range(n):
            kind=r.u32();r.take(3);size=r.u32()
            if kind not in widths or kind in streams or size>4000000:raise ValueError('Unsupported/duplicate/excessive mesh stream')
            streams[kind]=(size,r.take(size*widths[kind]))
        if not all(k in streams for k in (0,1,3,12)):raise ValueError('Missing positions/indices/normals/UV')
        nv,positions=streams[1];ni,raw_indices=streams[0]
        if not nv or not ni or ni%3 or streams[3][0]!=nv or streams[12][0]!=nv:raise ValueError('Inconsistent mesh streams')
        indices=struct.unpack('<'+str(ni)+'I',raw_indices)
        if max(indices)>=nv:raise ValueError('Mesh index exceeds vertices')
        vertices=[]
        for i in range(nv):
            pos=struct.unpack_from('<3f',positions,i*12);normal=struct.unpack_from('<3f',streams[3][1],i*12);uv=struct.unpack_from('<2f',streams[12][1],i*8)
            color=[1.,1.,1.,1.]
            for kind in (4,5):
                if kind in streams:
                    if streams[kind][0]!=nv:raise ValueError('Vertex color count mismatch')
                    bgra=streams[kind][1][i*4:i*4+4]
                    if kind==4:color[:3]=[bgra[2]/255,bgra[1]/255,bgra[0]/255]
                    else:color[3]=bgra[3]/255
            values=tuple(v*.01 for v in pos)+normal+uv+tuple(color)
            if not all(math.isfinite(v) and abs(v)<1e10 for v in values):raise ValueError('Nonfinite/excessive vertex')
            vertices.append(values)
        section=dict(material=material,vertices=vertices,indices=indices)
        # Optional native input research retains all original stream bytes.
        # The existing compact preview/export stays unchanged by default.
        if preserve_streams:section['source_streams']=streams
        sections.append(section)
        if element_version>2:r.array_skip(4);r.array_skip(4)
        if element_version>1:
            r.u8();groups=r.u32()
            if groups>4000000:raise ValueError('Excessive UV groups')
            if preserve_streams:section['source_uv_groups']=[]
            for _ in range(groups):
                if preserve_streams:
                    arrays=[]
                    for _ in range(2):
                        if r.u8()!=1:raise ValueError('Unsupported native UV group array version')
                        size=r.u32()
                        if size>4000000:raise ValueError('Excessive UV group array')
                        arrays.append(list(struct.unpack('<'+'I'*size,r.take(size*4))))
                    section['source_uv_groups'].append(dict(vertices=arrays[0],secondary=arrays[1],properties_hex=r.take(84).hex()))
                else:r.array_skip(4);r.array_skip(4);r.take(84)
        if element_version>3:r.array_skip(24,False);r.array_skip(4)
    return sections

def write_mesh(path, sections):
    with path.open('wb') as f:
        f.write(struct.pack('<8sIII',b'G3DX11M1',1,len(sections),48))
        for section in sections:
            name=section['material'].encode('utf8');v=section['vertices'];indices=section['indices']
            f.write(struct.pack('<IIII',len(name),len(v),len(indices),0));f.write(name)
            for vertex in v:f.write(struct.pack('<12f',*vertex))
            f.write(struct.pack('<'+str(len(indices))+'I',*indices))

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--game',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--match',default='ardea');args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    # Game precedence: .pak first, numbered patches override the same path.
    archives=sorted((args.game/'Data').glob('_compiledMesh.*'),key=lambda p:(p.suffix.lower()!='.pak',p.name.lower()))
    selected={};provenance=[]
    for archive in archives:
        a=Archive(archive)
        for key,entry in a.entries.items():
            if key.endswith('.xcmsh') and args.match.lower() in key:
                selected[key]=(archive,a.read(key))
        a.close()
    for key,(archive,raw) in sorted(selected.items()):
        if 'lod' in Path(key).stem.lower() or 'lowpoly' in key.lower():continue
        sections=parse_mesh(raw);name=Path(key).name;target=args.output/(Path(name).stem+'.g3mesh');write_mesh(target,sections)
        points=[v[:3] for s in sections for v in s['vertices']]
        info=dict(resource=key,archive=str(archive),source_sha256=hashlib.sha256(raw).hexdigest(),
            output=target.name,output_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
            sections=len(sections),vertices=len(points),triangles=sum(len(s['indices'])//3 for s in sections),
            materials=[s['material'] for s in sections],bounds_m=[[min(p[k] for p in points) for k in range(3)],[max(p[k] for p in points) for k in range(3)]])
        provenance.append(info);print(name,info['vertices'],info['triangles'],info['sections'])
    if not provenance:raise ValueError('No matching static meshes')
    (args.output/'manifest.json').write_text(json.dumps(dict(format_version=1,units='meters',up_axis='Y',assets=provenance),indent=2))
if __name__=='__main__':main()
