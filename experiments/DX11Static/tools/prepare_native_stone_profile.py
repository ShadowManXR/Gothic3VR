"""Build a bounded SM5 reference for the exact locally observed stone PS.

This is an offline equation port, not a general shader converter or permission
to replace native draws. Shadow slots come from the actual draw descriptors.
Original CSOs/constants remain local and are not distributable mod assets.
"""
import argparse
import ctypes as C
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct

EXPECTED = 0x26cb84265f99bd7d


def disassemble(data):
    compiler = C.WinDLL('d3dcompiler_47.dll')
    compiler.D3DDisassemble.argtypes = [C.c_void_p, C.c_size_t, C.c_uint, C.c_char_p, C.POINTER(C.c_void_p)]
    compiler.D3DDisassemble.restype = C.c_long
    blob = C.c_void_p()
    if compiler.D3DDisassemble(data, len(data), 0, None, C.byref(blob)) < 0 or not blob.value:
        raise ValueError('Native disassembly failed')
    table = C.cast(blob, C.POINTER(C.POINTER(C.c_void_p))).contents
    pointer = C.WINFUNCTYPE(C.c_void_p, C.c_void_p)(table[3])(blob)
    size = C.WINFUNCTYPE(C.c_size_t, C.c_void_p)(table[4])(blob)
    try:
        return C.string_at(pointer, size).rstrip(b'\0').decode()
    finally:
        C.WINFUNCTYPE(C.c_ulong, C.c_void_p)(table[2])(blob)


