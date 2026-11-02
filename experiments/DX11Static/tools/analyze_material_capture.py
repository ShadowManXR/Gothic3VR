"""Resolve captured constants by their *actual shader's* CTAB declarations.

Unused native registers retain older draw values; only declarations in the
matching validated shader can identify lighting/material inputs. No game DLL
is loaded and no material-equivalence/replacement flag is asserted.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def constant_table(data, stage):
    if not 8 <= len(data) <= 32768 or len(data) % 4:
        raise ValueError('Invalid shader size')
    version = struct.unpack_from('<I', data)[0]
    if version != (0xfffe0300 if stage == 'vertex' else 0xffff0300):
        raise ValueError('Shader stage/version mismatch')
    at, table, ended = 4, None, False
    while at + 4 <= len(data):
        instruction = struct.unpack_from('<I', data, at)[0]
        opcode = instruction & 0xffff
        if instruction == 0xffff:
            if at + 4 != len(data):
                raise ValueError('Trailing shader bytes')
            ended = True
            break
        words = (instruction >> 16) & 0x7fff if opcode == 0xfffe else (instruction >> 24) & 15
        end = at + 4 + words * 4
        if end > len(data):
            raise ValueError('Truncated shader instruction/comment')
        if opcode == 0xfffe and data[at + 4:at + 8] == b'CTAB':
            if table is not None:
                raise ValueError('Ambiguous shader constant table')
            table = (at + 8, end)
        at = end
    if not ended or table is None:
        raise ValueError('Missing shader END/CTAB')
    base, end = table

    def unpack(fmt, offset):
        size = struct.calcsize('<' + fmt)
        if offset < 0 or base + offset + size > end:
            raise ValueError('CTAB field outside comment')
        return struct.unpack_from('<' + fmt, data, base + offset)

    def string(offset):
        if offset < 0 or not base <= base + offset < end:
            raise ValueError('CTAB string outside comment')
        stop = data.find(b'\0', base + offset, min(end, base + offset + 2048))
        if stop < 0:
            raise ValueError('Unterminated CTAB string')
        return data[base + offset:stop].decode('ascii')

    def typedesc(offset, visited=frozenset()):
        if offset in visited or len(visited) >= 4:
            raise ValueError('Cyclic/excessive CTAB type nesting')
        kind, value_type, rows, columns, elements, count, member_offset = unpack('6HI', offset)
        if count > 16:
            raise ValueError('Excessive CTAB members')
        members = []
        for i in range(count):
            name, child = unpack('2I', member_offset + i * 8)
            members.append(dict(name=string(name), type=typedesc(child, visited | {offset})))
        return dict(kind=kind, value_type=value_type, rows=rows, columns=columns, elements=elements, members=members)

    size, creator, shader_version, count, info, flags, target = unpack('7I', 0)
    if size != 28 or count > 256 or shader_version != version:
        raise ValueError('Invalid CTAB header')
    constants = []
    for i in range(count):
        name, register_set, index, number, reserved, type_offset, default = unpack('I4H2I', info + i * 20)
        if register_set > 3 or number > 256:
            raise ValueError('Invalid shader register declaration')
        constants.append(dict(name=string(name), register_set=register_set, register_index=index,
                              register_count=number, type=typedesc(type_offset)))
    return dict(creator=string(creator), target=string(target), constants=constants)


def analyze(directory, output):
    log = directory / 'materials.jsonl'
    if log.stat().st_size > 4 * 1024 * 1024:
        raise ValueError('Excessive material evidence')
    rows = [json.loads(line) for line in log.read_text().splitlines()]
    if not 1 <= len(rows) <= 16 or any(r.get('type') != 'native_material_draw' for r in rows):
        raise ValueError('Incomplete/failed native material evidence')
    programs, results, seen = {}, [], set()
    for r in rows:
        key = (r['eye'], r['mesh'], r['material'])
        if key in seen or r['eye'] not in (0, 1) or r['frame'] <= 0 or r['native_draw_suppressed'] or r['gpu_readback']:
            raise ValueError('Invalid capture ownership/mode')
        seen.add(key)
        stages = {}
        for stage, prefix, limit in [('vertex', 'vs', 256), ('pixel', 'ps', 224)]:
            filename = r[stage + '_shader']
            if not re.fullmatch(prefix + r'-[0-9a-f]{1,16}\.cso', filename):
                raise ValueError('Nonlocal shader filename')
            if filename not in programs:
                data = (directory / filename).read_bytes()
                if filename != f'{prefix}-{fnv(data):x}.cso':
                    raise ValueError('Shader content identity changed')
                programs[filename] = dict(sha256=hashlib.sha256(data).hexdigest(), **constant_table(data, stage))
            floats = r[stage + ('_constants_c0_c255' if stage == 'vertex' else '_constants_c0_c223')]
            if len(floats) != limit * 4:
                raise ValueError('Captured register array length')
            bindings = []
            for declaration in programs[filename]['constants']:
                binding = dict(declaration)
                index, number, register_set = (declaration[k] for k in ('register_index', 'register_count', 'register_set'))
                if register_set == 3:
                    binding['sampler_state'] = r['samplers'][index] if stage == 'pixel' and index < 16 else None
                    binding['texture_pixels_captured'] = False
                else:
                    stride = 1 if register_set == 0 else 4
                    if register_set == 2:
                        values = floats
                    else:
                        values = r[stage + ('_bool_b0_b15' if register_set == 0 else '_int_i0_i15')]
                        if not r['integer_boolean_constants_valid']:
                            values = []
                    if (index + number) * stride > len(values):
                        raise ValueError('Declared shader input was not captured')
                    binding['values'] = values[index * stride:(index + number) * stride]
                    binding['finite'] = all(v is not None and math.isfinite(v) for v in binding['values'])
                bindings.append(binding)
            stages[stage] = dict(program=filename, bindings=bindings)
        deferred = bool(r.get('deferred_pool', False))
        matched_row = r.get('matched_instance_row', -1)
        if deferred:
            world = r['selected_world_cm']
            if len(world) != 16 or not 0 <= matched_row < 17 or any(v is None or not math.isfinite(v) for v in world):
                raise ValueError('Invalid deferred entity transform')
            expected = [world[c * 4 + row] for row in range(3) for c in range(4)]
            matches = []
            for binding in stages['vertex']['bindings']:
                t = binding['type']
                if binding['name'] not in ('matWorld', 'arrInstanceMatrices') or binding['register_set'] != 2:
                    continue
                if (t['kind'], t['value_type'], t['rows'], t['columns']) != (3, 3, 4, 3):
                    continue
                offset = 0 if binding['name'] == 'matWorld' else matched_row * 12
                # Export uses nine significant digits: round-trip all float32s
                # before exact comparison to avoid decimal spelling differences.
                packed = lambda values: struct.pack('<' + 'f' * len(values), *values)
                actual = binding['values'][offset:offset + 12]
                matches.append(len(actual) == 12 and packed(actual) == packed(expected))
            if not any(matches):
                raise ValueError('Deferred shader matrix does not belong to selected entity')
        results.append(dict(frame=r['frame'], eye=r['eye'], mesh=r['mesh'], material=r['material'],
                            states=r['states'], declaration=r['declaration'], stages=stages,
                            deferred_pool=deferred, matched_instance_row=matched_row))
    report = dict(captured_draws=len(results), unique_programs=len(programs), eyes=sorted({r['eye'] for r in rows}),
                  programs=programs, draws=results, game_loaded=False, unused_registers_not_interpreted=True,
                  native_material_parity=False)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2))
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--directory', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    report = analyze(args.directory, args.output)
    print(f"Resolved {report['captured_draws']} original material draws, {report['unique_programs']} programs; native parity remains unproven.")
