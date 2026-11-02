"""Bounded offline XLMP decoder based on Engine.dll 1.75.14 serialization.

Read/Write evidence: resource1cbb70/1cbe90, lightmap1d3de0/1d3e90,
bitmap1d46c0/1d48a0, FillVertices1d3520. No game DLL execution or SDK
layout assumptions. Current supported versions are deliberately explicit.
"""
import math
import struct
from import_gothic_mesh import Genome,LIMIT
from import_gothic_materials import object_header,properties


def parse_lightmap(data):
    if len(data)>LIMIT:raise ValueError('Excessive XLMP')
    r=Genome(data);header=object_header(r)
    if header['kind']!='eCResourceLightmap_PS':raise ValueError('Not a native lightmap')
    props=properties(r)
    if r.u16()!=4 or r.u16()!=30:raise ValueError('Unsupported resource/base lightmap version')
    flags=r.u32();priority=r.unpack('f')[0];mesh=r.string()
    kind=r.u32();scale=r.unpack('f')[0];count=r.u32()
    if kind not in (0,1) or not math.isfinite(scale) or not 0<scale<=10 or not 0<count<=4096:
        raise ValueError('Invalid lightmap resource fields')
    sections=[]
    for _ in range(count):
        present=r.u8()
        if present not in (0,1):raise ValueError('Invalid lightmap presence')
        if not present:sections.append(None);continue
        if r.u16()!=3:raise ValueError('Unsupported lightmap section version')
        arrays=[]
        for width in (4,12):
            if r.u8()!=1:raise ValueError('Unsupported lightmap array version')
            size=r.u32()
            if size>4000000:raise ValueError('Excessive lightmap vertex array')
            arrays.append(r.take(size*width))
        if len(arrays[0])//4!=len(arrays[1])//12:raise ValueError('Lightmap color/incident count mismatch')
        incident=list(struct.iter_unpack('<3f',arrays[1]))
        if any(not math.isfinite(v) or abs(v)>1e10 for row in incident for v in row):raise ValueError('Invalid incident vector')
        bitmaps=[];bitmap_count=r.u16()
        if bitmap_count>4096:raise ValueError('Excessive lightmap bitmaps')
        for _ in range(bitmap_count):
            if r.u32()!=0x11223344 or r.u16()!=1:raise ValueError('Unsupported bitmap magic/version')
            group=r.u32();x,y=r.unpack('2i');w,h=r.unpack('2I');size=r.u32()
            if group>=4000000 or w>16384 or h>16384 or size not in (0,w*h*4) or size>LIMIT:
                raise ValueError('Invalid bitmap dimensions/payload')
            pixels=r.take(size);horizon=b''
            if size:
                horizon_size=r.u32()
                if horizon_size not in (0,h*4):raise ValueError('Invalid bitmap horizon size')
                horizon=r.take(horizon_size)
            bitmaps.append(dict(group=group,source_offset=(x,y),width=w,height=h,pixels=pixels,horizon=horizon))
        sections.append(dict(colors=arrays[0],incident=arrays[1],bitmaps=bitmaps))
    # The AO grid belongs to a separate system and is not a static shader input.
    # Refuse unsupported data rather than silently skip an unbounded structure.
    if r.u8()!=0:raise ValueError('AO grid format not supported by this static importer')
    if r.pos!=header['end'] or r.pos!=r.end:raise ValueError('Unexpected lightmap trailing payload')
    return dict(mesh=mesh,kind=kind,scale=scale,sections=sections,resource_flags=flags,priority=priority,properties=props)


def build_lightmap_inputs(section,mesh,scale,atlas_size=512):
    """Pack original bitmaps locally and reproduce native FillVertices inputs.

    Atlas packing placement may differ from the native global atlas; source
    texels and native half-texel/scale UV equation are retained. This is not
    proof of equality to the live atlas or filtering at every atlas border.
    """
    n=len(mesh['vertices'])
    if not math.isfinite(scale) or not 0<scale<=10:raise ValueError('Invalid lightmap UV scale')
    if len(section['colors'])!=n*4 or len(section['incident'])!=n*12:raise ValueError('XLMP belongs to a different mesh section')
    if atlas_size not in (128,256,512,1024,2048):raise ValueError('Unsupported local atlas size')
    streams=mesh.get('source_streams',{});groups=mesh.get('source_uv_groups',[])
    if 73 not in streams or streams[73][0]!=n:raise ValueError('Native secondary UV stream missing')
    uv=list(struct.iter_unpack('<2f',streams[73][1]))
    if any(not math.isfinite(v) or abs(v)>1e10 for row in uv for v in row):raise ValueError('Invalid native lightmap UV')
    colors=list(struct.unpack('<'+'I'*n,section['colors']));incident=list(struct.iter_unpack('<3f',section['incident']))
    mapped_uv=[(1.,1.) for _ in range(n)];assigned={};atlas=bytearray(atlas_size*atlas_size*4)
    x=y=row_height=0;placements=[]
    for bitmap in section['bitmaps']:
        if not bitmap['pixels']:continue
        w,h=bitmap['width'],bitmap['height'];group=bitmap['group']
        if w+2>atlas_size or h+2>atlas_size or group>=len(groups):raise ValueError('Lightmap bitmap/group exceeds atlas')
        if x+w+2>atlas_size:x=0;y+=row_height;row_height=0
        if y+h+2>=atlas_size:raise ValueError('Native bitmap atlas budget exceeded')
        ox,oy=x+1,y+1
        for row in range(h):atlas[((oy+row)*atlas_size+ox)*4:((oy+row)*atlas_size+ox+w)*4]=bitmap['pixels'][row*w*4:(row+1)*w*4]
        # Isolated one-texel border avoids sampling another bitmap when a
        # native coordinate sits on the edge. Original edge texels are retained.
        for py in range(-1,h+1):
            for px in range(-1,w+1):
                if 0<=px<w and 0<=py<h:continue
                source=(min(h-1,max(0,py))*w+min(w-1,max(0,px)))*4
                target=((oy+py)*atlas_size+ox+px)*4;atlas[target:target+4]=bitmap['pixels'][source:source+4]
        for vertex in groups[group]['vertices']:
            if vertex>=n or vertex in assigned:raise ValueError('Ambiguous/invalid UV group vertex')
            assigned[vertex]=group;colors[vertex]&=0xff000000
            mapped_uv[vertex]=((ox+.5+uv[vertex][0]*scale)/atlas_size,(oy+.5+uv[vertex][1]*scale)/atlas_size)
        placements.append(dict(group=group,x=ox,y=oy,width=w,height=h));x+=w+2;row_height=max(row_height,h+2)
    stream=bytearray()
    for color,direction,texcoord in zip(colors,incident,mapped_uv):stream+=struct.pack('<I5f',color,*direction,*texcoord)
    return dict(additional_stream=bytes(stream),atlas=bytes(atlas),atlas_size=atlas_size,placements=placements,mapped_vertices=len(assigned))