def generate(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    if value != EXPECTED:
        raise ValueError('Unsupported native pixel program; profile must be reviewed separately')
    lines = [line.strip() for line in disassemble(data).splitlines()]
    definitions = {}
    for line in lines:
        if line.startswith('def '):
            register, *values = [item.strip() for item in line[4:].split(',')]
            if not re.fullmatch(r'c\d+', register) or len(values) != 4:
                raise ValueError('Unsupported immediate definition')
            definitions[register] = 'float4(' + ','.join(values) + ')'

    def source(text):
        match = re.fullmatch(r'(-?)(r[0-7]|v[0-8]|vPos|c\d+)(?:\.([xyzw]{1,4}))?', text)
        if not match:
            raise ValueError('Unsupported source: ' + text)
        neg, name, swizzle = match.groups()
        name = ('k' + name[1:]) if name in definitions else ('pc[' + name[1:] + ']') if name.startswith('c') else name
        if swizzle:
            name += '.' + swizzle.ljust(4, swizzle[-1])
        return ('-' if neg else '') + name

    body, instructions = [], 0
    for line in lines:
        if not line or line.startswith('//') or line.startswith(('def ', 'dcl', 'ps_')):
            continue
        opcode, args = line.split(None, 1)
        operands = [a.strip() for a in args.split(',')]
        destination = operands.pop(0)
        if not re.fullmatch(r'(r[0-7]|oC[01])(?:\.[xyzw]{1,4})?', destination):
            raise ValueError('Unsupported destination')
        base, _, mask = destination.partition('.')
        sat = opcode.endswith('_sat')
        opcode = opcode.removesuffix('_sat')
        if opcode in ('texld', 'texldp'):
            coordinate, sampler = operands
            if not re.fullmatch(r's[0-5]', sampler):
                raise ValueError('Unexpected texture binding')
            index = int(sampler[1:])
            uv = source(coordinate)
            if opcode == 'texldp':
                if index not in (1, 2):
                    raise ValueError('Only observed depth comparison slots can project')
                expr = f't{index}.SampleCmpLevelZero(s{index},({uv}).xy/({uv}).w,({uv}).z/({uv}).w).xxxx'
            else:
                if index in (1, 2):
                    raise ValueError('Depth sampler used without projection')
                expr = f't{index}.Sample(s{index},({uv}).xy)'
        else:
            values = [source(item) for item in operands]
            if opcode == 'mov': expr = values[0]
            elif opcode == 'add': expr = f'({values[0]})+({values[1]})'
            elif opcode == 'mul': expr = f'({values[0]})*({values[1]})'
            elif opcode == 'mad': expr = f'mad({values[0]},{values[1]},{values[2]})'
            elif opcode in ('min', 'max'): expr = f'{opcode}({values[0]},{values[1]})'
            elif opcode == 'pow': expr = f'pow(abs({values[0]}),{values[1]})'
            elif opcode == 'dp3': expr = f'dot(({values[0]}).xyz,({values[1]}).xyz).xxxx'
            elif opcode == 'rsq': expr = f'rsqrt(abs({values[0]}))'
            elif opcode == 'rcp': expr = f'rcp({values[0]})'
            elif opcode == 'nrm': expr = f'float4(normalize(({values[0]}).xyz),0)'
            elif opcode == 'cmp': expr = f'float4(({values[0]}).x>=0?({values[1]}).x:({values[2]}).x,({values[0]}).y>=0?({values[1]}).y:({values[2]}).y,({values[0]}).z>=0?({values[1]}).z:({values[2]}).z,({values[0]}).w>=0?({values[1]}).w:({values[2]}).w)'
            else: raise ValueError('Unsupported instruction: ' + opcode)
        if sat: expr = 'saturate(' + expr + ')'
        body.append('    ' + base + ('.' + mask if mask else '') + ' = (' + expr + ')' + ('.' + mask if mask else '') + ';')
        instructions += 1
    header = '// Exact observed profile; source CSO SHA256: ' + hashlib.sha256(data).hexdigest() + '\n'
    header += '// Equation reference only; real texture/geometry/shadow equivalence is separate.\n'
    header += 'cbuffer NativePixel:register(b0){float4 pc[224];}\n'
    for index in range(6):
        header += f'Texture2D<float{1 if index in (1,2) else 4}> t{index}:register(t{index});'
        header += f'{"SamplerComparisonState" if index in (1,2) else "SamplerState"} s{index}:register(s{index});\n'
    header += 'struct PixelInput {float4 position:SV_POSITION;'
    for index, semantic in enumerate(('COLOR0','COLOR1','TEXCOORD0','TEXCOORD2','TEXCOORD3','TEXCOORD4','TEXCOORD5','TEXCOORD6','TEXCOORD7')):
        if index==3:header+='float4 unused:TEXCOORD1;'
        header += f'float4 v{index}:{semantic};'
    header += '};\nstruct PixelOutput{float4 color:SV_Target0;float4 auxiliary:SV_Target1;};\nPixelOutput PS(PixelInput input){\n'
    header += '    float4 vPos=float4(input.position.xy-.5,0,0);\n'
    header += ''.join(f'    float4 v{i}=input.v{i};\n' for i in range(9))
    header += ''.join(f'    const float4 k{name[1:]}={value};\n' for name,value in definitions.items())
    header += '    float4 ' + ','.join(f'r{i}=0' for i in range(8)) + ',oC0=0,oC1=0;\n'
    header += '\n'.join(body) + '\n    PixelOutput output;output.color=oC0;output.auxiliary=oC1;return output;\n}\n'
    return header, instructions


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--capture', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    spec = importlib.util.spec_from_file_location('capture', Path(__file__).with_name('analyze_material_capture.py'))
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    args.output.mkdir(parents=True, exist_ok=True)
    analysis = decoder.analyze(args.capture, args.output / 'decoded.json')
    rows = sorted([json.loads(line) for line in (args.capture / 'materials.jsonl').read_text().splitlines()], key=lambda r:r['eye'])
    if len(rows) != 2 or [r['eye'] for r in rows] != [0,1] or len({r['pixel_shader'] for r in rows}) != 1:
        raise ValueError('Need the observed stereo program pair')
    data = (args.capture / rows[0]['pixel_shader']).read_bytes()
    hlsl, count = generate(data)
    (args.output / 'NativeStoneMaterial.hlsl').write_text(hlsl)
    (args.output / 'native.cso').write_bytes(data)
    (args.output / 'constants.bin').write_bytes(b'G3PSREG1' + struct.pack('<I', 2) + b''.join(struct.pack('<896f', *r['pixel_constants_c0_c223']) for r in rows))
    if {r['vertex_shader'] for r in rows}!={'vs-7a13cd383a26eb4b.cso'}:
        raise ValueError('Unsupported native stone vertex program')
    (args.output / 'native-vertex.cso').write_bytes((args.capture / rows[0]['vertex_shader']).read_bytes())
    (args.output / 'vertex-constants.bin').write_bytes(b'G3VSREG1' + struct.pack('<I',2) + b''.join(struct.pack('<1024f',*r['vertex_constants_c0_c255']) for r in rows))
    (args.output / 'NativeStoneVertex.hlsl').write_bytes((Path(__file__).resolve().parent.parent / 'shaders/NativeStoneVertex.hlsl').read_bytes())
    summary = dict(source_pixel_program=rows[0]['pixel_shader'], instruction_count=count, actual_eyes=analysis['eyes'],
                   source_materials=sorted({r['material'] for r in rows}), native_draw_suppression=False,
                   material_equivalence_proven=False)
    (args.output / 'profile.json').write_text(json.dumps(summary, indent=2))
    print(json.dumps(summary, indent=2))


if __name__ == '__main__': main()
