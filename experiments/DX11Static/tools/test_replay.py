"""Reject damaged or unrelated captured inputs before offline GPU replay."""
import argparse
import copy
import json
from pathlib import Path
import tempfile
from prepare_live_replay import prepare


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--log', type=Path, required=True)
    p.add_argument('--assets', type=Path, required=True)
    p.add_argument('--scratch-dir', type=Path, default=Path.cwd())
    args = p.parse_args()
    rows = [json.loads(line) for line in args.log.read_text(encoding='utf-8-sig').splitlines()]
    index = next(i for i, r in enumerate(rows) if r.get('type') == 'dx11_static_trial_eye')
    cases = []
    for field, value in [('resource', '../escaped.g3mesh'), ('eye', 2), ('frame', 0), ('native_view_cm', [0] * 15), ('native_projection_cm', [float('nan')] * 16)]:
        damaged = copy.deepcopy(rows)
        damaged[index][field] = value
        cases.append(damaged)
    duplicate = copy.deepcopy(rows)
    duplicate.append(duplicate[index])
    cases.append(duplicate)
    wrong_size = copy.deepcopy(rows)
    next(r for r in wrong_size if r.get('type') == 'dx11_static_trial_ready')['width'] = 32768
    cases.append(wrong_size)
    scratch = args.scratch_dir.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='g3dx11-replay-', dir=scratch) as temp:
        root = Path(temp)
        assert root.resolve().is_relative_to(scratch)
        good = prepare(args.log, args.assets, root / 'valid.g3replay')
        assert good['records'] >= 2 and not good['native_material_parity']
        for n, damaged in enumerate(cases):
            path = root / f'invalid-{n}.jsonl'
            path.write_text('\n'.join(json.dumps(r) for r in damaged))
            target = root / f'invalid-{n}.g3replay'
            try:
                prepare(path, args.assets, target)
            except ValueError:
                assert not target.exists()
            else:
                raise AssertionError('Damaged replay was accepted')
    print(f'PASS actual stereo records, provenance and {len(cases)} damaged/unsafe replay rejections')


if __name__ == '__main__':
    main()
